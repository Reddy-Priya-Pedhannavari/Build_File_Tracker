# Using BuildFileTracker With Your Own Projects

## Table of Contents
- [Overview](#overview)
- [One-Time Setup](#one-time-setup)
- [Quick Start - 3 Steps](#quick-start---3-steps)
- [Method 1: Universal Wrapper (Easiest)](#method-1-universal-wrapper-easiest)
- [Method 2: Manual Environment Variables](#method-2-manual-environment-variables)
- [Method 3: Integration Scripts](#method-3-integration-scripts)
- [Real-World Examples](#real-world-examples)
- [Report Generation](#report-generation)
- [Understanding the Reports](#understanding-the-reports)
- [Tips & Best Practices](#tips--best-practices)
- [Troubleshooting](#troubleshooting)

---

## Overview

BuildFileTracker helps you understand **exactly which files** from external packages/libraries are accessed during your build process. You don't need to modify your project at all - just wrap your build command!

### What You Get:
- ✅ List of every file accessed during build
- ✅ Access counts (how many times each file was read)
- ✅ Package categorization
- ✅ File type breakdown (.c, .h, .so, .o, etc.)
- ✅ Linking type classification (static, dynamic, import, etc.)
- ✅ Object-to-source mapping when debug info is present
- ✅ Reports in multiple formats (JSON, CSV, XLSX, XML, HTML)

---

## One-Time Setup

### Step 1: Build the BuildFileTracker Library

```bash
# Navigate to BuildFileTracker directory
cd d:\Build_File_Tracker

# Build the interception library
cd src
make

# Verify the library was created
ls -l libfiletracker.so
```

**Output**: You should see `libfiletracker.so` in the `src/` directory.

### Step 2: Install Python Dependencies (for report generation)

```bash
# From BuildFileTracker root directory
cd d:\Build_File_Tracker

# Activate virtual environment (if you have one)
source .venv/bin/activate  # Linux/Mac
# or
.venv\Scripts\Activate.ps1  # Windows PowerShell

# Install dependencies
pip install -r python/requirements.txt
```

**That's it for setup!** You only need to do this once.

---

## Quick Start - 3 Steps

Let's say you have a project at `/home/user/MyProject` that builds with `make`.

### Step 1: Navigate to Your Project
```bash
cd /home/user/MyProject
```

### Step 2: Track the Build
```bash
# Set environment variables
export LD_PRELOAD=/path/to/Build_File_Tracker/src/libfiletracker.so
export FILE_TRACKER_JSON=./build_tracking.json

# Run your normal build command
make clean && make all

# Unset the variable (important!)
unset LD_PRELOAD
```

### Step 3: Generate Reports
```bash
# Generate all report formats
python /path/to/Build_File_Tracker/python/report_generator.py build_tracking.json -f all

# Generate component grouping alongside standard reports
python /path/to/Build_File_Tracker/python/report_generator.py build_tracking.json -f all --components

# This creates:
# - build_tracking.json (raw data)
# - build_tracking.csv (Excel-compatible)
# - build_tracking.xlsx (Excel spreadsheet)
# - build_tracking.xml (XML format)
# - build_tracking_components.json (component-wise grouping)
# - build_tracking.html (Interactive web report)
```

**Done!** Open `build_tracking.xlsx` in Excel to see your results.

---

## Method 1: Universal Wrapper (Easiest)

The `track_build.sh` script handles everything automatically.

### Setup
```bash
# Make the script executable (first time only)
chmod +x /path/to/Build_File_Tracker/integrations/track_build.sh
```

### Usage
```bash
# Go to your project
cd /home/user/MyProject

# Use the wrapper with your build command
/path/to/Build_File_Tracker/integrations/track_build.sh make all

# Or with CMake
/path/to/Build_File_Tracker/integrations/track_build.sh cmake --build build

# Or with Ninja
/path/to/Build_File_Tracker/integrations/track_build.sh ninja

# Or with custom scripts
/path/to/Build_File_Tracker/integrations/track_build.sh ./my_custom_build.sh
```

### What Happens:
1. Script sets up tracking environment
2. Runs your build command
3. Automatically creates `build_tracking_report.json`
4. Cleans up environment variables

### Generate Reports
```bash
python /path/to/Build_File_Tracker/python/report_generator.py build_tracking_report.json -f all
```

---

## Method 2: Manual Environment Variables

For more control over output locations and settings.

### Basic Template
```bash
# Navigate to your project
cd /path/to/YourProject

# Set tracking environment
export LD_PRELOAD=/absolute/path/to/Build_File_Tracker/src/libfiletracker.so
export FILE_TRACKER_JSON=/path/to/output/report.json
export FILE_TRACKER_CSV=/path/to/output/report.csv  # Optional: CSV output

# Run your build
<your-build-command>

# Clean up
unset LD_PRELOAD
unset FILE_TRACKER_JSON
unset FILE_TRACKER_CSV
```

### Example: C/C++ Project with Make
```bash
cd ~/projects/my-cpp-app

export LD_PRELOAD=~/tools/Build_File_Tracker/src/libfiletracker.so
export FILE_TRACKER_JSON=./reports/build_$(date +%Y%m%d).json

make clean
make all

unset LD_PRELOAD
unset FILE_TRACKER_JSON

# Generate reports
python ~/tools/Build_File_Tracker/python/report_generator.py \
    ./reports/build_$(date +%Y%m%d).json -f all
```

### Example: CMake Project
```bash
cd ~/projects/my-cmake-project

export LD_PRELOAD=~/tools/Build_File_Tracker/src/libfiletracker.so
export FILE_TRACKER_JSON=./build_tracking.json

# Configure and build
mkdir -p build
cd build
cmake ..
cmake --build .

unset LD_PRELOAD
unset FILE_TRACKER_JSON

# Generate reports
python ~/tools/Build_File_Tracker/python/report_generator.py \
    ../build_tracking.json -f xlsx
```

### Example: Ninja Build
```bash
cd ~/projects/ninja-project

export LD_PRELOAD=/opt/buildfiletracker/src/libfiletracker.so
export FILE_TRACKER_JSON=./ninja_tracking.json

ninja clean
ninja

unset LD_PRELOAD
unset FILE_TRACKER_JSON

python /opt/buildfiletracker/python/report_generator.py ninja_tracking.json -f all
```

---

## Method 3: Integration Scripts

Create reusable tracking scripts for your projects.

### Example 1: Wrapper Script for Your Project

Create `track_my_build.sh` in your project:

```bash
#!/bin/bash
# track_my_build.sh - Track build for MyProject

# Configuration
TRACKER_HOME="/path/to/Build_File_Tracker"
OUTPUT_DIR="./tracking_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Setup
mkdir -p "$OUTPUT_DIR"
export LD_PRELOAD="$TRACKER_HOME/src/libfiletracker.so"
export FILE_TRACKER_JSON="$OUTPUT_DIR/build_$TIMESTAMP.json"

echo "Starting tracked build..."
echo "Report will be saved to: $FILE_TRACKER_JSON"

# Run build
make clean
make all

# Cleanup environment
unset LD_PRELOAD
unset FILE_TRACKER_JSON

# Generate reports
echo "Generating reports..."
python "$TRACKER_HOME/python/report_generator.py" \
    "$OUTPUT_DIR/build_$TIMESTAMP.json" -f all

echo "Done! Reports are in: $OUTPUT_DIR"
echo "Open: $OUTPUT_DIR/build_$TIMESTAMP.xlsx"
```

Make it executable and use:
```bash
chmod +x track_my_build.sh
./track_my_build.sh
```

### Example 2: Makefile Integration

Add to your project's `Makefile`:

```makefile
# Existing Makefile content...

# BuildFileTracker integration
TRACKER_LIB := /path/to/Build_File_Tracker/src/libfiletracker.so
TRACKER_REPORT := ./build_tracking.json
TRACKER_PYTHON := /path/to/Build_File_Tracker/python/report_generator.py

.PHONY: build-tracked track-report

# Build with tracking
build-tracked:
	@echo "Building with file tracking enabled..."
	@export LD_PRELOAD=$(TRACKER_LIB) && \
	export FILE_TRACKER_JSON=$(TRACKER_REPORT) && \
	$(MAKE) clean && \
	$(MAKE) all
	@echo "Build complete. Report saved to: $(TRACKER_REPORT)"

# Generate tracking reports
track-report:
	@echo "Generating tracking reports..."
	@python $(TRACKER_PYTHON) $(TRACKER_REPORT) -f all
	@echo "Reports generated!"

# Build and generate reports in one step
track-all: build-tracked track-report
```

Usage:
```bash
# Build with tracking
make build-tracked

# Generate reports
make track-report

# Or do both
make track-all
```

---

## Real-World Examples

### Example 1: Embedded Linux Project with Yocto

```bash
cd ~/yocto/poky

# Track a specific recipe build
export LD_PRELOAD=~/tools/Build_File_Tracker/src/libfiletracker.so
export FILE_TRACKER_JSON=./tracking/recipe_myapp.json

bitbake myapp

unset LD_PRELOAD
unset FILE_TRACKER_JSON

# Generate report
python ~/tools/Build_File_Tracker/python/report_generator.py \
    ./tracking/recipe_myapp.json -f xlsx

# Now you can see exactly which files from meta-layers were accessed!
```

### Example 2: Android Native Code (NDK)

```bash
cd ~/android-project

export LD_PRELOAD=/home/dev/Build_File_Tracker/src/libfiletracker.so
export FILE_TRACKER_JSON=./build_tracking.json

# Build native code
cd jni
ndk-build clean
ndk-build

unset LD_PRELOAD
unset FILE_TRACKER_JSON

# Generate report
python /home/dev/Build_File_Tracker/python/report_generator.py \
    ../build_tracking.json -f all
```

### Example 3: Kernel Module Build

```bash
cd ~/linux-modules/my-driver

export LD_PRELOAD=/opt/buildfiletracker/src/libfiletracker.so
export FILE_TRACKER_JSON=./module_build_tracking.json

make -C /lib/modules/$(uname -r)/build M=$PWD clean
make -C /lib/modules/$(uname -r)/build M=$PWD

unset LD_PRELOAD
unset FILE_TRACKER_JSON

python /opt/buildfiletracker/python/report_generator.py \
    module_build_tracking.json -f all
```

### Example 4: Multi-Package Build System

```bash
cd ~/workspace/multi-package-project

# Track entire workspace build
export LD_PRELOAD=/usr/local/lib/libfiletracker.so
export FILE_TRACKER_JSON=./workspace_tracking.json

# Build all packages
for package in package-a package-b package-c; do
    echo "Building $package..."
    cd $package
    make
    cd ..
done

unset LD_PRELOAD
unset FILE_TRACKER_JSON

# Generate comprehensive report
python /usr/local/bin/buildfiletracker/report_generator.py \
    workspace_tracking.json -f all

# Now see which files from which packages were actually used!
```

### Example 5: Continuous Integration (CI) Integration

Create `.gitlab-ci.yml` or similar:

```yaml
build_with_tracking:
  stage: build
  script:
    - export LD_PRELOAD=$CI_PROJECT_DIR/buildfiletracker/src/libfiletracker.so
    - export FILE_TRACKER_JSON=$CI_PROJECT_DIR/build_tracking.json
    - make clean
    - make all
    - unset LD_PRELOAD
    
generate_tracking_report:
  stage: report
  dependencies:
    - build_with_tracking
  script:
    - python buildfiletracker/python/report_generator.py build_tracking.json -f all
  artifacts:
    paths:
      - build_tracking.xlsx
      - build_tracking.html
    expire_in: 1 week
```

---

## Report Generation

### Available Formats

```bash
# JSON (raw data)
python report_generator.py input.json -f json

# CSV (Excel-compatible)
python report_generator.py input.json -f csv

# XLSX (Excel spreadsheet with formatting)
python report_generator.py input.json -f xlsx

# XML (structured data)
python report_generator.py input.json -f xml

# HTML (interactive web report)
python report_generator.py input.json -f html

# Summary (console output)
python report_generator.py input.json -f summary

# ALL formats at once
python report_generator.py input.json -f all
```

### Custom Output Location

```bash
# Specify output file
python report_generator.py input.json -o /custom/path/report.xlsx -f xlsx

# Generate multiple formats with custom names
python report_generator.py build_data.json -o ./reports/final_report -f all
# Creates: final_report.json, final_report.csv, final_report.xlsx, etc.
```

### Report Options

```bash
# Filter by package
python report_generator.py input.json -f xlsx --filter-package "opencv"

# Filter by file type
python report_generator.py input.json -f csv --filter-type "c,h"

# Sort by access count
python report_generator.py input.json -f xlsx --sort-by access_count
```

---

## Understanding the Reports

### JSON Report Structure

```json
{
  "report_type": "build_file_tracker",
  "timestamp": "1740499200",
  "summary": {
    "total_files": 234,
    "total_accesses": 1523,
    "unique_packages": 12
  },
  "accessed_files": [
    {
      "filepath": "/usr/include/opencv4/opencv2/core.hpp",
      "package": "opencv",
      "file_type": "hpp",
      "access_count": 45,
      "first_access": 1740499201,
      "last_access": 1740499245
    },
    {
      "filepath": "/usr/lib/x86_64-linux-gnu/libopencv_core.so.4.5",
      "package": "opencv",
      "file_type": "so",
      "access_count": 3,
      "first_access": 1740499300,
      "last_access": 1740499305
    }
  ],
  "by_package": {
    "opencv": {
      "file_count": 23,
      "total_accesses": 156,
      "file_types": ["hpp", "h", "so"]
    }
  },
  "by_file_type": {
    "h": 145,
    "c": 34,
    "so": 12,
    "hpp": 23
  }
}
```

### XLSX Report Tabs

1. **Summary**: Overview statistics
2. **All Files**: Complete list of accessed files
3. **By Package**: Grouped by package/library
4. **By File Type**: Grouped by extension
5. **Top Accessed**: Most frequently accessed files
6. **Timeline**: Access patterns over time

### CSV Report Columns

```
filepath,package,file_type,access_count,first_access,last_access
/usr/include/stdio.h,glibc,h,234,1740499201,1740499899
/usr/include/opencv2/core.hpp,opencv,hpp,45,1740499203,1740499245
```

---

## Tips & Best Practices

### 1. Always Clean Before Tracking

```bash
# This ensures you track a complete build
make clean
export LD_PRELOAD=...
make all
```

### 2. Use Timestamps in Report Names

```bash
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
export FILE_TRACKER_JSON=./reports/build_$TIMESTAMP.json
```

### 3. Track Different Build Configurations

```bash
# Debug build
export FILE_TRACKER_JSON=./debug_build.json
make clean && make debug

# Release build
export FILE_TRACKER_JSON=./release_build.json
make clean && make release

# Compare the reports!
```

### 4. Store Reports with Build Artifacts

```bash
# Include tracking in your build process
mkdir -p build/reports
export FILE_TRACKER_JSON=build/reports/tracking.json
make
python report_generator.py build/reports/tracking.json -f all
```

### 5. Automate Report Sharing

```bash
# After build, email the report
make build-tracked
python report_generator.py tracking.json -f xlsx
mail -s "Build Tracking Report" team@company.com < tracking.xlsx
```

### 6. Version Your Tracking Data

```bash
git add build/reports/tracking_v1.0.0.xlsx
git commit -m "Add file tracking for release v1.0.0"
```

---

## Troubleshooting

### Problem: No report generated

**Solution**: Check that environment variables are set before the build:
```bash
# Verify before building
echo $LD_PRELOAD
echo $FILE_TRACKER_JSON

# Should show the paths, not empty
```

### Problem: Report is empty

**Solution**: Ensure `libfiletracker.so` was built correctly:
```bash
ldd /path/to/libfiletracker.so
# Should show library dependencies

file /path/to/libfiletracker.so
# Should show "shared object"
```

### Problem: Build fails with tracking enabled

**Solution**: Some builds don't work with LD_PRELOAD. Try:
```bash
# Unset and rebuild normally
unset LD_PRELOAD
make clean && make

# Or use strace as alternative
strace -e trace=open,openat -f -o build.trace make
```

### Problem: Permission denied

**Solution**: Ensure the library has correct permissions:
```bash
chmod 755 /path/to/Build_File_Tracker/src/libfiletracker.so
```

### Problem: Wrong files tracked

**Solution**: Filter the report:
```bash
# Only show external packages (exclude system paths)
python report_generator.py tracking.json -f xlsx --exclude-path "/usr/lib,/lib"
```

### Problem: Report generation fails

**Solution**: Check Python dependencies:
```bash
pip install -r python/requirements.txt --upgrade
python -m pip check
```

---

## Advanced Usage

### Scripted Analysis

```python
# analyze_build.py
import json

with open('build_tracking.json') as f:
    data = json.load(f)

# Find most accessed files
files = data['accessed_files']
top_10 = sorted(files, key=lambda x: x['access_count'], reverse=True)[:10]

print("Top 10 Most Accessed Files:")
for f in top_10:
    print(f"{f['access_count']:>4} - {f['filepath']}")

# Find unused packages (in your dependency list but not accessed)
accessed_packages = set(f['package'] for f in files)
declared_packages = {'opencv', 'boost', 'ffmpeg'}  # Your dependencies
unused = declared_packages - accessed_packages

print(f"\nPotentially unused dependencies: {unused}")
```

### Comparing Builds

```bash
# Track two builds
export FILE_TRACKER_JSON=build_v1.json
make clean && make
export FILE_TRACKER_JSON=build_v2.json
make clean && make

# Compare (create a custom script)
python compare_tracking.py build_v1.json build_v2.json
```

---

## Next Steps

1. **Try it now**: Track a simple build in your project
2. **Analyze the results**: Look for unused dependencies
3. **Automate**: Add tracking to your build scripts
4. **Share**: Help your team understand dependencies

For more information:
- [Integration Guide](INTEGRATION.md)
- [API Documentation](API.md)
- [FAQ](FAQ.md)

---

**Questions?** Check the [FAQ](FAQ.md) or open an issue on GitHub!
