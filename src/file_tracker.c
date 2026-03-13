#define _GNU_SOURCE
#include "file_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#ifdef __linux__
#include <dlfcn.h>
#include <sys/syscall.h>
#ifndef SYS_openat
#define SYS_openat 257
#endif
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#endif

FileTracker* global_tracker = NULL;
static volatile int tracker_initialized = 0;
static volatile int tracker_initializing = 0;
static volatile int tracking_in_progress = 0;
volatile int library_ready = 0;

// Hash function for string
unsigned long hash_string(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % HASH_TABLE_SIZE;
}

// Initialize the tracker
void tracker_init(void) {
    if (tracker_initialized) return;

    // Use CAS to ensure only one thread/call initializes
    // Avoids pthread_mutex_lock during early library load when pthread may not be ready
#ifndef _WIN32
    if (!__sync_bool_compare_and_swap(&tracker_initializing, 0, 1))
        return; // another init in progress
#else
    tracker_initializing = 1;
#endif

    global_tracker = (FileTracker*)malloc(sizeof(FileTracker));
    if (!global_tracker) {
        tracker_initializing = 0;
        return;
    }
    
    memset(global_tracker->buckets, 0, sizeof(global_tracker->buckets));
    pthread_mutex_init(&global_tracker->lock, NULL);
    tracker_initialized = 1;
    library_ready = 1;

    // Register cleanup on exit
    atexit(tracker_cleanup);
}

// Extract package name and file type from path
void extract_package_info(const char* filepath, char* package_name, char* file_type) {
    const char* filename = strrchr(filepath, '/');
    if (!filename) filename = filepath;
    else filename++;
    
    // Extract file extension.
    // Ignore pure-numeric extensions (e.g. ".00", ".03") which appear on
    // Qualcomm QCN NV files and other binary data files — they are not real
    // source-file extensions.
    const char* ext = strrchr(filename, '.');
    if (ext && *(ext + 1) != '\0') {
        const char* p = ext + 1;
        int has_letter = 0;
        while (*p) {
            if (!isdigit((unsigned char)*p)) { has_letter = 1; break; }
            p++;
        }
        if (has_letter) {
            strncpy(file_type, ext + 1, 63);
            file_type[63] = '\0';
        } else {
            strcpy(file_type, "unknown");
        }
    } else {
        strcpy(file_type, "unknown");
    }

    // Extract package name from path.
    // Strategy: scan for well-known directory markers; if none found, fall
    // back to the immediate parent directory of the file.
    package_name[0] = '\0';

    static const struct { const char* marker; int offset; } markers[] = {
        { "/external/", 10 },
        { "/packages/", 10 },
        { "/recipes-",   9 },
        { "/lib/",        5 },
        { "/include/",    9 },
        { "/src/",        5 },
        { NULL, 0 }
    };
    for (int m = 0; markers[m].marker; m++) {
        const char* pos = strstr(filepath, markers[m].marker);
        if (pos) {
            const char* pkg_start = pos + markers[m].offset;
            const char* next_slash = strchr(pkg_start, '/');
            if (next_slash) {
                int len = (int)(next_slash - pkg_start);
                if (len > 0 && len < 255) {
                    strncpy(package_name, pkg_start, len);
                    package_name[len] = '\0';
                    break;
                }
            }
        }
    }

    // Fallback: use the immediate parent directory name
    if (package_name[0] == '\0' && filename > filepath + 1) {
        const char* parent_end = filename - 1;  /* the '/' before filename */
        const char* parent_start = parent_end - 1;
        while (parent_start > filepath && *parent_start != '/') parent_start--;
        if (*parent_start == '/') parent_start++;
        int len = (int)(parent_end - parent_start);
        if (len > 0 && len < 255) {
            strncpy(package_name, parent_start, len);
            package_name[len] = '\0';
        }
    }

    if (package_name[0] == '\0') {
        strcpy(package_name, "unknown");
    }
}

