#define _GNU_SOURCE
#include "file_tracker.h"
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <dlfcn.h>
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

FileTracker* global_tracker = NULL;
static int tracker_initialized = 0;

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
    
    global_tracker = (FileTracker*)malloc(sizeof(FileTracker));
    if (!global_tracker) {
        fprintf(stderr, "Failed to allocate memory for file tracker\n");
        return;
    }
    
    memset(global_tracker->buckets, 0, sizeof(global_tracker->buckets));
    pthread_mutex_init(&global_tracker->lock, NULL);
    tracker_initialized = 1;
    
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
    if (!tracker_initialized) tracker_init();
    if (!global_tracker) return;
    
    // Filter out non-relevant files
    if (!filepath || strlen(filepath) == 0) return;
    
    // Skip certain directories and file types
    if (strstr(filepath, "/proc/") || strstr(filepath, "/sys/") || 
        strstr(filepath, "/dev/") || strstr(filepath, "/tmp/.X")) {
        return;
    }
    
    // Get realpath to normalize the path
    char realpath_buf[MAX_PATH_LENGTH];
    if (realpath(filepath, realpath_buf) == NULL) {
        strncpy(realpath_buf, filepath, MAX_PATH_LENGTH - 1);
        realpath_buf[MAX_PATH_LENGTH - 1] = '\0';
    }
    
    unsigned long hash = hash_string(realpath_buf);
    
    pthread_mutex_lock(&global_tracker->lock);
    
    // Look for existing entry
    FileAccessEntry* current = global_tracker->buckets[hash];
    FileAccessEntry* prev = NULL;
    
    while (current) {
        if (strcmp(current->filepath, realpath_buf) == 0) {
            current->access_count++;
            pthread_mutex_unlock(&global_tracker->lock);
            return;
        }
        prev = current;
        current = current->next;
    }
    
    // Create new entry
    FileAccessEntry* new_entry = create_entry(realpath_buf);
    if (new_entry) {
        if (prev) {
            prev->next = new_entry;
        } else {
            global_tracker->buckets[hash] = new_entry;
        }
    }
    
    pthread_mutex_unlock(&global_tracker->lock);
}

// Write JSON report
void write_report_json(const char* output_file) {
    if (!global_tracker) return;
    
    FILE* f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "Failed to open output file: %s\n", output_file);
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
    
    printf("Report written to: %s\n", output_file);
}

// Write CSV report
void write_report_csv(const char* output_file) {
    if (!global_tracker) return;
    
    FILE* f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "Failed to open output file: %s\n", output_file);
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
    printf("Report written to: %s\n", output_file);
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
