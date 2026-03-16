#define _GNU_SOURCE

/*
 * Only openat is intercepted.
 *
 * On Linux/glibc 2.17+, ALL file-open paths (open, open64, fopen, fopen64,
 * openat) ultimately reach the openat(2) syscall through glibc. Hooking just
 * openat therefore captures every file access made by the build.
 *
 * Hooking open/open64/fopen separately causes symbol-version conflicts on
 * glibc 2.27 (Ubuntu 18.04) that crash programs compiled with
 * _FILE_OFFSET_BITS=64 (e.g. bash, python), so those hooks are removed.
 */

#ifdef __linux__
#include <dlfcn.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1L)
#endif
#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif
#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

#include "file_tracker.h"

/* Real openat pointer - resolved in constructor */
static int (*real_openat)(int, const char*, int, ...) = NULL;

/* Set to 1 once safe to track (after constructor completes) */
static volatile int lib_ready = 0;

/* Non-recursive guard: prevents track_file_access being called while
 * we are already inside a hook (e.g. triggered by malloc/pthread in tracker) */
static volatile int in_hook = 0;

/* Raw kernel fallback - used only when real_openat is not yet resolved */
static inline int raw_openat(int dirfd, const char* path, int flags, mode_t mode) {
#if defined(__linux__) && defined(SYS_openat)
    return (int)syscall(SYS_openat, dirfd, path, flags, mode);
#else
    errno = ENOSYS;
    return -1;
#endif
}

/* Run with lowest constructor priority so ALL other library constructors,
 * including glibc's own, complete before we install our hook. */
__attribute__((constructor(65535)))
static void interceptor_init(void) {
    real_openat = (int (*)(int, const char*, int, ...)) dlsym(RTLD_NEXT, "openat");
    lib_ready = 1;
}

/* ---- Intercepted openat -------------------------------------------------- */
int openat(int dirfd, const char* pathname, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }

    /* Resolve lazily in case something calls us before the constructor ran */
    if (!real_openat)
        real_openat = (int (*)(int, const char*, int, ...)) dlsym(RTLD_NEXT, "openat");

    /* Always call through - tracking is best-effort, never impede the caller */
    if (lib_ready && !in_hook) {
        if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR) {
            in_hook = 1;
            track_file_access(pathname);
            in_hook = 0;
        }
    }

    if (real_openat)
        return mode ? real_openat(dirfd, pathname, flags, mode)
                    : real_openat(dirfd, pathname, flags);

    return raw_openat(dirfd, pathname, flags, mode);
}
