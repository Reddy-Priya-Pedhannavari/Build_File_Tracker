/* BuildFileTracker - Complete API
 * Universal build file access monitoring with multiple backends
 * 
 * Copyright (c) 2026 BuildFileTracker Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef BUILDFILETRACKER_H
#define BUILDFILETRACKER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version Information */
#define BFT_VERSION_MAJOR 2
#define BFT_VERSION_MINOR 0
#define BFT_VERSION_PATCH 0
#define BFT_VERSION_STRING "2.0.0"

/* ============================================================================
 * ENUMERATIONS
 * ============================================================================ */

/* Tracking backend types */
typedef enum {
    BFT_BACKEND_AUTO = 0,        /* Auto-select best available backend */
    BFT_BACKEND_PRELOAD,         /* LD_PRELOAD (most compatible) */
    BFT_BACKEND_INOTIFY,         /* Linux inotify (real-time) */
    BFT_BACKEND_FANOTIFY,        /* Linux fanotify (requires CAP_SYS_ADMIN) */
    BFT_BACKEND_FSEVENTS,        /* macOS FSEvents */
    BFT_BACKEND_STRACE,          /* strace wrapper (fallback) */
    BFT_BACKEND_HYBRID           /* Combination of backends */
} bft_backend_t;

/* File access types (bitmask) */
typedef enum {
    BFT_ACCESS_NONE     = 0x00,
    BFT_ACCESS_READ     = 0x01,
    BFT_ACCESS_WRITE    = 0x02,
    BFT_ACCESS_EXECUTE  = 0x04,
    BFT_ACCESS_OPEN     = 0x08,
    BFT_ACCESS_CLOSE    = 0x10,
    BFT_ACCESS_STAT     = 0x20,
    BFT_ACCESS_DELETE   = 0x40,
    BFT_ACCESS_METADATA = 0x80
} bft_access_type_t;

/* Report output formats */
typedef enum {
    BFT_FORMAT_JSON = 0,
    BFT_FORMAT_CSV,
    BFT_FORMAT_XML,
    BFT_FORMAT_XLSX,
    BFT_FORMAT_SPDX_JSON,        /* SPDX 2.3 JSON format */
    BFT_FORMAT_SPDX_TAGVALUE,    /* SPDX Tag-Value format */
    BFT_FORMAT_CYCLONEDX_JSON,   /* CycloneDX JSON format */
    BFT_FORMAT_CYCLONEDX_XML,    /* CycloneDX XML format */
    BFT_FORMAT_MARKDOWN,         /* Human-readable Markdown */
    BFT_FORMAT_HTML              /* Interactive HTML report */
} bft_format_t;

/* Error codes */
typedef enum {
    BFT_OK = 0,
    BFT_ERROR_INVALID_PARAM = -1,
    BFT_ERROR_NO_MEMORY = -2,
    BFT_ERROR_BACKEND_INIT = -3,
    BFT_ERROR_ALREADY_RUNNING = -4,
    BFT_ERROR_NOT_RUNNING = -5,
    BFT_ERROR_FILE_IO = -6,
    BFT_ERROR_PERMISSION = -7,
    BFT_ERROR_NOT_SUPPORTED = -8,
    BFT_ERROR_TIMEOUT = -9
} bft_error_t;

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/* Forward declarations */
typedef struct bft_context bft_context_t;

/* File event structure */
typedef struct {
    char            *filepath;          /* Absolute file path */
    char            *package_name;      /* Resolved package name */
    char            *package_version;   /* Package version */
    uint32_t        access_flags;       /* Bitmask of bft_access_type_t */
    struct timespec timestamp;          /* When access occurred */
    uint32_t        pid;                /* Process ID */
    char            *process_name;      /* Process name */
    uint32_t        uid;                /* User ID */
    char            *build_phase;       /* Build phase (compile/link/etc) */
} bft_event_t;

/* Package information */
typedef struct {
    char            *name;              /* Package name */
    char            *version;           /* Package version */
    char            *license;           /* SPDX license identifier */
    char            *homepage;          /* Package homepage URL */
    char            *supplier;          /* Package supplier/vendor */
    char            *source_path;       /* Root path of package */
    uint32_t        total_files;        /* Total files in package */
    uint32_t        used_files;         /* Files actually used */
    char            **used_file_list;   /* Array of used file paths */
    double          usage_percentage;   /* Percentage of files used */
    char            *checksum;          /* SHA256 of package */
} bft_package_t;

/* Configuration structure */
typedef struct {
    /* Backend configuration */
    bft_backend_t   backend;            /* Tracking backend to use */
    
    /* Watch configuration */
    char            **watch_paths;      /* Paths to monitor */
    uint32_t        watch_path_count;
    char            **exclude_patterns; /* Glob patterns to exclude */
    uint32_t        exclude_count;
    bool            recursive;          /* Watch subdirectories */
    bool            follow_symlinks;    /* Follow symbolic links */
    
    /* Package configuration */
    char            **package_roots;    /* Package root directories */
    uint32_t        package_root_count;
    bool            auto_detect_packages; /* Auto-detect packages */
    
    /* Advanced options */
    bool            track_dependencies; /* Track include chains */
    bool            track_build_phases; /* Identify build phases */
    bool            capture_env;        /* Capture environment variables */
    uint32_t        buffer_size;        /* Event buffer size */
    uint32_t        flush_interval_ms;  /* Flush interval in milliseconds */
    
    /* Output configuration */
    char            *output_path;       /* Default output path */
    bft_format_t    output_format;      /* Default output format */
    bool            real_time_updates;  /* Update reports in real-time */
} bft_config_t;

