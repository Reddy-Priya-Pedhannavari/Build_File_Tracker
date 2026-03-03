# BuildFileTracker

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform Support](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-brightgreen)](docs/CROSS_PLATFORM_SETUP.md)

**Universal build file access monitoring library for tracking package dependencies across all build systems**

🌍 **Cross-Platform Support**: Windows 10/11 • Linux • macOS • WSL2

BuildFileTracker is a lightweight, powerful tool that monitors which files from external packages are actually used during your build process. It works seamlessly with CMake, Yocto, Make, and any other build system, generating comprehensive reports in multiple formats.

## 🎯 The Problem

When building software projects:
- **Package A** (open source) has 100 files (.c, .h, .o, etc.)
- **Your Project** only uses 5 files from Package A
- Traditional build systems compile everything
- You don't know which files are actually needed

## ✨ The Solution

BuildFileTracker uses **LD_PRELOAD** to intercept file operations during builds and tracks:
- ✅ Which files were actually accessed
- ✅ How many times each file was accessed
- ✅ Package names and file types
- ✅ Linking types (static, dynamic, import, etc.)
- ✅ Source mapping for object files (when debug info is available)
- ✅ Complete dependency mapping

**Result**: Know exactly which files your project actually uses!

## 🚀 Quick Start

### 1. Installation (3 minutes)

**Quick install:**
```bash
git clone https://github.com/yourusername/buildfiletracker.git
cd buildfiletracker
cd src && make                          # Linux/macOS only
pip install -r python/requirements.txt
```

**For detailed instructions:** See [Installation Guide](docs/INSTALLATION.md)  
**For platform-specific setup:** See [Cross-Platform Setup](docs/CROSS_PLATFORM_SETUP.md)

### 2. Track Your Build (1 minute)

#### Option 1: Using Wrapper Script (Recommended)

```bash
cd /path/to/your/project

# Linux/macOS
/path/to/buildfiletracker/integrations/track_build_universal.sh make all

# Windows
python C:\BuildFileTracker\python\tracker.py make all
```

#### Option 2: Manual Environment Variables

```bash
export LD_PRELOAD=/path/to/libfiletracker.so
export FILE_TRACKER_JSON=./report.json

# Run your build command
make all

# Reports are automatically generated
```

### 3. Generate Reports (1 minute)

```bash
# Generate all report formats
python3 python/report_generator.py report.json -f all

# Generate reports plus component grouping
python3 python/report_generator.py report.json -f all --components

# Generate specific format
python3 python/report_generator.py report.json -f xlsx
python3 python/report_generator.py report.json -f xml

# Generate summary
python3 python/report_generator.py report.json -f summary
```

## 📊 Example Output

### Scenario
- **Package A** has 20 files
- **Your project** only uses files `config.c` and `utils.h`

### Report Output (JSON)
```json
{
  "report_type": "build_file_tracker",
  "timestamp": "1740499200",
  "accessed_files": [
    {
      "filepath": "/path/to/package_a/config.c",
      "package": "package_a",
      "file_type": "c",
      "linking_type": "unknown",
      "access_count": 3
    },
    {
      "filepath": "/path/to/package_a/utils.h",
      "package": "package_a",
      "file_type": "h",
      "linking_type": "unknown",
      "access_count": 5
    }
  ]
}
```

### Summary Report
```
============================================================
Build File Tracker - Summary Report
============================================================

Total Unique Files Accessed: 2
Total File Accesses: 8

------------------------------------------------------------
Files by Package:
------------------------------------------------------------
  package_a                                :     2 files

------------------------------------------------------------
Files by Type:
------------------------------------------------------------
  c                                        :     1 files
  h                                        :     1 files
```

## 🔧 Build System Integration

### CMake

```cmake
# In your CMakeLists.txt
include(/path/to/buildfiletracker/integrations/cmake/FileTracker.cmake)

# Your project configuration...

# Build with tracking
# cmake --build . --target build_with_tracking
```

### Makefile

```makefile
# In your Makefile
include /path/to/buildfiletracker/integrations/makefile/filetracker.mk

# Then run:
# make build-with-tracking
```

### Python Build Scripts

```python
from buildfiletracker import track_build

with track_build():
    # Your build code here
    subprocess.run(['make', 'all'])
```

### Yocto

```bash
# Add to your layer's conf
inherit /path/to/buildfiletracker/integrations/yocto/filetracker.bbclass
```

## 📁 Project Structure

```
buildfiletracker/
├── src/                      # Core C library
│   ├── file_tracker.h        # Main header
│   ├── file_tracker.c        # Tracker implementation
│   ├── file_interceptor.c    # LD_PRELOAD hooks
│   └── Makefile              # Build configuration
├── python/                   # Report generation tools
│   ├── report_generator.py   # Multi-format reports
│   ├── analyzer.py           # Advanced analysis
│   └── requirements.txt      # Python dependencies
├── integrations/             # Build system helpers
│   ├── track_build.sh        # Universal wrapper
│   ├── cmake/                # CMake integration
│   ├── makefile/             # Makefile integration
│   ├── yocto/                # Yocto integration
│   └── python/               # Python integration
├── examples/                 # Example projects
│   ├── cmake_example/        # CMake demo
│   └── makefile_example/     # Makefile demo
└── docs/                     # Documentation
```

