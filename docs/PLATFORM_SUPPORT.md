# 🌍 BuildFileTracker - Platform & Architecture Support

## Complete Cross-Platform Support

BuildFileTracker now works on **ALL major platforms** and supports **ALL common CPU architectures**!

---

## ✅ Supported Platforms

### 1. **Linux** (Full Support)
- **Binary Format**: ELF
- **Dependency Tool**: `ldd`
- **Architecture Detection**: Direct ELF header parsing + `file` command
- **Distributions**: Ubuntu, Debian, RHEL, CentOS, Fedora, Arch, Alpine, etc.
- **Status**: ✅ **Fully Tested**

**Features:**
- ✅ Shared library dependencies (`.so` files)
- ✅ Architecture detection (direct binary parsing)
- ✅ Symbol extraction
- ✅ Package tracking (`.deb`, `.rpm`, `.apk`)

### 2. **macOS** (Full Support)
- **Binary Format**: Mach-O
- **Dependency Tool**: `otool -L`
- **Architecture Detection**: Direct Mach-O header parsing + `file` command
- **Versions**: macOS 10.15+ (Intel and Apple Silicon)
- **Status**: ✅ **Fully Tested**

**Features:**
- ✅ Dylib dependencies (`.dylib` files)
- ✅ Universal binaries (fat binaries)
- ✅ Apple Silicon (ARM64) detection
- ✅ Intel (x86_64) detection
- ✅ Framework dependencies

### 3. **Windows** (Full Support)
- **Binary Format**: PE (Portable Executable)
- **Dependency Tools**: `dumpbin` (MSVC) or `objdump` (MinGW/Cygwin)
- **Architecture Detection**: Direct PE header parsing + `dumpbin`
- **Versions**: Windows 10/11, Windows Server 2016+
- **Status**: ✅ **Fully Tested**

**Features:**
- ✅ DLL dependencies (`.dll` files)
- ✅ EXE analysis
- ✅ x86 and x64 detection
- ✅ ARM64 Windows detection
- ✅ Both MSVC and MinGW toolchains

### 4. **BSD Systems** (Supported)
- **Platforms**: FreeBSD, OpenBSD, NetBSD
- **Binary Format**: ELF
- **Status**: ✅ **Compatible** (uses same tools as Linux)

### 5. **Unix Systems** (Supported)
- **Platforms**: Solaris, AIX, HP-UX
- **Binary Format**: ELF or proprietary
- **Status**: ✅ **Compatible** (with platform-specific tools)

---

## 🖥️ Supported CPU Architectures

BuildFileTracker detects and tracks binaries compiled for **15+ architectures**:

### **Intel/AMD (x86 Family)**
| Architecture | Bits | Detection Method | Platforms | Status |
|--------------|------|------------------|-----------|--------|
| **x86** | 32-bit | ELF/PE/Mach-O headers | Linux, Windows, macOS | ✅ Full |
| **x86_64** (AMD64) | 64-bit | ELF/PE/Mach-O headers | Linux, Windows, macOS | ✅ Full |

### **ARM Family**
| Architecture | Bits | Detection Method | Platforms | Status |
|--------------|------|------------------|-----------|--------|
| **ARM** | 32-bit | ELF/PE/Mach-O headers | Linux, Windows | ✅ Full |
| **ARMv7** | 32-bit | ELF headers + file command | Linux, Android | ✅ Full |
| **ARM64** (AArch64) | 64-bit | ELF/PE/Mach-O headers | Linux, macOS, Windows | ✅ Full |

### **RISC Architectures**
| Architecture | Bits | Detection Method | Platforms | Status |
|--------------|------|------------------|-----------|--------|
| **RISC-V 32** | 32-bit | ELF headers | Linux | ✅ Full |
| **RISC-V 64** | 64-bit | ELF headers | Linux | ✅ Full |

### **MIPS**
| Architecture | Bits | Detection Method | Platforms | Status |
|--------------|------|------------------|-----------|--------|
| **MIPS** | 32-bit | ELF headers | Linux, routers | ✅ Full |
| **MIPS64** | 64-bit | ELF headers | Linux | ✅ Full |

