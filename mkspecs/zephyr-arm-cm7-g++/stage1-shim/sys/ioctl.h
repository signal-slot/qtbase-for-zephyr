/* Stage 1 stub: <sys/ioctl.h>
 *
 * Zephyr SDK newlib does not ship <sys/ioctl.h>.  Qt's qnet_unix_p.h
 * calls ::ioctl(fd, FIONREAD, ...) to query bytes-available.  Stage 1
 * needs the prototype + constant; the body is linked at Stage 2 via
 * Zephyr's POSIX compatibility layer.
 */
#ifndef QZEPHYR_STAGE1_SYS_IOCTL_H
#define QZEPHYR_STAGE1_SYS_IOCTL_H

#ifdef __cplusplus
extern "C" {
#endif

#define FIONREAD  0x541B
#define FIONBIO   0x5421

int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_SYS_IOCTL_H */
