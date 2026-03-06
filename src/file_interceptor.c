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
#include <sys/syscall.h>

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

// Prevent recursive initialization during library loading
static __thread int init_in_progress = 0;
static int initializing = 0;

// Initialize function pointers
static void init_interceptor(void) {
    static int initialized = 0;
    
    // Return early if already initialized
    if (initialized) return;
    
    // Prevent recursive calls during initialization
    if (init_in_progress || initializing) return;
    initializing = 1;
    
    // We must use syscall directly to avoid recursion
    // dlsym itself may call file functions which would cause infinite recursion
    // So we use a simple caching approach
    
    // Try to resolve functions only once, and cache the results
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
    
    initializing = 0;
    initialized = 1;
}

// Intercepted open function
int open(const char* pathname, int flags, ...) {
    // Prevent recursion during initialization
    if (initializing || init_in_progress) {
        // During library loading, use syscall directly
        va_list args;
        va_start(args, flags);
        mode_t mode = (flags & O_CREAT) ? va_arg(args, mode_t) : 0;
        va_end(args);
        
        // Call the real function directly
        if (mode) {
            return syscall(SYS_open, pathname, flags, mode);
        } else {
            return syscall(SYS_open, pathname, flags);
        }
    }
    
    init_in_progress = 1;
    init_interceptor();
    init_in_progress = 0;
    
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
    // Prevent recursion during initialization
    if (initializing || init_in_progress) {
        // During library loading, use syscall directly
        va_list args;
        va_start(args, flags);
        mode_t mode = (flags & O_CREAT) ? va_arg(args, mode_t) : 0;
        va_end(args);
        
        // Call the real function directly
        if (mode) {
            return syscall(SYS_open, pathname, flags | O_LARGEFILE, mode);
        } else {
            return syscall(SYS_open, pathname, flags | O_LARGEFILE);
        }
    }
    
    init_in_progress = 1;
    init_interceptor();
    init_in_progress = 0;
    
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
    // Prevent recursion during initialization
    if (initializing || init_in_progress) {
        // During library loading, call libc fopen directly via dlsym
        static FILE* (*libc_fopen)(const char*, const char*) = NULL;
        if (!libc_fopen) {
            libc_fopen = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen");
        }
        if (libc_fopen) {
            return libc_fopen(pathname, mode);
        }
        return NULL;
    }
    
    init_in_progress = 1;
    init_interceptor();
    init_in_progress = 0;
    
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
    // Prevent recursion during initialization
    if (initializing || init_in_progress) {
        // During library loading, call libc fopen64 directly via dlsym
        static FILE* (*libc_fopen64)(const char*, const char*) = NULL;
        if (!libc_fopen64) {
            libc_fopen64 = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen64");
        }
        if (libc_fopen64) {
            return libc_fopen64(pathname, mode);
        }
        return NULL;
    }
    
    init_in_progress = 1;
    init_interceptor();
    init_in_progress = 0;
    
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
    // Prevent recursion during initialization
    if (initializing || init_in_progress) {
        static int (*libc_access)(const char*, int) = NULL;
        if (!libc_access) {
            libc_access = (int (*)(const char*, int))dlsym(RTLD_NEXT, "access");
        }
        if (libc_access) {
            return libc_access(pathname, mode);
        }
        return -1;
    }
    
    init_in_progress = 1;
    init_interceptor();
    init_in_progress = 0;
    
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
    // Prevent recursion during initialization
    if (initializing || init_in_progress) {
        static int (*libc_stat)(const char*, struct stat*) = NULL;
        if (!libc_stat) {
            libc_stat = (int (*)(const char*, struct stat*))dlsym(RTLD_NEXT, "stat");
        }
        if (libc_stat) {
            return libc_stat(pathname, statbuf);
        }
        return -1;
    }
    
    init_in_progress = 1;
    init_interceptor();
    init_in_progress = 0;
    
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
    // Prevent recursion during initialization
    if (initializing || init_in_progress) {
        static int (*libc_lstat)(const char*, struct stat*) = NULL;
        if (!libc_lstat) {
            libc_lstat = (int (*)(const char*, struct stat*))dlsym(RTLD_NEXT, "lstat");
        }
        if (libc_lstat) {
            return libc_lstat(pathname, statbuf);
        }
        return -1;
    }
    
    init_in_progress = 1;
    init_interceptor();
    init_in_progress = 0;
    
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
