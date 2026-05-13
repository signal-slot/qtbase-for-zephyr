/* Stage 1 stub: <dirent.h>
 *
 * Zephyr SDK's newlib ships a sentinel <sys/dirent.h> whose contents are
 * just `#error "<dirent.h> not supported"`.  Bare-metal targets don't
 * normally have a directory model, so newlib refuses to expose one.
 * Qt's qfilesystemiterator_unix.cpp etc. still #include <dirent.h>
 * unconditionally, so we provide a header-only stub for Stage 1 (no
 * runtime opendir/readdir behaviour; Stage 2 in `west build` is where
 * a real implementation backed by Zephyr's FS layer would be added if
 * the application actually needs directory traversal).
 */
#ifndef QZEPHYR_STAGE1_DIRENT_H
#define QZEPHYR_STAGE1_DIRENT_H

#include <sys/types.h>

#define DT_UNKNOWN  0
#define DT_FIFO     1
#define DT_CHR      2
#define DT_DIR      4
#define DT_BLK      6
#define DT_REG      8
#define DT_LNK      10
#define DT_SOCK     12

#ifndef NAME_MAX
#  define NAME_MAX 255
#endif

struct dirent {
    ino_t  d_ino;
    off_t  d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char   d_name[NAME_MAX + 1];
};

typedef struct __dirstream DIR;

#ifdef __cplusplus
extern "C" {
#endif

DIR           *opendir(const char *name);
DIR           *fdopendir(int fd);
int            closedir(DIR *dirp);
struct dirent *readdir(DIR *dirp);
int            readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);
void           rewinddir(DIR *dirp);
void           seekdir(DIR *dirp, long loc);
long           telldir(DIR *dirp);
int            dirfd(DIR *dirp);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_DIRENT_H */
