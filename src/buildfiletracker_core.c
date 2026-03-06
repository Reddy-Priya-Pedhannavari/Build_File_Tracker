/* BuildFileTracker - Core Implementation with Multiple Backends
 * Copyright (c) 2026 BuildFileTracker Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define _GNU_SOURCE
#include "buildfiletracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <fnmatch.h>
#include <limits.h>
#include <errno.h>

#ifdef __linux__
#include <sys/inotify.h>
#include <sys/fanotify.h>
#include <sys/capability.h>
#endif

/* Internal hash table size */
#define HASH_TABLE_SIZE 10000
#define EVENT_BUFFER_SIZE 8192
#define MAX_WATCHES 1024

/* Internal structures */
typedef struct file_record {
    char                *filepath;
    char                *package_name;
    uint32_t            access_flags;
    struct timespec     first_access;
    struct timespec     last_access;
    uint32_t            access_count;
    struct file_record  *next;
} file_record_t;

typedef struct package_record {
    char                *name;
    char                *version;
    char                *license;
    char                *homepage;
    char                *supplier;
    char                *source_path;
    char                *checksum;
    size_t              root_path_len;
    uint32_t            total_files;
    struct package_record *next;
} package_record_t;

typedef struct watch_entry {
    int                 wd;         /* Watch descriptor */
    char                *path;
    bool                recursive;
    struct watch_entry  *next;
} watch_entry_t;

typedef struct exclude_pattern {
    char                *pattern;
    struct exclude_pattern *next;
} exclude_pattern_t;

/* Main context structure */
struct bft_context {
    /* Configuration */
    bft_config_t        config;
    bft_backend_t       active_backend;
    
    /* Tracking state */
    bool                running;
    pthread_t           watcher_thread;
    pthread_mutex_t     lock;
    
    /* Backend-specific */
    int                 inotify_fd;
    int                 fanotify_fd;
    
    /* Data structures */
    file_record_t       *file_records[HASH_TABLE_SIZE];
    package_record_t    *packages;
    watch_entry_t       *watches;
    exclude_pattern_t   *excludes;
    char                **extension_filters;
    uint32_t            extension_filter_count;
    
    /* Callbacks */
    bft_event_callback_t    event_callback;
    void                    *event_callback_data;
    bft_package_callback_t  package_callback;
    void                    *package_callback_data;
    
    /* Statistics */
    bft_stats_t         stats;
    
    /* Event buffer */
    bft_event_t         *event_buffer;
    uint32_t            event_buffer_size;
    uint32_t            event_buffer_pos;
};

/* Forward declarations of backend implementations */
static int backend_preload_init(bft_context_t *ctx);
static int backend_inotify_init(bft_context_t *ctx);
static int backend_fanotify_init(bft_context_t *ctx);
static void* watcher_thread_func(void *arg);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static uint32_t hash_string(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_TABLE_SIZE;
}

static char* resolve_path(const char *path) {
    char *resolved = realpath(path, NULL);
    if (!resolved) {
        return strdup(path);
    }
    return resolved;
}

static void get_timestamp(struct timespec *ts) {
    clock_gettime(CLOCK_REALTIME, ts);
}

static double timespec_diff_seconds(struct timespec *start, struct timespec *end) {
    double diff = (end->tv_sec - start->tv_sec);
    diff += (end->tv_nsec - start->tv_nsec) / 1000000000.0;
    return diff;
}

/* ============================================================================
 * CORE API IMPLEMENTATION
 * ============================================================================ */

void bft_config_default(bft_config_t *config) {
    if (!config) return;
    
    memset(config, 0, sizeof(bft_config_t));
    config->backend = BFT_BACKEND_AUTO;
    config->recursive = true;
    config->follow_symlinks = false;
    config->auto_detect_packages = true;
    config->track_dependencies = false;
    config->track_build_phases = false;
    config->capture_env = false;
    config->buffer_size = EVENT_BUFFER_SIZE;
    config->flush_interval_ms = 1000;
    config->output_format = BFT_FORMAT_JSON;
    config->real_time_updates = false;
}

