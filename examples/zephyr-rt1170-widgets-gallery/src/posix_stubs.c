/*
 * libc function stubs the Tier 2 picture-less smoke test needs at link
 * time but never actually calls at runtime.
 *
 * Picolibc on Cortex-M7 (the default libc on RT1170-EVKB) does not
 * provide a number of POSIX functions that Qt corelib references
 * unconditionally from its UNIX file engine / lockfile / sysinfo
 * paths.  None of these are on the QCoreApplication + QTimer dispatch
 * path that this test exercises, so weak stubs returning -1/ENOSYS
 * (or a benign zero/NULL) are enough to let ld finish.  A future Tier
 * that actually needs filesystem locking, real hostname, etc. can
 * provide strong overrides.
 *
 * All symbols are weak so picolibc/newlib can override them at link
 * time on host builds (where these exist for real).
 */

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

#define STUB_FAIL do { errno = ENOSYS; return -1; } while (0)

__attribute__((weak)) int gethostname(char *name, size_t len)         { (void)name; (void)len; STUB_FAIL; }
/* QCoreApplication ctor on Q_OS_UNIX calls getuid() to decide whether
 * the app is running as root.  Picolibc does not provide these; report
 * a benign non-root uid/gid so the path through QCoreApplicationPrivate
 * doesn't hit a wild-pointer jump. */
__attribute__((weak)) uid_t getuid(void)                              { return 1000; }
__attribute__((weak)) uid_t geteuid(void)                             { return 1000; }
__attribute__((weak)) gid_t getgid(void)                              { return 1000; }
__attribute__((weak)) gid_t getegid(void)                             { return 1000; }
__attribute__((weak)) int truncate(const char *p, off_t len)          { (void)p; (void)len; STUB_FAIL; }
__attribute__((weak)) int flock(int fd, int op)                       { (void)fd; (void)op; STUB_FAIL; }
__attribute__((weak)) int fchmod(int fd, mode_t m)                    { (void)fd; (void)m; STUB_FAIL; }
__attribute__((weak)) int chmod(const char *p, mode_t m)              { (void)p; (void)m; STUB_FAIL; }
__attribute__((weak)) int access(const char *p, int m)                { (void)p; (void)m; STUB_FAIL; }
__attribute__((weak)) int link(const char *o, const char *n)          { (void)o; (void)n; STUB_FAIL; }
__attribute__((weak)) int symlink(const char *t, const char *l)       { (void)t; (void)l; STUB_FAIL; }
__attribute__((weak)) ssize_t readlink(const char *p, char *b, size_t s) { (void)p; (void)b; (void)s; STUB_FAIL; }
__attribute__((weak)) char *realpath(const char *p, char *r)          { (void)p; (void)r; errno = ENOSYS; return NULL; }
__attribute__((weak)) char *getcwd(char *b, size_t s)                 { (void)b; (void)s; errno = ENOSYS; return NULL; }

/*
 * passwd / group entries.  Picolibc's <pwd.h> / <grp.h> may not be on
 * the include path; declare the minimal struct shape we need locally.
 */
struct passwd; struct group;
__attribute__((weak)) struct passwd *getpwuid(uid_t uid)              { (void)uid; errno = ENOSYS; return NULL; }
__attribute__((weak)) struct group  *getgrgid(gid_t gid)              { (void)gid; errno = ENOSYS; return NULL; }
