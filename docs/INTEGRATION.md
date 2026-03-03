# Integration Guide

## Overview

BuildFileTracker can be integrated into any build system. This guide covers the most common scenarios.

## Universal Integration

### Shell Script Wrapper

The simplest integration for any build system:

```bash
#!/bin/bash
# build_with_tracking.sh

export LD_PRELOAD=/path/to/libfiletracker.so
export FILE_TRACKER_JSON=./report.json
export FILE_TRACKER_CSV=./report.csv

# Your build commands
$@

# Cleanup
unset LD_PRELOAD
```

Usage:
```bash
./build_with_tracking.sh make all
./build_with_tracking.sh ninja
./build_with_tracking.sh ./custom_build.sh
```

## CMake Integration

### Method 1: Include Module

Add to your `CMakeLists.txt`:

```cmake
# Add BuildFileTracker module path
list(APPEND CMAKE_MODULE_PATH "/path/to/buildfiletracker/integrations/cmake")

# Include the module
include(FileTracker OPTIONAL)

# Optional: Set custom output directory
set(FILETRACKER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/tracking_reports")

# Enable tracking for specific targets
if(COMMAND enable_file_tracking)
    enable_file_tracking(my_executable)
endif()
```

Build with tracking:
```bash
mkdir build && cd build
cmake ..
make build_with_tracking
```

### Method 2: CMake Command Line

```bash
cmake -DFILETRACKER_LIB=/path/to/libfiletracker.so \
      -DFILETRACKER_OUTPUT_DIR=./reports \
      ..
make build_with_tracking
```

### Method 3: Environment Variables

```bash
export LD_PRELOAD=/path/to/libfiletracker.so
export FILE_TRACKER_JSON=./report.json
cmake --build build
```

## Makefile Integration

### Method 1: Include Module

Add to your `Makefile`:

```makefile
# Include BuildFileTracker
include /path/to/buildfiletracker/integrations/makefile/filetracker.mk

# Your existing rules...
all: myprogram

myprogram: main.o utils.o
	$(CC) -o $@ $^

# Use the provided target
# make build-with-tracking
```

### Method 2: Custom Rules

```makefile
FILETRACKER_LIB = /path/to/libfiletracker.so
FILETRACKER_OUTPUT = ./reports

.PHONY: track-build
track-build:
	@mkdir -p $(FILETRACKER_OUTPUT)
	LD_PRELOAD=$(FILETRACKER_LIB) \
	FILE_TRACKER_JSON=$(FILETRACKER_OUTPUT)/report.json \
	$(MAKE) all
```

## Autotools Integration

Add to your `configure.ac`:

```bash
AC_ARG_WITH([filetracker],
    [AS_HELP_STRING([--with-filetracker=PATH],
        [enable file tracking with BuildFileTracker])],
    [filetracker_lib=$withval],
    [filetracker_lib=no])

if test "x$filetracker_lib" != "xno"; then
    AC_SUBST([FILETRACKER_LIB], [$filetracker_lib])
    AC_SUBST([LD_PRELOAD], [$filetracker_lib])
fi
```

Build with tracking:
```bash
./configure --with-filetracker=/path/to/libfiletracker.so
make
```

## Meson Integration

Create `meson.build`:

```python
project('myproject', 'c')

# Check for FileTracker
filetracker_lib = get_option('filetracker_lib')
if filetracker_lib != ''
    message('BuildFileTracker enabled: ' + filetracker_lib)
    add_global_arguments('-DLD_PRELOAD=' + filetracker_lib, language: 'c')
endif

# Your project configuration...
```

Build:
```bash
meson setup build -Dfiletracker_lib=/path/to/libfiletracker.so
ninja -C build
```

## Yocto/OpenEmbedded Integration

### Method 1: Recipe Integration

Add to your recipe (`.bb` file):

```python
# Inherit filetracker class
inherit /path/to/buildfiletracker/integrations/yocto/filetracker

# FileTracker will automatically track this build
```

### Method 2: Global Configuration

Add to `local.conf`:

```bash
FILETRACKER_LIB = "${TOPDIR}/../buildfiletracker/src/libfiletracker.so"
FILETRACKER_OUTPUT_DIR = "${TMPDIR}/file_tracking"
FILETRACKER_ENABLED = "1"
```

### Method 3: bbappend

Create a `.bbappend` file:

```python
do_compile_prepend() {
    export LD_PRELOAD="${FILETRACKER_LIB}"
    export FILE_TRACKER_JSON="${WORKDIR}/file_report.json"
}

do_compile_append() {
    unset LD_PRELOAD
}
```

## Bazel Integration

Create a wrapper script `tools/track_bazel.sh`:

```bash
#!/bin/bash
export LD_PRELOAD=/path/to/libfiletracker.so
export FILE_TRACKER_JSON=bazel-out/report.json
bazel "$@"
unset LD_PRELOAD
```

Usage:
```bash
./tools/track_bazel.sh build //...
```

## Python Build Systems

### setuptools

Create `build_tracked.py`:

```python
from setuptools import setup
from buildfiletracker import BuildFileTracker
import subprocess

tracker = BuildFileTracker(output_dir='./build_reports')
tracker.enable()

# Run setuptools
import setup
setup.run()

tracker.disable()
```

### pip

```bash
./integrations/track_build.sh pip install -e .
```

### Poetry

```bash
./integrations/track_build.sh poetry build
```

## Rust Cargo Integration

### Cargo Wrapper

```bash
# track_cargo.sh
export LD_PRELOAD=/path/to/libfiletracker.so
export FILE_TRACKER_JSON=./cargo_report.json
cargo "$@"
unset LD_PRELOAD
```

