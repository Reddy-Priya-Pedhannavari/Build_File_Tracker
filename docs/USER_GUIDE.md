# User Guide

## Table of Contents
1. [Quick Start](#quick-start)
2. [Basic Usage](#basic-usage)
3. [Report Formats](#report-formats)
4. [Advanced Features](#advanced-features)
5. [Best Practices](#best-practices)

## Quick Start

### Step 1: Build the Library

```bash
cd src
make
```

### Step 2: Track a Build

```bash
./integrations/track_build.sh make all
```

### Step 3: View Report

```bash
python3 python/report_generator.py build_reports/file_access_*.json -f summary
```

## Basic Usage

### Method 1: Universal Wrapper Script

The easiest way to track any build:

```bash
./integrations/track_build.sh <your-build-command>
```

Examples:
```bash
./integrations/track_build.sh make
./integrations/track_build.sh cmake --build build
./integrations/track_build.sh ninja -C build
./integrations/track_build.sh ./build_script.sh
```

### Method 2: Manual LD_PRELOAD

For more control:

```bash
export LD_PRELOAD=/path/to/libfiletracker.so
export FILE_TRACKER_JSON=./my_report.json
export FILE_TRACKER_CSV=./my_report.csv

# Run your build
make all

# Cleanup
unset LD_PRELOAD
```

### Method 3: Build System Integration

#### CMake
```cmake
include(/path/to/FileTracker.cmake)
# Then: make build_with_tracking
```

#### Makefile
```makefile
include /path/to/filetracker.mk
# Then: make build-with-tracking
```

## Report Formats

### JSON Report

Structured data, ideal for automation:

```bash
python3 python/report_generator.py report.json -f json -o output
```

**Use cases:**
- CI/CD pipelines
- Automated analysis
- API integration

### CSV Report

Spreadsheet-compatible:

```bash
python3 python/report_generator.py report.json -f csv -o output
```

**Use cases:**
- Excel analysis
- Data visualization
- Sharing with non-technical stakeholders

### XML Report

Standard structured format:

```bash
python3 python/report_generator.py report.json -f xml -o output
```

**Use cases:**
- Tool integration
- Enterprise systems
- Legacy system compatibility

### XLSX Report

Native Excel with formatting:

```bash
python3 python/report_generator.py report.json -f xlsx -o output
```

**Use cases:**
- Professional reports
- Stakeholder presentations
- Detailed analysis

### Summary Report

Human-readable text:

```bash
python3 python/report_generator.py report.json -f summary -o output
```

**Use cases:**
- Quick overview
- Build logs
- Documentation

### Generate All Formats

```bash
python3 python/report_generator.py report.json -f all -o output
```

## Advanced Features

### Package-Specific Analysis

Get detailed report for a specific package:

```bash
python3 python/report_generator.py report.json --package package_name -o pkg_report
```

### Top Accessed Files

Find the most frequently accessed files:

```bash
python3 python/analyzer.py report.json --top 20
```

### Package Statistics

View statistics by package:

```bash
python3 python/analyzer.py report.json --stats
```

Output:
```
Package Usage Statistics:
================================================================================

package_a:
  Files: 15
  Total Accesses: 243
  File Types: {'h': 8, 'c': 7}

package_b:
  Files: 3
  Total Accesses: 45
  File Types: {'h': 2, 'c': 1}
```

### Dependency Graph

Generate a visual dependency graph:

```bash
python3 python/analyzer.py report.json --graph dependencies.txt
```

### Filter by Extension

Analyze specific file types:

```bash
# Header files only
python3 python/analyzer.py report.json --extension .h

# C source files
python3 python/analyzer.py report.json --extension .c

# Object files
python3 python/analyzer.py report.json --extension .o
```

### Filter by Package

Focus on specific packages:

```bash
python3 python/analyzer.py report.json --package openssl
```

## Best Practices

### 1. Clean Build for Accurate Results

Always start with a clean build:

```bash
make clean
./integrations/track_build.sh make all
```

### 2. Organized Output Directory

Keep reports organized:

```bash
export FILETRACKER_OUTPUT_DIR=./reports/$(date +%Y%m%d)
```

### 3. Descriptive Filenames

Use timestamps or build identifiers:

```bash
export FILE_TRACKER_JSON=report_$(git rev-parse --short HEAD).json
```

### 4. Regular Tracking

Track builds regularly to detect dependency changes:

```bash
# In your CI/CD pipeline
./integrations/track_build.sh make all
python3 python/report_generator.py report.json -f all
# Archive reports
```

### 5. Compare Reports

Track changes over time:

```bash
# Generate baseline
python3 python/analyzer.py baseline_report.json --stats > baseline_stats.txt

# After changes
python3 python/analyzer.py new_report.json --stats > new_stats.txt

# Compare
diff baseline_stats.txt new_stats.txt
```

### 6. Filter Noise

The tracker automatically filters system files, but you can customize:

Edit `src/file_tracker.c` to add more filters:
```c
// Skip certain directories
if (strstr(filepath, "/proc/") || strstr(filepath, "/sys/") || 
    strstr(filepath, "/your/custom/path/")) {
    return;
}
```

### 7. Integration with CI/CD

#### Jenkins Example
```groovy
stage('Track Build') {
    steps {
        sh './integrations/track_build.sh make all'
        sh 'python3 python/report_generator.py report.json -f all'
        archiveArtifacts artifacts: 'report_*.*'
    }
}
```

#### GitHub Actions Example
```yaml
- name: Build with tracking
  run: ./integrations/track_build.sh make all

- name: Generate reports
  run: python3 python/report_generator.py report.json -f all

- name: Upload reports
  uses: actions/upload-artifact@v2
  with:
    name: build-reports
    path: report_*.*
```

## Common Workflows

### Workflow 1: Dependency Audit

```bash
# 1. Clean build with tracking
make clean
./integrations/track_build.sh make all

# 2. Generate comprehensive reports
python3 python/report_generator.py report.json -f all

# 3. Analyze by package
python3 python/analyzer.py report.json --stats

# 4. Review specific packages
python3 python/report_generator.py report.json --package suspicious_pkg
```

### Workflow 2: Build Optimization

```bash
# 1. Track current build
./integrations/track_build.sh make all

# 2. Identify top accessed files
python3 python/analyzer.py report.json --top 50

# 3. Find unused files (requires full package path)
python3 python/analyzer.py report.json --package mypackage

# 4. Optimize build to only include necessary files
```

### Workflow 3: License Compliance

```bash
# 1. Track build
./integrations/track_build.sh make all

# 2. Generate package reports
python3 python/report_generator.py report.json --package gpl_library_a
python3 python/report_generator.py report.json --package gpl_library_b

# 3. Document actual usage
python3 python/report_generator.py report.json -f xlsx
# Review in Excel, add license information
```

## Tips and Tricks

### Exclude Test Builds

```bash
# Only track production builds
if [ "$BUILD_TYPE" = "production" ]; then
    ./integrations/track_build.sh make all
else
    make all
fi
```

### Multiple Build Configurations

```bash
# Debug build
export FILE_TRACKER_JSON=report_debug.json
./integrations/track_build.sh make debug

# Release build
export FILE_TRACKER_JSON=report_release.json
./integrations/track_build.sh make release

# Compare
python3 python/analyzer.py report_debug.json --stats > debug_stats.txt
python3 python/analyzer.py report_release.json --stats > release_stats.txt
diff debug_stats.txt release_stats.txt
```

### Automated Reporting

Create a script:

```bash
#!/bin/bash
# auto_report.sh

./integrations/track_build.sh $@
REPORT=$(ls -t build_reports/file_access_*.json | head -1)
python3 python/report_generator.py $REPORT -f all
echo "Reports generated in build_reports/"
```

## Troubleshooting

See [FAQ](FAQ.md) for common issues and solutions.

## Next Steps

- Explore [Integration Guide](INTEGRATION.md) for build system integration
- Check [API Reference](API.md) for programmatic usage
- Review [Examples](../examples/) for real-world scenarios