## 📖 Documentation

### Getting Started
- [Installation Guide](docs/INSTALLATION.md) - How to install on your platform
- [Cross-Platform Setup](docs/CROSS_PLATFORM_SETUP.md) - Platform-specific setup (Linux, macOS, Windows, WSL2)
- [Quick Start Guide](docs/USER_GUIDE.md) - Basic usage and examples

### Detailed Guides
- [Using with Your Projects](docs/USING_WITH_YOUR_PROJECTS.md) - Integration with your own builds
- [Integration Guide](docs/INTEGRATION.md) - Integrate with build systems (CMake, Make, Yocto, etc.)
- [Platform Support](docs/PLATFORM_SUPPORT.md) - Platform and architecture compatibility

### Feature Documentation
- [File Types Guide](docs/FILE_TYPES.md) - Comprehensive file type tracking
- [File Types Quick Reference](docs/FILE_TYPES_QUICK_REF.md) - Quick lookup for file types
- [Linking Types Guide](docs/LINKING_TYPES.md) - Linking detection and analysis

### Reference
- [API Reference](docs/API.md) - C library API documentation
- [FAQ](docs/FAQ.md) - Frequently asked questions

## 🎓 Examples

### Example 1: CMake Project

```bash
cd examples/cmake_example
mkdir build && cd build
cmake ..
make build_with_tracking
```

### Example 2: Makefile Project

```bash
cd examples/makefile_example
make build-with-tracking
```

Both examples demonstrate tracking file access from external packages.

## 📈 Advanced Features

### Package-Specific Reports

```bash
python3 python/report_generator.py report.json -p package_name
```

### Analysis Tools

```bash
# Show top accessed files
python3 python/analyzer.py report.json --top 20

# Show package statistics
python3 python/analyzer.py report.json --stats

# Generate dependency graph
python3 python/analyzer.py report.json --graph deps.txt

# Filter by extension
python3 python/analyzer.py report.json --extension .h
```

### Report Formats

- **JSON** - Machine-readable, ideal for CI/CD
- **CSV** - Import into Excel/Google Sheets
- **XML** - Standard structured format
- **XLSX** - Native Excel format with formatting
- **TXT** - Human-readable summary

## 🔍 How It Works

1. **Interception**: Uses `LD_PRELOAD` to intercept file system calls (`open`, `fopen`, `stat`, etc.)
2. **Tracking**: Maintains a hash table of accessed files with metadata
3. **Filtering**: Smart filtering to track only relevant build files
4. **Reporting**: Generates comprehensive reports on build completion

### Intercepted Functions

- `open()` / `open64()`
- `fopen()` / `fopen64()`
- `access()`
- `stat()` / `lstat()`

## 🖥️ Platform Support

- ✅ **Linux** - Full support with LD_PRELOAD
- ⚠️ **macOS** - Partial support (requires SIP configuration)
- ⚠️ **Windows** - Limited (consider using WSL)

## 🛠️ Requirements

### Build Requirements
- GCC or Clang
- Make
- pthread library

### Report Generation Requirements
- Python 3.7+
- openpyxl (for XLSX export)

```bash
pip install -r python/requirements.txt
```

## 🤝 Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details.

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🐛 Troubleshooting

### Library not found
```bash
# Make sure libfiletracker.so is built
cd src && make

# Check LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/buildfiletracker/src:$LD_LIBRARY_PATH
```

### No reports generated
```bash
# Verify environment variables are set
echo $LD_PRELOAD
echo $FILE_TRACKER_JSON

# Check permissions
ls -l /path/to/output/directory
```

### Build fails with tracking
```bash
# Try without tracking first
make clean && make

# Then enable tracking
make build-with-tracking
```

## 📧 Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/buildfiletracker/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/buildfiletracker/discussions)
- **Email**: your.email@example.com

## 🌟 Features

- [x] Universal build system support
- [x] Multiple report formats (JSON, CSV, XML, XLSX)
- [x] CMake integration
- [x] Makefile integration
- [x] Python integration API
- [x] Yocto integration
- [x] Thread-safe operation
- [x] Package dependency analysis
- [x] File type statistics
- [ ] Windows native support
- [ ] GUI report viewer
- [ ] Real-time monitoring
- [ ] Database storage backend

## 📚 Use Cases

1. **Dependency Optimization** - Identify unused dependencies
2. **Build Optimization** - Only compile necessary files
3. **License Compliance** - Track which GPL components are used
4. **Security Auditing** - Know exact file dependencies
5. **Package Trimming** - Create minimal package distributions
6. **CI/CD Integration** - Automated dependency reports

## 🎯 Real-World Example

```bash
# Before BuildFileTracker
Package A: 10 MB (50 files)
Your Build: Uses all 50 files

# After BuildFileTracker
Package A: 10 MB (50 files)
Your Build: Actually uses 5 files (1 MB)
Savings: 9 MB, 90% reduction!
```

## 🙏 Acknowledgments

- Inspired by inotify and build system profiling tools
- Thanks to the open source community
- Special thanks to contributors

---

**Made with ❤️ for developers who want to understand their builds better**

⭐ Star this repository if you find it useful!
