// Tier 8: Qt official Coffee demo on MIMXRT1170-EVKB + RK055HDMIPI4MA0.
//
// Wraps Qt's stock demos/coffee QML module with the Zephyr-side setup
// the qzephyr QPA needs: printk progress markers, QT_QPA_PLATFORM=zephyr
// and QT_QUICK_BACKEND=software forced before QGuiApplication, plus
// registration of the embedded Roboto-Regular TTF.

#include <QtGui/QGuiApplication>
#include <QtGui/QFontDatabase>
#include <QtQml/QQmlApplicationEngine>
#include <QtCore/QByteArray>

#include <zephyr/kernel.h>

extern "C" unsigned char g_embedded_roboto_ttf[];
extern "C" unsigned int g_embedded_roboto_ttf_len;

int main(int argc, char *argv[])
{
    printk("[qt-coffee] pre-Qt main(); arg count = %d\n", argc);
    qputenv("QT_QPA_PLATFORM", QByteArray("zephyr"));
    qputenv("QT_QUICK_BACKEND", QByteArray("software"));

    QGuiApplication app(argc, argv);
    printk("[qt-coffee] QGuiApplication constructed\n");

    const QByteArray fontData(reinterpret_cast<const char *>(g_embedded_roboto_ttf),
                              int(g_embedded_roboto_ttf_len));
    const int fontId = QFontDatabase::addApplicationFontFromData(fontData);
    if (fontId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            QGuiApplication::setFont(QFont(families.first()));
            printk("[qt-coffee] using font: %s\n",
                   families.first().toUtf8().constData());
        }
    }

    QQmlApplicationEngine engine;
    engine.loadFromModule("demos.coffee", "Main");
    printk("[qt-coffee] engine.loadFromModule done; rootObjects=%d\n",
           int(engine.rootObjects().size()));
    if (engine.rootObjects().isEmpty()) {
        printk("[qt-coffee] FATAL: no root objects loaded\n");
        return -1;
    }

    printk("[qt-coffee] entering exec()\n");
    const int rc = QCoreApplication::exec();
    printk("[qt-coffee] exec() returned %d\n", rc);
    return rc;
}
