/* Stage 1 stub: <pthread.h>
 *
 * Zephyr SDK's newlib/picolibc ship no <pthread.h>.  Qt's
 * qv4stacklimits.cpp (in qtdeclarative) includes it on every
 * Q_OS_UNIX platform but only uses it inside Linux/Darwin/etc
 * branches we don't reach.  Provide just enough types/prototypes
 * for the file to parse; the Cortex-M7 build has FEATURE_thread=OFF
 * so no real threading code is reached at runtime.
 */
#ifndef QZEPHYR_STAGE1_PTHREAD_H
#define QZEPHYR_STAGE1_PTHREAD_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long pthread_t;
typedef int           pthread_attr_t;
typedef int           pthread_key_t;
typedef int           pthread_mutex_t;
typedef int           pthread_mutexattr_t;
typedef int           pthread_cond_t;
typedef int           pthread_condattr_t;
typedef int           pthread_once_t;

#define PTHREAD_ONCE_INIT 0
#define PTHREAD_MUTEX_INITIALIZER 0
#define PTHREAD_COND_INITIALIZER  0

pthread_t pthread_self(void);

#ifdef __cplusplus
}
#endif

#endif /* QZEPHYR_STAGE1_PTHREAD_H */