/* Statistics structure */
typedef struct {
    uint64_t        total_events;       /* Total events recorded */
    uint64_t        filtered_events;    /* Events filtered out */
    uint64_t        unique_files;       /* Unique files accessed */
    uint64_t        total_packages;     /* Registered packages */
    uint64_t        packages_used;      /* Packages with usage */
    struct timespec start_time;         /* When tracking started */
    struct timespec end_time;           /* When tracking stopped */
    double          runtime_seconds;    /* Total runtime */
} bft_stats_t;

/* Callback function types */
typedef void (*bft_event_callback_t)(const bft_event_t *event, void *userdata);
typedef void (*bft_package_callback_t)(const bft_package_t *package, void *userdata);

/* ============================================================================
 * CORE API - Context Management
 * ============================================================================ */

/**
 * Initialize a new BuildFileTracker context with default configuration
 * @return New context handle, or NULL on error
 */
bft_context_t* bft_create(void);

/**
 * Initialize context with custom configuration
 * @param config Configuration structure (copied internally)
 * @return New context handle, or NULL on error
 */
bft_context_t* bft_create_with_config(const bft_config_t *config);

/**
 * Get default configuration
 * @param config Configuration structure to fill
 */
void bft_config_default(bft_config_t *config);

/**
 * Start tracking file accesses
 * @param ctx Context handle
 * @return BFT_OK on success, error code on failure
 */
int bft_start(bft_context_t *ctx);

/**
 * Stop tracking file accesses
 * @param ctx Context handle
 * @return BFT_OK on success, error code on failure
 */
int bft_stop(bft_context_t *ctx);

/**
 * Check if tracking is active
 * @param ctx Context handle
 * @return true if tracking, false otherwise
 */
bool bft_is_running(bft_context_t *ctx);

/**
 * Destroy context and free all resources
 * @param ctx Context handle
 */
void bft_destroy(bft_context_t *ctx);

/* ============================================================================
 * WATCH PATH MANAGEMENT
 * ============================================================================ */

/**
 * Add a path to watch for file accesses
 * @param ctx Context handle
 * @param path Path to watch (directory or specific file)
 * @param recursive Watch subdirectories recursively
 * @return BFT_OK on success, error code on failure
 */
int bft_add_watch(bft_context_t *ctx, const char *path, bool recursive);

/**
 * Remove a watch path
 * @param ctx Context handle
 * @param path Path to stop watching
 * @return BFT_OK on success, error code on failure
 */
int bft_remove_watch(bft_context_t *ctx, const char *path);

/**
 * Add exclude pattern (glob style)
 * @param ctx Context handle
 * @param pattern Glob pattern (e.g., "*.pyc", "__pycache__/*")
 * @return BFT_OK on success, error code on failure
 */
int bft_add_exclude(bft_context_t *ctx, const char *pattern);

/**
 * Clear all exclude patterns
 * @param ctx Context handle
 */
void bft_clear_excludes(bft_context_t *ctx);

/* ============================================================================
 * PACKAGE MANAGEMENT
 * ============================================================================ */

/**
 * Register a package for tracking
 * @param ctx Context handle
 * @param name Package name
 * @param version Package version (can be NULL)
 * @param root_path Root directory of package
 * @param license SPDX license identifier (can be NULL)
 * @return BFT_OK on success, error code on failure
 */
int bft_register_package(bft_context_t *ctx, 
                         const char *name,
                         const char *version,
                         const char *root_path,
                         const char *license);

/**
 * Register package with full metadata
 * @param ctx Context handle
 * @param pkg Package structure with all details
 * @return BFT_OK on success, error code on failure
 */
int bft_register_package_full(bft_context_t *ctx, const bft_package_t *pkg);

/**
 * Auto-detect packages in a directory
 * Scans for common package managers (npm, pip, cargo, etc.)
 * @param ctx Context handle
 * @param search_root Root directory to search
 * @return Number of packages detected, or negative error code
 */
int bft_auto_detect_packages(bft_context_t *ctx, const char *search_root);

/**
 * Get package information
 * @param ctx Context handle
 * @param name Package name
 * @param pkg Package structure to fill (caller must free)
 * @return BFT_OK on success, error code on failure
 */
int bft_get_package(bft_context_t *ctx, const char *name, bft_package_t *pkg);

/**
 * Get all registered packages
 * @param ctx Context handle
 * @param packages Array of package structures (caller must free)
 * @param count Number of packages returned
 * @return BFT_OK on success, error code on failure
 */
int bft_get_all_packages(bft_context_t *ctx, bft_package_t **packages, uint32_t *count);

/**
 * Free package structure memory
 * @param pkg Package structure to free
 */
