# BuildFileTracker - File Types Quick Reference

## All Supported File Types (60+)

### Source Code
```
C/C++:       .c .cpp .cc .cxx .C .c++ .h .hpp .hxx .hh .h++ .s .S .asm
Python:      .py .pyc .pyo .pyd
Java:        .java .class
JavaScript:  .js .mjs .ts .tsx
C#:          .cs
Rust:        .rs .rlib
Go:          .go
```

### Binaries & Libraries
```
Objects:     .o .obj
Static:      .a .lib
Shared:      .so .dylib .dll
Executable:  .exe .out (no extension)
WebAssembly: .wasm
Node:        .node
```

### Archives & Packages
```
General:     .zip .tar .tar.gz .tgz .tar.bz2 .7z .rar
Java:        .jar .war .ear
Python:      .whl .egg
Linux:       .deb .rpm
Windows:     .msi
macOS:       .pkg
Android:     .apk
.NET:        .nupkg
```

### Configuration & Data
```
Config:      .json .xml .yaml .yml .toml .ini .conf .cfg .properties
Database:    .db .sqlite .sqlite3 .sql
```

### Build System Files
```
Make:        Makefile *.mk
CMake:       CMakeLists.txt *.cmake
Ninja:       build.ninja
Bazel:       BUILD *.bzl
Gradle:      build.gradle *.gradle
Maven:       pom.xml
```

### Resources
```
Images:      .png .jpg .jpeg .gif .svg .ico .bmp
Fonts:       .ttf .otf .woff .woff2
Audio:       .mp3 .wav .ogg
Video:       .mp4 .avi .mkv
```

### Documentation
```
Markup:      .md .html .htm
Text:        .txt
PDF:         .pdf
```

### Security
```
Certs:       .pem .crt .cer .der
Keys:        .key .pub .ppk
```

### Build Artifacts
```
LLVM:        .bc .ll
Precompiled: .pch .gch
Debug:       .pdb .dSYM
```

## Quick Usage

### Track Everything
```python
from buildfiletracker import BuildFileTracker

tracker = BuildFileTracker()
# Don't set extension filter → tracks all types
tracker.run_command(["make", "all"])
```

### Track Specific Categories
```python
from buildfiletracker.filetypes import (
    EXTENSIONS_SOURCE,    # All source
    EXTENSIONS_BINARY,    # All binaries
    EXTENSIONS_LIBRARY,   # All libraries
    EXTENSIONS_PYTHON,    # Python files
    EXTENSIONS_JAVA,      # Java files
)

tracker = BuildFileTracker()
tracker.set_extension_filter(EXTENSIONS_BINARY)
```

### Analyze File Types
```python
from buildfiletracker.filetypes import FileTypeDetector

info = FileTypeDetector.analyze_file("libssl.so.3")
print(f"Type: {info.filetype}")
print(f"Categories: {info.categories}")
print(f"Architecture: {info.architecture}")
print(f"Dependencies: {info.dependencies}")
```

### Filter by Category
```python
from buildfiletracker.filetypes import FileCategory

for filepath in tracker.file_access_map.keys():
    info = FileTypeDetector.analyze_file(filepath)
    
    if FileCategory.LIBRARY in info.categories:
        print(f"Library: {filepath}")
    
    if FileCategory.BINARY in info.categories:
        print(f"Binary: {filepath}")
```

## Binary Analysis Features

### Dependencies (ldd, otool)
```python
info = FileTypeDetector.analyze_file("myapp")
for dep in info.dependencies:
    print(f"Links to: {dep}")
```

### Architecture Detection
```python
info = FileTypeDetector.analyze_file("mylib.so")
print(f"Architecture: {info.architecture}")  # x86_64, arm64, etc.
print(f"64-bit: {info.is_64bit}")
```

### Checksums
```python
info = FileTypeDetector.analyze_file("binary.dll")
print(f"SHA256: {info.checksum_sha256}")
```

### Archive Contents
```python
info = FileTypeDetector.analyze_file("app.jar")
print(f"Contains {len(info.archive_contents)} files:")
for item in info.archive_contents:
    print(f"  - {item}")
```

## File Type Enum Values

```python
from buildfiletracker.filetypes import FileType

FileType.C_SOURCE
FileType.CPP_SOURCE
FileType.C_HEADER
FileType.PYTHON_SOURCE
FileType.JAVA_SOURCE
FileType.JAVASCRIPT
FileType.TYPESCRIPT
FileType.CSHARP_SOURCE
FileType.RUST_SOURCE
FileType.GO_SOURCE
FileType.OBJECT
FileType.STATIC_LIB
FileType.SHARED_LIB
FileType.EXECUTABLE
FileType.ARCHIVE_ZIP
FileType.ARCHIVE_TAR
FileType.JAVA_JAR
FileType.JAVA_WAR
FileType.PYTHON_WHEEL
FileType.PYTHON_EGG
FileType.NODE_ADDON
FileType.WASM
FileType.JSON
FileType.XML
FileType.YAML
FileType.TOML
FileType.MAKEFILE
FileType.CMAKE
FileType.IMAGE
FileType.FONT
FileType.PDF
FileType.DATABASE
FileType.CERTIFICATE
FileType.DEB
FileType.RPM
FileType.APK
# ... and 35+ more!
```

## Category Enum Values

```python
from buildfiletracker.filetypes import FileCategory

FileCategory.SOURCE          # Source code
FileCategory.BINARY          # Compiled binaries
FileCategory.LIBRARY         # Libraries (static/shared)
FileCategory.ARCHIVE         # Archives (zip, tar, jar)
FileCategory.CONFIG          # Configuration files
FileCategory.RESOURCE        # Images, fonts, etc.
FileCategory.DOCUMENTATION   # Docs
FileCategory.BUILD           # Build system files
FileCategory.DATA            # Data files
FileCategory.PACKAGE         # OS packages
```

## Example: Complete Analysis

```python
from buildfiletracker import BuildFileTracker, ReportFormat
from buildfiletracker.filetypes import FileTypeDetector, FileCategory

# Track everything
tracker = BuildFileTracker()
tracker.run_command(["make", "all"])

# Analyze by category
by_category = {}
for filepath in tracker.file_access_map.keys():
    info = FileTypeDetector.analyze_file(filepath)
    
    for category in info.categories:
        if category not in by_category:
            by_category[category] = []
        by_category[category].append(filepath)

# Report
print("Files by Category:")
for category, files in by_category.items():
    print(f"\n{category.value.upper()}:")
    for f in files[:5]:
        info = FileTypeDetector.analyze_file(f)
        print(f"  {f}")
        if info.is_library:
            print(f"    → Links: {', '.join(info.dependencies[:3])}")
        if info.archive_contents:
            print(f"    → Contains {len(info.archive_contents)} files")

# Generate SBOM with all file types
tracker.generate_report("complete_sbom.spdx.json", ReportFormat.SPDX_JSON)
```

---

**See [FILE_TYPES.md](FILE_TYPES.md) for complete documentation.**
