# Installation & Setup Guide

Quick installation steps. For **detailed platform-specific setup**, see [Cross-Platform Setup Guide](CROSS_PLATFORM_SETUP.md).

## Prerequisites

- **Python 3.7+** (required for all platforms)
- **Build tools**: GCC/Clang (Linux/macOS), MSVC or MinGW (Windows)
- **Linux**: GNU Make, pthread library
- **macOS**: Xcode Command Line Tools
- **Windows**: Use WSL2 (recommended) or compile on Windows

## Installation Steps

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/buildfiletracker.git
cd buildfiletracker
```

### 2. Build the C Library (Linux/macOS only)

```bash
cd src
make
```

This creates `libfiletracker.so` (Linux) or `libfiletracker.dylib` (macOS).

> **Windows users**: Skip this step. Use the Python tracker instead.

### 3. Install Python Dependencies

```bash
pip install -r python/requirements.txt
```

Or use a virtual environment:

```bash
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r python/requirements.txt
```

### 4. Verify Installation

#### Linux/macOS:
```bash
# Check library exists
ls -l src/libfiletracker.so

# Test tracking
export LD_PRELOAD=$PWD/src/libfiletracker.so
export FILE_TRACKER_JSON=/tmp/test.json
ls -la
unset LD_PRELOAD

# Verify report created
cat /tmp/test.json
```

#### Windows:
```powershell
# Test Python tracker
python python/tracker.py python --version
# Check report created
ls build_tracking.json
```

---

## System-Wide Installation (Optional)

### Linux/macOS: Install Library System-Wide

```bash
cd src
sudo make install
# Installs to /usr/local/lib/ and runs ldconfig
```

Then you don't need to set `LD_PRELOAD` explicitly (the library is in the system path).

## Configuration

### Environment Variables

| Variable | Purpose |
|----------|---------|
| `LD_PRELOAD` | Path to libfiletracker.so (Linux/macOS) |
| `FILE_TRACKER_JSON` | Output path for JSON report |
| `FILE_TRACKER_CSV` | Output path for CSV report |
| `FILETRACKER_LIB` | Custom library path |
| `FILETRACKER_OUTPUT_DIR` | Custom output directory |

### Quick Setup

```bash
export FILETRACKER_HOME=/path/to/buildfiletracker
export LD_PRELOAD=$FILETRACKER_HOME/src/libfiletracker.so
export FILE_TRACKER_JSON=./build_tracking.json
```

For more details, see [Cross-Platform Setup](CROSS_PLATFORM_SETUP.md).

## Troubleshooting

### Basic Issues

**"Library not found" on Linux/macOS:**
```bash
# Add to library path
export LD_LIBRARY_PATH=/path/to/buildfiletracker/src:$LD_LIBRARY_PATH

# Or install system-wide
cd src && sudo make install
```

**"Permission Denied" when running scripts:**
```bash
chmod +x integrations/*.sh
chmod +x python/*.py
```

**"Python module not found":**
```bash
pip install --upgrade -r python/requirements.txt
```

### Platform-Specific Issues

For detailed troubleshooting on your specific platform, see:
- **Linux/macOS**: [Cross-Platform Setup → Troubleshooting](CROSS_PLATFORM_SETUP.md#troubleshooting)
- **Windows/WSL2**: [Cross-Platform Setup → Windows Setup](CROSS_PLATFORM_SETUP.md#windows)
- **macOS SIP Issues**: [Cross-Platform Setup → macOS Setup](CROSS_PLATFORM_SETUP.md#macos)

## Next Steps

1. **Quick Start**: [User Guide](USER_GUIDE.md) - Basic usage in 15 minutes
2. **Your Platform**: [Cross-Platform Setup](CROSS_PLATFORM_SETUP.md) - Platform-specific details
3. **Your Project**: [Using with Your Projects](USING_WITH_YOUR_PROJECTS.md) - Integration examples
4. **Build System**: [Integration Guide](INTEGRATION.md) - Your specific build system

Or see the [Documentation Index](INDEX.md) for a complete guide.
