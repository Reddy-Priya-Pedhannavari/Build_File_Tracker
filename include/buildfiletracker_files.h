/**
 * buildfiletracker_files.h - File Type Detection and Analysis
 * 
 * Comprehensive file type support for tracking ALL file formats used in builds:
 * - Source code (.c, .cpp, .h, .py, .js, etc.)
 * - Binary files (.o, .obj, .so, .dll, .a, .lib)
 * - Archives (.zip, .tar, .gz, .jar, .war)
 * - Resources (.png, .json, .xml, .ttf, .woff)
 * - Documentation (.md, .txt, .pdf)
 * - And more...
 */

#ifndef BUILDFILETRACKER_FILES_H
#define BUILDFILETRACKER_FILES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FILE TYPE CATEGORIES
 * ============================================================================ */

typedef enum {
    BFT_FILETYPE_UNKNOWN = 0,
    
    /* Source Code */
    BFT_FILETYPE_C_SOURCE,           /* .c */
    BFT_FILETYPE_CPP_SOURCE,         /* .cpp, .cc, .cxx, .C */
    BFT_FILETYPE_C_HEADER,           /* .h, .hpp, .hxx */
    BFT_FILETYPE_ASSEMBLY,           /* .s, .S, .asm */
    
    /* Object/Binary Files */
    BFT_FILETYPE_OBJECT,             /* .o, .obj */
    BFT_FILETYPE_STATIC_LIB,         /* .a, .lib */
    BFT_FILETYPE_SHARED_LIB,         /* .so, .dylib, .dll */
    BFT_FILETYPE_EXECUTABLE,         /* .exe, .out, no extension */
    
    /* Archives */
    BFT_FILETYPE_ARCHIVE_ZIP,        /* .zip */
    BFT_FILETYPE_ARCHIVE_TAR,        /* .tar, .tar.gz, .tar.bz2, .tgz */
    BFT_FILETYPE_ARCHIVE_7Z,         /* .7z */
    BFT_FILETYPE_ARCHIVE_RAR,        /* .rar */
    
    /* Java/JVM */
    BFT_FILETYPE_JAVA_SOURCE,        /* .java */
    BFT_FILETYPE_JAVA_CLASS,         /* .class */
    BFT_FILETYPE_JAVA_JAR,           /* .jar */
    BFT_FILETYPE_JAVA_WAR,           /* .war */
    BFT_FILETYPE_JAVA_EAR,           /* .ear */
    
    /* Python */
    BFT_FILETYPE_PYTHON_SOURCE,      /* .py */
    BFT_FILETYPE_PYTHON_COMPILED,    /* .pyc, .pyo */
    BFT_FILETYPE_PYTHON_WHEEL,       /* .whl */
    BFT_FILETYPE_PYTHON_EGG,         /* .egg */
    BFT_FILETYPE_PYTHON_SO,          /* .pyd, .so (compiled extension) */
    
    /* JavaScript/Node */
    BFT_FILETYPE_JAVASCRIPT,         /* .js, .mjs */
    BFT_FILETYPE_TYPESCRIPT,         /* .ts, .tsx */
    BFT_FILETYPE_NODE_ADDON,         /* .node */
    BFT_FILETYPE_WASM,               /* .wasm */
    
    /* .NET */
    BFT_FILETYPE_CSHARP_SOURCE,      /* .cs */
    BFT_FILETYPE_DOTNET_DLL,         /* .dll (managed) */
    BFT_FILETYPE_DOTNET_EXE,         /* .exe (managed) */
    BFT_FILETYPE_DOTNET_NUPKG,       /* .nupkg */
    
    /* Rust */
    BFT_FILETYPE_RUST_SOURCE,        /* .rs */
    BFT_FILETYPE_RUST_LIB,           /* .rlib */
    
    /* Go */
    BFT_FILETYPE_GO_SOURCE,          /* .go */
    BFT_FILETYPE_GO_ARCHIVE,         /* .a */
    
    /* Build/Config Files */
    BFT_FILETYPE_MAKEFILE,           /* Makefile, *.mk */
    BFT_FILETYPE_CMAKE,              /* CMakeLists.txt, *.cmake */
    BFT_FILETYPE_NINJA,              /* build.ninja */
    BFT_FILETYPE_BAZEL,              /* BUILD, *.bzl */
    BFT_FILETYPE_GRADLE,             /* build.gradle, *.gradle */
    BFT_FILETYPE_MAVEN,              /* pom.xml */
    
    /* Data/Config Files */
    BFT_FILETYPE_JSON,               /* .json */
    BFT_FILETYPE_XML,                /* .xml */
    BFT_FILETYPE_YAML,               /* .yaml, .yml */
    BFT_FILETYPE_TOML,               /* .toml */
    BFT_FILETYPE_INI,                /* .ini, .conf, .cfg */
    BFT_FILETYPE_PROPERTIES,         /* .properties */
    
    /* Documentation */
    BFT_FILETYPE_MARKDOWN,           /* .md */
    BFT_FILETYPE_TEXT,               /* .txt */
    BFT_FILETYPE_PDF,                /* .pdf */
    BFT_FILETYPE_HTML,               /* .html, .htm */
    
    /* Resources */
    BFT_FILETYPE_IMAGE,              /* .png, .jpg, .jpeg, .gif, .svg, .ico */
    BFT_FILETYPE_FONT,               /* .ttf, .otf, .woff, .woff2 */
    BFT_FILETYPE_AUDIO,              /* .mp3, .wav, .ogg */
    BFT_FILETYPE_VIDEO,              /* .mp4, .avi, .mkv */
    
    /* Database */
    BFT_FILETYPE_DATABASE,           /* .db, .sqlite, .sqlite3 */
    BFT_FILETYPE_SQL,                /* .sql */
    
    /* Security */
    BFT_FILETYPE_CERTIFICATE,        /* .pem, .crt, .cer, .der */
    BFT_FILETYPE_KEY,                /* .key, .pub, .ppk */
    
    /* Package Formats */
    BFT_FILETYPE_DEB,                /* .deb */
    BFT_FILETYPE_RPM,                /* .rpm */
    BFT_FILETYPE_PKG,                /* .pkg */
    BFT_FILETYPE_MSI,                /* .msi */
    BFT_FILETYPE_APK,                /* .apk */
    
    /* Intermediate/Build Artifacts */
    BFT_FILETYPE_LLVM_BC,            /* .bc (LLVM bitcode) */
    BFT_FILETYPE_LLVM_IR,            /* .ll (LLVM IR) */
    BFT_FILETYPE_PRECOMPILED,        /* .pch, .gch */
    BFT_FILETYPE_DEBUG_INFO,         /* .pdb, .dSYM, .dwarf */
    
    BFT_FILETYPE_MAX
} bft_filetype_t;