### **PowerPC**
| Architecture | Bits | Detection Method | Platforms | Status |
|--------------|------|------------------|-----------|--------|
| **PowerPC** | 32-bit | ELF headers | Linux, AIX | ✅ Full |
| **PowerPC64** | 64-bit | ELF headers | Linux, AIX | ✅ Full |

### **SPARC**
| Architecture | Bits | Detection Method | Platforms | Status |
|--------------|------|------------------|-----------|--------|
| **SPARC** | 32-bit | ELF headers | Solaris, Linux | ✅ Full |
| **SPARC64** | 64-bit | ELF headers | Solaris, Linux | ✅ Full |

### **IBM Mainframe**
| Architecture | Bits | Detection Method | Platforms | Status |
|--------------|------|------------------|-----------|--------|
| **s390x** | 64-bit | ELF headers | Linux (IBM Z) | ✅ Full |

### **Other Architectures**
| Architecture | Bits | Detection Method | Platforms | Status |
|--------------|------|------------------|-----------|--------|
| **LoongArch64** | 64-bit | ELF headers | Linux (Chinese CPUs) | ✅ Full |

---

## 🔬 Architecture Detection Methods

BuildFileTracker uses **multiple detection methods** for maximum accuracy:

### 1. **Direct Binary Header Parsing** (Primary)
Most accurate method - reads binary format directly:

```python
# Reads ELF e_machine field (offset 0x12)
# Reads PE Machine field (offset PE+0x04)
# Reads Mach-O cputype field (offset 0x04)
```

**Advantages:**
- ✅ Works without external tools
- ✅ Platform-independent
- ✅ Fastest method
- ✅ Works on all platforms

**Supported Formats:**
- **ELF**: Linux, BSD, Unix binaries
- **PE**: Windows `.exe`, `.dll` files
- **Mach-O**: macOS binaries and frameworks

### 2. **file Command** (Fallback)
Uses Unix `file` command for detection:

```bash
file /path/to/binary
# Output: ELF 64-bit LSB executable, x86-64, version 1 (SYSV)
```

**Advantages:**
- ✅ Very detailed output
- ✅ Handles unusual formats
- ✅ Available on most Unix-like systems

### 3. **dumpbin** (Windows Fallback)
Uses Visual Studio `dumpbin` on Windows:

```cmd
dumpbin /HEADERS binary.exe
```

**Advantages:**
- ✅ Native Windows tool
- ✅ Detailed PE information
- ✅ Shows subsystem and linker version

---

## 🛠️ Platform-Specific Tools

### Linux Tools
```bash
# Dependency extraction
ldd /usr/lib/libssl.so

# Architecture detection
file /usr/bin/gcc
readelf -h /usr/bin/gcc

# Symbol listing
nm -D /usr/lib/libssl.so
objdump -T /usr/lib/libssl.so
```

### macOS Tools
```bash
# Dependency extraction
otool -L /usr/lib/libssl.dylib

# Architecture detection (including Universal binaries)
file /usr/bin/gcc
lipo -info /usr/bin/gcc

# Symbol listing
nm -g /usr/lib/libssl.dylib
otool -tV /usr/lib/libssl.dylib
```

### Windows Tools
```cmd
REM Dependency extraction (Visual Studio)
dumpbin /DEPENDENTS library.dll

REM Dependency extraction (MinGW)
objdump -p library.dll

REM Architecture detection
dumpbin /HEADERS program.exe

REM Symbol listing
dumpbin /EXPORTS library.dll
```

---

## 📊 Usage Examples

### Detect Architecture Across Platforms

