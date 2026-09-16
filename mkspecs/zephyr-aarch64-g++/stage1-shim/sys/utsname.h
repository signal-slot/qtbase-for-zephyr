/* Stage 1 stub: <sys/utsname.h>
 *
 * Zephyr SDK's newlib/picolibc does not ship <sys/utsname.h>.  Qt's
 * qsysinfo.cpp includes it and calls uname() to populate
 * QSysInfo::kernelType/kernelVersion/machineHostName.  Stage 1 of the
 * two-stage Qt-on-Zephyr build is just generating static archives, so
 * we provide a minimal stub that satisfies the compile.  Stage 2
 * (west build) will override this with a real Zephyr-aware
 * implementation via the Zephyr application's CMakeLists.
 */
#ifndef QZEPHYR_STAGE1_SYS_UTSNAME_H
#define QZEPHYR_STAGE1_SYS_UTSNAME_H

#define _UTSNAME_LENGTH 65

struct utsname {
    char sysname[_UTSNAME_LENGTH];
    char nodename[_UTSNAME_LENGTH];
    char release[_UTSNAME_LENGTH];
    char version[_UTSNAME_LENGTH];
    char machine[_UTSNAME_LENGTH];
};

#ifdef __cplusplus
extern "C" {
#endif

int uname(struct utsname *buf);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_SYS_UTSNAME_H */
