# API Reference

## C Library API

### Core Functions

#### `void tracker_init(void)`

Initialize the file tracker system.

**Description**: Sets up the hash table and mutex locks. Called automatically when first file is accessed.

**Thread Safety**: Yes

**Example**:
```c
#include "file_tracker.h"

int main() {
    tracker_init();
    // Your code
    tracker_cleanup();
    return 0;
}
```

---

#### `void tracker_cleanup(void)`

Cleanup and generate reports.

**Description**: Frees all memory, writes reports to files specified by environment variables, and destroys mutexes.

**Environment Variables**:
- `FILE_TRACKER_JSON`: Path for JSON output
- `FILE_TRACKER_CSV`: Path for CSV output

**Thread Safety**: Yes

**Note**: Automatically registered with `atexit()`.

---

#### `void track_file_access(const char* filepath)`

Track access to a specific file.

**Parameters**:
- `filepath`: Absolute or relative path to the file

**Description**: Records file access in the hash table. If the file is already tracked, increments its access count.

**Thread Safety**: Yes

**Example**:
```c
track_file_access("/path/to/package_a/file.c");
```

---

#### `void write_report_json(const char* output_file)`

Write tracking data to JSON file.

**Parameters**:
- `output_file`: Path for output JSON file

**Description**: Generates a JSON report with all tracked files.

**Format**:
```json
{
  "report_type": "build_file_tracker",
  "timestamp": "1740499200",
  "accessed_files": [...]
}
```

---

#### `void write_report_csv(const char* output_file)`

Write tracking data to CSV file.

**Parameters**:
- `output_file`: Path for output CSV file

**Description**: Generates a CSV report.

**Format**:
```csv
filepath,package,file_type,access_count
/path/to/file.c,package_a,c,5
```

---

### Data Structures

#### `FileAccessEntry`

```c
typedef struct FileAccessEntry {
    char filepath[MAX_PATH_LENGTH];      // Full file path
    unsigned long access_count;           // Number of accesses
    char package_name[256];               // Detected package name
    char file_type[64];                   // File extension
    struct FileAccessEntry* next;         // Hash table chain
} FileAccessEntry;
```

#### `FileTracker`

```c
typedef struct {
    FileAccessEntry* buckets[HASH_TABLE_SIZE];
    pthread_mutex_t lock;
} FileTracker;
```

---

### Intercepted Functions

These functions are automatically intercepted when `LD_PRELOAD` is set:

#### `int open(const char* pathname, int flags, ...)`
Intercepts file open operations.

#### `FILE* fopen(const char* pathname, const char* mode)`
Intercepts standard C file open.

#### `int access(const char* pathname, int mode)`
Intercepts file access checks.

#### `int stat(const char* pathname, struct stat* statbuf)`
Intercepts file stat operations.

---

## Python API

### ReportGenerator Class

#### `__init__(self, input_json: str)`

Initialize report generator.

**Parameters**:
- `input_json`: Path to input JSON file from tracker

**Raises**:
- `FileNotFoundError`: If input file doesn't exist
- `json.JSONDecodeError`: If invalid JSON

**Example**:
```python
from report_generator import ReportGenerator

generator = ReportGenerator('report.json')
```

---

#### `generate_json(self, output_file: str, pretty: bool = True)`

Generate JSON report.

**Parameters**:
- `output_file`: Output file path
- `pretty`: Whether to format JSON (default: True)

**Example**:
```python
generator.generate_json('output.json', pretty=True)
```

---

#### `generate_csv(self, output_file: str)`

Generate CSV report.

**Parameters**:
- `output_file`: Output file path

**Example**:
```python
generator.generate_csv('output.csv')
```

---

#### `generate_xml(self, output_file: str)`

Generate XML report.

**Parameters**:
- `output_file`: Output file path

**Example**:
```python
generator.generate_xml('output.xml')
```

---

#### `generate_xlsx(self, output_file: str)`

