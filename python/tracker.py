#!/usr/bin/env python3
"""
BuildFileTracker - Universal Cross-Platform Tracker
Automatically selects appropriate tracking method based on OS and available tools
"""

import os
import sys
import subprocess
import platform
import shutil
from pathlib import Path
from typing import List, Optional


def is_linux_or_macos():
    """Check if running on Linux or macOS"""
    return platform.system() in ('Linux', 'Darwin')


def has_ld_preload():
    """Check if LD_PRELOAD is available (Linux/macOS)"""
    return shutil.which('ld.so') is not None or shutil.which('dyld') is not None


def get_libfiletracker_path():
    """Find libfiletracker.so/dylib"""
    possible_paths = [
        './src/libfiletracker.so',
        '../src/libfiletracker.so',
        '/usr/local/lib/libfiletracker.so',
        '/usr/lib/libfiletracker.so',
        './src/libfiletracker.dylib',
        '../src/libfiletracker.dylib',
        '/usr/local/lib/libfiletracker.dylib',
        '/usr/lib/libfiletracker.dylib',
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            return os.path.abspath(path)
    
    return None


def track_with_ld_preload(build_command: List[str], output_json: str, output_csv: str = None):
    """
    Track using LD_PRELOAD (Linux/macOS)
    
    Args:
        build_command: List containing the build command
        output_json: Path to output JSON file
        output_csv: Optional path to output CSV file
    """
    lib_path = get_libfiletracker_path()
    if not lib_path:
        print("Error: libfiletracker.so/dylib not found")
        print("Build it first: cd src && make")
        return False
    
    # Set environment variables
    env = os.environ.copy()
    env['LD_PRELOAD'] = lib_path
    env['FILE_TRACKER_JSON'] = output_json
    if output_csv:
        env['FILE_TRACKER_CSV'] = output_csv
    
    print(f"Tracking with LD_PRELOAD: {lib_path}")
    print(f"Output: {output_json}")
    
    # Run build command
    try:
        result = subprocess.run(build_command, env=env)
        return result.returncode == 0
    except Exception as e:
        print(f"Error running build command: {e}")
        return False


def track_with_windows_tracker(build_command: List[str], output_json: str, output_csv: str = None):
    """
    Track using Python-based Windows tracker
    
    Args:
        build_command: List containing the build command
        output_json: Path to output JSON file
        output_csv: Optional path to output CSV file
    """
    from windows_tracker import get_tracker
    
    print("Tracking with Python-based Windows tracker")
    print(f"Output: {output_json}")
    
    # Get tracker (watchdog if available, fallback to PowerShell)
    tracker = get_tracker(output_json, output_csv)
    
    # Start tracking in background thread
    import threading
    tracker_thread = threading.Thread(target=tracker.start, args=([os.getcwd()],), daemon=False)
    tracker_thread.start()
    
    # Run build command
    try:
        result = subprocess.run(build_command)
        return_code = result.returncode
    except KeyboardInterrupt:
        print("\nBuild interrupted")
        return_code = 1
    except Exception as e:
        print(f"Error running build command: {e}")
        return_code = 1
    
    # Stop tracking
    tracker.stop()
    
    return return_code == 0


def auto_detect_tracker(build_command: List[str], output_json: str = None, output_csv: str = None) -> bool:
    """
    Auto-detect and use appropriate tracker for current platform
    
    Args:
        build_command: Build command as list
        output_json: Output JSON file path
        output_csv: Optional output CSV file path
    
    Returns:
        True if successful, False otherwise
    """
    if not output_json:
        output_json = 'build_tracking.json'
    
    system = platform.system()
    
    print(f"BuildFileTracker - {system} Platform")
    print(f"Build command: {' '.join(build_command)}")
    print("-" * 60)
    
    if system in ('Linux', 'Darwin'):  # Linux or macOS
        return track_with_ld_preload(build_command, output_json, output_csv)
    elif system == 'Windows':
        return track_with_windows_tracker(build_command, output_json, output_csv)
    else:
        print(f"Error: Unsupported platform: {system}")
        return False


def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Cross-platform build file tracker',
        epilog='Examples:\n' + 
               '  python tracker.py make all\n' +
               '  python tracker.py cmake --build .\n' +
               '  python tracker.py -o myreport.json cmake --build .',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument('build_command', nargs='+',
                       help='Build command to track (e.g., make, cmake --build .)')
    parser.add_argument('-o', '--output-json', default='build_tracking.json',
                       help='Output JSON file (default: build_tracking.json)')
    parser.add_argument('-c', '--output-csv',
                       help='Output CSV file (optional)')
    
    args = parser.parse_args()
    
    success = auto_detect_tracker(args.build_command, args.output_json, args.output_csv)
    
    if success:
        print("\n✓ Tracking completed successfully")
        print(f"Reports generated:")
        print(f"  - {args.output_json}")
        if args.output_csv:
            print(f"  - {args.output_csv}")
        sys.exit(0)
    else:
        print("\n✗ Tracking failed")
        sys.exit(1)


if __name__ == '__main__':
    main()