/* File category groupings */
typedef enum {
    BFT_CATEGORY_SOURCE = 1 << 0,
    BFT_CATEGORY_BINARY = 1 << 1,
    BFT_CATEGORY_LIBRARY = 1 << 2,
    BFT_CATEGORY_ARCHIVE = 1 << 3,
    BFT_CATEGORY_CONFIG = 1 << 4,
    BFT_CATEGORY_RESOURCE = 1 << 5,
    BFT_CATEGORY_DOCUMENTATION = 1 << 6,
    BFT_CATEGORY_BUILD = 1 << 7,
    BFT_CATEGORY_DATA = 1 << 8,
    BFT_CATEGORY_PACKAGE = 1 << 9,
} bft_file_category_t;

/* ============================================================================
 * FILE INFORMATION
 * ============================================================================ */

typedef struct {
    char filepath[4096];
    bft_filetype_t type;
    uint32_t category_flags;
    
    /* File metadata */
    uint64_t size;
    uint64_t mtime;
    char checksum_sha256[65];
    
    /* Binary-specific info */
    bool is_binary;
    bool is_executable;
    bool is_library;
    bool is_32bit;
    bool is_64bit;
    
    /* Library/Binary dependencies */
    char **dependencies;      /* List of linked libraries */
    size_t dependency_count;
    
    /* Symbols (for libraries/objects) */
    char **exported_symbols;
    size_t exported_count;
    char **imported_symbols;
    size_t imported_count;
    
    /* Archive contents (for .jar, .zip, etc.) */
    char **archive_contents;
    size_t content_count;
    
    /* Language/platform info */
    char language[32];        /* C, C++, Python, Java, etc. */
    char platform[32];        /* linux, windows, darwin, etc. */
    char architecture[32];    /* x86_64, arm64, etc. */
    
} bft_file_info_t;

/* ============================================================================
 * FILE TYPE DETECTION
 * ============================================================================ */

/**
 * Detect file type from path/extension
 */
bft_filetype_t bft_detect_filetype(const char *filepath);

/**
 * Detect file type from content (magic bytes)
 */
bft_filetype_t bft_detect_filetype_from_content(const char *filepath);

/**
 * Get file type name
 */
const char* bft_filetype_name(bft_filetype_t type);

/**
 * Get file category flags
 */
uint32_t bft_filetype_category(bft_filetype_t type);

/**
 * Check if file type is in category
 */
bool bft_filetype_is_category(bft_filetype_t type, bft_file_category_t category);

/* ============================================================================
 * FILE ANALYSIS
 * ============================================================================ */

/**
 * Analyze file and extract metadata
 */
int bft_analyze_file(const char *filepath, bft_file_info_t *info);

/**
 * Get binary dependencies (linked libraries)
 * Works on: .so, .dll, .exe, .dylib
 */
int bft_get_binary_dependencies(const char *filepath, 
                                char ***out_deps, 
                                size_t *out_count);

/**
 * Get exported symbols from binary
 * Works on: .so, .dll, .a, .o
 */
int bft_get_exported_symbols(const char *filepath,
                             char ***out_symbols,
                             size_t *out_count);

/**
 * List archive contents
 * Works on: .jar, .zip, .tar, .whl, .egg
 */
int bft_list_archive_contents(const char *filepath,
                              char ***out_files,
                              size_t *out_count);

/**
 * Detect binary architecture
 */
int bft_detect_architecture(const char *filepath,
                            char *arch,
                            size_t arch_size,
                            bool *is_32bit,
                            bool *is_64bit);

/**
 * Calculate file checksum (SHA256)
 */
int bft_calculate_checksum(const char *filepath,
                           char *checksum,
                           size_t checksum_size);

/**
 * Free file info structure
 */
void bft_free_file_info(bft_file_info_t *info);

/* ============================================================================
 * FILTERING PRESETS
 * ============================================================================ */

/* Common extension sets */
extern const char* BFT_EXTENSIONS_SOURCE[];
extern const char* BFT_EXTENSIONS_C_CPP[];
extern const char* BFT_EXTENSIONS_BINARY[];
extern const char* BFT_EXTENSIONS_LIBRARY[];
extern const char* BFT_EXTENSIONS_PYTHON[];
extern const char* BFT_EXTENSIONS_JAVA[];
extern const char* BFT_EXTENSIONS_JAVASCRIPT[];
extern const char* BFT_EXTENSIONS_CONFIG[];
extern const char* BFT_EXTENSIONS_RESOURCE[];
extern const char* BFT_EXTENSIONS_ALL[];

#ifdef __cplusplus
}
#endif

#endif /* BUILDFILETRACKER_FILES_H */