Generate Excel report.

**Parameters**:
- `output_file`: Output file path

**Requires**: openpyxl library

**Example**:
```python
generator.generate_xlsx('output.xlsx')
```

---

#### `generate_summary(self, output_file: str)`

Generate text summary report.

**Parameters**:
- `output_file`: Output file path

**Example**:
```python
generator.generate_summary('summary.txt')
```

---

#### `generate_package_report(self, package_name: str, output_file: str)`

Generate report for specific package.

**Parameters**:
- `package_name`: Name of package to report
- `output_file`: Output file path

**Example**:
```python
generator.generate_package_report('openssl', 'openssl_report.txt')
```

---

### FileAccessAnalyzer Class

#### `__init__(self, input_json: str)`

Initialize analyzer.

**Parameters**:
- `input_json`: Path to input JSON file

**Example**:
```python
from analyzer import FileAccessAnalyzer

analyzer = FileAccessAnalyzer('report.json')
```

---

#### `get_package_dependencies(self) -> Dict[str, Set[str]]`

Get files grouped by package.

**Returns**: Dictionary mapping package names to file lists

**Example**:
```python
deps = analyzer.get_package_dependencies()
for package, files in deps.items():
    print(f"{package}: {len(files)} files")
```

---

#### `get_most_accessed_files(self, top_n: int = 10) -> List[Dict]`

Get most frequently accessed files.

**Parameters**:
- `top_n`: Number of top files to return

**Returns**: List of file dictionaries sorted by access count

**Example**:
```python
top_files = analyzer.get_most_accessed_files(20)
for file in top_files:
    print(f"{file['filepath']}: {file['access_count']} accesses")
```

---

#### `get_package_usage_stats(self) -> Dict[str, Dict]`

Calculate statistics per package.

**Returns**: Dictionary with package statistics

**Example**:
```python
stats = analyzer.get_package_usage_stats()
for pkg, data in stats.items():
    print(f"{pkg}: {data['file_count']} files, {data['total_accesses']} accesses")
```

---

#### `filter_by_package(self, package_name: str) -> List[Dict]`

Filter files by package name.

**Parameters**:
- `package_name`: Package to filter

**Returns**: List of file dictionaries

**Example**:
```python
openssl_files = analyzer.filter_by_package('openssl')
```

---

#### `filter_by_extension(self, extension: str) -> List[Dict]`

Filter files by extension.

**Parameters**:
- `extension`: File extension (with or without dot)

**Returns**: List of file dictionaries

**Example**:
```python
header_files = analyzer.filter_by_extension('.h')
```

---

#### `generate_dependency_graph(self, output_file: str)`

Generate text-based dependency graph.

**Parameters**:
- `output_file`: Output file path

**Example**:
```python
analyzer.generate_dependency_graph('deps.txt')
```

---

### BuildFileTracker Class (Python Integration)

#### `__init__(self, library_path: Optional[str] = None, output_dir: Optional[str] = None, json_output: Optional[str] = None, csv_output: Optional[str] = None)`

Initialize tracker for Python builds.

**Parameters**:
- `library_path`: Path to libfiletracker.so (auto-detect if None)
- `output_dir`: Output directory for reports
- `json_output`: Custom JSON output path
- `csv_output`: Custom CSV output path

**Example**:
```python
from buildfiletracker import BuildFileTracker

tracker = BuildFileTracker(
    library_path='/path/to/libfiletracker.so',
    output_dir='./reports'
)
```

---

#### `enable(self)`

Enable file tracking.

**Example**:
```python
tracker.enable()
# Run build commands
tracker.disable()
```

---

#### `disable(self)`

Disable file tracking.

---

#### `run_command(self, command: list) -> int`

Run command with tracking.

**Parameters**:
- `command`: Command and arguments as list

**Returns**: Exit code

**Example**:
```python
exit_code = tracker.run_command(['make', 'all'])
```

