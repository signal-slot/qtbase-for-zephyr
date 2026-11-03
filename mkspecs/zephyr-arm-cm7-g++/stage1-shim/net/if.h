/* Stage 1 stub: <net/if.h> — interface flags + name <-> index helpers.
 * Bodies linked at Stage 2 against Zephyr's net_if_foreach() world.
 */
#ifndef QZEPHYR_STAGE1_NET_IF_H
#define QZEPHYR_STAGE1_NET_IF_H

#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IFNAMSIZ 16

#define IFF_UP          0x1
#define IFF_BROADCAST   0x2
#define IFF_DEBUG       0x4
#define IFF_LOOPBACK    0x8
#define IFF_POINTOPOINT 0x10
#define IFF_RUNNING     0x40
#define IFF_NOARP       0x80
#define IFF_PROMISC     0x100
#define IFF_ALLMULTI    0x200
#define IFF_MULTICAST   0x1000

struct if_nameindex {
    unsigned int if_index;
    char        *if_name;
};

unsigned int          if_nametoindex(const char *ifname);
char                 *if_indextoname(unsigned int ifindex, char *ifname);
struct if_nameindex  *if_nameindex(void);
void                  if_freenameindex(struct if_nameindex *ptr);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_NET_IF_H */