bft_context_t* bft_create(void) {
    bft_config_t config;
    bft_config_default(&config);
    return bft_create_with_config(&config);
}

bft_context_t* bft_create_with_config(const bft_config_t *config) {
    if (!config) return NULL;
    
    bft_context_t *ctx = calloc(1, sizeof(bft_context_t));
    if (!ctx) return NULL;
    
    /* Copy configuration */
    memcpy(&ctx->config, config, sizeof(bft_config_t));
    
    /* Initialize mutex */
    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        free(ctx);
        return NULL;
    }
    
    /* Allocate event buffer */
    ctx->event_buffer_size = config->buffer_size;
    ctx->event_buffer = calloc(ctx->event_buffer_size, sizeof(bft_event_t));
    if (!ctx->event_buffer) {
        pthread_mutex_destroy(&ctx->lock);
        free(ctx);
        return NULL;
    }
    
    /* Initialize statistics */
    get_timestamp(&ctx->stats.start_time);
    
    /* Determine backend */
    if (config->backend == BFT_BACKEND_AUTO) {
        ctx->active_backend = bft_get_recommended_backend();
    } else {
        ctx->active_backend = config->backend;
    }
    
    ctx->inotify_fd = -1;
    ctx->fanotify_fd = -1;
    
    return ctx;
}

int bft_start(bft_context_t *ctx) {
    if (!ctx) return BFT_ERROR_INVALID_PARAM;
    if (ctx->running) return BFT_ERROR_ALREADY_RUNNING;
    
    int result = BFT_OK;
    
    /* Initialize backend */
    switch (ctx->active_backend) {
        case BFT_BACKEND_INOTIFY:
            result = backend_inotify_init(ctx);
            break;
        case BFT_BACKEND_FANOTIFY:
            result = backend_fanotify_init(ctx);
            break;
        case BFT_BACKEND_PRELOAD:
            result = backend_preload_init(ctx);
            break;
        default:
            /* Try inotify first, fall back to preload */
            result = backend_inotify_init(ctx);
            if (result != BFT_OK) {
                result = backend_preload_init(ctx);
                if (result == BFT_OK) {
                    ctx->active_backend = BFT_BACKEND_PRELOAD;
                }
            }
            break;
    }
    
    if (result != BFT_OK) {
        return result;
    }
    
    /* Start watcher thread if using inotify/fanotify */
    if (ctx->active_backend == BFT_BACKEND_INOTIFY || 
        ctx->active_backend == BFT_BACKEND_FANOTIFY) {
        ctx->running = true;
        if (pthread_create(&ctx->watcher_thread, NULL, watcher_thread_func, ctx) != 0) {
            ctx->running = false;
            return BFT_ERROR_BACKEND_INIT;
        }
    } else {
        ctx->running = true;
    }
    
    get_timestamp(&ctx->stats.start_time);
    
    return BFT_OK;
}

int bft_stop(bft_context_t *ctx) {
    if (!ctx) return BFT_ERROR_INVALID_PARAM;
    if (!ctx->running) return BFT_ERROR_NOT_RUNNING;
    
    ctx->running = false;
    
    /* Wait for watcher thread */
    if (ctx->active_backend == BFT_BACKEND_INOTIFY || 
        ctx->active_backend == BFT_BACKEND_FANOTIFY) {
        pthread_join(ctx->watcher_thread, NULL);
    }
    
    /* Record end time */
    get_timestamp(&ctx->stats.end_time);
    ctx->stats.runtime_seconds = timespec_diff_seconds(
        &ctx->stats.start_time, &ctx->stats.end_time
    );
    
    /* Flush any pending events */
    bft_flush(ctx);
    
    return BFT_OK;
}

