// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qplatformintegrationplugin.h>
#include "qzephyrintegration.h"

QT_BEGIN_NAMESPACE

class QZephyrIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "zephyr.json")
public:
    QPlatformIntegration *create(const QString &system, const QStringList &paramList) override;
};

QPlatformIntegration *QZephyrIntegrationPlugin::create(const QString &system, const QStringList &paramList)
{
    Q_UNUSED(paramList);
    if (!system.compare(QLatin1String("zephyr"), Qt::CaseInsensitive))
        return new QZephyrIntegration;

    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"