// Create a new entry
FileAccessEntry* create_entry(const char* filepath) {
    FileAccessEntry* entry = (FileAccessEntry*)malloc(sizeof(FileAccessEntry));
    if (!entry) return NULL;
    
    strncpy(entry->filepath, filepath, MAX_PATH_LENGTH - 1);
    entry->filepath[MAX_PATH_LENGTH - 1] = '\0';
    entry->access_count = 1;
    entry->next = NULL;
    
    extract_package_info(filepath, entry->package_name, entry->file_type);
    
    return entry;
}

// Track file access
void track_file_access(const char* filepath) {
    if (tracking_in_progress) return;
    tracking_in_progress = 1;

    if (!tracker_initialized) {
        tracker_init();
        if (!tracker_initialized) {
            tracking_in_progress = 0;
            return;
        }
    }

    // Skip relative paths — real source files accessed during a build are
    // always opened by absolute path. Relative-path opens are typically
    // proprietary config/binary data files (e.g. QCN NV files) that are
    // irrelevant to source-file dependency tracking.
    if (filepath[0] != '/') {
        tracking_in_progress = 0;
        return;
    }

    // Skip system pseudo-filesystems and noise
    if (strstr(filepath, "/proc/") || strstr(filepath, "/sys/") ||
        strstr(filepath, "/dev/")  || strstr(filepath, "/tmp/.X")) {
        tracking_in_progress = 0;
        return;
    }

    // Optional: restrict tracking to files inside a specific directory tree.
    // Set FILE_TRACKER_FILTER_DIR=/path/to/build to narrow results.
    const char* filter_dir = getenv("FILE_TRACKER_FILTER_DIR");
    if (filter_dir && filter_dir[0] != '\0') {
        size_t flen = strlen(filter_dir);
        if (strncmp(filepath, filter_dir, flen) != 0) {
            tracking_in_progress = 0;
            return;
        }
    }
    
    // Use the raw path directly - avoid realpath() which triggers more syscalls
    char norm_path[MAX_PATH_LENGTH];
    strncpy(norm_path, filepath, MAX_PATH_LENGTH - 1);
    norm_path[MAX_PATH_LENGTH - 1] = '\0';

    unsigned long hash = hash_string(norm_path);

    pthread_mutex_lock(&global_tracker->lock);

    // Look for existing entry
    FileAccessEntry* current = global_tracker->buckets[hash];
    FileAccessEntry* prev = NULL;

    while (current) {
        if (strcmp(current->filepath, norm_path) == 0) {
            current->access_count++;
            pthread_mutex_unlock(&global_tracker->lock);
            tracking_in_progress = 0;
            return;
        }
        prev = current;
        current = current->next;
    }
    
    // Create new entry
    FileAccessEntry* new_entry = create_entry(norm_path);
    if (new_entry) {
        if (prev) {
            prev->next = new_entry;
        } else {
            global_tracker->buckets[hash] = new_entry;
        }
    }
    
    pthread_mutex_unlock(&global_tracker->lock);
    tracking_in_progress = 0;
}

// Helper function to create directories recursively (mkdir -p equivalent)
static int create_directory_recursive(const char* path) {
    if (!path) return -1;

    // Work on the directory portion of the file path
    char* dir_path = strdup(path);
    if (!dir_path) return -1;

    char* last_slash = strrchr(dir_path, '/');
    if (!last_slash) {
        free(dir_path);
        return 0;
    }
    *last_slash = '\0';

    // Walk through each component and create if missing
    char tmp[MAX_PATH_LENGTH];
    snprintf(tmp, sizeof(tmp), "%s", dir_path);
    size_t len = strlen(tmp);
    if (len == 0) { free(dir_path); return 0; }

    for (size_t i = 1; i <= len; i++) {
        if (tmp[i] == '/' || tmp[i] == '\0') {
            char save = tmp[i];
            tmp[i] = '\0';
            struct stat st;
            if (stat(tmp, &st) != 0) {
#ifdef _WIN32
                _mkdir(tmp);
#else
                mkdir(tmp, 0755);
#endif
            }
            tmp[i] = save;
        }
    }

    free(dir_path);
    return 0;
}

