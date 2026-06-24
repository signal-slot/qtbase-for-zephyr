#include <zephyr/kernel.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

#include <QApplication>
#include <QLabel>
#include <QFile>
#include <QDir>
#include <QTextStream>

static FATFS fat_fs;
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

static bool mount_sd()
{
    if (disk_access_init("SD") != 0) {
        printk("[qt-fatfs] disk_access_init failed\n");
        return false;
    }
    mp.mnt_point = "/SD:";
    if (fs_mount(&mp) != 0) {
        printk("[qt-fatfs] mount failed\n");
        return false;
    }
    printk("[qt-fatfs] SD card mounted at /SD:\n");
    return true;
}

int main(int /*argc_zephyr*/, char * /*argv_zephyr*/[])
{
    printk("\n[qt-fatfs] === Qt + FAT filesystem test ===\n");

    bool sd_ok = mount_sd();

    static char arg0[] = "qt-fatfs";
    static char *argv[] = { arg0, nullptr };
    int argc = 1;
    qputenv("QT_QPA_PLATFORM", "zephyr");

    QApplication app(argc, argv);

    QString result;
    if (!sd_ok) {
        result = "SD card not available.\nInsert a FAT32 SD card and reset.";
    } else {
        QString path = "/SD:/qt_test.txt";

        QFile wf(path);
        if (wf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&wf);
            out << "Hello from Qt 6 on Zephyr!\n";
            out << "QFile write test OK.\n";
            wf.close();
            result += "Wrote: " + path + "\n\n";
            printk("[qt-fatfs] QFile write OK\n");
        } else {
            result += "Write failed: " + wf.errorString() + "\n";
            printk("[qt-fatfs] QFile write failed\n");
        }

        QFile rf(path);
        if (rf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = rf.readAll();
            rf.close();
            result += "Read back:\n" + content;
            printk("[qt-fatfs] QFile read OK\n");
        } else {
            result += "Read failed: " + rf.errorString() + "\n";
            printk("[qt-fatfs] QFile read failed\n");
        }
    }

    printk("[qt-fatfs] result: %s\n", result.toUtf8().constData());

    QLabel label;
    label.setText(result);
    label.setStyleSheet("QLabel { font-size: 28px; padding: 40px; }");
    label.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    label.showFullScreen();

    return app.exec();
}
