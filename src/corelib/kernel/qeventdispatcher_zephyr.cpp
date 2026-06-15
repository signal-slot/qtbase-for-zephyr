// Copyright (C) 2025 The Qt Company Ltd.
// Copyright (C) 2026 Tasuku Suzuki.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeventdispatcher_zephyr_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qthread.h>
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/private/qthread_p.h>

// Weak hook -- Stage 2 GUI apps provide a strong definition that
// forwards to QWindowSystemInterface::sendWindowSystemEvents() so QPA
// window-system events (mouse, key, expose) end up at QWindow::event().
// Without it those events sit on the QPA queue while our dispatcher
// only drains QObject posted events + Zephyr timers.
extern "C" __attribute__((weak)) void qzephyr_drain_qpa_events();

#include <zephyr/kernel.h>

#include <chrono>
#include <limits>

QT_BEGIN_NAMESPACE

using namespace std::chrono;

// qthread_unix.cpp normally provides these QThread / QThreadPrivate
// definitions, but that TU is excluded from the Zephyr build (it pulls
// in pthread / select / poll).  Their absence is what
// QCoreApplication's ctor trips over: it calls createEventDispatcher()
// while wiring the main thread, and without a real definition the
// linker just leaves the symbol unresolved.  Provide the Zephyr-flavoured
// versions here so QCoreApplication boots cleanly.
QAbstractEventDispatcher *QThreadPrivate::createEventDispatcher(QThreadData *)
{
    return new QEventDispatcherZephyr;
}

void QThread::sleep(std::chrono::nanoseconds ns)
{
    const auto ms = duration_cast<milliseconds>(ns).count();
    if (ms > 0)
        k_msleep(static_cast<int32_t>(qMin<qint64>(ms, std::numeric_limits<int32_t>::max())));
}

// k_timer carries a single user_data void*; we stash the ZephyrTimerInfo
// pointer there so the expiry callback (runs in ISR context) can flag the
// timer for the dispatcher to pick up on its next iteration.
void QEventDispatcherZephyrPrivate::timerCallback(k_timer *timer)
{
    auto *info = static_cast<ZephyrTimerInfo *>(k_timer_user_data_get(timer));
    if (info)
        info->expired.storeRelaxed(1);
}

QEventDispatcherZephyrPrivate::QEventDispatcherZephyrPrivate()
{
    k_poll_signal_init(&wakeupSignal);
    k_poll_event_init(&wakeupEvent, K_POLL_TYPE_SIGNAL,
                      K_POLL_MODE_NOTIFY_ONLY, &wakeupSignal);
}

QEventDispatcherZephyrPrivate::~QEventDispatcherZephyrPrivate()
{
    for (ZephyrTimerInfo *info : std::as_const(timerDict)) {
        k_timer_stop(&info->timer);
        delete info;
    }
    timerDict.clear();
    // The dispatcher is being destroyed, so the kernel can no longer reach
    // these timers -- the storage that was kept alive for recycling (retired
    // but not-yet-recycled, plus the idle free pool) is finally released.
    qDeleteAll(pendingRecycle);
    pendingRecycle.clear();
    qDeleteAll(freePool);
    freePool.clear();
}

int QEventDispatcherZephyrPrivate::dispatchTimers()
{
    int n = 0;
    // Snapshot the timer *ids* (values, not pointers) so a slot in a
    // prior iteration that unregisters a timer cannot leave a dangling
    // pointer in our walk list.  Re-lookup each id fresh per iteration
    // and skip if it has been unregistered since the snapshot.
    const QList<Qt::TimerId> snapshot = timerDict.keys();
    for (Qt::TimerId id : snapshot) {
        auto it = timerDict.constFind(id);
        if (it == timerDict.cend())
            continue;
        ZephyrTimerInfo *info = it.value();
        if (!info->expired.fetchAndStoreRelaxed(0))
            continue;
        if (!info->obj)
            continue;
        QTimerEvent ev(qToUnderlying(id));
        QCoreApplication::sendEvent(info->obj, &ev);
        ++n;
    }
    return n;
}

QEventDispatcherZephyr::QEventDispatcherZephyr(QObject *parent)
    : QAbstractEventDispatcherV2(*new QEventDispatcherZephyrPrivate, parent)
{
}

