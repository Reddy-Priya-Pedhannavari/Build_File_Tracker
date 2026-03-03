# Frequently Asked Questions (FAQ)

## General Questions

### Q: What is BuildFileTracker?

BuildFileTracker is a universal build monitoring tool that tracks which files from external packages are actually used during your build process. It helps you understand real dependencies vs. declared dependencies.

### Q: How does it work?

It uses LD_PRELOAD (on Linux) to intercept file system calls during the build process. When your build accesses files (open, read, etc.), BuildFileTracker records this information and generates detailed reports.

### Q: Do I need to modify my code?

No! BuildFileTracker works by intercepting system calls, so you don't need to modify your source code or build scripts. Just wrap your build command or set environment variables.

### Q: Is it safe to use in production builds?

Yes, but it's primarily designed for analysis. The overhead is minimal, but you typically want to use it during development or CI/CD analysis, not in production deployments.

## Compatibility

### Q: Which operating systems are supported?

- **Linux**: Full support (recommended)
- **macOS**: Partial support (requires SIP configuration or DYLD_INSERT_LIBRARIES)
- **Windows**: Use WSL2 (Windows Subsystem for Linux)

### Q: Which build systems are supported?

All of them! BuildFileTracker works at the system call level, so it supports:
- Make
- CMake
- Autotools
- Meson
- Bazel
- Yocto/OpenEmbedded
- Cargo (Rust)
- Go build
- npm/yarn
- Maven/Gradle
- Any custom build system

### Q: Can I use it with interpreted languages?

Yes! While it's designed for compiled languages, it can track file access for any build process, including:
- Python (setuptools, Poetry)
- Node.js (npm, webpack)
- Ruby (gem build)
- Java (Maven, Gradle)

## Technical Questions

### Q: What's the performance impact?

Minimal. The interception adds microseconds per file operation. In practice, the overhead is typically <5% of total build time, often much less.

### Q: Will it track ALL files?

It tracks files accessed during the build. Some filters are applied automatically:
- `/proc/` and `/sys/` (Linux virtual filesystems)
- `/dev/` (device files)
- Temporary X11 sockets

You can customize filters by modifying `src/file_tracker.c`.

### Q: How much memory does it use?

Very little. The hash table scales with the number of unique files accessed. For typical builds with thousands of files, memory usage is <100MB.

### Q: Is it thread-safe?

Yes! All operations on the tracking data structure are protected by mutexes, making it safe for parallel builds (make -j, ninja, etc.).

### Q: Does it work with ccache/sccache?

Yes, but you'll track cache accesses too. For the most accurate results, do a clean build:
```bash
ccache -C  # Clear ccache
make clean
./integrations/track_build.sh make all
```

## Usage Questions

### Q: How do I track a simple make build?

```bash
./integrations/track_build.sh make all
```

Or manually:
```bash
export LD_PRELOAD=/path/to/libfiletracker.so
export FILE_TRACKER_JSON=./report.json
make all
```

### Q: How do I generate reports?

```bash
# All formats
python3 python/report_generator.py report.json -f all

# Specific format
python3 python/report_generator.py report.json -f xlsx
```

### Q: Can I track multiple builds and compare?

Yes! Use different output files:
```bash
# Build 1
FILE_TRACKER_JSON=report_v1.json ./integrations/track_build.sh make all

# Build 2 (after changes)
FILE_TRACKER_JSON=report_v2.json ./integrations/track_build.sh make all

# Compare statistics
python3 python/analyzer.py report_v1.json --stats > stats_v1.txt
python3 python/analyzer.py report_v2.json --stats > stats_v2.txt
diff stats_v1.txt stats_v2.txt
```

### Q: How do I track a package-specific dependency?

```bash
# Generate package report
python3 python/report_generator.py report.json --package openssl

# Analyze package usage
python3 python/analyzer.py report.json --package openssl
```

### Q: Can I filter by file type?

Yes:
```bash
# Show only header files
python3 python/analyzer.py report.json --extension .h

# Show only C source files
python3 python/analyzer.py report.json --extension .c
```

## Troubleshooting

### Q: "Library not found" error

```bash
# Check if library built
ls -l src/libfiletracker.so

# If not, build it
cd src && make

# Verify path in scripts
export FILETRACKER_LIB=$PWD/src/libfiletracker.so
```

### Q: No report generated

Common causes:
1. **Environment variables not set**
   ```bash
   echo $FILE_TRACKER_JSON  # Should show path
   ```

2. **Output directory doesn't exist**
   ```bash
   mkdir -p ./reports
   export FILE_TRACKER_JSON=./reports/report.json
   ```

3. **Permissions issue**
   ```bash
   chmod 755 ./reports
   ```

4. **Build completed too quickly**
   - Report is generated on exit
   - Ensure build actually compiled/linked files

### Q: LD_PRELOAD not working on macOS

macOS has System Integrity Protection (SIP) that restricts LD_PRELOAD.

**Solutions:**
1. Use DYLD_INSERT_LIBRARIES (limited):
   ```bash
   export DYLD_INSERT_LIBRARIES=/path/to/libfiletracker.so
   export DYLD_FORCE_FLAT_NAMESPACE=1
   ```

