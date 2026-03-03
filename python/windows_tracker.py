#!/usr/bin/env python3
"""
BuildFileTracker - Windows File Access Tracker
Pure Python implementation for Windows using file system monitoring
Works on Windows 10/11 and can be used as fallback on other platforms
"""

import json
import os
import sys
import subprocess
import threading
import time
from pathlib import Path
from datetime import datetime
from collections import defaultdict
from typing import Dict, Set, List, Optional
import shutil

try:
    from watchdog.observers import Observer
    from watchdog.events import FileSystemEventHandler, FileModifiedEvent, FileAccessedEvent
    HAS_WATCHDOG = True
except ImportError:
    HAS_WATCHDOG = False


class FileAccessTracker(FileSystemEventHandler):
    """Tracks file access events using watchdog"""
    
    def __init__(self, output_json: str = None, output_csv: str = None):
        """
        Initialize tracker
        
        Args:
            output_json: Path to output JSON file
            output_csv: Path to output CSV file
        """
        self.output_json = output_json or "build_tracking.json"
        self.output_csv = output_csv or "build_tracking.csv"
        
        self.accessed_files: Dict[str, Dict] = {}
        self.lock = threading.Lock()
        self.start_time = datetime.now()
        self.observer = None
        
    def on_modified(self, event):
        """Handle file modified event"""
        if not event.is_directory:
            self._record_access(event.src_path)
    
    def on_accessed(self, event):
        """Handle file accessed event"""
        if not event.is_directory:
            self._record_access(event.src_path)
    
    def _record_access(self, filepath: str):
        """Record file access"""
        with self.lock:
            # Normalize path
            filepath = str(Path(filepath).resolve())
            
            if filepath not in self.accessed_files:
                self.accessed_files[filepath] = {
                    'filepath': filepath,
                    'file_type': Path(filepath).suffix.lstrip('.').lower() or 'unknown',
                    'access_count': 0,
                    'first_access': datetime.now().isoformat(),
                    'last_access': datetime.now().isoformat(),
                    'package': self._detect_package(filepath)
                }
            
            self.accessed_files[filepath]['access_count'] += 1
            self.accessed_files[filepath]['last_access'] = datetime.now().isoformat()
    
    def _detect_package(self, filepath: str) -> str:
        """Detect package name from filepath"""
        # Common package paths
        package_indicators = {
            'Python': ['site-packages', 'dist-packages', 'lib/python'],
            'VisualStudio': ['Program Files', 'MSVC', 'VC\\Tools'],
            'MinGW': ['mingw', 'mingw-w64'],
            'vcpkg': ['vcpkg', 'vcpkg_installed'],
            'Conan': ['.conan', 'conan'],
            'BoostBuild': ['boost'],
            'OpenSSL': ['openssl'],
            'Zlib': ['zlib'],
            'Curl': ['curl'],
        }
        
        filepath_lower = filepath.lower()
        
        for package, patterns in package_indicators.items():
            for pattern in patterns:
                if pattern.lower() in filepath_lower:
                    return package
        
        # Try to use directory name as package
        parts = Path(filepath).parts
        if len(parts) > 1:
            # Check if it's in a standard location
            if 'Program Files' in filepath or 'Program Files (x86)' in filepath:
                return parts[-1] if parts else 'unknown'
        
        return 'unknown'
    
    def start(self, watch_paths: List[str] = None, recursive: bool = True):
        """
        Start watching directories
        
        Args:
            watch_paths: List of paths to watch (default: current directory)
            recursive: Whether to watch recursively
        """
        if not HAS_WATCHDOG:
            print("Error: watchdog library required for Windows tracking")
            print("Install with: pip install watchdog")
            return False
        
        if watch_paths is None:
            watch_paths = [os.getcwd()]
        
        self.observer = Observer()
        
        for path in watch_paths:
            if os.path.exists(path):
                self.observer.schedule(self, path, recursive=recursive)
                print(f"Watching: {path}")
        
        self.observer.start()
        print(f"Tracker started. Recording to: {self.output_json}")
        return True
    
    def stop(self):
        """Stop tracking and generate output files"""
        if self.observer:
            self.observer.stop()
            self.observer.join()
        
        self._write_json()
        self._write_csv()
        
        print(f"Tracking stopped.")
        print(f"JSON report: {self.output_json}")
        print(f"CSV report: {self.output_csv}")
        print(f"Total files accessed: {len(self.accessed_files)}")
    
    def _write_json(self):
        """Write JSON output file"""
        report = {
            'report_type': 'build_file_tracker',
            'timestamp': int(self.start_time.timestamp()),
            'start_time': self.start_time.isoformat(),
            'end_time': datetime.now().isoformat(),
            'accessed_files': list(self.accessed_files.values()),
            'summary': {
                'total_files': len(self.accessed_files),
                'total_accesses': sum(f['access_count'] for f in self.accessed_files.values())
            }
        }
        
        with open(self.output_json, 'w') as f:
            json.dump(report, f, indent=2)
    
    def _write_csv(self):
        """Write CSV output file"""
        import csv
        
        with open(self.output_csv, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['Filepath', 'Package', 'File Type', 'Access Count'])
            
            for file_info in sorted(self.accessed_files.values(), 
                                   key=lambda x: x['access_count'], 
                                   reverse=True):
                writer.writerow([
                    file_info['filepath'],
                    file_info['package'],
                    file_info['file_type'],
                    file_info['access_count']
                ])