QEventDispatcherZephyr::QEventDispatcherZephyr(QEventDispatcherZephyrPrivate &dd, QObject *parent)
    : QAbstractEventDispatcherV2(dd, parent)
{
}

QEventDispatcherZephyr::~QEventDispatcherZephyr() = default;

bool QEventDispatcherZephyr::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QEventDispatcherZephyr);

    // Quiescent point: recycle any timers retired during a previous iteration.
    // By now k_timer_stop() has settled and no timer-ISR work references them,
    // so their storage can safely re-enter the free pool for reuse.
    d->recyclePending();

    if (d->interrupt.fetchAndStoreRelaxed(0))
        return false;

    emit awake();

    auto *thisThreadData = QThreadData::current();
    // Drain QPA window-system events first (mouse / key / expose); they
    // arrive from the input thread and need to be delivered to QWindow
    // handlers before we process the corresponding posted Qt events.
    if (qzephyr_drain_qpa_events)
        qzephyr_drain_qpa_events();
    QCoreApplicationPrivate::sendPostedEvents(nullptr, 0, thisThreadData);

    int nevents = d->dispatchTimers();

    const bool canWait = (flags & QEventLoop::WaitForMoreEvents)
                         && !d->interrupt.loadRelaxed()
                         && nevents == 0;

    if (canWait) {
        // Pick the soonest pending timer deadline; K_FOREVER means
        // "block until wakeUp() is raised".
        k_timeout_t wait_timeout = K_FOREVER;
        bool haveTimerDeadline = false;
        for (const ZephyrTimerInfo *info : std::as_const(d->timerDict)) {
            const k_ticks_t remaining = k_timer_remaining_ticks(&info->timer);
            if (!haveTimerDeadline || remaining < wait_timeout.ticks) {
                wait_timeout.ticks = remaining;
                haveTimerDeadline = true;
            }
        }

        // Bound the block to 50 ms.  QtQuick's basic render loop drives
        // continuous animation by re-arming a short (~16 ms) update timer each
        // frame; if that chain ever lapses, only the app's long-interval timers
        // remain and the loop would sleep for seconds, so the UI updates only
        // when an input (touch) happens to wake it.  Capping the wait makes the
        // loop re-check ~20x/s, so animations advance and repaint without input.
        const k_timeout_t cap = K_MSEC(50);
        if (!haveTimerDeadline || wait_timeout.ticks > cap.ticks)
            wait_timeout = cap;

        emit aboutToBlock();

        k_poll_event ev = d->wakeupEvent;
        ev.state = K_POLL_STATE_NOT_READY;
        k_poll(&ev, 1, wait_timeout);

        if (ev.state == K_POLL_STATE_SIGNALED) {
            k_poll_signal_reset(&d->wakeupSignal);
            d->wakeupRaised.storeRelaxed(0);
        }

        emit awake();

        if (qzephyr_drain_qpa_events)
            qzephyr_drain_qpa_events();
        QCoreApplicationPrivate::sendPostedEvents(nullptr, 0, thisThreadData);
        nevents += d->dispatchTimers();
    }

    return nevents > 0;
}

void QEventDispatcherZephyr::registerSocketNotifier(QSocketNotifier *notifier)
{
    // Zephyr's POSIX subset doesn't surface socket fds in a way that maps
    // cleanly onto QSocketNotifier semantics.  Apps that need network IO
    // go through Qt's high-level QNetwork* classes which do not depend on
    // a working socket notifier on this platform.
    Q_UNUSED(notifier);
}

void QEventDispatcherZephyr::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
}