Usage:
```bash
./track_cargo.sh build --release
```

### cargo-make

In `Makefile.toml`:

```toml
[tasks.build-tracked]
script = '''
export LD_PRELOAD=/path/to/libfiletracker.so
export FILE_TRACKER_JSON=./report.json
cargo build --release
unset LD_PRELOAD
'''
```

## Go Build Integration

```bash
# track_go.sh
export LD_PRELOAD=/path/to/libfiletracker.so
export FILE_TRACKER_JSON=./go_report.json
go build "$@"
unset LD_PRELOAD
```

## Custom Build Scripts

### Example: Complex Build Script

```bash
#!/bin/bash
# my_build.sh with tracking

set -e

# Configuration
TRACKER_LIB="${FILETRACKER_LIB:-./libfiletracker.so}"
REPORT_DIR="./build_reports/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$REPORT_DIR"

# Enable tracking
export LD_PRELOAD="$TRACKER_LIB"
export FILE_TRACKER_JSON="$REPORT_DIR/report.json"
export FILE_TRACKER_CSV="$REPORT_DIR/report.csv"

echo "Building with file tracking..."

# Your build steps
./configure
make clean
make all
make install DESTDIR=./install

# Disable tracking
unset LD_PRELOAD

echo "Build complete!"
echo "Reports: $REPORT_DIR"

# Generate additional formats
python3 python/report_generator.py "$FILE_TRACKER_JSON" -f all

# Summary
python3 python/analyzer.py "$FILE_TRACKER_JSON" --stats
```

## CI/CD Integration

### Jenkins

```groovy
pipeline {
    agent any
    
    environment {
        FILETRACKER_LIB = '/path/to/libfiletracker.so'
        LD_PRELOAD = "${FILETRACKER_LIB}"
        FILE_TRACKER_JSON = "./reports/build_${BUILD_NUMBER}.json"
    }
    
    stages {
        stage('Build') {
            steps {
                sh 'make all'
            }
        }
        
        stage('Generate Reports') {
            steps {
                sh 'python3 python/report_generator.py ${FILE_TRACKER_JSON} -f all'
            }
        }
        
        stage('Archive') {
            steps {
                archiveArtifacts artifacts: 'reports/*', fingerprint: true
            }
        }
    }
}
```

### GitHub Actions

```yaml
name: Build with Tracking

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Setup BuildFileTracker
      run: |
        git clone https://github.com/yourusername/buildfiletracker.git
        cd buildfiletracker/src
        make
    
    - name: Build with Tracking
      env:
        LD_PRELOAD: ${{ github.workspace }}/buildfiletracker/src/libfiletracker.so
        FILE_TRACKER_JSON: ./report.json
      run: make all
    
    - name: Generate Reports
      run: |
        python3 buildfiletracker/python/report_generator.py report.json -f all
    
    - name: Upload Reports
      uses: actions/upload-artifact@v2
      with:
        name: build-reports
        path: report_*
```

### GitLab CI

```yaml
build_with_tracking:
  stage: build
  script:
    - export LD_PRELOAD=/path/to/libfiletracker.so
    - export FILE_TRACKER_JSON=./report.json
    - make all
    - python3 python/report_generator.py report.json -f all
  artifacts:
    paths:
      - report_*
    expire_in: 1 week
```

### Travis CI

```yaml
before_script:
  - cd buildfiletracker/src && make && cd ../..
  - export LD_PRELOAD=$PWD/buildfiletracker/src/libfiletracker.so
  - export FILE_TRACKER_JSON=./report.json

script:
  - make all
  - python3 buildfiletracker/python/report_generator.py report.json -f all

after_success:
  - cat report_summary.txt
```

## Docker Integration

### Dockerfile

```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    python3-pip

# Copy BuildFileTracker
COPY buildfiletracker /opt/buildfiletracker
WORKDIR /opt/buildfiletracker/src
RUN make

# Set environment
ENV LD_PRELOAD=/opt/buildfiletracker/src/libfiletracker.so
ENV FILE_TRACKER_JSON=/reports/report.json

# Your build commands
WORKDIR /workspace
CMD ["make", "all"]
```

## Best Practices

### 1. Conditional Tracking

Only enable tracking when needed:

```makefile
ifdef ENABLE_TRACKING
    export LD_PRELOAD=/path/to/libfiletracker.so
    export FILE_TRACKER_JSON=./report.json
endif
```

### 2. Multiple Configurations

Track different build configurations separately:

```bash
# Debug build
FILE_TRACKER_JSON=report_debug.json make debug

# Release build
FILE_TRACKER_JSON=report_release.json make release
```

### 3. Timestamped Reports

Always include timestamps:

```bash
export FILE_TRACKER_JSON="report_$(date +%Y%m%d_%H%M%S).json"
```

### 4. Report Archival

Keep historical reports:

```bash
mkdir -p reports/archive
mv report_*.json reports/archive/
```

## Troubleshooting

### Library Not Loaded

```bash
# Check library exists
ls -l /path/to/libfiletracker.so

# Check permissions
chmod +x /path/to/libfiletracker.so

# Verify LD_PRELOAD
echo $LD_PRELOAD
```

### Reports Not Generated

```bash
# Check environment variables
echo $FILE_TRACKER_JSON

# Check directory permissions
ls -ld ./reports

# Check disk space
df -h
```

## Next Steps

- Review [Examples](../examples/) for complete integration examples
- Check [API Reference](API.md) for programmatic usage
- Read [FAQ](FAQ.md) for common issues