bool bft_is_running(bft_context_t *ctx) {
    return ctx ? ctx->running : false;
}

void bft_destroy(bft_context_t *ctx) {
    if (!ctx) return;
    
    if (ctx->running) {
        bft_stop(ctx);
    }
    
    /* Close file descriptors */
    if (ctx->inotify_fd >= 0) {
        close(ctx->inotify_fd);
    }
    if (ctx->fanotify_fd >= 0) {
        close(ctx->fanotify_fd);
    }
    
    /* Free file records */
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        file_record_t *record = ctx->file_records[i];
        while (record) {
            file_record_t *next = record->next;
            free(record->filepath);
            free(record->package_name);
            free(record);
            record = next;
        }
    }
    
    /* Free packages */
    package_record_t *pkg = ctx->packages;
    while (pkg) {
        package_record_t *next = pkg->next;
        free(pkg->name);
        free(pkg->version);
        free(pkg->license);
        free(pkg->homepage);
        free(pkg->supplier);
        free(pkg->source_path);
        free(pkg->checksum);
        free(pkg);
        pkg = next;
    }
    
    /* Free watches */
    watch_entry_t *watch = ctx->watches;
    while (watch) {
        watch_entry_t *next = watch->next;
        free(watch->path);
        free(watch);
        watch = next;
    }
    
    /* Free excludes */
    exclude_pattern_t *exclude = ctx->excludes;
    while (exclude) {
        exclude_pattern_t *next = exclude->next;
        free(exclude->pattern);
        free(exclude);
        exclude = next;
    }
    
    /* Free extension filters */
    for (uint32_t i = 0; i < ctx->extension_filter_count; i++) {
        free(ctx->extension_filters[i]);
    }
    free(ctx->extension_filters);
    
    /* Free event buffer */
    free(ctx->event_buffer);
    
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
}

/* ============================================================================
 * WATCH PATH MANAGEMENT
 * ============================================================================ */

int bft_add_watch(bft_context_t *ctx, const char *path, bool recursive) {
    if (!ctx || !path) return BFT_ERROR_INVALID_PARAM;
    
    char *resolved = resolve_path(path);
    if (!resolved) return BFT_ERROR_FILE_IO;
    
    watch_entry_t *entry = malloc(sizeof(watch_entry_t));
    entry->path = resolved;
    entry->recursive = recursive;
    entry->wd = -1;
    
    /* If using inotify, add watch now */
    if (ctx->active_backend == BFT_BACKEND_INOTIFY && ctx->inotify_fd >= 0) {
        uint32_t mask = IN_ACCESS | IN_OPEN | IN_CLOSE_NOWRITE | 
                        IN_CLOSE_WRITE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM;
        entry->wd = inotify_add_watch(ctx->inotify_fd, resolved, mask);
        if (entry->wd < 0) {
            free(resolved);
            free(entry);
            return BFT_ERROR_BACKEND_INIT;
        }
    }
    
    pthread_mutex_lock(&ctx->lock);
    entry->next = ctx->watches;
    ctx->watches = entry;
    pthread_mutex_unlock(&ctx->lock);
    
    /* If recursive, add subdirectories */
    if (recursive) {
        DIR *dir = opendir(resolved);
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_type == DT_DIR && 
                    strcmp(ent->d_name, ".") != 0 &&
                    strcmp(ent->d_name, "..") != 0) {
                    char subpath[PATH_MAX];
                    snprintf(subpath, sizeof(subpath), "%s/%s", resolved, ent->d_name);
                    bft_add_watch(ctx, subpath, true);
                }
            }
            closedir(dir);
        }
    }
    
    return BFT_OK;
}

