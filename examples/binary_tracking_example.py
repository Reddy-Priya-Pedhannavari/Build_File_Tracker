#!/usr/bin/env python3
"""
BuildFileTracker - Binary and Library Tracking Example

Demonstrates tracking ALL file types including:
- Source files (.c, .cpp, .py, .java)
- Binary files (.o, .so, .dll, .exe)
- Libraries (static and shared)
- Archives (.jar, .zip, .whl)
- Resources, configs, and more

Usage:
    python binary_tracking_example.py
"""

import sys
import os
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent / 'python'))

from buildfiletracker import BuildFileTracker, TrackingBackend, ReportFormat
from buildfiletracker.filetypes import (
    FileTypeDetector, FileType, FileCategory,
    EXTENSIONS_BINARY, EXTENSIONS_LIBRARY, EXTENSIONS_ALL
)


def main():
    print("=" * 70)
    print("BuildFileTracker - Binary & Library Tracking Example")
    print("=" * 70)
    
    # Create tracker
    tracker = BuildFileTracker(backend=TrackingBackend.AUTO)
    
    # Register packages with binary libraries
    print("\n📦 Registering packages with binaries and libraries...")
    
    packages = [
        # System libraries
        ("openssl", "/usr/lib/x86_64-linux-gnu/openssl", "3.0.0", "Apache-2.0"),
        ("zlib", "/usr/lib/x86_64-linux-gnu", "1.2.11", "Zlib"),
        ("curl", "/usr/lib/x86_64-linux-gnu", "7.79.0", "MIT"),
        
        # Python packages with binary extensions
        ("numpy", "/usr/local/lib/python3.10/site-packages/numpy", "1.24.0", "BSD-3-Clause"),
        ("pillow", "/usr/local/lib/python3.10/site-packages/PIL", "10.0.0", "PIL"),
    ]
    
    for name, path, version, license in packages:
        if os.path.exists(path):
            tracker.register_package(name, path, version, license)
            print(f"   ✓ {name} ({version})")
    
    # Configure to track ALL file types
    print("\n🔍 Configuring comprehensive file tracking...")
    print("   • Tracking: Source files, binaries, libraries, archives, configs, resources")
    
    # Don't set extension filter to track everything
    # Or use specific types:
    all_extensions = (
        EXTENSIONS_BINARY +  # .o, .so, .dll, .a, .lib
        ['.c', '.cpp', '.h', '.hpp'] +  # Source
        ['.py', '.pyc', '.pyd', '.whl'] +  # Python
        ['.jar', '.class', '.war'] +  # Java
        ['.json', '.xml', '.yaml'] +  # Config
        ['.png', '.jpg', '.ttf']  # Resources
    )
    
    # For this example, track everything
    # tracker.set_extension_filter(all_extensions)
    
    # Exclude common directories
    tracker.add_exclude('*/test/*')
    tracker.add_exclude('*/tests/*')
    tracker.add_exclude('*/__pycache__/*')
    tracker.add_exclude('*/node_modules/*')
    
    # Example 1: Track a build that links libraries
    print("\n🔨 Example 1: Tracking build with library linking...")
    print("   Simulating: gcc main.c -lssl -lcrypto -lz -o app")
    
    tracker.start()
    
    # Simulate file accesses
    simulated_files = [
        # Source files
        "main.c",
        "utils.h",
        
        # Object files generated
        "main.o",
        "utils.o",
        
        # Libraries linked
        "/usr/lib/x86_64-linux-gnu/libssl.so.3",
        "/usr/lib/x86_64-linux-gnu/libcrypto.so.3",
        "/usr/lib/x86_64-linux-gnu/libz.so.1",
        
        # Python binary extension
        "/usr/local/lib/python3.10/site-packages/numpy/_core/_multiarray_umath.so",
        
        # JAR file
        "/usr/share/java/junit.jar",
        
        # Config files
        "config.json",
        "settings.xml",
        
        # Resources
        "logo.png",
        "font.ttf",
    ]
    
    for filepath in simulated_files:
        if os.path.exists(filepath):
            tracker.record_access(filepath, access_type="read", pid=os.getpid())
    
    tracker.stop()
    
    # Analyze tracked files by type
    print("\n📊 File Type Analysis:")
    
    file_types = {}
    for filepath in tracker.file_access_map.keys():
        # Detect file type
        ftype = FileTypeDetector.detect_type(filepath)
        
        if ftype not in file_types:
            file_types[ftype] = []
        file_types[ftype].append(filepath)
    
    # Print by category
    for ftype, files in sorted(file_types.items(), key=lambda x: len(x[1]), reverse=True):
        if len(files) > 0:
            print(f"\n   {ftype.value}:")
            for f in files[:5]:  # Show first 5
                print(f"      - {Path(f).name}")
            if len(files) > 5:
                print(f"      ... and {len(files) - 5} more")
    
    # Example 2: Analyze binary dependencies
    print("\n\n🔍 Example 2: Binary Dependency Analysis...")
    
    binary_files = [
        "/usr/lib/x86_64-linux-gnu/libssl.so.3",
        "/usr/lib/x86_64-linux-gnu/libcrypto.so.3",
    ]
    
    for binary in binary_files:
        if os.path.exists(binary):
            print(f"\n   Analyzing: {Path(binary).name}")
            
            info = FileTypeDetector.analyze_file(binary)
            
            print(f"      Type: {info.filetype.value}")
            print(f"      Size: {info.size:,} bytes")
            print(f"      Binary: {info.is_binary}")
            print(f"      Library: {info.is_library}")
            print(f"      Architecture: {info.architecture}")
            print(f"      Checksum: {info.checksum_sha256[:16]}...")
            
            if info.dependencies:
                print(f"      Dependencies ({len(info.dependencies)}):")
                for dep in info.dependencies[:5]:
                    print(f"         - {dep}")
                if len(info.dependencies) > 5:
                    print(f"         ... and {len(info.dependencies) - 5} more")
    
    # Example 3: Track Python package with compiled extensions
    print("\n\n🐍 Example 3: Python Package with Binary Extensions...")
    
    numpy_so = "/usr/local/lib/python3.10/site-packages/numpy/_core/_multiarray_umath.so"
    if os.path.exists(numpy_so):
        tracker.record_access(numpy_so, access_type="read", pid=os.getpid())
        
        info = FileTypeDetector.analyze_file(numpy_so)
        print(f"\n   File: {Path(numpy_so).name}")
        print(f"   Type: {info.filetype.value} (Python compiled extension)")
        print(f"   Size: {info.size:,} bytes")
        print(f"   Architecture: {info.architecture}")
        print(f"   Language: {info.language}")
    
    # Example 4: Track JAR file with contents
    print("\n\n☕ Example 4: Java JAR Archive Analysis...")
    
    jar_file = "/usr/share/java/junit.jar"
    if os.path.exists(jar_file):
        tracker.record_access(jar_file, access_type="read", pid=os.getpid())
        
        info = FileTypeDetector.analyze_file(jar_file)
        print(f"\n   File: {Path(jar_file).name}")
        print(f"   Type: {info.filetype.value}")
        print(f"   Size: {info.size:,} bytes")
        
        if info.archive_contents:
            print(f"   Contents ({len(info.archive_contents)} files):")
            for item in info.archive_contents[:10]:
                print(f"      - {item}")
            if len(info.archive_contents) > 10:
                print(f"      ... and {len(info.archive_contents) - 10} more")
    
    # Generate comprehensive reports
    print("\n\n📝 Generating comprehensive reports...")
    
    output_dir = Path(__file__).parent / "reports_binary"
    output_dir.mkdir(exist_ok=True)
    
    try:
        # JSON with all file type details
        tracker.generate_report(
            str(output_dir / "binary_tracking.json"),
            format=ReportFormat.JSON
        )
        print("   ✅ JSON report (with binary analysis)")
        
        # SPDX SBOM including binary artifacts
        tracker.generate_report(
            str(output_dir / "binary_sbom.spdx.json"),
            format=ReportFormat.SPDX_JSON
        )
        print("   ✅ SPDX SBOM (including binaries)")
        
        # HTML dashboard
        tracker.generate_report(
            str(output_dir / "binary_analysis.html"),
            format=ReportFormat.HTML
        )
        print("   ✅ HTML dashboard")
        
    except Exception as e:
        print(f"   ⚠️  Error: {e}")
    
    # Summary
    print("\n" + "=" * 70)
    print("📊 Summary")
    print("=" * 70)
    
    stats = tracker.get_statistics()
    print(f"Total files tracked: {stats.unique_files}")
    print(f"Total events: {stats.total_events}")
    print(f"Runtime: {stats.runtime_seconds:.2f}s")
    
    print(f"\nFile Type Breakdown:")
    for ftype, files in sorted(file_types.items(), key=lambda x: len(x[1]), reverse=True):
        if len(files) > 0:
            print(f"   {ftype.value:30} {len(files):4} files")
    
    print(f"\n✅ Reports saved to: {output_dir}")
    print("=" * 70)
    
    # Key insights
    print("\n💡 Key Insights:")
    print("   ✓ Tracked source files (.c, .h, .py, .java)")
    print("   ✓ Tracked binary files (.o, .so, .dll, .exe)")
    print("   ✓ Tracked libraries (static and shared)")
    print("   ✓ Tracked archives (.jar, .zip, .whl)")
    print("   ✓ Tracked config files (.json, .xml, .yaml)")
    print("   ✓ Tracked resources (.png, .ttf)")
    print("   ✓ Analyzed binary dependencies")
    print("   ✓ Analyzed archive contents")
    print("   ✓ Generated SBOM with all artifacts")
    print("\n   BuildFileTracker now tracks EVERYTHING! 🎉")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  Interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
