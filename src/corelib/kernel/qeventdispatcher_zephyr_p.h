// Copyright (C) 2025 The Qt Company Ltd.
// Copyright (C) 2026 Tasuku Suzuki.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVENTDISPATCHER_ZEPHYR_P_H
#define QEVENTDISPATCHER_ZEPHYR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qatomic.h>
#include <QtCore/private/qabstracteventdispatcher_p.h>

#include <zephyr/kernel.h>

class QSocketNotifier;

QT_BEGIN_NAMESPACE

class QEventDispatcherZephyrPrivate;

class Q_CORE_EXPORT QEventDispatcherZephyr : public QAbstractEventDispatcherV2
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherZephyr)

public:
    explicit QEventDispatcherZephyr(QObject *parent = nullptr);
    ~QEventDispatcherZephyr() override;

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

    void registerSocketNotifier(QSocketNotifier *notifier) override;
    void unregisterSocketNotifier(QSocketNotifier *notifier) override;

    void registerTimer(Qt::TimerId timerId, Duration interval, Qt::TimerType timerType,
                       QObject *object) override;
    bool unregisterTimer(Qt::TimerId timerId) override;
    bool unregisterTimers(QObject *object) override;
    QList<TimerInfoV2> timersForObject(QObject *object) const override;
    Duration remainingTime(Qt::TimerId timerId) const override;

    void wakeUp() override;
    void interrupt() override;

protected:
    QEventDispatcherZephyr(QEventDispatcherZephyrPrivate &dd, QObject *parent = nullptr);
};

struct ZephyrTimerInfo
{
    QObject *obj = nullptr;
    Qt::TimerId timerId = Qt::TimerId::Invalid;
    QAbstractEventDispatcher::Duration interval{};
    Qt::TimerType timerType = Qt::CoarseTimer;
    k_timer timer{};
    // Set from the k_timer expiry callback; cleared by the dispatcher
    // before invoking the user slot.
    QAtomicInt expired{0};
};

class Q_CORE_EXPORT QEventDispatcherZephyrPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherZephyr)

public:
    QEventDispatcherZephyrPrivate();
    ~QEventDispatcherZephyrPrivate() override;

    QHash<Qt::TimerId, ZephyrTimerInfo *> timerDict;

    // A ZephyrTimerInfo embeds its k_timer.  Returning that storage to the
    // heap in unregisterTimer() is unsafe: a stopped k_timer's _timeout dnode
    // can still be touched by the kernel timeout subsystem in a narrow window,
    // and once the chunk has been recycled into an unrelated object (a
    // QPainterState, a QTransform, ...) that stray write corrupts it -- which
    // is exactly the random-victim heap corruption observed on hardware.
    //
    // So a ZephyrTimerInfo's storage is NEVER returned to the heap while the
    // dispatcher lives: timer memory always stays timer memory, so any late
    // kernel write lands on an inert k_timer and is harmless.  Retired infos
    // are parked in pendingRecycle for one processEvents() iteration (a
    // quiescent point on the dispatcher thread, after any timer-ISR work has
    // settled) and then moved to freePool for reuse by registerTimer().
    QList<ZephyrTimerInfo *> pendingRecycle;
    QList<ZephyrTimerInfo *> freePool;
    ZephyrTimerInfo *acquireTimerInfo();
    void retireTimer(ZephyrTimerInfo *info);
    void recyclePending();

    // wakeUp() may be called from any thread (including ISR context);
    // processEvents() blocks via k_poll on this signal.
    k_poll_signal wakeupSignal{};
    k_poll_event wakeupEvent{};

    QAtomicInt interrupt{0};
    QAtomicInt wakeupRaised{0};

    static void timerCallback(k_timer *timer);
    int dispatchTimers();

    QList<QSocketNotifier *> socketNotifiers;
    int activateSocketNotifiers();
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_ZEPHYR_P_H