int bft_remove_watch(bft_context_t *ctx, const char *path) {
    if (!ctx || !path) return BFT_ERROR_INVALID_PARAM;
    
    char *resolved = resolve_path(path);
    
    pthread_mutex_lock(&ctx->lock);
    
    watch_entry_t **prev = &ctx->watches;
    watch_entry_t *entry = ctx->watches;
    
    while (entry) {
        if (strcmp(entry->path, resolved) == 0) {
            *prev = entry->next;
            
            if (entry->wd >= 0 && ctx->inotify_fd >= 0) {
                inotify_rm_watch(ctx->inotify_fd, entry->wd);
            }
            
            free(entry->path);
            free(entry);
            free(resolved);
            pthread_mutex_unlock(&ctx->lock);
            return BFT_OK;
        }
        prev = &entry->next;
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&ctx->lock);
    free(resolved);
    return BFT_ERROR_INVALID_PARAM;
}

int bft_add_exclude(bft_context_t *ctx, const char *pattern) {
    if (!ctx || !pattern) return BFT_ERROR_INVALID_PARAM;
    
    exclude_pattern_t *exclude = malloc(sizeof(exclude_pattern_t));
    exclude->pattern = strdup(pattern);
    
    pthread_mutex_lock(&ctx->lock);
    exclude->next = ctx->excludes;
    ctx->excludes = exclude;
    pthread_mutex_unlock(&ctx->lock);
    
    return BFT_OK;
}

void bft_clear_excludes(bft_context_t *ctx) {
    if (!ctx) return;
    
    pthread_mutex_lock(&ctx->lock);
    
    exclude_pattern_t *exclude = ctx->excludes;
    while (exclude) {
        exclude_pattern_t *next = exclude->next;
        free(exclude->pattern);
        free(exclude);
        exclude = next;
    }
    ctx->excludes = NULL;
    
    pthread_mutex_unlock(&ctx->lock);
}

/* Continue in next section... */
const char* bft_version(void) {
    return BFT_VERSION_STRING;
}

const char* bft_strerror(int error) {
    switch (error) {
        case BFT_OK: return "Success";
        case BFT_ERROR_INVALID_PARAM: return "Invalid parameter";
        case BFT_ERROR_NO_MEMORY: return "Out of memory";
        case BFT_ERROR_BACKEND_INIT: return "Backend initialization failed";
        case BFT_ERROR_ALREADY_RUNNING: return "Already running";
        case BFT_ERROR_NOT_RUNNING: return "Not running";
        case BFT_ERROR_FILE_IO: return "File I/O error";
        case BFT_ERROR_PERMISSION: return "Permission denied";
        case BFT_ERROR_NOT_SUPPORTED: return "Not supported";
        case BFT_ERROR_TIMEOUT: return "Timeout";
        default: return "Unknown error";
    }
}

bool bft_backend_available(bft_backend_t backend) {
    switch (backend) {
#ifdef __linux__
        case BFT_BACKEND_INOTIFY:
            return true;
        case BFT_BACKEND_FANOTIFY:
            return (geteuid() == 0); /* Requires root */
#endif
        case BFT_BACKEND_PRELOAD:
            return true;
        case BFT_BACKEND_STRACE:
            return (access("/usr/bin/strace", X_OK) == 0);
        default:
            return false;
    }
}

bft_backend_t bft_get_recommended_backend(void) {
#ifdef __linux__
    if (bft_backend_available(BFT_BACKEND_INOTIFY)) {
        return BFT_BACKEND_INOTIFY;
    }
#endif
    return BFT_BACKEND_PRELOAD;
}

int bft_flush(bft_context_t *ctx) {
    if (!ctx) return BFT_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&ctx->lock);
    int count = ctx->event_buffer_pos;
    ctx->event_buffer_pos = 0;
    pthread_mutex_unlock(&ctx->lock);
    
    return count;
}

/* Backend implementations would continue here...
 * For brevity, showing structure only  */

static int backend_preload_init(bft_context_t *ctx) {
    /* Setup LD_PRELOAD environment */
    return BFT_OK;
}

