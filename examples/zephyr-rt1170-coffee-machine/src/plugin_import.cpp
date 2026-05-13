// Force the static qzephyr platform plugin's symbols out of
// libqzephyr.a and pre-set QT_QPA_PLATFORM=zephyr before QGuiApplication
// even gets constructed.

#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtCore/QtPlugin>
#include <cstdlib>

namespace {
__attribute__((constructor)) void zephyr_qpa_default()
{
    if (!std::getenv("QT_QPA_PLATFORM"))
        setenv("QT_QPA_PLATFORM", "zephyr", 0);
}
} // namespace

Q_IMPORT_PLUGIN(QZephyrIntegrationPlugin)
