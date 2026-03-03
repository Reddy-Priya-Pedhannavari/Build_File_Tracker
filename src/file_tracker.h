#ifndef FILE_TRACKER_H
#define FILE_TRACKER_H

#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>
    typedef HANDLE pthread_mutex_t;
#else
    #include <pthread.h>
#endif

#define MAX_PATH_LENGTH 4096
#define HASH_TABLE_SIZE 10000

// File access entry structure
typedef struct FileAccessEntry {
    char filepath[MAX_PATH_LENGTH];
    unsigned long access_count;
    char package_name[256];
    char file_type[64];
    struct FileAccessEntry* next;
} FileAccessEntry;

// Hash table for tracking files
typedef struct {
    FileAccessEntry* buckets[HASH_TABLE_SIZE];
    pthread_mutex_t lock;
} FileTracker;

// Global tracker instance
extern FileTracker* global_tracker;

// Function declarations
void tracker_init(void);
void tracker_cleanup(void);
void track_file_access(const char* filepath);
void write_report_json(const char* output_file);
void write_report_csv(const char* output_file);
unsigned long hash_string(const char* str);
FileAccessEntry* create_entry(const char* filepath);
void extract_package_info(const char* filepath, char* package_name, char* file_type);

#endif // FILE_TRACKER_H
