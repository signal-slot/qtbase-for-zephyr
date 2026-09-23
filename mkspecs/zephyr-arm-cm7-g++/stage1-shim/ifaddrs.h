/* Stage 1 stub: <ifaddrs.h> — Linux/BSD getifaddrs() interface list.
 *
 * Zephyr does NOT provide getifaddrs() — the interface enumeration is
 * done with `net_if_foreach()` from <zephyr/net/net_if.h>.  Qt's
 * qnetworkinterface_unix.cpp calls getifaddrs() unconditionally.
 *
 * This header declares the API so Stage 1 compiles; the qnetworkinterface
 * Zephyr backend (qt5/qtbase/src/network/kernel/qnetworkinterface_zephyr.cpp)
 * supersedes the Unix path at CMake-config time, so the unresolved
 * getifaddrs() symbol is never linked.
 */
#ifndef QZEPHYR_STAGE1_IFADDRS_H
#define QZEPHYR_STAGE1_IFADDRS_H

#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ifaddrs {
    struct ifaddrs  *ifa_next;
    char            *ifa_name;
    unsigned int     ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
    union {
        struct sockaddr *ifu_broadaddr;
        struct sockaddr *ifu_dstaddr;
    } ifa_ifu;
#define ifa_broadaddr ifa_ifu.ifu_broadaddr
#define ifa_dstaddr   ifa_ifu.ifu_dstaddr
    void            *ifa_data;
};

int   getifaddrs(struct ifaddrs **ifap);
void  freeifaddrs(struct ifaddrs *ifa);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_IFADDRS_H */
