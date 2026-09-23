/* Stage 1 stub: <netdb.h> — DNS resolution (getaddrinfo, gethostbyname).
 * Bodies linked at Stage 2 via Zephyr's CONFIG_DNS_RESOLVER.
 *
 * Zephyr does NOT provide gethostbyaddr_r — only the synchronous
 * getaddrinfo() / freeaddrinfo() / getnameinfo() set.  Qt's QHostInfo
 * Unix backend gates the legacy gethostbyaddr_r path on a feature test
 * that fails on Zephyr (no _GNU_SOURCE bundle), so the missing call is
 * benign at link time.
 */
#ifndef QZEPHYR_STAGE1_NETDB_H
#define QZEPHYR_STAGE1_NETDB_H

#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NI_MAXHOST  1025
#define NI_MAXSERV  32

#define NI_NUMERICHOST  0x01
#define NI_NUMERICSERV  0x02
#define NI_NOFQDN       0x04
#define NI_NAMEREQD     0x08
#define NI_DGRAM        0x10

#define AI_PASSIVE      0x0001
#define AI_CANONNAME    0x0002
#define AI_NUMERICHOST  0x0004
#define AI_V4MAPPED     0x0008
#define AI_ALL          0x0010
#define AI_ADDRCONFIG   0x0020
#define AI_NUMERICSERV  0x0400

/* getaddrinfo() error codes — match Zephyr DNS_EAI_*. */
#define EAI_BADFLAGS    -1
#define EAI_NONAME      -2
#define EAI_AGAIN       -3
#define EAI_FAIL        -4
#define EAI_NODATA      -5
#define EAI_FAMILY      -6
#define EAI_SOCKTYPE    -7
#define EAI_SERVICE     -8
#define EAI_MEMORY      -10
#define EAI_SYSTEM      -11
#define EAI_OVERFLOW    -12

struct addrinfo {
    int               ai_flags;
    int               ai_family;
    int               ai_socktype;
    int               ai_protocol;
    socklen_t         ai_addrlen;
    struct sockaddr  *ai_addr;
    char             *ai_canonname;
    struct addrinfo  *ai_next;
};

struct hostent {
    char  *h_name;
    char **h_aliases;
    int    h_addrtype;
    int    h_length;
    char **h_addr_list;
};

int             getaddrinfo(const char *node, const char *service,
                            const struct addrinfo *hints, struct addrinfo **res);
void            freeaddrinfo(struct addrinfo *res);
int             getnameinfo(const struct sockaddr *sa, socklen_t salen,
                            char *host, socklen_t hostlen,
                            char *serv, socklen_t servlen, int flags);
const char     *gai_strerror(int errcode);

struct hostent *gethostbyname(const char *name);
struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_NETDB_H */
