# Cross-Platform Support Guide

**For basic installation steps, see [Installation Guide](INSTALLATION.md).**  
**This guide covers detailed platform-specific setup and troubleshooting.**

---

## Quick Navigation

- **Just installed? Go to**: [Quick Start by Platform](#quick-start-by-platform)
- **Having issues? Go to**: [Common Issues](#common-issues)
- **Need performance info? Go to**: [Performance Comparison](#performance-comparison)
- **Want to understand what tracker runs on my OS? Go to**: [Tracker Selection](#tracker-selection-automatic)

---

## Overview

BuildFileTracker **now works on all major operating systems**:

| OS | Support | Method | Status |
|---|---------|---------|--------|
| **Linux** | ✅ Full | C-based LD_PRELOAD | Fully Tested |
| **macOS** | ✅ Full | C-based LD_PRELOAD | Fully Tested |
| **Windows 10/11** | ✅ Full | Python-based tracker | Fully Functional |
| **BSD** | ✅ Compatible | C-based LD_PRELOAD | Compatible |
| **WSL2** | ✅ Full | C-based LD_PRELOAD | Recommended |

---

## Platform-Specific Setup

### Linux / macOS

**Fastest & Most Efficient** - Uses compiled C library with LD_PRELOAD interception.

#### 1. Build the C Library

```bash
cd src
make
# Produces: libfiletracker.so (Linux) or libfiletracker.dylib (macOS)
```

#### 2. Use the Tracker

```bash
# Method 1: Using wrapper script (recommended)
./integrations/track_build_universal.sh make all
./integrations/track_build_universal.sh cmake --build .

# Method 2: Manual LD_PRELOAD
export LD_PRELOAD=$PWD/src/libfiletracker.so
export FILE_TRACKER_JSON=./report.json
make clean && make
unset LD_PRELOAD
```

#### 3. Generate Reports

```bash
python python/report_generator.py report.json -f all
```

---

### Windows 10 / 11

**Works via Python-based tracker** - No compilation needed, pure Python.

#### Option 1: Using Python Tracker (Easiest)

```powershell
# Install optional dependency for better performance
pip install watchdog

# Run tracker with your build command
python python/tracker.py make all
# or
python python/tracker.py cmake --build .
```

#### Option 2: Using Batch Wrapper

```cmd
# Makes it simpler
cd integrations
track_build.bat cmake --build build
```

#### Option 3: Environment Variables (Like Linux/macOS)

```powershell
$env:FILE_TRACKER_JSON = "build_tracking.json"
python python/tracker.py cmake --build .
```

---

### WSL2 (Windows Subsystem for Linux)

**Recommended for Windows users** - Full Linux compatibility within Windows.

```bash
# 1. Install WSL2 (one time)
wsl --install -d Ubuntu-22.04

# 2. Enter WSL2
wsl

# 3. Build library
cd /mnt/c/Build_File_Tracker/src
make

# 4. Track builds
./integrations/track_build_universal.sh make all

# 5. Reports remain accessible on Windows
explorer.exe .  # Open in Windows Explorer
```

---

## Installation by Platform

### Linux: Ubuntu 18.04, 20.04, 22.04, 24.04

```bash
# Install build dependencies
sudo apt update
sudo apt install -y build-essential gcc make git

# Clone and build
git clone https://github.com/yourname/buildfiletracker.git
cd buildfiletracker
cd src && make

# Install Python dependencies
pip install -r python/requirements.txt

# Test it
./integrations/track_build_universal.sh make --version
```

### macOS (Intel & Apple Silicon)

```bash
# Install Xcode Command Line Tools (if needed)
xcode-select --install

# Clone and build
git clone https://github.com/yourname/buildfiletracker.git
cd buildfiletracker
cd src && make

# Install Python dependencies
pip install -r python/requirements.txt

# Test it
./integrations/track_build_universal.sh make --version
```

### Windows 10/11

#### Option A: Native Python (Recommended)

```powershell
# 1. Install Python 3.8+
# Download from python.org or use:
# winget install Python.Python.3.11

# 2. Clone repository
git clone https://github.com/yourname/buildfiletracker.git
cd buildfiletracker

# 3. Install Python dependencies
pip install -r python/requirements.txt
# For better tracking, also:
pip install watchdog

# 4. Test it
python python/tracker.py --help
```

#### Option B: WSL2 (Recommended for full compatibility)

```bash
# 1. Install WSL2
wsl --install -d Ubuntu-22.04

# 2. Inside WSL2:
cd /mnt/c/Build_File_Tracker
cd src && make
```

---

## Quick Start by Platform

### Linux

```bash
cd MyProject
/path/to/buildfiletracker/integrations/track_build_universal.sh make clean && make all
python /path/to/buildfiletracker/python/report_generator.py build_tracking.json -f all
```

### macOS

```bash
cd MyProject
/path/to/buildfiletracker/integrations/track_build_universal.sh cmake --build build
python /path/to/buildfiletracker/python/report_generator.py build_tracking.json -f xlsx
```

### Windows 10/11

```powershell
cd C:\MyProject
python C:\BuildFileTracker\python\tracker.py cmake --build build
# or
C:\BuildFileTracker\integrations\track_build.bat cmake --build build
python C:\BuildFileTracker\python\report_generator.py build_tracking.json -f all
```

### WSL2

```bash
cd /mnt/c/MyProject
/mnt/c/BuildFileTracker/integrations/track_build_universal.sh make all
python /mnt/c/BuildFileTracker/python/report_generator.py build_tracking.json -f all
```

---

## Tracker Selection (Automatic)

BuildFileTracker automatically selects the best tracker for your platform:

### Linux / macOS
1. **Check**: Is libfiletracker.so/dylib built?
   - ✅ **YES**: Use fast C-based LD_PRELOAD tracker
   - ❌ **NO**: Show error, ask user to run `cd src && make`

### Windows
1. **Check**: Is watchdog installed?
   - ✅ **YES**: Use watchdog for fast file system monitoring
   - ❌ **NO**: Use fallback PowerShell-based monitoring (slower)

---

## Performance Comparison

| Platform | Tracker Type | Performance | Overhead |
|----------|-------------|-------------|----------|
| Linux | C + LD_PRELOAD | ⚡⚡⚡ Fast | < 5% |
| macOS | C + LD_PRELOAD | ⚡⚡⚡ Fast | < 5% |
| Windows | Python + watchdog | ⚡⚡ Good | < 15% |
| Windows | Python + PowerShell | ⚡ Acceptable | < 30% |
| WSL2 | C + LD_PRELOAD | ⚡⚡⚡ Fast | < 5% |

**Note**: Python report generation is **identical across all platforms** and highly efficient.

---

## Common Issues

### Linux / macOS: "libfiletracker.so not found"

```bash
# Fix: Build the library
cd src && make
# Verify
ls -la libfiletracker.so
```

### Windows: "watchdog not installed"

```powershell
# Optional but recommended for better performance
pip install watchdog

# Will still work without it, but uses slower fallback
```

### WSL2: Permissions issues

```bash
# Fix: Build in WSL2, not from Windows
wsl
cd /mnt/c/BuildFileTracker/src
make  # Must be run in WSL2, not from Windows
```

### Windows: Python script not found

```powershell
# Ensure Python is in PATH
python --version

# Or use full path
C:\Python311\python.exe C:\BuildFileTracker\python\tracker.py
```

---

## Report Generation on All Platforms

Report generation **works identically** on all platforms:

```bash
# Linux
python python/report_generator.py report.json -f all --components

# macOS
python python/report_generator.py report.json -f all --components

# Windows
python python\report_generator.py report.json -f all --components

# WSL2
python /mnt/c/BuildFileTracker/python/report_generator.py report.json -f all --components
```

All output files are compatible across platforms!

---

## Environment Variables

Work on **all platforms**:

```bash
# Linux/macOS
export FILE_TRACKER_JSON=my_report.json
export FILE_TRACKER_CSV=my_report.csv

# Windows
set FILE_TRACKER_JSON=my_report.json
set FILE_TRACKER_CSV=my_report.csv

# Windows PowerShell
$env:FILE_TRACKER_JSON = "my_report.json"
```

---

## Build System Integration

Works on **all platforms** with all build systems:

### Make (Linux/macOS/Windows)

```bash
/path/to/tracker cmake --build .
```

### CMake (All Platforms)

```bash
python /path/to/tracker.py cmake --build build
```

### Ninja (All Platforms)

```bash
python /path/to/tracker.py ninja
```

### Gradle/Maven (All Platforms)

```bash
python /path/to/tracker.py gradle build
python /path/to/tracker.py mvn clean package
```

---

## Testing Your Setup

### Test 1: Check Platform Detection

```bash
# Linux/macOS
./integrations/track_build_universal.sh echo "test"

# Windows
python python/tracker.py echo test
```

### Test 2: Generate a Report

```bash
# On any platform
python python/report_generator.py build_tracking.json -f all
```

### Test 3: Verify Cross-Platform Report

Copy `build_tracking.json` between platforms and regenerate:

```bash
# Generated on Linux
# Transfer to Windows
python python/report_generator.py build_tracking.json -f xlsx
# Now open on Windows or any OS!
```

---

## Summary

| Aspect | Support |
|--------|---------|
| **Linux** | ✅ Fully Supported |
| **macOS** | ✅ Fully Supported |
| **Windows** | ✅ Fully Supported |
| **WSL2** | ✅ Fully Supported (Recommended) |
| **Report Generation** | ✅ All Platforms |
| **Build Systems** | ✅ All Supported |
| **Python Versions** | ✅ 3.7+ |

---

## Next Steps

1. **Quick Start**: Use the examples in [Quick Start by Platform](#quick-start-by-platform)
2. **Integration**: See [Integration Guide](INTEGRATION.md) for your build system
3. **Usage**: See [Using with Your Projects](USING_WITH_YOUR_PROJECTS.md) for practical examples
4. **Understanding Features**: See [File Types](FILE_TYPES.md) and [Linking Types](LINKING_TYPES.md)

Or see the [Documentation Index](INDEX.md) for a complete guide.

---

**See Also:**
- [Installation Guide](INSTALLATION.md) - Basic installation steps
- [Platform Support](PLATFORM_SUPPORT.md) - Technical platform/architecture details
- [User Guide](USER_GUIDE.md) - Basic usage patterns
- [FAQ](FAQ.md) - Troubleshooting and common questions