```python
from buildfiletracker.filetypes import FileTypeDetector

# Linux x86_64 binary
info = FileTypeDetector.analyze_file("/usr/lib/x86_64-linux-gnu/libssl.so.3")
print(f"Architecture: {info.architecture}")  # x86_64

# macOS ARM64 (Apple Silicon)
info = FileTypeDetector.analyze_file("/usr/lib/libSystem.dylib")
print(f"Architecture: {info.architecture}")  # arm64

# Windows x64 DLL
info = FileTypeDetector.analyze_file("C:\\Windows\\System32\\kernel32.dll")
print(f"Architecture: {info.architecture}")  # x86_64

# Raspberry Pi ARM
info = FileTypeDetector.analyze_file("/usr/lib/arm-linux-gnueabihf/libc.so.6")
print(f"Architecture: {info.architecture}")  # arm

# RISC-V Linux
info = FileTypeDetector.analyze_file("/lib/riscv64-linux-gnu/libc.so.6")
print(f"Architecture: {info.architecture}")  # riscv64
```

### Extract Dependencies - Cross Platform

```python
# Linux: Uses ldd
info = FileTypeDetector.analyze_file("/usr/bin/python3")
print(f"Dependencies: {info.dependencies}")
# ['libpython3.10.so.1.0', 'libc.so.6', 'libm.so.6']

# macOS: Uses otool
info = FileTypeDetector.analyze_file("/usr/bin/python3")
print(f"Dependencies: {info.dependencies}")
# ['/usr/lib/libSystem.B.dylib', '/usr/lib/libobjc.A.dylib']

# Windows: Uses dumpbin or objdump
info = FileTypeDetector.analyze_file("C:\\Python310\\python.exe")
print(f"Dependencies: {info.dependencies}")
# ['KERNEL32.dll', 'USER32.dll', 'ADVAPI32.dll']
```

### Multi-Architecture Build Tracking

```python
from buildfiletracker import BuildFileTracker

# Track cross-compiled build
tracker = BuildFileTracker()

# Build for multiple targets
tracker.run_command(["make", "ARCH=x86_64"])
tracker.run_command(["make", "ARCH=arm64"])
tracker.run_command(["make", "ARCH=riscv64"])

# Get architecture breakdown
arch_map = {}
for filepath, info in tracker.file_access_map.items():
    file_info = FileTypeDetector.analyze_file(filepath)
    if file_info.architecture != 'unknown':
        arch = file_info.architecture
        arch_map.setdefault(arch, []).append(filepath)

print("\\nArchitecture Summary:")
for arch, files in arch_map.items():
    print(f"  {arch}: {len(files)} files")
```

---

## 🧪 Testing on Different Platforms

### Linux Example
```bash
# Build and track
python3 -c "
from buildfiletracker import BuildFileTracker

tracker = BuildFileTracker()
tracker.run_command(['gcc', '-o', 'myapp', 'main.c', '-lssl'])
tracker.generate_report('report.json')
"

# Check tracked binaries
cat report.json | jq '.files[] | select(.type | contains("lib"))'
```

### macOS Example
```bash
# Track Xcode build
python3 -c "
from buildfiletracker import BuildFileTracker

tracker = BuildFileTracker()
tracker.run_command(['xcodebuild', '-project', 'MyApp.xcodeproj'])
tracker.generate_report('report.json')
"
```

### Windows Example (PowerShell)
```powershell
# Track Visual Studio build
python -c "
from buildfiletracker import BuildFileTracker

tracker = BuildFileTracker()
tracker.run_command(['msbuild', 'MyApp.sln'])
tracker.generate_report('report.json')
"
```

---

## 🐳 Container/Embedded Support

### Docker (Multi-Arch)
```dockerfile
# Works with multi-architecture Docker images
FROM --platform=linux/amd64 ubuntu:22.04    # x86_64
FROM --platform=linux/arm64 ubuntu:22.04    # ARM64
FROM --platform=linux/riscv64 ubuntu:22.04  # RISC-V
```

### Embedded Systems
- ✅ **Raspberry Pi** (ARM, ARM64)
- ✅ **BeagleBone** (ARM)
- ✅ **RISC-V development boards**
- ✅ **MIPS routers** (OpenWrt)
- ✅ **Android NDK** builds (ARM, ARM64, x86, x86_64)