void bft_free_package(bft_package_t *pkg);

/* ============================================================================
 * STATISTICS & REPORTING
 * ============================================================================ */

/**
 * Get current statistics
 * @param ctx Context handle
 * @param stats Statistics structure to fill
 * @return BFT_OK on success, error code on failure
 */
int bft_get_stats(bft_context_t *ctx, bft_stats_t *stats);

/**
 * Generate report in specified format
 * @param ctx Context handle
 * @param output_path Output file path
 * @param format Report format
 * @return BFT_OK on success, error code on failure
 */
int bft_generate_report(bft_context_t *ctx, const char *output_path, bft_format_t format);

/**
 * Generate multiple report formats at once
 * @param ctx Context handle
 * @param base_path Base path for output files (extensions added automatically)
 * @param formats Array of formats to generate
 * @param format_count Number of formats
 * @return BFT_OK on success, error code on failure
 */
int bft_generate_reports(bft_context_t *ctx, const char *base_path, 
                         const bft_format_t *formats, uint32_t format_count);

/**
 * Get usage report for specific package
 * @param ctx Context handle
 * @param package_name Package name
 * @param output_path Output file path
 * @param format Report format
 * @return BFT_OK on success, error code on failure
 */
int bft_generate_package_report(bft_context_t *ctx, const char *package_name,
                                const char *output_path, bft_format_t format);

/* ============================================================================
 * EVENT CALLBACKS
 * ============================================================================ */

/**
 * Set callback for real-time file access events
 * @param ctx Context handle
 * @param callback Callback function
 * @param userdata User data passed to callback
 * @return BFT_OK on success, error code on failure
 */
int bft_set_event_callback(bft_context_t *ctx, bft_event_callback_t callback, void *userdata);

/**
 * Set callback for package usage updates
 * @param ctx Context handle
 * @param callback Callback function
 * @param userdata User data passed to callback
 * @return BFT_OK on success, error code on failure
 */
int bft_set_package_callback(bft_context_t *ctx, bft_package_callback_t callback, void *userdata);

/**
 * Free event structure
 * @param event Event to free
 */
void bft_free_event(bft_event_t *event);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * Get version string
 * @return Version string (e.g., "2.0.0")
 */
const char* bft_version(void);

/**
 * Get error message for error code
 * @param error Error code
 * @return Human-readable error message
 */
const char* bft_strerror(int error);

/**
 * Check if backend is available on this system
 * @param backend Backend type to check
 * @return true if available, false otherwise
 */
bool bft_backend_available(bft_backend_t backend);

/**
 * Get recommended backend for current system
 * @return Recommended backend type
 */
bft_backend_t bft_get_recommended_backend(void);

/**
 * Set log level (0=none, 1=error, 2=warning, 3=info, 4=debug)
 * @param level Log level
 */
void bft_set_log_level(int level);

/**
 * Flush pending events to disk
 * @param ctx Context handle
 * @return Number of events flushed, or negative error code
 */
int bft_flush(bft_context_t *ctx);

/* ============================================================================
 * ADVANCED FEATURES
 * ============================================================================ */

/**
 * Track only specific file extensions
 * @param ctx Context handle
 * @param extensions Array of extensions (e.g., [".c", ".h", ".cpp"])
 * @param count Number of extensions
 * @return BFT_OK on success, error code on failure
 */
int bft_set_extension_filter(bft_context_t *ctx, const char **extensions, uint32_t count);

/**
 * Get dependency graph for a file (include chains)
 * @param ctx Context handle
 * @param filepath File to analyze
 * @param dependencies Array of dependent files (caller must free)
 * @param count Number of dependencies
 * @return BFT_OK on success, error code on failure
 */
int bft_get_dependencies(bft_context_t *ctx, const char *filepath, 
                         char ***dependencies, uint32_t *count);

/**
 * Export raw event log
 * @param ctx Context handle
 * @param output_path Output file path
 * @return BFT_OK on success, error code on failure
 */
int bft_export_event_log(bft_context_t *ctx, const char *output_path);

/**
 * Import previous event log (for cumulative tracking)
 * @param ctx Context handle
 * @param input_path Input file path
 * @return Number of events imported, or negative error code
 */
int bft_import_event_log(bft_context_t *ctx, const char *input_path);

/**
 * Reset all tracking data (keep configuration)
 * @param ctx Context handle
 */
void bft_reset(bft_context_t *ctx);

/* ============================================================================
 * BUILD SYSTEM INTEGRATION HELPERS
 * ============================================================================ */

/**
 * Wrap a build command with tracking
 * @param ctx Context handle
 * @param command Command array (NULL-terminated)
 * @return Exit code of command, or negative error code
 */
int bft_run_command(bft_context_t *ctx, const char **command);

/**
 * Setup environment for LD_PRELOAD tracking
 * Call before executing build commands
 * @param ctx Context handle
 * @return BFT_OK on success, error code on failure
 */
int bft_setup_preload_env(bft_context_t *ctx);

/**
 * Cleanup LD_PRELOAD environment
 * @param ctx Context handle
 */
void bft_cleanup_preload_env(bft_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* BUILDFILETRACKER_H */