class PowerShellTracker:
    """Fallback tracker using PowerShell Get-FileSystemWatcher"""
    
    def __init__(self, output_json: str = None, output_csv: str = None):
        """Initialize tracker"""
        self.output_json = output_json or "build_tracking.json"
        self.output_csv = output_csv or "build_tracking.csv"
        self.accessed_files: Dict[str, Dict] = {}
        self.ps_process = None
    
    def start(self, watch_paths: List[str] = None):
        """Start PowerShell monitoring (basic implementation)"""
        if watch_paths is None:
            watch_paths = [os.getcwd()]
        
        print("Warning: Using basic PowerShell tracking (less efficient)")
        print("Recommended: pip install watchdog")
        
        # Create monitoring loop
        self._run_monitoring(watch_paths)
    
    def _run_monitoring(self, paths: List[str]):
        """Monitor using dir listing approach (slower but works)"""
        import difflib
        
        print("Monitoring (Ctrl+C to stop)...")
        
        # Get initial state
        initial_state = {}
        for root_path in paths:
            for root, dirs, files in os.walk(root_path):
                for file in files:
                    filepath = os.path.join(root, file)
                    initial_state[filepath] = os.path.getmtime(filepath) if os.path.exists(filepath) else 0
        
        try:
            while True:
                time.sleep(1)  # Check every second
                
                current_state = {}
                for root_path in paths:
                    for root, dirs, files in os.walk(root_path):
                        for file in files:
                            filepath = os.path.join(root, file)
                            current_state[filepath] = os.path.getmtime(filepath) if os.path.exists(filepath) else 0
                
                # Find new/modified files
                for filepath, mtime in current_state.items():
                    if filepath not in initial_state or current_state[filepath] != initial_state[filepath]:
                        self._record_access(filepath)
                
                initial_state = current_state
        except KeyboardInterrupt:
            print("\nMonitoring stopped")
    
    def _record_access(self, filepath: str):
        """Record file access"""
        filepath = str(Path(filepath).resolve())
        
        if filepath not in self.accessed_files:
            self.accessed_files[filepath] = {
                'filepath': filepath,
                'file_type': Path(filepath).suffix.lstrip('.').lower() or 'unknown',
                'access_count': 0,
                'first_access': datetime.now().isoformat(),
                'last_access': datetime.now().isoformat(),
                'package': self._detect_package(filepath)
            }
        
        self.accessed_files[filepath]['access_count'] += 1
        self.accessed_files[filepath]['last_access'] = datetime.now().isoformat()
    
    def _detect_package(self, filepath: str) -> str:
        """Detect package from filepath"""
        return 'unknown'  # Simplified for fallback
    
    def stop(self):
        """Stop tracking and save"""
        self._write_json()
        self._write_csv()
    
    def _write_json(self):
        """Write JSON output"""
        report = {
            'report_type': 'build_file_tracker',
            'timestamp': int(time.time()),
            'accessed_files': list(self.accessed_files.values()),
            'summary': {
                'total_files': len(self.accessed_files),
                'total_accesses': sum(f['access_count'] for f in self.accessed_files.values())
            }
        }
        
        with open(self.output_json, 'w') as f:
            json.dump(report, f, indent=2)
    
    def _write_csv(self):
        """Write CSV output"""
        import csv
        
        with open(self.output_csv, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['Filepath', 'Package', 'File Type', 'Access Count'])
            
            for file_info in sorted(self.accessed_files.values(), 
                                   key=lambda x: x['access_count'], 
                                   reverse=True):
                writer.writerow([
                    file_info['filepath'],
                    file_info['package'],
                    file_info['file_type'],
                    file_info['access_count']
                ])


def get_tracker(output_json: str = None, output_csv: str = None):
    """Get appropriate tracker for current platform"""
    if HAS_WATCHDOG:
        return FileAccessTracker(output_json, output_csv)
    else:
        return PowerShellTracker(output_json, output_csv)


def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Track build file access on Windows'
    )
    parser.add_argument('-o', '--output-json', default='build_tracking.json',
                       help='Output JSON file')
    parser.add_argument('-c', '--output-csv', default='build_tracking.csv',
                       help='Output CSV file')
    parser.add_argument('-p', '--path', action='append', default=[],
                       help='Path(s) to watch (can be used multiple times)')
    
    args = parser.parse_args()
    
    tracker = get_tracker(args.output_json, args.output_csv)
    
    watch_paths = args.path if args.path else [os.getcwd()]
    tracker.start(watch_paths)
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nStopping tracker...")
        tracker.stop()


if __name__ == '__main__':
    main()
