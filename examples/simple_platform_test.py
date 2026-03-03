# Cross-Platform Binary Analysis Demo
import sys
import platform
sys.path.insert(0, 'python')

from buildfiletracker.filetypes import FileTypeDetector, FileType

print("=" * 70)
print("BuildFileTracker - Cross-Platform Test")
print("=" * 70)
print(f"\nPlatform: {platform.system()}")
print(f"Architecture: {platform.machine()}")
print(f"Python: {sys.version.split()[0]}")

# Test on Windows system binaries
test_files = [
    r"C:\Windows\System32\kernel32.dll",
    r"C:\Windows\System32\user32.dll",
    r"C:\Windows\System32\cmd.exe",
]

print(f"\nTesting {len(test_files)} Windows system binaries:\n")

for filepath in test_files:
    try:
        info = FileTypeDetector.analyze_file(filepath)
        print(f"File: {filepath}")
        print(f"  Type: {info.filetype.value}")
        print(f"  Architecture: {info.architecture}")
        print(f"  Size: {info.size:,} bytes")
        if info.dependencies:
            print(f"  Dependencies: {len(info.dependencies)} found")
        print()
    except Exception as e:
        print(f"  Error: {e}\n")

print("\n" + "=" * 70)
print("Platform Support Summary")
print("=" * 70)
print("\nSupported Platforms:")
print("  - Linux (ELF binaries, ldd)")
print("  - macOS (Mach-O binaries, otool)")  
print("  - Windows (PE binaries, dumpbin/objdump)")
print("  - BSD/Unix systems")

print("\nSupported Architectures:")
print("  - x86, x86_64 (Intel/AMD)")
print("  - ARM, ARM64 (including Apple Silicon)")
print("  - RISC-V 32/64")
print("  - MIPS 32/64")
print("  - PowerPC 32/64")
print("  - SPARC 32/64")
print("  - IBM s390x")
print("  - LoongArch64")

print("\nDetection Methods:")
print("  1. Binary header parsing (ELF/PE/Mach-O)")
print("  2. Platform tools (ldd, otool, dumpbin)")
print("  3. file command fallback")

print("\n" + "=" * 70)