### Yocto/Embedded Linux
```python
# Track Yocto build (any architecture)
tracker = BuildFileTracker()
tracker.run_command(['bitbake', 'core-image-minimal'])

# Detects target architecture from generated binaries
```

---

## 🔧 Troubleshooting

### Missing Tools

**Linux:**
```bash
# Install file command
sudo apt-get install file        # Debian/Ubuntu
sudo yum install file            # RHEL/CentOS

# Install ldd (usually included with glibc)
sudo apt-get install libc-bin
```

**macOS:**
```bash
# file and otool are included with macOS
# If missing, install Xcode Command Line Tools
xcode-select --install
```

**Windows:**
```powershell
# Install Visual Studio (includes dumpbin)
# Or install MinGW-w64 (includes objdump)

# Verify dumpbin
where dumpbin

# Verify objdump
where objdump
```

### Architecture Not Detected

If architecture shows as `unknown`:

1. **Check binary format** - Make sure the file is actually a binary
2. **Install tools** - Ensure `file`/`dumpbin` is available
3. **Direct parsing works** - The library can read headers directly without tools
4. **Check file permissions** - Ensure the binary is readable

### Dependencies Not Extracted

If dependencies list is empty:

1. **Static vs Dynamic** - Static binaries have no dependencies
2. **Tool availability** - Check if `ldd`/`otool`/`dumpbin` exists
3. **Permissions** - Binary must be readable and (for ldd) executable
4. **Cross-compiled** - `ldd` won't work on cross-compiled binaries

---

## 📈 Platform Support Summary

| Feature | Linux | macOS | Windows | BSD | Status |
|---------|-------|-------|---------|-----|--------|
| File type detection | ✅ | ✅ | ✅ | ✅ | Full |
| Architecture detection | ✅ | ✅ | ✅ | ✅ | Full |
| Dependency extraction | ✅ | ✅ | ✅ | ✅ | Full |
| Binary header parsing | ✅ | ✅ | ✅ | ✅ | Full |
| Symbol extraction | ✅ | ✅ | ✅ | ✅ | Full |
| Archive analysis | ✅ | ✅ | ✅ | ✅ | Full |
| Checksum calculation | ✅ | ✅ | ✅ | ✅ | Full |

| Architecture | Support | Platforms | Detection |
|--------------|---------|-----------|-----------|
| x86 (32-bit) | ✅ Full | All | ELF/PE/Mach-O |
| x86_64 (64-bit) | ✅ Full | All | ELF/PE/Mach-O |
| ARM (32-bit) | ✅ Full | Linux, Windows | ELF/PE |
| ARM64 (AArch64) | ✅ Full | All | ELF/PE/Mach-O |
| RISC-V 32/64 | ✅ Full | Linux | ELF |
| MIPS 32/64 | ✅ Full | Linux, routers | ELF |
| PowerPC 32/64 | ✅ Full | Linux, AIX | ELF |
| SPARC 32/64 | ✅ Full | Solaris, Linux | ELF |
| s390x | ✅ Full | Linux (IBM Z) | ELF |
| LoongArch64 | ✅ Full | Linux | ELF |

---

## 🎯 Conclusion

**BuildFileTracker is truly universal:**

✅ **3 major platforms** (Linux, macOS, Windows) + BSD/Unix  
✅ **15+ CPU architectures** (x86, ARM, RISC-V, MIPS, PowerPC, SPARC, s390x)  
✅ **Multiple detection methods** (header parsing, file command, platform tools)  
✅ **Cross-compilation support** (detects target architecture)  
✅ **Container-friendly** (works in Docker, LXC, etc.)  
✅ **Embedded systems** (Raspberry Pi, BeagleBone, OpenWrt)  

**No matter what platform or architecture you're building for, BuildFileTracker can track it!** 🎉

---

**Questions? See [FAQ.md](FAQ.md) or [GitHub Issues](https://github.com/yourusername/buildfiletracker/issues)**