static int backend_inotify_init(bft_context_t *ctx) {
#ifdef __linux__
    ctx->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (ctx->inotify_fd < 0) {
        return BFT_ERROR_BACKEND_INIT;
    }
    return BFT_OK;
#else
    return BFT_ERROR_NOT_SUPPORTED;
#endif
}

static int backend_fanotify_init(bft_context_t *ctx) {
#ifdef __linux__
    ctx->fanotify_fd = fanotify_init(FAN_CLASS_NOTIF, O_RDONLY);
    if (ctx->fanotify_fd < 0) {
        return BFT_ERROR_BACKEND_INIT;
    }
    return BFT_OK;
#else
    return BFT_ERROR_NOT_SUPPORTED;
#endif
}

static void* watcher_thread_func(void *arg) {
    bft_context_t *ctx = (bft_context_t*)arg;
    
#ifdef __linux__
    if (ctx->active_backend == BFT_BACKEND_INOTIFY) {
        char buffer[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
        
        while (ctx->running) {
            ssize_t len = read(ctx->inotify_fd, buffer, sizeof(buffer));
            if (len < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    break;
                }
                usleep(10000); /* 10ms */
                continue;
            }
            
            const struct inotify_event *event;
            for (char *ptr = buffer; ptr < buffer + len;
                 ptr += sizeof(struct inotify_event) + event->len) {
                event = (const struct inotify_event *) ptr;
                
                if (event->len > 0 && !(event->mask & IN_ISDIR)) {
                    /* Find the watch path */
                    watch_entry_t *watch = ctx->watches;
                    while (watch) {
                        if (watch->wd == event->wd) {
                            char fullpath[4096];
                            snprintf(fullpath, sizeof(fullpath), "%s/%s", 
                                    watch->path, event->name);
                            
                            /* Record event */
                            bft_event_t file_event = {
                                .pid = 0,  /* inotify doesn't provide PID */
                                .timestamp = {0},
                            };
                            get_timestamp(&file_event.timestamp);
                            strncpy(file_event.path, fullpath, sizeof(file_event.path) - 1);
                            
                            /* Determine access type */
                            if (event->mask & (IN_ACCESS | IN_OPEN)) {
                                file_event.access = BFT_ACCESS_READ;
                            } else if (event->mask & (IN_MODIFY | IN_CLOSE_WRITE)) {
                                file_event.access = BFT_ACCESS_WRITE;
                            } else {
                                file_event.access = BFT_ACCESS_STAT;
                            }
                            
                            /* Find package */
                            package_record_t *pkg = find_package_for_path(ctx, fullpath);
                            if (pkg) {
                                strncpy(file_event.package, pkg->name, 
                                       sizeof(file_event.package) - 1);
                            }
                            
                            /* Record event */
                            record_event(ctx, &file_event);
                            break;
                        }
                        watch = watch->next;
                    }
                }
            }
        }
    } else if (ctx->active_backend == BFT_BACKEND_FANOTIFY) {
        char buffer[4096];
        
        while (ctx->running) {
            ssize_t len = read(ctx->fanotify_fd, buffer, sizeof(buffer));
            if (len < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    break;
                }
                usleep(10000);
                continue;
            }
            
            const struct fanotify_event_metadata *metadata;
            metadata = (struct fanotify_event_metadata *) buffer;
            
            while (FAN_EVENT_OK(metadata, len)) {
                if (metadata->fd >= 0) {
                    char procpath[64], filepath[4096];
                    snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", metadata->fd);
                    ssize_t path_len = readlink(procpath, filepath, sizeof(filepath) - 1);
                    
                    if (path_len > 0) {
                        filepath[path_len] = '\0';
                        
                        /* Record event */
                        bft_event_t file_event = {
                            .pid = metadata->pid,
                            .access = (metadata->mask & FAN_OPEN) ? BFT_ACCESS_READ : BFT_ACCESS_STAT,
                            .timestamp = {0},
                        };
                        get_timestamp(&file_event.timestamp);
                        strncpy(file_event.path, filepath, sizeof(file_event.path) - 1);
                        
                        package_record_t *pkg = find_package_for_path(ctx, filepath);
                        if (pkg) {
                            strncpy(file_event.package, pkg->name, 
                                   sizeof(file_event.package) - 1);
                        }
                        
                        record_event(ctx, &file_event);
                    }
                    
                    close(metadata->fd);
                }
                metadata = FAN_EVENT_NEXT(metadata, len);
            }
        }
    }
