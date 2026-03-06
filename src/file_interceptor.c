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

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1L)
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include "file_tracker.h"

// Function pointers for original functions
static int (*real_open)(const char*, int, ...) = NULL;
static int (*real_open64)(const char*, int, ...) = NULL;
static FILE* (*real_fopen)(const char*, const char*) = NULL;
static FILE* (*real_fopen64)(const char*, const char*) = NULL;
static int (*real_access)(const char*, int) = NULL;
static int (*real_stat)(const char*, struct stat*) = NULL;
static int (*real_lstat)(const char*, struct stat*) = NULL;

// Initialize function pointers
static void init_interceptor(void) {
    static int initialized = 0;
    if (initialized) return;
    
    // Initialize function pointers safely
    if (!real_open) {
        real_open = (int (*)(const char*, int, ...))dlsym(RTLD_NEXT, "open");
    }
    if (!real_open64) {
        real_open64 = (int (*)(const char*, int, ...))dlsym(RTLD_NEXT, "open64");
    }
    if (!real_fopen) {
        real_fopen = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen");
    }
    if (!real_fopen64) {
        real_fopen64 = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen64");
    }
    if (!real_access) {
        real_access = (int (*)(const char*, int))dlsym(RTLD_NEXT, "access");
    }
    if (!real_stat) {
        real_stat = (int (*)(const char*, struct stat*))dlsym(RTLD_NEXT, "stat");
    }
    if (!real_lstat) {
        real_lstat = (int (*)(const char*, struct stat*))dlsym(RTLD_NEXT, "lstat");
    }
    
    initialized = 1;
}

// Intercepted open function
int open(const char* pathname, int flags, ...) {
    init_interceptor();
    
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    
    // Track only read operations
    if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR) {
        track_file_access(pathname);
    }
    
    // Call real function with NULL check
    if (real_open) {
        if (mode) {
            return real_open(pathname, flags, mode);
        } else {
            return real_open(pathname, flags);
        }
    } else {
        // Fallback if dlsym failed
        errno = ENOSYS;
        return -1;
    }
}

// Intercepted open64 function
int open64(const char* pathname, int flags, ...) {
    init_interceptor();
    
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    
    // Track only read operations
    if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR) {
        track_file_access(pathname);
    }
    
    // Call real function with NULL check
    if (real_open64) {
        if (mode) {
            return real_open64(pathname, flags, mode);
        } else {
            return real_open64(pathname, flags);
        }
    } else {
        // Fallback if dlsym failed
        errno = ENOSYS;
        return -1;
    }
}

// Intercepted fopen function
FILE* fopen(const char* pathname, const char* mode) {
    init_interceptor();
    
    // Track if opening for reading
    if (mode && (mode[0] == 'r' || strchr(mode, '+'))) {
        track_file_access(pathname);
    }
    
    // Call real function with NULL check
    if (real_fopen) {
        return real_fopen(pathname, mode);
    } else {
        // Fallback if dlsym failed
        errno = ENOSYS;
        return NULL;
    }
}

// Intercepted fopen64 function
FILE* fopen64(const char* pathname, const char* mode) {
    init_interceptor();
    
    // Track if opening for reading
    if (mode && (mode[0] == 'r' || strchr(mode, '+'))) {
        track_file_access(pathname);
    }
    
    // Call real function with NULL check
    if (real_fopen64) {
        return real_fopen64(pathname, mode);
    } else {
        // Fallback if dlsym failed
        errno = ENOSYS;
        return NULL;
    }
}

// Intercepted access function
int access(const char* pathname, int mode) {
    init_interceptor();
    track_file_access(pathname);
    
    // Call real function with NULL check
    if (real_access) {
        return real_access(pathname, mode);
    } else {
        // Fallback if dlsym failed
        errno = ENOSYS;
        return -1;
    }
}

// Intercepted stat function
int stat(const char* pathname, struct stat* statbuf) {
    init_interceptor();
    track_file_access(pathname);
    
    // Call real function with NULL check
    if (real_stat) {
        return real_stat(pathname, statbuf);
    } else {
        // Fallback if dlsym failed
        errno = ENOSYS;
        return -1;
    }
}

// Intercepted lstat function
int lstat(const char* pathname, struct stat* statbuf) {
    init_interceptor();
    track_file_access(pathname);
    
    // Call real function with NULL check
    if (real_lstat) {
        return real_lstat(pathname, statbuf);
    } else {
        // Fallback if dlsym failed
        errno = ENOSYS;
        return -1;
    }
}
