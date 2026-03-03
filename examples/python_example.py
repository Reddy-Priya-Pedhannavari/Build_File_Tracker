#!/usr/bin/env python3
"""
BuildFileTracker - Python API Example
Demonstrates tracking file usage in a build and generating reports

Usage:
    python python_example.py
"""

import sys
import os
from pathlib import Path

# Add buildfiletracker to path
sys.path.insert(0, str(Path(__file__).parent.parent / 'python'))

from buildfiletracker import (
    BuildFileTracker,
    TrackingBackend,
    ReportFormat,
    PackageInfo
)


def main():
    print("="*70)
    print("BuildFileTracker - Python API Example")
    print("="*70)
    
    # Create tracker with automatic backend selection
    tracker = BuildFileTracker(backend=TrackingBackend.AUTO)
    
    # Register packages
    example_dir = Path(__file__).parent / "cmake_example"
    
    print("\n📦 Registering packages...")
    tracker.register_package(
        name="package_a",
        root_path=str(example_dir / "external" / "package_a"),
        version="1.0.0",
        license="MIT",
        homepage="https://example.com/package_a"
    )
    
    tracker.register_package(
        name="package_b",
        root_path=str(example_dir / "external" / "package_b"),
        version="2.1.0",
        license="Apache-2.0"
    )
    
    print(f"✅ Registered {tracker.stats.total_packages} packages")
    
    # Add watch paths
    tracker.add_watch(str(example_dir / "external"), recursive=True)
    
    # Set filters
    tracker.set_extension_filter(['.c', '.h', '.cpp', '.hpp'])
    tracker.add_exclude('*/test/*')
    tracker.add_exclude('*/docs/*')
    
    # Start tracking
    print("\n🔍 Starting tracking...")
    tracker.start()
    
    # Method 1: Run a build command directly
    if (example_dir / "CMakeLists.txt").exists():
        print("\n🔨 Building CMake example...")
        
        build_dir = example_dir / "build"
        build_dir.mkdir(exist_ok=True)
        
        # Configure
        result = tracker.run_command(
            ["cmake", ".."],
            cwd=str(build_dir)
        )
        
        if result.returncode == 0:
            # Build
            result = tracker.run_command(
                ["cmake", "--build", "."],
                cwd=str(build_dir)
            )
    else:
        print("\n⚠️  CMake example not found, simulating file access...")
        # Method 2: Record file access manually
        tracker.record_access(
            str(example_dir / "external" / "package_a" / "used_file.c"),
            access_type="read",
            pid=os.getpid()
        )
        tracker.record_access(
            str(example_dir / "external" / "package_a" / "used_file.h"),
            access_type="read",
            pid=os.getpid()
        )
        tracker.record_access(
            str(example_dir / "external" / "package_b" / "helper.c"),
            access_type="read",
            pid=os.getpid()
        )
    
    # Stop tracking
    print("\n⏹️  Stopping tracking...")
    tracker.stop()
    
    # Get statistics
    stats = tracker.get_statistics()
    print(f"\n📊 Statistics:")
    print(f"   Total packages: {stats.total_packages}")
    print(f"   Packages used: {stats.packages_used}")
    print(f"   Files tracked: {stats.unique_files}")
    print(f"   Total events: {stats.total_events}")
    print(f"   Runtime: {stats.runtime_seconds:.2f}s")
    
    # Print package details
    print(f"\n📦 Package Usage:")
    for pkg in sorted(tracker.get_all_packages(), 
                     key=lambda p: p.usage_percentage, reverse=True):
        print(f"   {pkg.name:20} {pkg.usage_percentage:6.2f}% "
              f"({pkg.used_files_count}/{pkg.total_files} files)")
        if pkg.used_files:
            print(f"      Used files:")
            for filepath in sorted(pkg.used_files)[:5]:
                print(f"         - {Path(filepath).name}")
            if len(pkg.used_files) > 5:
                print(f"         ... and {len(pkg.used_files) - 5} more")
    
    # Generate reports
    print("\n📝 Generating reports...")
    output_dir = Path(__file__).parent / "reports"
    output_dir.mkdir(exist_ok=True)
    
    try:
        # Individual format examples
        tracker.generate_report(
            str(output_dir / "report.json"),
            format=ReportFormat.JSON
        )
        print("   ✅ JSON report generated")
        
        tracker.generate_report(
            str(output_dir / "report.csv"),
            format=ReportFormat.CSV
        )
        print("   ✅ CSV report generated")
        
        tracker.generate_report(
            str(output_dir / "report_spdx.json"),
            format=ReportFormat.SPDX_JSON
        )
        print("   ✅ SPDX JSON report generated")
        
        tracker.generate_report(
            str(output_dir / "report_cyclonedx.json"),
            format=ReportFormat.CYCLONEDX_JSON
        )
        print("   ✅ CycloneDX JSON report generated")
        
        tracker.generate_report(
            str(output_dir / "report.html"),
            format=ReportFormat.HTML
        )
        print("   ✅ HTML report generated")
        
        tracker.generate_report(
            str(output_dir / "report.md"),
            format=ReportFormat.MARKDOWN
        )
        print("   ✅ Markdown report generated")
        
        # Generate all formats at once
        tracker.generate_all_reports(str(output_dir), base_name="complete")
        print("   ✅ All formats generated (complete_*)")
        
    except Exception as e:
        print(f"   ⚠️  Error generating reports: {e}")
    
    # Print summary
    tracker.print_summary()
    
    print(f"\n✅ Reports saved to: {output_dir}")
    print("="*70)


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
