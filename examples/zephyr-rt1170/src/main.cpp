// Stage 2 smoke-test for Tier 2 (Qt6 on RT1170-EVKB, picture-less boot).
//
// Verifies that:
//   * QCoreApplication can be instantiated on Zephyr/Cortex-M7
//   * Qt's event dispatcher (Zephyr-runtime variant, compiled in Stage 2)
//     services a QTimer single-shot callback
//   * exec() returns cleanly
//
// All status messages go to printk() so they show up on the EVKB's
// LPUART1 console regardless of how Qt's logging is configured.

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include <zephyr/kernel.h>

int main(int argc, char *argv[])
{
    printk("[qt-zephyr-rt1170] pre-Qt main(); arg count = %d\n", argc);

    QCoreApplication app(argc, argv);

    printk("[qt-zephyr-rt1170] QCoreApplication constructed\n");

    QTimer::singleShot(1000, []() {
        printk("[qt-zephyr-rt1170] QTimer fired at 1s; quitting\n");
        QCoreApplication::quit();
    });

    const int ret = app.exec();
    printk("[qt-zephyr-rt1170] app.exec() returned %d\n", ret);
    return ret;
}
