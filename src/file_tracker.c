#define _GNU_SOURCE
#include "file_tracker.h"
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <dlfcn.h>
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

// Real fopen function pointer for writing reports (bypass LD_PRELOAD)
static FILE* (*real_fopen_for_reports)(const char*, const char*) = NULL;
static void init_real_fopen(void) {
    if (!real_fopen_for_reports) {
#ifdef __linux__
        real_fopen_for_reports = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen");
#else
        real_fopen_for_reports = fopen;
#endif
        if (!real_fopen_for_reports) {
            real_fopen_for_reports = fopen;
        }
    }
}

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
    
    // Extract file extension
    const char* ext = strrchr(filename, '.');
    if (ext) {
        strncpy(file_type, ext + 1, 63);
        file_type[63] = '\0';
    } else {
        strcpy(file_type, "unknown");
    }
    
    // Try to extract package name from path
    // Look for patterns like /package-name/ or /packagename-version/
    package_name[0] = '\0';
    
    // Simple heuristic: look for directory names after lib, include, or src
    if (strstr(filepath, "/lib/")) {
        const char* lib_pos = strstr(filepath, "/lib/");
        const char* next_slash = strchr(lib_pos + 5, '/');
        if (next_slash) {
            int len = next_slash - (lib_pos + 5);
            if (len > 0 && len < 255) {
                strncpy(package_name, lib_pos + 5, len);
                package_name[len] = '\0';
            }
        }
    } else if (strstr(filepath, "/include/")) {
        const char* inc_pos = strstr(filepath, "/include/");
        const char* next_slash = strchr(inc_pos + 9, '/');
        if (next_slash) {
            int len = next_slash - (inc_pos + 9);
            if (len > 0 && len < 255) {
                strncpy(package_name, inc_pos + 9, len);
                package_name[len] = '\0';
            }
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

    // Lazy initialization - only initialize when we actually need to track
    if (!tracker_initialized) {
        tracker_init();
        if (!tracker_initialized) {
            tracking_in_progress = 0;
            return; // Failed to initialize
        }
    }
    
    if (!global_tracker) {
        tracking_in_progress = 0;
        return;
    }
    
    // Filter out non-relevant files
    if (!filepath || strlen(filepath) == 0) {
        tracking_in_progress = 0;
        return;
    }
    
    // Skip certain directories and file types
    if (strstr(filepath, "/proc/") || strstr(filepath, "/sys/") || 
        strstr(filepath, "/dev/") || strstr(filepath, "/tmp/.X")) {
        tracking_in_progress = 0;
        return;
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

// Write JSON report
void write_report_json(const char* output_file) {
    if (!global_tracker || !output_file) return;
    
    init_real_fopen();
    
    // Create parent directory if it doesn't exist
    create_directory_recursive(output_file);
    
    FILE* f = real_fopen_for_reports(output_file, "w");
    if (!f) {
        fprintf(stderr, "Failed to open output file: %s (errno: %d)\n", output_file, errno);
        return;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"report_type\": \"build_file_tracker\",\n");
    fprintf(f, "  \"timestamp\": \"%ld\",\n", time(NULL));
    fprintf(f, "  \"accessed_files\": [\n");
    
    int first = 1;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        FileAccessEntry* entry = global_tracker->buckets[i];
        while (entry) {
            if (!first) fprintf(f, ",\n");
            fprintf(f, "    {\n");
            fprintf(f, "      \"filepath\": \"%s\",\n", entry->filepath);
            fprintf(f, "      \"package\": \"%s\",\n", entry->package_name);
            fprintf(f, "      \"file_type\": \"%s\",\n", entry->file_type);
            fprintf(f, "      \"access_count\": %lu\n", entry->access_count);
            fprintf(f, "    }");
            first = 0;
            entry = entry->next;
        }
    }
    
    fprintf(f, "\n  ]\n");
    fprintf(f, "}\n");
    fclose(f);
}

// Write CSV report
void write_report_csv(const char* output_file) {
    if (!global_tracker || !output_file) return;
    
    init_real_fopen();
    
    // Create parent directory if it doesn't exist
    create_directory_recursive(output_file);
    
    FILE* f = real_fopen_for_reports(output_file, "w");
    if (!f) {
        fprintf(stderr, "Failed to open output file: %s (errno: %d)\n", output_file, errno);
        return;
    }
    
    fprintf(f, "filepath,package,file_type,access_count\n");
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        FileAccessEntry* entry = global_tracker->buckets[i];
        while (entry) {
            fprintf(f, "\"%s\",\"%s\",\"%s\",%lu\n", 
                    entry->filepath, entry->package_name, 
                    entry->file_type, entry->access_count);
            entry = entry->next;
        }
    }
    
    fclose(f);
}

// Cleanup tracker
void tracker_cleanup(void) {
    if (!global_tracker) return;
    
    // Get output file from environment variable
    const char* output_json = getenv("FILE_TRACKER_JSON");
    const char* output_csv = getenv("FILE_TRACKER_CSV");
    
    if (output_json) {
        write_report_json(output_json);
    }
    if (output_csv) {
        write_report_csv(output_csv);
    }
    
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
