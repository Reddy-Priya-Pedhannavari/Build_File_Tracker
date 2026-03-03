# Linking Types Tracking - Comprehensive Guide

BuildFileTracker **automatically detects and reports the linking type** for each file in JSON, CSV, XLSX, and other report formats. This comprehensive guide explains linking types, how detection works, and how to analyze reports.

---

## Table of Contents

1. [What is Linking?](#what-is-linking)
2. [Linking Types Tracked](#linking-types-tracked)
3. [How Detection Works](#how-detection-works)
4. [Reports & Output Formats](#reports--output-formats)
5. [Analyzing Reports](#analyzing-reports)
6. [Platform-Specific Linking](#platform-specific-linking)
7. [Practical Examples](#practical-examples)
8. [Advanced Analysis](#advanced-analysis)

---

## What is Linking?

**Linking** is the process of combining compiled code with libraries to create an executable or shared library. There are multiple types:

- **Static Linking**: Library code is copied into your binary at compile time
- **Dynamic Linking**: Library is loaded at runtime; binary contains references only
- **Load-Time Linking**: Dependencies resolved when program starts
- **Runtime Linking**: Dependencies loaded on-demand (plugins, dlopen)
- **Weak Linking**: Optional dependencies that may or may not be present

---

## Linking Types Tracked

### 1. **Static Linking** ✅

When static libraries are linked into your binary at compile time.

**File Types:**
- **Linux/Unix/macOS**: `.a` (archive)
- **Windows**: `.lib` (static library)

**Detection Example:**
```json
{
  "filepath": "/usr/lib/x86_64-linux-gnu/libcrypto.a",
  "file_type": "a",
  "linking_type": "static",
  "access_count": 1,
  "package": "openssl"
}
```

**Build Command:**
```bash
gcc main.c -static /usr/lib/libcrypto.a -o app
```

**Characteristics:**
- ✅ **Pros**: No runtime dependencies, faster execution
- ❌ **Cons**: Larger binary size, no shared code between processes

---

### 2. **Dynamic Linking (Shared Libraries)** ✅

Most common linking type - libraries loaded at runtime.

**File Types:**
- **Linux**: `.so` (shared object)
- **macOS**: `.dylib` (dynamic library)
- **Windows**: `.dll` (dynamic-link library)

**Detection Example:**
```json
{
  "filepath": "/usr/lib/x86_64-linux-gnu/libssl.so.3",
  "file_type": "so",
  "linking_type": "dynamic",
  "access_count": 2,
  "package": "openssl",
  "dependencies": [
    "libcrypto.so.3",
    "libc.so.6"
  ]
}
```

**Build Command:**
```bash
gcc main.c -lssl -lcrypto -o app
```

**Characteristics:**
- ✅ **Pros**: Smaller binaries, shared code, updatable without recompilation
- ❌ **Cons**: Runtime dependency, slightly slower startup

**Tools Used for Dependency Extraction:**
- **Linux**: `ldd` - list dynamic dependencies
- **macOS**: `otool -L` - list dynamic libraries
- **Windows**: `dumpbin /DEPENDENTS` or `objdump -p`

---

### 3. **Import Library Linking (Windows)** ✅

Windows-specific: `.lib` files that reference `.dll` files.

**File Types:**
- **Windows**: `.lib` (import library for DLL)

**Detection Example:**
```json
{
  "filepath": "C:\\Windows\\System32\\kernel32.lib",
  "file_type": "lib",
  "linking_type": "import",
  "import_target": "kernel32.dll",
  "package": "windows-sdk"
}
```

**Build Command:**
```cmd
cl.exe main.c kernel32.lib user32.lib
```

**Note**: Distinguishing static `.lib` from import `.lib` requires header analysis. BuildFileTracker detects Windows SDK libraries as "import" type.

---

### 4. **Object Files** ✅

Compiled but not yet linked object files.

**File Types:**
- **Linux/macOS**: `.o` (object file)
- **Windows**: `.obj` (object file)

**Detection Example:**
```json
{
  "filepath": "build/main.o",
  "file_type": "o",
  "linking_type": "object",
  "package": "my_project",
  "source_paths": ["src/main.c"]
}
```

**When Tracked:**
Object files are accessed during:
- Compilation phase (created)
- Linking phase (read to extract symbols)
- Build artifact archiving

**With Debug Info**: Source mapping shows original `.c`/`.cpp` files from DWARF information.

---

### 5. **Python Extensions** ✅

Python compiled native extensions.

**File Types:**
- **Linux**: `.so`
- **Windows**: `.pyd`

**Detection Example:**
```json
{
  "filepath": "/usr/lib/python3.10/site-packages/numpy/_core.so",
  "file_type": "so",
  "linking_type": "python_extension",
  "python_version": "3.10",
  "architecture": "x86_64",
  "package": "numpy"
}
```

**Build Command:**
```bash
python setup.py build_ext --inplace
```

**Characteristics:**
- Compiled C/C++ code that loads into Python interpreter
- Links against Python development libraries
- Platform & Python version specific

---

### 6. **Framework Linking (macOS)** ✅

macOS-specific: Frameworks bundle headers, libraries, and resources.

**File Types:**
- **macOS**: `.framework` directories

**Detection Example:**
```json
{
  "filepath": "/System/Library/Frameworks/Foundation.framework/Foundation",
  "file_type": "framework",
  "linking_type": "framework",
  "package": "Foundation",
  "framework_version": "C"
}
```

**Build Command:**
```bash
clang main.m -framework Cocoa -framework Foundation -o app
```

**Note**: macOS frameworks bundle headers, dylibs, and resources into a structured directory.

---

### 7. **Runtime Plugins** ✅

Libraries loaded on-demand during execution (dlopen, LoadLibrary).

**File Types:**
- **Linux**: `.so` (in plugin directories)
- **macOS**: `.dylib` or `.bundle`
- **Windows**: `.dll` (in plugin directories)

**Detection Example:**
```json
{
  "filepath": "/usr/lib/plugins/libmyplugin.so",
  "file_type": "so",
  "linking_type": "runtime_plugin",
  "access_count": 1,
  "loaded_by": "myapp",
  "package": "myplugin"
}
```

**Examples:**
- Python C Extensions (`.so`, `.pyd`)
- Browser Plugins (`.so`, `.dll`)
- Game Engine Plugin Modules
- Apache Modules (`mod_*.so`)

---

### 8. **Link-Time Optimization (LTO) Bitcode** ✅

Whole-program optimization during linking.

**File Types:**
- **LLVM**: `.bc` (bitcode)
- **GCC**: `.o` with LTO sections

**Detection Example:**
```json
{
  "filepath": "module.bc",
  "file_type": "bc",
  "linking_type": "lto_bitcode",
  "optimization_level": "O3"
}
```

---

### 9. **Unknown/Other** ✅

Files with extensions not specifically recognized.

**Detection Example:**
```json
{
  "filepath": "custom_file.xyz",
  "file_type": "xyz",
  "linking_type": "unknown",
  "package": "custom"
}
```

---

## How Detection Works

### File Access Tracking

BuildFileTracker uses **LD_PRELOAD** (Linux/macOS) or **Python file monitoring** (Windows) to intercept:
- `open()`, `fopen()` - File opens
- `dlopen()` - Runtime library loading
- `mmap()` - Memory-mapped libraries

### Extension-Based Detection

The detection algorithm:

1. **Extract file extension** (`.so`, `.a`, `.dll`, etc.)
2. **Check for special paths**:
   - `.framework` → "framework"
   - `site-packages` → "python_extension"
   - `plugins`, `plugin` → "runtime_plugin"
3. **Apply heuristics**:
   - Windows system directories `.lib` → "import"
   - Other `.lib` → "static"
4. **Further classification**:
   - `.a`, `.lib` (non-system) → "static"
   - `.so`, `.dll`, `.dylib` → "dynamic"
   - `.o`, `.obj` → "object"
   - `.pyd` → "python_extension"
   - `.bc` → "lto_bitcode"
   - Other → "unknown"

### Source Mapping for Object Files

When debug information is available, DWARF extraction locates original source files:

```bash
# Tool selection (automatic):
# macOS:  llvm-dwarfdump (preferred)
# Linux:  readelf or dwarfdump
# Win:    objdump or WinDbg
```

**Result in Report:**
```json
{
  "filepath": "build/main.o",
  "linking_type": "object",
  "source_paths": ["src/main.c", "src/main.h"]
}
```

---

## Reports & Output Formats

### JSON Report Format

Every file includes the `linking_type` field:

```json
{
  "report_type": "build_file_tracker",
  "timestamp": "1740499200",
  "summary": {
    "total_files": 12,
    "total_accesses": 45,
    "unique_packages": 3
  },
  "accessed_files": [
    {
      "filepath": "/usr/include/openssl/ssl.h",
      "package": "openssl",
      "file_type": "h",
      "linking_type": "dynamic",
      "access_count": 8
    },
    {
      "filepath": "/usr/lib/x86_64-linux-gnu/libssl.so.3",
      "package": "openssl",
      "file_type": "so",
      "linking_type": "dynamic",
      "access_count": 2
    },
    {
      "filepath": "/usr/lib/x86_64-linux-gnu/libcrypto.a",
      "package": "openssl",
      "file_type": "a",
      "linking_type": "static",
      "access_count": 1
    }
  ],
  "by_linking_type": {
    "dynamic": 3,
    "static": 2,
    "object": 5,
    "unknown": 2
  }
}
```

### CSV Report Format

Includes `Linking Type` column:

```
Filepath,Package,File Type,Linking Type,Access Count
/usr/include/openssl/ssl.h,openssl,h,dynamic,8
/usr/lib/x86_64-linux-gnu/libssl.so.3,openssl,so,dynamic,2
/usr/lib/x86_64-linux-gnu/libcrypto.a,openssl,a,static,1
main.c,my_project,c,unknown,3
```

### Excel (XLSX) Report Format

Spreadsheet with **Linking Type** column and conditional formatting.

| Filepath | Package | File Type | Linking Type | Access Count |
|----------|---------|-----------|--------------|--------------|
| /usr/include/openssl/ssl.h | openssl | h | dynamic | 8 |
| /usr/lib/libssl.so.3 | openssl | so | dynamic | 2 |
| /usr/lib/libcrypto.a | openssl | a | static | 1 |

### Summary Report Format

```
======================================================================
Build File Tracker - Summary Report
======================================================================

Report Date: 2026-02-26 14:30:45
Total Unique Files Accessed: 45
Total File Accesses: 234

----------------------------------------------------------------------
Linking Type Breakdown:
----------------------------------------------------------------------
  dynamic              :    18 files ( 40.00%)
  static               :     8 files ( 17.78%)
  python_extension     :     5 files ( 11.11%)
  object               :     3 files (  6.67%)
  framework            :     2 files (  4.44%)
  unknown              :     9 files ( 20.00%)

----------------------------------------------------------------------
Files by Package:
----------------------------------------------------------------------
  openssl                                  :    12 files
  numpy                                    :     8 files
  glibc                                    :     6 files
```

### Component Grouping Output

Generate with `--components` flag:

```bash
python report_generator.py build_tracking.json -f all --components
```

Produces `<output>_components.json`:

```json
{
  "report_type": "build_file_tracker_components",
  "component_grouping": "package",
  "components": {
    "openssl": {
      "sources": ["src/crypto.c"],
      "headers": ["include/openssl/ssl.h"],
      "objects": ["build/ssl.o"],
      "source_paths_from_objects": ["src/main.c"],
      "libraries": {
        "static": ["libcrypto.a"],
        "dynamic": ["libssl.so.3"]
      }
    }
  }
}
```

---

## Analyzing Reports

### 1. Identify Static vs Dynamic Linking

```python
import json

with open('build_tracking.json') as f:
    data = json.load(f)

# Count by linking type
static_files = [f for f in data['accessed_files'] 
                if f['linking_type'] == 'static']
dynamic_files = [f for f in data['accessed_files'] 
                 if f['linking_type'] == 'dynamic']

print(f"Static libraries: {len(static_files)}")
print(f"Dynamic libraries: {len(dynamic_files)}")
```

### 2. Find Python Extensions

```python
# Find all Python extensions used
python_exts = [f for f in data['accessed_files'] 
               if f['linking_type'] == 'python_extension']

print(f"Python extensions: {len(python_exts)}")
for ext in python_exts:
    print(f"  - {ext['filepath']} ({ext['package']})")
```

### 3. Detect Unnecessary Static Linking

```python
# Check for static libs where dynamic versions exist
static_libs = [f for f in data['accessed_files'] 
              if f['linking_type'] == 'static']

for lib in static_libs:
    dynamic_version = lib['filepath'].replace('.a', '.so')
    print(f"{lib['filepath']}")
    print(f"  Consider dynamic: {dynamic_version}")
```

### 4. Extract Linking Type Summary

```bash
# Quick command-line analysis
grep '"linking_type"' build_tracking.json | \
    sed 's/.*"linking_type": "\([^"]*\)".*/\1/' | \
    sort | uniq -c | sort -rn
```

Output:
```
     18 dynamic
      8 static
      5 python_extension
      3 object
      2 framework
      9 unknown
```

---

## Platform-Specific Linking

### Linux Linking

```bash
# Static linking
gcc main.c -static -o app_static
# Tracked: main.c, *.a files, libc.a

# Dynamic linking (default)
gcc main.c -o app_dynamic
# Tracked: main.c, *.so files

# Mixed linking
gcc main.c -static-libgcc -o app_mixed
# Tracked: libgcc.a (static), libc.so (dynamic)
```

### macOS Linking

```bash
# Framework linking
clang main.m -framework Foundation -o app
# Tracked: Foundation.framework

# Two-level namespace (default)
clang main.c -dynamiclib -o libexample.dylib
# Dependencies: Fully qualified paths

# Weak linking (optional dependencies)
clang main.c -weak_loptional -o app
# Optional library can be missing at runtime
```

### Windows Linking

```cmd
REM Dynamic linking via import library
cl.exe main.c kernel32.lib user32.lib
REM Tracked: main.c, *.lib (import), *.dll (runtime)

REM Static linking
cl.exe main.c /MT libstatic.lib
REM Tracked: main.c, libstatic.lib
```

---

## Practical Examples

### Example 1: GCC with Mixed Linking

```bash
export LD_PRELOAD=./libfiletracker.so
export FILE_TRACKER_JSON=./report.json

gcc main.c -static /usr/lib/libcrypto.a -lssl -o app

unset LD_PRELOAD
```

**Generated Report:**
```json
[
  {"filepath": "main.c", "linking_type": "unknown"},
  {"filepath": "/usr/lib/libcrypto.a", "linking_type": "static"},
  {"filepath": "/usr/lib/libssl.so", "linking_type": "dynamic"}
]
```

### Example 2: CMake Project

```bash
export LD_PRELOAD=./libfiletracker.so
export FILE_TRACKER_JSON=./report.json

cmake --build . --config Release

unset LD_PRELOAD
```

**Generated Report:**
```json
[
  {"filepath": "CMakeLists.txt", "linking_type": "unknown"},
  {"filepath": "src/main.cpp", "linking_type": "unknown"},
  {"filepath": "/usr/lib/libboost_system.so", "linking_type": "dynamic"},
  {"filepath": "/usr/lib/libssl.a", "linking_type": "static"}
]
```

### Example 3: Python Package Build

```bash
export FILE_TRACKER_JSON=./report.json
python setup.py build_ext --inplace
```

**Generated Report:**
```json
[
  {"filepath": "setup.py", "linking_type": "unknown"},
  {"filepath": "/usr/include/python3.10/Python.h", "linking_type": "dynamic"},
  {"filepath": "myext.so", "linking_type": "python_extension"}
]
```

### Example 4: Compare Static vs Dynamic Builds

```bash
# Build 1: Static
gcc main.c -static -o app_static

# Build 2: Dynamic  
gcc main.c -o app_dynamic

# Size comparison
ls -lh app_*
# app_static: 2.3 MB (includes all code)
# app_dynamic: 16 KB (depends on .so files)
```

---

## Advanced Analysis

### Visualize Dependency Tree

```python
import json
import networkx as nx
import matplotlib.pyplot as plt

with open('build_tracking.json') as f:
    data = json.load(f)

G = nx.DiGraph()

# Add nodes for each library
for file in data['accessed_files']:
    if file['file_type'] in ['so', 'dll', 'dylib', 'a', 'lib']:
        G.add_node(file['filepath'])
        
        # Add edges for dependencies
        if 'dependencies' in file:
            for dep in file['dependencies']:
                G.add_edge(file['filepath'], dep)

# Draw graph
pos = nx.spring_layout(G)
nx.draw(G, pos, with_labels=True, node_color='lightblue', 
        node_size=3000, font_size=8)
plt.savefig('dependency_graph.png', dpi=150, bbox_inches='tight')
```

### Calculate Linking Efficiency

```python
def analyze_efficiency(tracking_data):
    """Calculate what percentage of each library is actually used"""
    
    results = []
    
    for lib in tracking_data['accessed_files']:
        if lib['file_type'] == 'a':  # Static library
            # Count symbols used vs available
            used_symbols = len(lib.get('used_symbols', []))
            total_symbols = lib.get('total_symbols', 1)
            
            efficiency = (used_symbols / total_symbols * 100) if total_symbols > 0 else 0
            
            results.append({
                'library': lib['filepath'],
                'used': used_symbols,
                'total': total_symbols,
                'efficiency': efficiency
            })
    
    # Show low-efficiency libraries
    low_efficiency = [r for r in results if r['efficiency'] < 10]
    
    if low_efficiency:
        print("⚠️  Low Efficiency Libraries:")
        for item in low_efficiency:
            print(f"  {item['library']}: {item['efficiency']:.1f}%")
            print(f"    Using {item['used']} of {item['total']} symbols")
            print(f"    Consider: Extract needed symbols or use dynamic linking")
```

### Generate Loading Timeline

```python
# Show in what order libraries were accessed during build
import json

with open('build_tracking.json') as f:
    data = json.load(f)

libraries = [f for f in data['accessed_files'] 
            if f['linking_type'] in ['static', 'dynamic']]

# Sort by access sequence (approximated by file order)
for i, lib in enumerate(libraries, 1):
    print(f"{i:2}. {lib['linking_type']:10} - {lib['filepath']}")
```

---

## Summary

| Linking Type | File Extension | Platform | Use Case |
|--------------|----------------|----------|----------|
| Static | `.a`, `.lib` | Linux/Windows/macOS | Embedded, standalone binaries |
| Dynamic | `.so`, `.dll`, `.dylib` | All | Standard applications |
| Import | `.lib` | Windows | DLL linking on Windows |
| Object | `.o`, `.obj` | All | Intermediate compilation |
| Python Extension | `.so`, `.pyd` | All | Python native modules |
| Framework | `.framework` | macOS | macOS/iOS applications |
| Runtime Plugin | `.so`, `.dll` | All | Plugins, extensions |
| LTO Bitcode | `.bc` | All | Whole-program optimization |

---

## Next Steps

1. **Generate a report** from your next build
2. **Analyze linking patterns** using Python scripts or command-line tools
3. **Optimize** by adjusting static/dynamic linking as needed
4. **Reduce dependencies** by identifying truly used vs. unused libraries

See also:
- [Using with Your Projects](USING_WITH_YOUR_PROJECTS.md)
- [File Types Guide](FILE_TYPES.md)
- [Binary Tracking Example](../examples/binary_tracking_example.py)
