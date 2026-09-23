/* Stage 1 stub: <arpa/inet.h> — IP address presentation conversions.
 * Bodies linked at Stage 2 via Zephyr's CONFIG_NET_SOCKETS_POSIX_NAMES.
 */
#ifndef QZEPHYR_STAGE1_ARPA_INET_H
#define QZEPHYR_STAGE1_ARPA_INET_H

#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t        htonl(uint32_t hostlong);
uint16_t        htons(uint16_t hostshort);
uint32_t        ntohl(uint32_t netlong);
uint16_t        ntohs(uint16_t netshort);

in_addr_t       inet_addr(const char *cp);
char           *inet_ntoa(struct in_addr in);
int             inet_pton(int af, const char *src, void *dst);
const char     *inet_ntop(int af, const void *src, char *dst, socklen_t size);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_ARPA_INET_H */
