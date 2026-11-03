/* Stage 1 stub: <sys/socket.h>
 *
 * The Zephyr SDK's newlib-nano headers do not include <sys/socket.h> at
 * all (bare-metal newlib has no networking).  qtbase/src/network compiles
 * unconditionally against the POSIX BSD-sockets API, so Stage 1 needs
 * just enough type + prototype declarations to make those translation
 * units pass through the compiler.  Real bodies are linked at Stage 2
 * (`west build`) via Zephyr's CONFIG_NET_SOCKETS_POSIX_NAMES layer.
 *
 * Layout note: the first 8 bytes of POSIX `struct sockaddr_in` (sin_family,
 * sin_port, sin_addr.s_addr) are identical to Zephyr's `struct net_sockaddr_in`.
 * The trailing sin_zero[8] padding is benign — Zephyr's zsock_bind/connect
 * accept addrlen values greater than sizeof(struct net_sockaddr_in).
 */
#ifndef QZEPHYR_STAGE1_SYS_SOCKET_H
#define QZEPHYR_STAGE1_SYS_SOCKET_H

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t sa_family_t;
typedef uint32_t socklen_t;

/* Address families — match Zephyr NET_PF_* / net_compat.h aliases. */
#define AF_UNSPEC      0
#define AF_INET        1
#define AF_INET6       2
#define AF_PACKET      3
#define AF_CAN         4

#define PF_UNSPEC      AF_UNSPEC
#define PF_INET        AF_INET
#define PF_INET6       AF_INET6

/* Socket types — match Zephyr NET_SOCK_*. */
#define SOCK_STREAM    1
#define SOCK_DGRAM     2
#define SOCK_RAW       3

/* shutdown() how values. */
#define SHUT_RD        0
#define SHUT_WR        1
#define SHUT_RDWR      2

/* setsockopt levels. */
#define SOL_SOCKET     1

/* setsockopt SOL_SOCKET option names. */
#define SO_DEBUG          1
#define SO_REUSEADDR      2
#define SO_TYPE           3
#define SO_ERROR          4
#define SO_DONTROUTE      5
#define SO_BROADCAST      6
#define SO_SNDBUF         7
#define SO_RCVBUF         8
#define SO_KEEPALIVE      9
#define SO_OOBINLINE      10
#define SO_LINGER         13
#define SO_REUSEPORT      15
#define SO_RCVLOWAT       18
#define SO_SNDLOWAT       19
#define SO_RCVTIMEO       20
#define SO_SNDTIMEO       21
#define SO_ACCEPTCONN     30
#define SO_PROTOCOL       38
#define SO_DOMAIN         39
#define SO_BINDTODEVICE   25
#define SO_PRIORITY       12
#define SO_TIMESTAMPING   37
#define SO_TIMESTAMPNS    35
#define SO_TXTIME         61
#define SO_RCVBUFFORCE    33
#define SO_SNDBUFFORCE    32

/* recv/send flags. */
#define MSG_PEEK       0x02
#define MSG_TRUNC      0x20
#define MSG_DONTWAIT   0x40
#define MSG_WAITALL    0x100
#define MSG_CTRUNC     0x08
#define MSG_OOB        0x01
#define MSG_EOR        0x80
#define MSG_ERRQUEUE   0x2000
#define MSG_NOSIGNAL   0x4000

/* Linger payload for SO_LINGER. */
struct linger {
    int l_onoff;
    int l_linger;
};

/* Generic sockaddr.  Cast to sockaddr_in / sockaddr_in6 by callers. */
struct sockaddr {
    sa_family_t sa_family;
    char        sa_data[14];
};

struct sockaddr_storage {
    sa_family_t ss_family;
    char        __ss_padding[126];
} __attribute__((aligned(8)));

/* iovec + msghdr — used by sendmsg/recvmsg. */
struct iovec {
    void  *iov_base;
    size_t iov_len;
};

struct msghdr {
    void         *msg_name;
    socklen_t     msg_namelen;
    struct iovec *msg_iov;
    size_t        msg_iovlen;
    void         *msg_control;
    size_t        msg_controllen;
    int           msg_flags;
};

struct cmsghdr {
    socklen_t cmsg_len;
    int       cmsg_level;
    int       cmsg_type;
};

/* CMSG_ helpers (minimal). */
#define CMSG_ALIGN(len) (((len) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1))
#define CMSG_SPACE(len) (CMSG_ALIGN(len) + CMSG_ALIGN(sizeof(struct cmsghdr)))
#define CMSG_LEN(len)   (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#define CMSG_DATA(cmsg) ((unsigned char *)((cmsg) + 1))
#define CMSG_FIRSTHDR(mhdr) \
    ((size_t)(mhdr)->msg_controllen >= sizeof(struct cmsghdr) \
     ? (struct cmsghdr *)(mhdr)->msg_control : (struct cmsghdr *)0)
#define CMSG_NXTHDR(mhdr, cmsg) ((struct cmsghdr *)0)

int     accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int     bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int     connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int     getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int     getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int     getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int     listen(int sockfd, int backlog);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
int     setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int     shutdown(int sockfd, int how);
int     socket(int domain, int type, int protocol);
int     socketpair(int domain, int type, int protocol, int sv[2]);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_SYS_SOCKET_H */