void QEventDispatcherZephyr::registerTimer(Qt::TimerId timerId, Duration interval,
                                            Qt::TimerType timerType, QObject *object)
{
    Q_D(QEventDispatcherZephyr);
    Q_ASSERT(object);

    ZephyrTimerInfo *info = d->acquireTimerInfo();
    info->obj = object;
    info->timerId = timerId;
    info->interval = interval;
    info->timerType = timerType;
    info->expired.storeRelaxed(0);

    k_timer_init(&info->timer, &QEventDispatcherZephyrPrivate::timerCallback, nullptr);
    k_timer_user_data_set(&info->timer, info);

    d->timerDict.insert(timerId, info);

    // K_MSEC takes a 32-bit ms count.  Clamp negatives to 0 (timer fires
    // immediately) and very long intervals to INT32_MAX ms (~24 days).
    const auto ms = duration_cast<milliseconds>(interval).count();
    qint32 clamped;
    if (ms <= 0)
        clamped = 0;
    else if (ms >= std::numeric_limits<qint32>::max())
        clamped = std::numeric_limits<qint32>::max();
    else
        clamped = qint32(ms);
    k_timer_start(&info->timer, K_MSEC(clamped), K_MSEC(clamped));
}

// Hand out storage for a new timer, reusing a recycled ZephyrTimerInfo when
// one is idle so that timer memory is never round-tripped through the heap.
ZephyrTimerInfo *QEventDispatcherZephyrPrivate::acquireTimerInfo()
{
    if (!freePool.isEmpty())
        return freePool.takeLast();
    return new ZephyrTimerInfo;
}

// Stop a timer and park its ZephyrTimerInfo for deferred recycling.  Reusing
// the storage immediately would race the kernel timeout subsystem (the
// embedded k_timer's _timeout dnode may still be touched right after stop);
// recyclePending() returns it to the free pool at the top of the next
// processEvents() iteration, a quiescent point on the dispatcher thread.  The
// storage is never freed back to the heap, so even a stray late kernel write
// only ever lands on an inert k_timer.
void QEventDispatcherZephyrPrivate::retireTimer(ZephyrTimerInfo *info)
{
    k_timer_stop(&info->timer);
    info->obj = nullptr;          // ignore any stray late expiry in dispatchTimers
    pendingRecycle.append(info);
}

void QEventDispatcherZephyrPrivate::recyclePending()
{
    if (pendingRecycle.isEmpty())
        return;
    freePool.append(pendingRecycle);
    pendingRecycle.clear();
}

bool QEventDispatcherZephyr::unregisterTimer(Qt::TimerId timerId)
{
    Q_D(QEventDispatcherZephyr);
    ZephyrTimerInfo *info = d->timerDict.take(timerId);
    if (!info)
        return false;
    d->retireTimer(info);
    return true;
}

bool QEventDispatcherZephyr::unregisterTimers(QObject *object)
{
    Q_D(QEventDispatcherZephyr);
    if (!object)
        return false;
    bool any = false;
    const auto keys = d->timerDict.keys();
    for (Qt::TimerId id : keys) {
        ZephyrTimerInfo *info = d->timerDict.value(id);
        if (info && info->obj == object) {
            d->timerDict.remove(id);
            d->retireTimer(info);
            any = true;
        }
    }
    return any;
}

QList<QAbstractEventDispatcher::TimerInfoV2>
QEventDispatcherZephyr::timersForObject(QObject *object) const
{
    Q_D(const QEventDispatcherZephyr);
    QList<TimerInfoV2> result;
    for (const ZephyrTimerInfo *info : std::as_const(d->timerDict)) {
        if (info->obj == object)
            result.append({info->interval, info->timerId, info->timerType});
    }
    return result;
}

QAbstractEventDispatcher::Duration
QEventDispatcherZephyr::remainingTime(Qt::TimerId timerId) const
{
    Q_D(const QEventDispatcherZephyr);
    const ZephyrTimerInfo *info = d->timerDict.value(timerId);
    if (!info)
        return Duration::min();
    const k_ticks_t ticks = k_timer_remaining_ticks(&info->timer);
    return milliseconds(k_ticks_to_ms_floor64(ticks));
}

void QEventDispatcherZephyr::wakeUp()
{
    Q_D(QEventDispatcherZephyr);
    // Coalesce repeated wakeUp() calls between processEvents() iterations.
    if (d->wakeupRaised.fetchAndStoreRelaxed(1) == 0)
        k_poll_signal_raise(&d->wakeupSignal, 1);
}

void QEventDispatcherZephyr::interrupt()
{
    Q_D(QEventDispatcherZephyr);
    d->interrupt.storeRelaxed(1);
    wakeUp();
}

QT_END_NAMESPACE
