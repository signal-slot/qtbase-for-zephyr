/* Stage 1 stub: <netinet/in.h> — IPv4/IPv6 address types and constants.
 * Bodies linked at Stage 2 via Zephyr's CONFIG_NET_SOCKETS_POSIX_NAMES.
 *
 * Struct layouts: the first 8 bytes of POSIX sockaddr_in
 * (sin_family, sin_port, sin_addr) match Zephyr net_sockaddr_in exactly;
 * the trailing sin_zero[8] padding is benign at Stage 2 link time.
 * sockaddr_in6 likewise matches net_sockaddr_in6 in its first fields.
 */
#ifndef QZEPHYR_STAGE1_NETINET_IN_H
#define QZEPHYR_STAGE1_NETINET_IN_H

#include <sys/socket.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

#define INET_ADDRSTRLEN  16
#define INET6_ADDRSTRLEN 46

#define INADDR_ANY        ((in_addr_t)0x00000000)
#define INADDR_LOOPBACK   ((in_addr_t)0x7f000001)
#define INADDR_BROADCAST  ((in_addr_t)0xffffffff)
#define INADDR_NONE       ((in_addr_t)0xffffffff)

/* Protocol numbers — match Zephyr NET_IPPROTO_*. */
#define IPPROTO_IP        0
#define IPPROTO_ICMP      1
#define IPPROTO_IGMP      2
#define IPPROTO_IPIP      4
#define IPPROTO_TCP       6
#define IPPROTO_UDP       17
#define IPPROTO_IPV6      41
#define IPPROTO_ICMPV6    58
#define IPPROTO_RAW       255

/* IP-level setsockopt options (subset Qt actually touches). */
#define IP_TOS              1
#define IP_TTL              2
#define IP_HDRINCL          3
#define IP_RECVOPTS         6
#define IP_RETOPTS          7
#define IP_MULTICAST_IF     32
#define IP_MULTICAST_TTL    33
#define IP_MULTICAST_LOOP   34
#define IP_ADD_MEMBERSHIP   35
#define IP_DROP_MEMBERSHIP  36
#define IP_PKTINFO          8
#define IP_RECVTOS          13
#define IP_DONTFRAGMENT     14
#define IP_MTU_DISCOVER     10

/* IPv6 setsockopt options. */
#define IPV6_UNICAST_HOPS    16
#define IPV6_MULTICAST_IF    17
#define IPV6_MULTICAST_HOPS  18
#define IPV6_MULTICAST_LOOP  19
#define IPV6_ADD_MEMBERSHIP  20
#define IPV6_DROP_MEMBERSHIP 21
#define IPV6_V6ONLY          26
#define IPV6_RECVPKTINFO     49
#define IPV6_PKTINFO         50
#define IPV6_RECVHOPLIMIT    51
#define IPV6_HOPLIMIT        52

#define IPV6_JOIN_GROUP   IPV6_ADD_MEMBERSHIP
#define IPV6_LEAVE_GROUP  IPV6_DROP_MEMBERSHIP

struct in_addr {
    in_addr_t s_addr;   /* IPv4 address, network byte order. */
};

struct in6_addr {
    union {
        uint8_t  s6_addr[16];
        uint16_t s6_addr16[8];
        uint32_t s6_addr32[4];
    };
};

struct sockaddr_in {
    sa_family_t    sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

struct sockaddr_in6 {
    sa_family_t     sin6_family;
    in_port_t       sin6_port;
    uint32_t        sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t        sin6_scope_id;
};

struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;
    unsigned int    ipv6mr_interface;
};

struct ip_mreq {
    struct in_addr imr_multiaddr;
    struct in_addr imr_interface;
};

struct ip_mreqn {
    struct in_addr imr_multiaddr;
    struct in_addr imr_address;
    int            imr_ifindex;
};

struct in_pktinfo {
    int            ipi_ifindex;
    struct in_addr ipi_spec_dst;
    struct in_addr ipi_addr;
};

struct in6_pktinfo {
    struct in6_addr ipi6_addr;
    unsigned int    ipi6_ifindex;
};

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;

#define IN6ADDR_ANY_INIT      {{{0}}}
#define IN6ADDR_LOOPBACK_INIT {{{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1}}}

/* glibc / BSD compatibility: htonX / ntohX are reachable through
 * <netinet/in.h> in addition to <arpa/inet.h>.  qtbase relies on this. */
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_NETINET_IN_H */
