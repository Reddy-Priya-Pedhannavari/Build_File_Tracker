#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
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
    
    real_open = (int (*)(const char*, int, ...))dlsym(RTLD_NEXT, "open");
    real_open64 = (int (*)(const char*, int, ...))dlsym(RTLD_NEXT, "open64");
    real_fopen = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen");
    real_fopen64 = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen64");
    real_access = (int (*)(const char*, int))dlsym(RTLD_NEXT, "access");
    real_stat = (int (*)(const char*, struct stat*))dlsym(RTLD_NEXT, "stat");
    real_lstat = (int (*)(const char*, struct stat*))dlsym(RTLD_NEXT, "lstat");
    
    tracker_init();
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
    
    if (mode) {
        return real_open(pathname, flags, mode);
    } else {
        return real_open(pathname, flags);
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
    
    if (mode) {
        return real_open64(pathname, flags, mode);
    } else {
        return real_open64(pathname, flags);
    }
}

// Intercepted fopen function
FILE* fopen(const char* pathname, const char* mode) {
    init_interceptor();
    
    // Track if opening for reading
    if (mode && (mode[0] == 'r' || strchr(mode, '+'))) {
        track_file_access(pathname);
    }
    
    return real_fopen(pathname, mode);
}

// Intercepted fopen64 function
FILE* fopen64(const char* pathname, const char* mode) {
    init_interceptor();
    
    // Track if opening for reading
    if (mode && (mode[0] == 'r' || strchr(mode, '+'))) {
        track_file_access(pathname);
    }
    
    return real_fopen64(pathname, mode);
}

// Intercepted access function
int access(const char* pathname, int mode) {
    init_interceptor();
    track_file_access(pathname);
    return real_access(pathname, mode);
}

// Intercepted stat function
int stat(const char* pathname, struct stat* statbuf) {
    init_interceptor();
    track_file_access(pathname);
    return real_stat(pathname, statbuf);
}

// Intercepted lstat function
int lstat(const char* pathname, struct stat* statbuf) {
    init_interceptor();
    track_file_access(pathname);
    return real_lstat(pathname, statbuf);
}
