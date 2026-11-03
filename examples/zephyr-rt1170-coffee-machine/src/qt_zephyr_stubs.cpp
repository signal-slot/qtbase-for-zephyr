// Qt-internal symbol stubs for the Tier 2 smoke test on RT1170-EVKB.
//
// libQt6Core.a was built without these UNIX-specific translation units:
//   qstandardpaths_unix.cpp - QStandardPaths::writableLocation / standardLocations
//
// Their symbols are still referenced from portable Qt code that DOES
// live inside libQt6Core.a (qsettings.cpp uses QStandardPaths), so we
// must provide a definition or the linker either fails or -- with
// --unresolved-symbols=ignore-all -- silently binds the call sites to
// FCB junk and the firmware faults the moment any of them is touched.
//
// These stubs return "no result" semantics.  Calling them at runtime is
// harmless; the Tier 2 picture-less smoke test never reaches them.

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QStandardPaths>

QT_BEGIN_NAMESPACE

QString QStandardPaths::writableLocation(QStandardPaths::StandardLocation)
{
    return QString();
}

QStringList QStandardPaths::standardLocations(QStandardPaths::StandardLocation)
{
    return QStringList();
}

QT_END_NAMESPACE
