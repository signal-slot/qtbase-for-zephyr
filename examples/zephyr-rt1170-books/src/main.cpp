#include "bookwindow.h"

#include <QApplication>

#include <zephyr/kernel.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

extern "C" void qzephyr_sqlite_patch_vfs(void);

static FATFS fat_fs;
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

static bool mount_sd()
{
    if (disk_access_init("SD") != 0) {
        printk("[books] disk_access_init failed\n");
        return false;
    }
    mp.mnt_point = "/SD:";
    if (fs_mount(&mp) != 0) {
        printk("[books] mount failed\n");
        return false;
    }
    printk("[books] SD card mounted at /SD:\n");
    return true;
}

int main(int /*argc_zephyr*/, char * /*argv_zephyr*/[])
{
    printk("\n[books] === boot ===\n");

    qzephyr_sqlite_patch_vfs();
    mount_sd();

    static char arg0[] = "books";
    static char *argv[] = { arg0, nullptr };
    int argc = 1;

    qputenv("QT_QPA_PLATFORM", "zephyr");
    QApplication app(argc, argv);

    BookWindow win;
    win.showFullScreen();

    return app.exec();
}
