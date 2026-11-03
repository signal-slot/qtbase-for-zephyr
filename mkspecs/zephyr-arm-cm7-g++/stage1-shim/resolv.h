/* Stage 1 stub: <resolv.h> — legacy BIND-style DNS API.
 *
 * Zephyr has no resolver library, only the getaddrinfo() front in
 * CONFIG_DNS_RESOLVER.  Qt's qnet_unix_p.h:36 includes <resolv.h>
 * unconditionally (`#if !VXWORKS`), so we provide an empty header.
 * Nothing in qtbase actually calls res_init() / res_query() etc.
 */
#ifndef QZEPHYR_STAGE1_RESOLV_H
#define QZEPHYR_STAGE1_RESOLV_H
#endif
