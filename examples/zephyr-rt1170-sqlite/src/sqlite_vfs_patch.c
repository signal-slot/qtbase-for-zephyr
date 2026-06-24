#include <zephyr/kernel.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sqlite3.h>

static ssize_t zephyr_pread(int fd, void *buf, size_t count, off_t offset)
{
    off_t old = lseek(fd, 0, SEEK_CUR);
    if (old == (off_t)-1)
        return -1;
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
        lseek(fd, old, SEEK_SET);
        return -1;
    }
    ssize_t n = read(fd, buf, count);
    int saved = errno;
    lseek(fd, old, SEEK_SET);
    errno = saved;
    return n;
}

static ssize_t zephyr_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    off_t old = lseek(fd, 0, SEEK_CUR);
    if (old == (off_t)-1)
        return -1;
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1)
        return -1;
    ssize_t n = write(fd, buf, count);
    int saved = errno;
    lseek(fd, old, SEEK_SET);
    errno = saved;
    return n;
}

void sqlite_patch_vfs_for_zephyr(void)
{
    sqlite3_initialize();

    sqlite3_vfs *vfs = sqlite3_vfs_find("unix");
    if (!vfs || !vfs->xSetSystemCall)
        return;

    vfs->xSetSystemCall(vfs, "pread", (sqlite3_syscall_ptr)zephyr_pread);
    vfs->xSetSystemCall(vfs, "pwrite", (sqlite3_syscall_ptr)zephyr_pwrite);

    sqlite3_vfs *none = sqlite3_vfs_find("unix-none");
    if (none) {
        if (none->xSetSystemCall) {
            none->xSetSystemCall(none, "pread", (sqlite3_syscall_ptr)zephyr_pread);
            none->xSetSystemCall(none, "pwrite", (sqlite3_syscall_ptr)zephyr_pwrite);
        }
        sqlite3_vfs_register(none, 1);
    }
}