#endif
    
    return NULL;
}

/* ============================================================================
 * PACKAGE MANAGEMENT
 * ============================================================================ */

int bft_register_package(bft_context_t *ctx, const bft_package_t *pkg) {
    if (!ctx || !pkg || !pkg->name || !pkg->root_path) {
        return BFT_ERROR_INVALID_PARAM;
    }
    
    char *resolved = resolve_path(pkg->root_path);
    if (!resolved) return BFT_ERROR_FILE_IO;
    
    /* Create package record */
    package_record_t *record = calloc(1, sizeof(package_record_t));
    if (!record) {
        free(resolved);
        return BFT_ERROR_MEMORY;
    }
    
    strncpy(record->name, pkg->name, sizeof(record->name) - 1);
    strncpy(record->version, pkg->version ? pkg->version : "unknown", 
            sizeof(record->version) - 1);
    strncpy(record->license, pkg->license ? pkg->license : "unknown",
            sizeof(record->license) - 1);
    record->root_path = resolved;
    
    if (pkg->homepage) {
        record->homepage = strdup(pkg->homepage);
    }
    if (pkg->supplier) {
        record->supplier = strdup(pkg->supplier);
    }
    if (pkg->checksum) {
        strncpy(record->checksum, pkg->checksum, sizeof(record->checksum) - 1);
    }
    
    /* Count total files */
    record->total_files = count_files_recursive(resolved);
    
    /* Add to hash table */
    pthread_mutex_lock(&ctx->lock);
    uint32_t hash = hash_string(pkg->name) % HASH_TABLE_SIZE;
    record->next = ctx->packages[hash];
    ctx->packages[hash] = record;
    ctx->stats.total_packages++;
    pthread_mutex_unlock(&ctx->lock);
    
    return BFT_OK;
}

int bft_get_package(bft_context_t *ctx, const char *name, bft_package_t *out_pkg) {
    if (!ctx || !name || !out_pkg) return BFT_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&ctx->lock);
    
    uint32_t hash = hash_string(name) % HASH_TABLE_SIZE;
    package_record_t *pkg = ctx->packages[hash];
    
    while (pkg) {
        if (strcmp(pkg->name, name) == 0) {
            /* Copy to output */
            strncpy(out_pkg->name, pkg->name, sizeof(out_pkg->name) - 1);
            strncpy(out_pkg->version, pkg->version, sizeof(out_pkg->version) - 1);
            strncpy(out_pkg->license, pkg->license, sizeof(out_pkg->license) - 1);
            out_pkg->root_path = pkg->root_path;
            out_pkg->homepage = pkg->homepage;
            out_pkg->supplier = pkg->supplier;
            out_pkg->checksum = pkg->checksum;
            out_pkg->total_files = pkg->total_files;
            out_pkg->used_files_count = pkg->used_files_count;
            
            pthread_mutex_unlock(&ctx->lock);
            return BFT_OK;
        }
        pkg = pkg->next;
    }
    
    pthread_mutex_unlock(&ctx->lock);
    return BFT_ERROR_NOT_FOUND;
}