2. Disable SIP (not recommended for production):
   - Boot into Recovery Mode
   - Run: `csrutil disable`
   - Reboot

3. Use Docker/VM with Linux

### Q: Reports show system libraries

This is usually normal - your build does use system headers. To focus on external packages:

```bash
# Filter by package name
python3 python/analyzer.py report.json --package mypackage

# Or post-process JSON
jq '.accessed_files[] | select(.package != "unknown")' report.json
```

### Q: Different results on different machines

Normal! File paths differ between machines. Focus on:
- Package names
- File types
- Relative access patterns

### Q: "Error: Invalid JSON" when generating reports

The JSON might be corrupted or incomplete:

1. **Check file size**
   ```bash
   ls -lh report.json
   ```

2. **Validate JSON**
   ```bash
   python3 -m json.tool report.json
   ```

3. **Check build succeeded**
   - If build failed, report might be incomplete

4. **Try rebuilding**
   ```bash
   rm report.json
   ./integrations/track_build.sh make all
   ```

## Integration Questions

### Q: How do I integrate with CMake?

Add to `CMakeLists.txt`:
```cmake
include(/path/to/FileTracker.cmake)
```

Then build:
```bash
make build_with_tracking
```

See [Integration Guide](INTEGRATION.md) for details.

### Q: How do I integrate with CI/CD?

Example for GitHub Actions:
```yaml
- name: Build with tracking
  run: ./integrations/track_build.sh make all

- name: Generate reports
  run: python3 python/report_generator.py report.json -f all

- name: Upload
  uses: actions/upload-artifact@v2
  with:
    name: reports
    path: report_*
```

See [Integration Guide](INTEGRATION.md) for more CI/CD examples.

### Q: Can I use it in Docker?

Yes! Example Dockerfile:
```dockerfile
FROM ubuntu:22.04
COPY buildfiletracker /opt/buildfiletracker
WORKDIR /opt/buildfiletracker/src
RUN make
ENV LD_PRELOAD=/opt/buildfiletracker/src/libfiletracker.so
ENV FILE_TRACKER_JSON=/reports/report.json
```

## Advanced Questions

### Q: Can I customize what gets tracked?

Yes! Edit `src/file_tracker.c`, function `track_file_access()`:

```c
// Skip custom paths
if (strstr(filepath, "/my/custom/path/")) {
    return;
}

// Only track specific extensions
if (!strstr(filepath, ".c") && !strstr(filepath, ".h")) {
    return;
}
```

Then rebuild:
```bash
cd src && make clean && make
```

### Q: Can I add custom metadata to reports?

Yes! You can post-process the JSON:

```python
import json

with open('report.json', 'r') as f:
    data = json.load(f)

# Add custom fields
data['build_version'] = '1.2.3'
data['build_machine'] = 'ci-server-01'

with open('report_enhanced.json', 'w') as f:
    json.dump(data, f, indent=2)
```

### Q: Can I track file writes separately from reads?

Currently, only read operations are tracked. To track writes, modify `src/file_interceptor.c`:

```c
// In open() function, track all opens
track_file_access(pathname);

// Or separate read/write tracking
if ((flags & O_ACCMODE) == O_WRONLY) {
    track_file_write(pathname);
}
```

### Q: Can I get real-time monitoring?

Not currently. Reports are generated when the program exits. For real-time monitoring, consider:
- Using strace/ltrace in parallel
- Implementing a real-time output mode (future feature)

### Q: Database storage instead of JSON?

You can implement custom storage:

1. Modify `src/file_tracker.c`
2. Replace `write_report_json()` with database writes
3. Or post-process JSON into database:

```python
import json
import sqlite3

conn = sqlite3.connect('tracking.db')
with open('report.json') as f:
    data = json.load(f)
    for file in data['accessed_files']:
        conn.execute('INSERT INTO files VALUES (?,?,?,?)',
                    (file['filepath'], file['package'], 
                     file['file_type'], file['access_count']))
```

## Performance Questions

### Q: How can I speed up report generation?

1. **Use CSV for large datasets** (faster than JSON)
2. **Skip XLSX** if not needed (requires openpyxl processing)
3. **Filter early**:
   ```bash
   # Only track specific directories
   export FILETRACKER_INCLUDE_PATTERN="/path/to/packages/*"
   ```

### Q: Large report files?

For builds accessing 100,000+ files:

1. **Use CSV** (more compact)
2. **Compress reports**:
   ```bash
   gzip report.json
   ```
3. **Filter in C code** (before writing)
4. **Split reports by package** (custom implementation)

## Licensing and Contributions

### Q: What's the license?

GNU GPLv3 - free for commercial and personal use, with copyleft requirements.

### Q: Can I contribute?

Yes! Contributions welcome:
1. Fork the repository
2. Create a feature branch
3. Submit a pull request

### Q: How do I report bugs?

File an issue on GitHub with:
- OS and version
- Build system
- Command used
- Error message
- Relevant excerpts from build log

##Still Have Questions?

- Check the [User Guide](USER_GUIDE.md)
- Review [Examples](../examples/)
- Open an issue on GitHub
- Email: your.email@example.com
