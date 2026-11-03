// Copyright (C) 2025 Signal Slot Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"

#ifndef Q_OS_ZEPHYR
#  error "This file is for Zephyr only"
#endif

QT_BEGIN_NAMESPACE

uint QNetworkInterfaceManager::interfaceIndexFromName(const QString &name)
{
    Q_UNUSED(name);
    return 0;
}

QString QNetworkInterfaceManager::interfaceNameFromIndex(uint index)
{
    Q_UNUSED(index);
    return QString();
}

QList<QNetworkInterfacePrivate *> QNetworkInterfaceManager::scan()
{
    return {};
}

QT_END_NAMESPACE
