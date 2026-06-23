/* Stage 1 stub: <netinet/tcp.h> — TCP setsockopt options.  Bodies linked
 * at Stage 2 via Zephyr's CONFIG_NET_SOCKETS_POSIX_NAMES.
 */
#ifndef QZEPHYR_STAGE1_NETINET_TCP_H
#define QZEPHYR_STAGE1_NETINET_TCP_H

#define TCP_NODELAY        1
#define TCP_MAXSEG         2
#define TCP_KEEPIDLE       4
#define TCP_KEEPINTVL      5
#define TCP_KEEPCNT        6
#define TCP_USER_TIMEOUT   18
#define TCP_CONGESTION     13
#define TCP_FASTOPEN       23
#define TCP_FASTOPEN_CONNECT 30

#endif /* QZEPHYR_STAGE1_NETINET_TCP_H */
