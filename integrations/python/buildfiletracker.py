#!/usr/bin/env python3
"""
BuildFileTracker Python Integration
Use this module to programmatically enable file tracking in Python-based build systems
"""

import os
import sys
import subprocess
import time
from pathlib import Path
from typing import Optional, Callable
from contextlib import contextmanager

class BuildFileTracker:
    """Context manager for file tracking during builds"""
    
    def __init__(self, 
                 library_path: Optional[str] = None,
                 output_dir: Optional[str] = None,
                 json_output: Optional[str] = None,
                 csv_output: Optional[str] = None):
        """
        Initialize BuildFileTracker
        
        Args:
            library_path: Path to libfiletracker.so (auto-detect if None)
            output_dir: Directory for output reports
            json_output: Path for JSON report
            csv_output: Path for CSV report
        """
        self.library_path = library_path or self._find_library()
        self.output_dir = output_dir or './build_reports'
        self.timestamp = time.strftime('%Y%m%d_%H%M%S')
        
        self.json_output = json_output or os.path.join(
            self.output_dir, f'file_access_{self.timestamp}.json')
        self.csv_output = csv_output or os.path.join(
            self.output_dir, f'file_access_{self.timestamp}.csv')
        
        self.original_ld_preload = None
        
        # Create output directory
        Path(self.output_dir).mkdir(parents=True, exist_ok=True)
    
    def _find_library(self) -> str:
        """Auto-detect library path"""
        possible_paths = [
            './src/libfiletracker.so',
            '../src/libfiletracker.so',
            '/usr/local/lib/libfiletracker.so',
            '/usr/lib/libfiletracker.so',
        ]
        
        for path in possible_paths:
            if os.path.exists(path):
                return os.path.abspath(path)
        
        raise FileNotFoundError(
            "Could not find libfiletracker.so. Please specify library_path.")
    
    def enable(self):
        """Enable file tracking"""
        if not os.path.exists(self.library_path):
            raise FileNotFoundError(f"Library not found: {self.library_path}")
        
        # Save original LD_PRELOAD if it exists
        self.original_ld_preload = os.environ.get('LD_PRELOAD')
        
        # Set environment variables
        os.environ['LD_PRELOAD'] = self.library_path
        os.environ['FILE_TRACKER_JSON'] = self.json_output
        os.environ['FILE_TRACKER_CSV'] = self.csv_output
        
        print(f"BuildFileTracker enabled")
        print(f"  Library: {self.library_path}")
        print(f"  JSON: {self.json_output}")
        print(f"  CSV: {self.csv_output}")
    
    def disable(self):
        """Disable file tracking"""
        # Restore original LD_PRELOAD or remove it
        if self.original_ld_preload:
            os.environ['LD_PRELOAD'] = self.original_ld_preload
        elif 'LD_PRELOAD' in os.environ:
            del os.environ['LD_PRELOAD']
        
        # Clean up other environment variables
        for var in ['FILE_TRACKER_JSON', 'FILE_TRACKER_CSV']:
            if var in os.environ:
                del os.environ[var]
        
        print(f"BuildFileTracker disabled")
    
    def __enter__(self):
        """Context manager entry"""
        self.enable()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.disable()
        return False
    
    def run_command(self, command: list) -> int:
        """
        Run a command with file tracking enabled
        
        Args:
            command: Command and arguments as list
            
        Returns:
            Exit code of the command
        """
        with self:
            result = subprocess.run(command)
            return result.returncode
    
    def track_function(self, func: Callable, *args, **kwargs):
        """
        Track file access during function execution
        
        Args:
            func: Function to execute
            *args: Positional arguments for func
            **kwargs: Keyword arguments for func
            
        Returns:
            Result of func
        """
        with self:
            return func(*args, **kwargs)


@contextmanager
def track_build(output_dir: str = './build_reports', 
                library_path: Optional[str] = None):
    """
    Context manager for tracking builds
    
    Usage:
        with track_build():
            # Your build code here
            subprocess.run(['make', 'all'])
    """
    tracker = BuildFileTracker(library_path=library_path, output_dir=output_dir)
    tracker.enable()
    try:
        yield tracker
    finally:
        tracker.disable()


def main():
    """Command-line interface"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Run a command with build file tracking'
    )
    parser.add_argument('command', nargs='+', help='Command to execute')
    parser.add_argument('-l', '--library', help='Path to libfiletracker.so')
    parser.add_argument('-o', '--output-dir', default='./build_reports',
                       help='Output directory for reports')
    
    args = parser.parse_args()
    
    tracker = BuildFileTracker(
        library_path=args.library,
        output_dir=args.output_dir
    )
    
    exit_code = tracker.run_command(args.command)
    sys.exit(exit_code)


if __name__ == '__main__':
    main()