// Count total tracked entries across all hash buckets
static unsigned long count_tracked_files(void) {
    unsigned long count = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        FileAccessEntry* e = global_tracker->buckets[i];
        while (e) { count++; e = e->next; }
    }
    return count;
}

// Open the output file bypassing our own LD_PRELOAD openat hook.
// On Linux we use syscall(SYS_openat) directly; on other platforms fall back
// to the regular open().
static int open_output_file(const char* path) {
#ifdef __linux__
    return (int)syscall(SYS_openat, AT_FDCWD, path,
                        O_WRONLY | O_CREAT | O_APPEND, 0644);
#else
    return open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
#endif
}

// Append all tracked entries as JSONL (one JSON object per line) to the
// shared output file.  Because every process uses O_APPEND and each
// write() call is a single line well under PIPE_BUF (4096 bytes), the
// writes are atomic on Linux — no per-PID splitting needed.
static void flush_to_jsonl(const char* output_file) {
    if (!global_tracker || !output_file) return;
    if (count_tracked_files() == 0) return;

    create_directory_recursive(output_file);

    int fd = open_output_file(output_file);
    if (fd < 0) {
        fprintf(stderr, "BuildFileTracker: cannot open %s (errno %d)\n",
                output_file, errno);
        return;
    }

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        FileAccessEntry* entry = global_tracker->buckets[i];
        while (entry) {
            // JSON-escape the filepath (handle '"' and '\\')
            char escaped[MAX_PATH_LENGTH * 2];
            const char* src = entry->filepath;
            char* dst = escaped;
            while (*src && (dst - escaped) < (int)sizeof(escaped) - 2) {
                if (*src == '"' || *src == '\\') *dst++ = '\\';
                *dst++ = *src++;
            }
            *dst = '\0';

            char line[2048];
            int len = snprintf(line, sizeof(line),
                "{\"filepath\":\"%s\",\"package\":\"%s\","
                "\"file_type\":\"%s\",\"access_count\":%lu}\n",
                escaped, entry->package_name,
                entry->file_type, entry->access_count);
            if (len > 0 && len < (int)sizeof(line))
                write(fd, line, (size_t)len);
            entry = entry->next;
        }
    }
    close(fd);
}

// Append all tracked entries as CSV rows to the shared output file.
static void flush_to_csv(const char* output_file) {
    if (!global_tracker || !output_file) return;
    if (count_tracked_files() == 0) return;

    create_directory_recursive(output_file);

    int fd = open_output_file(output_file);
    if (fd < 0) return;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        FileAccessEntry* entry = global_tracker->buckets[i];
        while (entry) {
            char line[MAX_PATH_LENGTH + 256];
            int len = snprintf(line, sizeof(line), "\"%s\",\"%s\",\"%s\",%lu\n",
                               entry->filepath, entry->package_name,
                               entry->file_type, entry->access_count);
            if (len > 0 && len < (int)sizeof(line))
                write(fd, line, (size_t)len);
            entry = entry->next;
        }
    }
    close(fd);
}

// Cleanup tracker
void tracker_cleanup(void) {
    if (!global_tracker) return;

    const char* output_json = getenv("FILE_TRACKER_JSON");
    const char* output_csv  = getenv("FILE_TRACKER_CSV");

    // All processes append to the SAME file — no per-PID splitting.
    // O_APPEND + write() is atomic for lines < PIPE_BUF on Linux.
    if (output_json) flush_to_jsonl(output_json);
    if (output_csv)  flush_to_csv(output_csv);
    
    // Free all entries
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        FileAccessEntry* entry = global_tracker->buckets[i];
        while (entry) {
            FileAccessEntry* next = entry->next;
            free(entry);
            entry = next;
        }
    }
    
    pthread_mutex_destroy(&global_tracker->lock);
    free(global_tracker);
    global_tracker = NULL;
    tracker_initialized = 0;
}
