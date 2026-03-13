#define _GNU_SOURCE

#ifdef __linux__
#include <dlfcn.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
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

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifdef __linux__
#include <sys/syscall.h>
#ifndef SYS_openat
#ifdef __NR_openat
#define SYS_openat __NR_openat
#endif
#endif
#endif

#include "file_tracker.h"

// Real function pointers resolved once at library load via constructor
static int   (*real_open)   (const char*, int, ...)       = NULL;
static int   (*real_open64) (const char*, int, ...)       = NULL;
static int   (*real_openat) (int, const char*, int, ...)  = NULL;
static FILE* (*real_fopen)  (const char*, const char*)    = NULL;
static FILE* (*real_fopen64)(const char*, const char*)    = NULL;

// Thread-local reentrancy guard - prevents recursive hook calls
// NOTE: must NOT be __thread - TLS is not ready during early bash/glibc startup
// on Ubuntu 18.04 (glibc 2.27), causing a segfault. track_file_access() has
// its own __thread guard (tracking_in_progress) that prevents actual recursion.
static volatile int in_hook = 0;

// Set to 1 once constructor finishes - all hooks bail out until then
static volatile int lib_ready = 0;

// Raw syscall fallback used when real pointers are not yet available
static inline int raw_openat(int dirfd, const char* path, int flags, mode_t mode) {
#if defined(__linux__) && defined(SYS_openat)
    return (int)syscall(SYS_openat, dirfd, path, flags, mode);
#else
    errno = ENOSYS;
    return -1;
#endif
}

// Resolve all real function pointers.
// Called from constructor AND lazily from each hook as fallback.
static void resolve_funcs(void) {
    if (!real_open)
        real_open    = (int   (*)(const char*, int, ...))      dlsym(RTLD_NEXT, "open");
    if (!real_open64)
        real_open64  = (int   (*)(const char*, int, ...))      dlsym(RTLD_NEXT, "open64");
    if (!real_openat)
        real_openat  = (int   (*)(int, const char*, int, ...)) dlsym(RTLD_NEXT, "openat");
    if (!real_fopen)
        real_fopen   = (FILE* (*)(const char*, const char*))   dlsym(RTLD_NEXT, "fopen");
    if (!real_fopen64)
        real_fopen64 = (FILE* (*)(const char*, const char*))   dlsym(RTLD_NEXT, "fopen64");
}

// Run with lowest user priority (65535 = last) so ALL other library constructors
// have already run before ours. This avoids crashes from early constructor calls.
__attribute__((constructor(65535)))
static void interceptor_init(void) {
    resolve_funcs();
    lib_ready = 1;
}

// Intercepted open
int open(const char* pathname, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap; va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    // Lazily resolve if constructor hasn't run yet
    if (!real_open) resolve_funcs();
    // Must always call through - never fail when real function is available
    if (in_hook || !real_open)
        return raw_openat(AT_FDCWD, pathname, flags, mode);

    if (lib_ready) {
        in_hook = 1;
        if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR)
            track_file_access(pathname);
        in_hook = 0;
    }
    return mode ? real_open(pathname, flags, mode) : real_open(pathname, flags);
}

// Intercepted open64
int open64(const char* pathname, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap; va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    if (!real_open64) resolve_funcs();
    if (in_hook || !real_open64)
        return raw_openat(AT_FDCWD, pathname, flags | O_LARGEFILE, mode);

    if (lib_ready) {
        in_hook = 1;
        if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR)
            track_file_access(pathname);
        in_hook = 0;
    }
    return mode ? real_open64(pathname, flags, mode) : real_open64(pathname, flags);
}

// Intercepted openat
int openat(int dirfd, const char* pathname, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap; va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    if (!real_openat) resolve_funcs();
    if (in_hook || !real_openat)
        return raw_openat(dirfd, pathname, flags, mode);

    if (lib_ready) {
        in_hook = 1;
        if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR)
            track_file_access(pathname);
        in_hook = 0;
    }
    return mode ? real_openat(dirfd, pathname, flags, mode)
                : real_openat(dirfd, pathname, flags);
}

// Intercepted fopen
FILE* fopen(const char* pathname, const char* mode) {
    if (!real_fopen) resolve_funcs();
    // CRITICAL: always call real_fopen - never return NULL due to our overhead
    if (!real_fopen) { errno = ENOSYS; return NULL; }
    if (lib_ready && !in_hook) {
        in_hook = 1;
        if (mode && (mode[0] == 'r' || strchr(mode, '+')))
            track_file_access(pathname);
        in_hook = 0;
    }
    return real_fopen(pathname, mode);
}

// Intercepted fopen64
FILE* fopen64(const char* pathname, const char* mode) {
    if (!real_fopen64) resolve_funcs();
    if (!real_fopen64) { errno = ENOSYS; return NULL; }
    if (lib_ready && !in_hook) {
        in_hook = 1;
        if (mode && (mode[0] == 'r' || strchr(mode, '+')))
            track_file_access(pathname);
        in_hook = 0;
    }
    return real_fopen64(pathname, mode);
}