---

#### `track_function(self, func: Callable, *args, **kwargs)`

Track function execution.

**Parameters**:
- `func`: Function to execute
- `*args, **kwargs`: Function arguments

**Returns**: Function result

**Example**:
```python
def my_build():
    subprocess.run(['make', 'all'])

tracker.track_function(my_build)
```

---

### Context Manager

#### `track_build(output_dir: str = './build_reports', library_path: Optional[str] = None)`

Context manager for tracking.

**Parameters**:
- `output_dir`: Output directory
- `library_path`: Library path (auto-detect if None)

**Example**:
```python
from buildfiletracker import track_build

with track_build(output_dir='./reports'):
    subprocess.run(['make', 'all'])
```

---

## Environment Variables

### Core Variables

| Variable | Description | Example |
|----------|-------------|---------|
| `LD_PRELOAD` | Path to libfiletracker.so | `/path/to/libfiletracker.so` |
| `FILE_TRACKER_JSON` | JSON output path | `./report.json` |
| `FILE_TRACKER_CSV` | CSV output path | `./report.csv` |

### Integration Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `FILETRACKER_LIB` | Custom library path | Auto-detect |
| `FILETRACKER_OUTPUT_DIR` | Output directory | `./build_reports` |
| `FILETRACKER_ENABLED` | Enable/disable tracking | `1` |

---

## CMake Functions

### `enable_file_tracking(TARGET_NAME)`

Enable tracking for a CMake target.

**Parameters**:
- `TARGET_NAME`: Name of the CMake target

**Example**:
```cmake
add_executable(myapp main.c)
enable_file_tracking(myapp)
```

---

## Return Values and Error Codes

### C Library

- All intercepted functions return the same values as their original counterparts
- Tracking failures are logged to stderr but don't affect the build
- Exit code 0 indicates successful report generation

### Python Tools

- `0`: Success
- `1`: File not found or invalid JSON
- `2`: Missing dependencies (e.g., openpyxl)

---

## Thread Safety

All C library functions are thread-safe and can be used in parallel builds:
- CMake with `-j`
- Make with `-j`
- Ninja (parallel by default)

Python tools are not thread-safe for concurrent report generation to the same file.

---

## Examples

### Complete C Example

```c
#include "file_tracker.h"
#include <stdio.h>

int main() {
    // Tracking is automatic with LD_PRELOAD
    FILE* f = fopen("data.txt", "r");
    if (f) {
        // File access is tracked
        fclose(f);
    }
    
    // Report generated automatically on exit
    return 0;
}
```

### Complete Python Example

```python
from buildfiletracker import track_build
import subprocess

# Method 1: Context manager
with track_build(output_dir='./reports'):
    subprocess.run(['make', 'all'])

# Method 2: Manual control
from buildfiletracker import BuildFileTracker

tracker = BuildFileTracker()
tracker.enable()
subprocess.run(['make', 'all'])
tracker.disable()

# Generate reports
from report_generator import ReportGenerator

generator = ReportGenerator('reports/file_access_*.json')
generator.generate_xlsx('final_report.xlsx')
generator.generate_summary('summary.txt')
```

---

## Best Practices

1. **Always clean before tracking**: `make clean && track_build.sh make all`
2. **Use absolute paths**: For library path and output files
3. **Check reports exist**: Before processing
4. **Handle errors**: Check return codes and exceptions
5. **Filter appropriately**: Customize filtering in C code for large projects

---

## Limitations

1. **Platform**: LD_PRELOAD is Linux/Unix specific
2. **Statically linked binaries**: Cannot be intercepted
3. **Direct syscalls**: Not intercepted (rare in normal builds)
4. **Memory**: Hash table grows with unique file count

---

## Further Reading

- [User Guide](USER_GUIDE.md)
- [Integration Guide](INTEGRATION.md)
- [FAQ](FAQ.md)
- [Examples](../examples/)
