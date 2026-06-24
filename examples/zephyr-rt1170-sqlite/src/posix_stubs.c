#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * Zephyr FatFS lseek rejects seeks beyond EOF (returns EINVAL).
 * POSIX requires lseek beyond EOF to succeed (sparse file semantics).
 * SQLite relies on this: it seeks to offset 24 on a 0-byte file to read
 * the change counter. We wrap lseek to track a "virtual position" when
 * the seek target exceeds the current file size. Subsequent reads from
 * that position return 0 (EOF).
 */
extern off_t __real_lseek(int fd, off_t offset, int whence);
extern ssize_t __real_read(int fd, void *buf, size_t count);

#define MAX_TRACKED_FDS 8
static struct {
    int fd;
    off_t virt_pos;
} s_virt[MAX_TRACKED_FDS];

static int virt_slot(int fd)
{
    for (int i = 0; i < MAX_TRACKED_FDS; i++)
        if (s_virt[i].fd == fd)
            return i;
    return -1;
}

static int virt_alloc(int fd)
{
    int slot = virt_slot(fd);
    if (slot >= 0) return slot;
    for (int i = 0; i < MAX_TRACKED_FDS; i++) {
        if (s_virt[i].fd == 0) {
            s_virt[i].fd = fd;
            s_virt[i].virt_pos = -1;
            return i;
        }
    }
    return -1;
}

off_t __wrap_lseek(int fd, off_t offset, int whence)
{
    off_t result = __real_lseek(fd, offset, whence);
    if (result >= 0) {
        int slot = virt_slot(fd);
        if (slot >= 0)
            s_virt[slot].virt_pos = -1;
        return result;
    }

    if (errno == EINVAL && whence == SEEK_SET) {
        struct stat st;
        if (fstat(fd, &st) == 0 && offset > st.st_size) {
            __real_lseek(fd, 0, SEEK_END);
            int slot = virt_alloc(fd);
            if (slot >= 0)
                s_virt[slot].virt_pos = offset;
            return offset;
        }
    }
    return -1;
}

ssize_t __wrap_read(int fd, void *buf, size_t count)
{
    int slot = virt_slot(fd);
    if (slot >= 0 && s_virt[slot].virt_pos >= 0) {
        return 0;
    }
    return __real_read(fd, buf, count);
}

/* --- POSIX stubs for functions missing in Zephyr --- */

int lstat(const char *path, struct stat *buf)
{
    return stat(path, buf);
}

ssize_t readlink(const char *path, char *buf, size_t bufsiz)
{
    (void)path; (void)buf; (void)bufsiz;
    errno = EINVAL;
    return -1;
}

char *getcwd(char *buf, size_t size)
{
    if (size < 2) {
        errno = ERANGE;
        return NULL;
    }
    buf[0] = '/';
    buf[1] = '\0';
    return buf;
}

int flock(int fd, int operation)
{
    (void)fd; (void)operation;
    return 0;
}

int access(const char *path, int amode)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return -1;
    (void)amode;
    return 0;
}

int utimes(const char *path, const void *times)
{
    (void)path; (void)times;
    return 0;
}

unsigned int geteuid(void)
{
    return 1000;
}

unsigned int getuid(void)
{
    return 1000;
}
