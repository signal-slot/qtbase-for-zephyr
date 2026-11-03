// Tier 6: Qt official Widgets Gallery on MIMXRT1170-EVKB + RK055HDMIPI4MA0.
//
// Wraps Qt's stock examples/widgets/gallery WidgetGallery class with the
// Zephyr-side setup our QPA plugin needs: printk progress markers,
// QT_QPA_PLATFORM=zephyr forced before QApplication, and registration
// of the embedded Roboto-Regular TTF so QPainter::drawText has glyphs.

#include <QtWidgets/QApplication>
#include <QtGui/QStyleHints>
#include <QtGui/QFontDatabase>
#include <QtCore/QByteArray>

#include <zephyr/kernel.h>

#include "widgetgallery.h"

extern "C" unsigned char g_embedded_roboto_ttf[];
extern "C" unsigned int g_embedded_roboto_ttf_len;

int main(int argc, char *argv[])
{
    printk("[qt-widgets-gallery] pre-Qt main(); arg count = %d\n", argc);
    qputenv("QT_QPA_PLATFORM", QByteArray("zephyr"));

    QApplication app(argc, argv);
    printk("[qt-widgets-gallery] QApplication constructed\n");

    const QByteArray fontData(reinterpret_cast<const char *>(g_embedded_roboto_ttf),
                              int(g_embedded_roboto_ttf_len));
    const int fontId = QFontDatabase::addApplicationFontFromData(fontData);
    if (fontId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            QApplication::setFont(QFont(families.first()));
            printk("[qt-widgets-gallery] using font: %s\n",
                   families.first().toUtf8().constData());
        }
    } else {
        printk("[qt-widgets-gallery] addApplicationFontFromData failed\n");
    }

    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);

    WidgetGallery gallery;
    gallery.show();
    printk("[qt-widgets-gallery] WidgetGallery shown; entering exec()\n");

    const int rc = QCoreApplication::exec();
    printk("[qt-widgets-gallery] exec() returned %d\n", rc);
    return rc;
}