int bft_get_all_packages(bft_context_t *ctx, bft_package_t **out_packages, size_t *out_count) {
    if (!ctx || !out_packages || !out_count) return BFT_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&ctx->lock);
    
    /* Count packages */
    size_t count = 0;
    for (size_t i = 0; i < HASH_TABLE_SIZE; i++) {
        package_record_t *pkg = ctx->packages[i];
        while (pkg) {
            count++;
            pkg = pkg->next;
        }
    }
    
    if (count == 0) {
        *out_packages = NULL;
        *out_count = 0;
        pthread_mutex_unlock(&ctx->lock);
        return BFT_OK;
    }
    
    /* Allocate array */
    bft_package_t *packages = calloc(count, sizeof(bft_package_t));
    if (!packages) {
        pthread_mutex_unlock(&ctx->lock);
        return BFT_ERROR_MEMORY;
    }
    
    /* Fill array */
    size_t idx = 0;
    for (size_t i = 0; i < HASH_TABLE_SIZE; i++) {
        package_record_t *pkg = ctx->packages[i];
        while (pkg) {
            bft_package_t *out = &packages[idx++];
            strncpy(out->name, pkg->name, sizeof(out->name) - 1);
            strncpy(out->version, pkg->version, sizeof(out->version) - 1);
            strncpy(out->license, pkg->license, sizeof(out->license) - 1);
            out->root_path = pkg->root_path;
            out->homepage = pkg->homepage;
            out->supplier = pkg->supplier;
            out->checksum = pkg->checksum;
            out->total_files = pkg->total_files;
            out->used_files_count = pkg->used_files_count;
            
            pkg = pkg->next;
        }
    }
    
    pthread_mutex_unlock(&ctx->lock);
    
    *out_packages = packages;
    *out_count = count;
    return BFT_OK;
}

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static uint32_t count_files_recursive(const char *dir_path) {
    uint32_t count = 0;
    DIR *dir = opendir(dir_path);
    if (!dir) return 0;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char fullpath[4096];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, entry->d_name);
        
        if (entry->d_type == DT_DIR) {
            count += count_files_recursive(fullpath);
        } else if (entry->d_type == DT_REG) {
            count++;
        }
    }
    
    closedir(dir);
    return count;
}

static package_record_t* find_package_for_path(bft_context_t *ctx, const char *filepath) {
    /* Find which package owns this file */
    for (size_t i = 0; i < HASH_TABLE_SIZE; i++) {
        package_record_t *pkg = ctx->packages[i];
        while (pkg) {
            if (strncmp(filepath, pkg->root_path, strlen(pkg->root_path)) == 0) {
                return pkg;
            }
            pkg = pkg->next;
        }
    }
    return NULL;
}

static void record_event(bft_context_t *ctx, const bft_event_t *event) {
    pthread_mutex_lock(&ctx->lock);
    
    /* Add to event buffer */
    if (ctx->event_buffer_pos < ctx->event_buffer_size) {
        memcpy(&ctx->event_buffer[ctx->event_buffer_pos], event, sizeof(bft_event_t));
        ctx->event_buffer_pos++;
    }
    
    /* Update statistics */
    ctx->stats.total_events++;
    
    /* Track unique file */
    file_record_t *file = find_or_create_file_record(ctx, event->path);
    if (file) {
        file->access_count++;
        
        /* Update package stats */
        if (event->package[0] != '\0') {
            uint32_t hash = hash_string(event->package) % HASH_TABLE_SIZE;
            package_record_t *pkg = ctx->packages[hash];
            while (pkg) {
                if (strcmp(pkg->name, event->package) == 0) {
                    if (!file->counted_in_package) {
                        pkg->used_files_count++;
                        file->counted_in_package = true;
                    }
                    break;
                }
                pkg = pkg->next;
            }
        }
    }
    
    /* Call user callback if set */
    if (ctx->callback) {
        pthread_mutex_unlock(&ctx->lock);
        ctx->callback(event, ctx->callback_user_data);
        pthread_mutex_lock(&ctx->lock);
    }
    
    pthread_mutex_unlock(&ctx->lock);
}
