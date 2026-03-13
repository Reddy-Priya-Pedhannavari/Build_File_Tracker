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
static __thread int in_hook = 0;

// Raw syscall fallback used when real pointers are not yet available
static inline int raw_openat(int dirfd, const char* path, int flags, mode_t mode) {
#if defined(__linux__) && defined(SYS_openat)
    return (int)syscall(SYS_openat, dirfd, path, flags, mode);
#else
    errno = ENOSYS;
    return -1;
#endif
}

// Runs once when the shared library is loaded - before any application code.
// Using __attribute__((constructor)) eliminates all lazy-init race conditions.
__attribute__((constructor))
static void interceptor_init(void) {
    in_hook = 1; // block recursive hook calls triggered by dlsym
    real_open    = (int   (*)(const char*, int, ...))      dlsym(RTLD_NEXT, "open");
    real_open64  = (int   (*)(const char*, int, ...))      dlsym(RTLD_NEXT, "open64");
    real_openat  = (int   (*)(int, const char*, int, ...)) dlsym(RTLD_NEXT, "openat");
    real_fopen   = (FILE* (*)(const char*, const char*))   dlsym(RTLD_NEXT, "fopen");
    real_fopen64 = (FILE* (*)(const char*, const char*))   dlsym(RTLD_NEXT, "fopen64");
    in_hook = 0;
}

// Intercepted open
int open(const char* pathname, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap; va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    if (in_hook || !real_open)
        return raw_openat(AT_FDCWD, pathname, flags, mode);

    in_hook = 1;
    if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR)
        track_file_access(pathname);
    in_hook = 0;

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
    if (in_hook || !real_open64)
        return raw_openat(AT_FDCWD, pathname, flags | O_LARGEFILE, mode);

    in_hook = 1;
    if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR)
        track_file_access(pathname);
    in_hook = 0;

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
    if (in_hook || !real_openat)
        return raw_openat(dirfd, pathname, flags, mode);

    in_hook = 1;
    if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR)
        track_file_access(pathname);
    in_hook = 0;

    return mode ? real_openat(dirfd, pathname, flags, mode)
                : real_openat(dirfd, pathname, flags);
}

// Intercepted fopen
FILE* fopen(const char* pathname, const char* mode) {
    if (in_hook || !real_fopen) {
        if (real_fopen) return real_fopen(pathname, mode);
        errno = ENOSYS; return NULL;
    }
    in_hook = 1;
    if (mode && (mode[0] == 'r' || strchr(mode, '+')))
        track_file_access(pathname);
    in_hook = 0;
    return real_fopen(pathname, mode);
}

// Intercepted fopen64
FILE* fopen64(const char* pathname, const char* mode) {
    if (in_hook || !real_fopen64) {
        if (real_fopen64) return real_fopen64(pathname, mode);
        errno = ENOSYS; return NULL;
    }
    in_hook = 1;
    if (mode && (mode[0] == 'r' || strchr(mode, '+')))
        track_file_access(pathname);
    in_hook = 0;
    return real_fopen64(pathname, mode);
}


