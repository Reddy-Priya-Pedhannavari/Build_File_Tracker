# File Type Tracking - Complete Guide

BuildFileTracker can track **all file types** used in your projects, not just source code. This includes binaries, libraries, archives, configurations, resources, and more.

## 📋 Table of Contents

1. [Overview](#overview)
2. [Supported File Types](#supported-file-types)
3. [Binary Analysis](#binary-analysis)
4. [Archive Analysis](#archive-analysis)
5. [File Type Detection](#file-type-detection)
6. [Practical Examples](#practical-examples)
7. [API Reference](#api-reference)

---

## Overview

Modern software projects use many types of files:
- **Source code** (.c, .py, .java)
- **Compiled binaries** (.o, .so, .dll, .exe)
- **Libraries** (static and shared)
- **Archives** (.jar, .zip, .whl)
- **Configuration** (.json, .xml, .yaml)
- **Resources** (.png, .ttf)
- **Documentation** (.md, .pdf)
- **And many more...**

BuildFileTracker tracks **ALL** of these to give you complete visibility into your build dependencies.

---

## Supported File Types

### 1. Source Code (30+ formats)

```python
from buildfiletracker.filetypes import EXTENSIONS_SOURCE

print(EXTENSIONS_SOURCE)
# ['.c', '.cpp', '.h', '.py', '.java', '.js', '.ts', '.rs', '.go', '.cs', ...]
```

**Languages Supported:**
- C, C++, Assembly
- Python (including .pyc, .pyo)
- Java
- JavaScript, TypeScript
- C#, .NET
- Rust
- Go
- And more...

### 2. Binary & Object Files

```python
from buildfiletracker.filetypes import EXTENSIONS_BINARY

print(EXTENSIONS_BINARY)
# ['.o', '.obj', '.so', '.dll', '.dylib', '.exe', '.a', '.lib']
```

**Types:**
- **Object Files**: `.o` (Linux/macOS), `.obj` (Windows)
- **Static Libraries**: `.a` (Linux/macOS), `.lib` (Windows)
- **Shared Libraries**: `.so` (Linux), `.dylib` (macOS), `.dll` (Windows)
- **Executables**: `.exe` (Windows), no extension (Linux/macOS)

**Special Features:**
- ✅ Dependency extraction (what libraries does this binary link?)
- ✅ Architecture detection (x86, x86_64, arm, arm64)
- ✅ SHA256 checksum calculation
- ✅ Symbol analysis (exported/imported symbols)

### 3. Archives & Packages

**Archive Types:**
- ZIP: `.zip`
- TAR: `.tar`, `.tar.gz`, `.tgz`, `.tar.bz2`
- 7-Zip: `.7z`
- RAR: `.rar`

**Language-Specific Archives:**
- Java: `.jar`, `.war`, `.ear`
- Python: `.whl` (wheel), `.egg`
- Node.js: Contains `.node` binary addons

**Package Formats:**
- Debian: `.deb`
- Red Hat: `.rpm`
- macOS: `.pkg`
- Windows: `.msi`
- Android: `.apk`

**Archive Analysis:**
```python
from buildfiletracker.filetypes import FileTypeDetector

info = FileTypeDetector.analyze_file("myapp.jar")
print(f"Contains {len(info.archive_contents)} files:")
for item in info.archive_contents:
    print(f"  - {item}")
```

### 4. Configuration & Data Files

- JSON: `.json`
- XML: `.xml`
- YAML: `.yaml`, `.yml`
- TOML: `.toml` (Rust, Python)
- INI: `.ini`, `.conf`, `.cfg`
- Properties: `.properties` (Java)

### 5. Build System Files

**Automatically Detected:**
- Make: `Makefile`, `*.mk`
- CMake: `CMakeLists.txt`, `*.cmake`
- Ninja: `build.ninja`
- Bazel: `BUILD`, `*.bzl`
- Gradle: `build.gradle`, `*.gradle`
- Maven: `pom.xml`

### 6. Resources

- **Images**: PNG, JPEG, GIF, SVG, ICO, BMP
- **Fonts**: TTF, OTF, WOFF, WOFF2
- **Audio**: MP3, WAV, OGG
- **Video**: MP4, AVI, MKV

### 7. Documentation

- Markdown: `.md`
- Text: `.txt`
- PDF: `.pdf`
- HTML: `.html`, `.htm`

### 8. Security Files

- **Certificates**: `.pem`, `.crt`, `.cer`, `.der`
- **Keys**: `.key`, `.pub`, `.ppk`

### 9. Database Files

- SQLite: `.db`, `.sqlite`, `.sqlite3`
- SQL: `.sql`

### 10. Build Artifacts

- LLVM Bitcode: `.bc`
- LLVM IR: `.ll`
- Precompiled Headers: `.pch`, `.gch`
- Debug Symbols: `.pdb` (Windows), `.dSYM` (macOS)

---

## Binary Analysis

### Extract Dependencies

```python
from buildfiletracker.filetypes import FileTypeDetector

# Analyze shared library
info = FileTypeDetector.analyze_file("/usr/lib/libssl.so.3")

print("Dependencies:")
for dep in info.dependencies:
    print(f"  - {dep}")

# Output:
# Dependencies:
#   - libcrypto.so.3
#   - libc.so.6
#   - ld-linux-x86-64.so.2
```

### Detect Architecture

```python
info = FileTypeDetector.analyze_file("myapp")

print(f"Architecture: {info.architecture}")
print(f"64-bit: {info.is_64bit}")
print(f"32-bit: {info.is_32bit}")

# Output:
# Architecture: x86_64
# 64-bit: True
# 32-bit: False
```

### Calculate Checksums

```python
info = FileTypeDetector.analyze_file("libfoo.so")

print(f"SHA256: {info.checksum_sha256}")

# Output:
# SHA256: a3c5f7e92b...
```

### Complete Binary Analysis

```python
info = FileTypeDetector.analyze_file("/usr/lib/libpython3.10.so")

print(f"Type: {info.filetype}")
print(f"Category: {', '.join(c.value for c in info.categories)}")
print(f"Size: {info.size:,} bytes")
print(f"Platform: {info.platform}")
print(f"Architecture: {info.architecture}")
print(f"Language: {info.language}")
print(f"Binary: {info.is_binary}")
print(f"Library: {info.is_library}")
print(f"Dependencies: {len(info.dependencies)}")
```

---

## Archive Analysis

### List Archive Contents

```python
from buildfiletracker.filetypes import FileTypeDetector

# Java JAR
info = FileTypeDetector.analyze_file("junit-4.13.jar")
print(f"JAR contains {len(info.archive_contents)} files")
for item in info.archive_contents[:10]:
    print(f"  {item}")

# Python Wheel
info = FileTypeDetector.analyze_file("numpy-1.24.0-cp310-cp310-linux_x86_64.whl")
print(f"Wheel contains {len(info.archive_contents)} files")

# ZIP Archive
info = FileTypeDetector.analyze_file("resources.zip")
for item in info.archive_contents:
    print(item)
```

### Analyze Python Packages

```python
# Track Python wheel installation
tracker = BuildFileTracker()
tracker.run_command(["pip", "install", "numpy"])

# Find all .whl files accessed
for filepath in tracker.file_access_map.keys():
    if filepath.endswith('.whl'):
        info = FileTypeDetector.analyze_file(filepath)
        print(f"Package: {Path(filepath).name}")
        print(f"  Files: {len(info.archive_contents)}")
        print(f"  Binary extensions: {sum(1 for f in info.archive_contents if f.endswith('.so'))}")
```

---

## File Type Detection

### Automatic Detection

```python
from buildfiletracker.filetypes import FileTypeDetector, FileType

# Detect from extension
ftype = FileTypeDetector.detect_type("main.cpp")
print(ftype)  # FileType.CPP_SOURCE

# Detect from magic bytes (for binaries)
ftype = FileTypeDetector.detect_type("libfoo.so")
print(ftype)  # FileType.SHARED_LIB

# Detect special filenames
ftype = FileTypeDetector.detect_type("Makefile")
print(ftype)  # FileType.MAKEFILE

ftype = FileTypeDetector.detect_type("CMakeLists.txt")
print(ftype)  # FileType.CMAKE
```

### Check Categories

```python
from buildfiletracker.filetypes import FileTypeDetector, FileCategory

info = FileTypeDetector.analyze_file("libssl.so")

if FileCategory.LIBRARY in info.categories:
    print("This is a library!")

if FileCategory.BINARY in info.categories:
    print("This is a binary file!")
```

### Extension Presets

```python
from buildfiletracker.filetypes import (
    EXTENSIONS_SOURCE,    # All source code
    EXTENSIONS_C_CPP,     # Just C/C++
    EXTENSIONS_BINARY,    # All binaries
    EXTENSIONS_LIBRARY,   # All libraries
    EXTENSIONS_PYTHON,    # Python files
    EXTENSIONS_JAVA,      # Java files
    EXTENSIONS_CONFIG,    # Config files
    EXTENSIONS_ALL,       # Everything
)

# Use with tracker
tracker = BuildFileTracker()
tracker.set_extension_filter(EXTENSIONS_BINARY)  # Track only binaries
```

---

## Practical Examples

### Example 1: Track All File Types in Build

```python
from buildfiletracker import BuildFileTracker, ReportFormat
from buildfiletracker.filetypes import FileTypeDetector, FileType

tracker = BuildFileTracker()
tracker.register_package("mylib", "/usr/local/mylib")

# Don't set extension filter → tracks everything
tracker.start()
tracker.run_command(["make", "all"])
tracker.stop()

# Analyze by file type
by_type = {}
for filepath in tracker.file_access_map.keys():
    ftype = FileTypeDetector.detect_type(filepath)
    by_type[ftype] = by_type.get(ftype, 0) + 1

print("Files by type:")
for ftype, count in sorted(by_type.items(), key=lambda x: x[1], reverse=True):
    print(f"  {ftype.value:30} {count:4} files")
```

### Example 2: Binary Dependency Report

```python
from buildfiletracker.filetypes import FileCategory

tracker = BuildFileTracker()
tracker.run_command(["make", "build"])

print("Binary Dependencies:")
for filepath in tracker.file_access_map.keys():
    info = FileTypeDetector.analyze_file(filepath)
    
    if FileCategory.LIBRARY in info.categories:
        print(f"\n{filepath}")
        print(f"  Architecture: {info.architecture}")
        print(f"  Size: {info.size:,} bytes")
        print(f"  Checksum: {info.checksum_sha256[:16]}...")
        
        if info.dependencies:
            print(f"  Depends on:")
            for dep in info.dependencies:
                print(f"    - {dep}")
```

### Example 3: Find Unused Binaries

```python
tracker = BuildFileTracker()
tracker.register_package("vendor", "/vendor/libs", version="1.0")

tracker.run_command(["make", "test"])

# Find all .so files in vendor
all_libs = list(Path("/vendor/libs").rglob("*.so"))

# Which were actually used?
used_libs = [f for f in tracker.file_access_map.keys() if f.endswith('.so')]

unused = set(str(lib) for lib in all_libs) - set(used_libs)

print(f"Unused libraries ({len(unused)}):")
for lib in unused:
    print(f"  {lib}")
```

### Example 4: Python Package Analysis

```python
import site

tracker = BuildFileTracker()

# Register site-packages
for site_dir in site.getsitepackages():
    tracker.add_watch(site_dir, recursive=True)

# Run Python application
tracker.run_command(["python", "myapp.py"])

# Analyze Python files
print("Python Dependencies:")
for filepath in tracker.file_access_map.keys():
    ftype = FileTypeDetector.detect_type(filepath)
    
    if ftype in (FileType.PYTHON_SOURCE, FileType.PYTHON_COMPILED, 
                 FileType.PYTHON_SO, FileType.PYTHON_WHEEL):
        print(f"  {Path(filepath).name}")
```

### Example 5: Multi-Language Project

```python
from buildfiletracker.filetypes import FileType

tracker = BuildFileTracker()
tracker.run_command(["gradle", "build"])  # Builds Java + native

# Categorize by language
languages = {}
for filepath in tracker.file_access_map.keys():
    info = FileTypeDetector.analyze_file(filepath)
    lang = info.language
    
    if lang not in languages:
        languages[lang] = []
    languages[lang].append(filepath)

print("Languages used:")
for lang, files in languages.items():
    print(f"  {lang}: {len(files)} files")
```

---

## API Reference

### FileType Enum

```python
from buildfiletracker.filetypes import FileType

# Access all file types
FileType.C_SOURCE
FileType.CPP_SOURCE
FileType.OBJECT
FileType.SHARED_LIB
FileType.JAVA_JAR
FileType.PYTHON_WHEEL
# ... and 50+ more
```

### FileCategory Enum

```python
from buildfiletracker.filetypes import FileCategory

FileCategory.SOURCE
FileCategory.BINARY
FileCategory.LIBRARY
FileCategory.ARCHIVE
FileCategory.CONFIG
FileCategory.RESOURCE
FileCategory.DOCUMENTATION
FileCategory.BUILD
FileCategory.DATA
FileCategory.PACKAGE
```

### FileInfo Dataclass

```python
@dataclass
class FileInfo:
    filepath: str
    filetype: FileType
    categories: Set[FileCategory]
    
    # Metadata
    size: int
    mtime: float
    checksum_sha256: str
    
    # Binary info
    is_binary: bool
    is_executable: bool
    is_library: bool
    is_32bit: bool
    is_64bit: bool
    
    # Dependencies
    dependencies: List[str]           # Linked libraries
    exported_symbols: List[str]       # Exported symbols
    imported_symbols: List[str]       # Imported symbols
    archive_contents: List[str]       # Files in archive
    
    # Platform info
    language: str
    platform: str
    architecture: str
```

### FileTypeDetector Class

```python
class FileTypeDetector:
    @classmethod
    def detect_type(cls, filepath: str) -> FileType:
        """Detect file type from path and magic bytes"""
    
    @classmethod
    def analyze_file(cls, filepath: str) -> FileInfo:
        """Complete file analysis with all metadata"""
    
    @classmethod
    def get_categories(cls, filetype: FileType) -> Set[FileCategory]:
        """Get categories for a file type"""
```

---

## Summary

BuildFileTracker now provides **complete file type tracking**:

✅ **60+ file types** across all categories  
✅ **Binary dependency extraction** from .so, .dll, .dylib  
✅ **Archive content listing** for .jar, .zip, .whl  
✅ **Architecture detection** (x86, x86_64, arm, arm64)  
✅ **Checksum calculation** for verification  
✅ **Multi-language support** (C/C++, Python, Java, JavaScript, Rust, Go, C#)  
✅ **Automatic detection** from extensions and magic bytes  

**Now you can track EVERYTHING in your builds!** 🎉

See also:
- [API Reference](API.md) - Complete API documentation
- [Examples](../examples/) - Working code examples
- [User Guide](USER_GUIDE.md) - Step-by-step tutorials
