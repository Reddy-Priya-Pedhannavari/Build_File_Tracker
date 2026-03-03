#!/usr/bin/env python3
"""
BuildFileTracker - Complete Python API
Universal build file tracking with multiple backends and comprehensive reporting

Copyright (c) 2026 BuildFileTracker Contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""

import os
import sys
import json
import csv
import subprocess
import threading
import tempfile
import hashlib
from pathlib import Path
from typing import List, Dict, Optional, Set, Callable, Any
from dataclasses import dataclass, field, asdict
from enum import Enum
from datetime import datetime
import fnmatch
import xml.etree.ElementTree as ET

try:
    from openpyxl import Workbook
    from openpyxl.styles import Font, PatternFill, Alignment
    HAS_XLSX = True
except ImportError:
    HAS_XLSX = False

try:
    from buildfiletracker.filetypes import (
        FileType, FileCategory, FileInfo, FileTypeDetector,
        EXTENSIONS_SOURCE, EXTENSIONS_BINARY, EXTENSIONS_LIBRARY, EXTENSIONS_ALL
    )
    HAS_FILETYPES = True
except ImportError:
    HAS_FILETYPES = False
    FileType = None
    FileCategory = None
    FileInfo = None
    FileTypeDetector = None

__version__ = "2.0.0"

# ============================================================================
# ENUMERATIONS
# ============================================================================

class TrackingBackend(Enum):
    """Available tracking backends"""
    AUTO = "auto"
    PRELOAD = "preload"         # LD_PRELOAD (most compatible)
    INOTIFY = "inotify"         # Linux inotify (real-time)
    FANOTIFY = "fanotify"       # Linux fanotify (requires root)
    STRACE = "strace"           # strace wrapper (fallback)
    HYBRID = "hybrid"           # Multiple backends


class ReportFormat(Enum):
    """Report output formats"""
    JSON = "json"
    CSV = "csv"
    XML = "xml"
    XLSX = "xlsx"
    SPDX_JSON = "spdx_json"
    SPDX_TAGVALUE = "spdx_tagvalue"
    CYCLONEDX_JSON = "cyclonedx_json"
    CYCLONEDX_XML = "cyclonedx_xml"
    MARKDOWN = "markdown"
    HTML = "html"


class AccessType(Enum):
    """File access types"""
    READ = "read"
    WRITE = "write"
    EXECUTE = "execute"
    DELETE = "delete"
    METADATA = "metadata"


# ============================================================================
# DATA CLASSES
# ============================================================================

@dataclass
class FileEvent:
    """Represents a single file access event"""
    filepath: str
    timestamp: datetime
    access_type: str
    pid: int
    process_name: str = ""
    package_name: Optional[str] = None
    uid: int = 0
    
    def to_dict(self) -> Dict:
        return {
            "filepath": self.filepath,
            "timestamp": self.timestamp.isoformat(),
            "access_type": self.access_type,
            "pid": self.pid,
            "process_name": self.process_name,
            "package_name": self.package_name,
            "uid": self.uid
        }


@dataclass
class PackageInfo:
    """Package metadata and usage information"""
    name: str
    version: str = "unknown"
    license: str = "unknown"
    homepage: str = ""
    supplier: str = ""
    root_path: str = ""
    total_files: int = 0
    used_files: Set[str] = field(default_factory=set)
    checksum: str = ""
    
    @property
    def used_files_count(self) -> int:
        return len(self.used_files)
    
    @property
    def usage_percentage(self) -> float:
        if self.total_files == 0:
            return 0.0
        return (len(self.used_files) / self.total_files) * 100
    
    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "version": self.version,
            "license": self.license,
            "homepage": self.homepage,
            "supplier": self.supplier,
            "root_path": self.root_path,
            "total_files": self.total_files,
            "used_files_count": self.used_files_count,
            "usage_percentage": round(self.usage_percentage, 2),
            "used_files": sorted(list(self.used_files)),
            "checksum": self.checksum
        }


@dataclass
class TrackingStatistics:
    """Tracking statistics"""
    total_events: int = 0
    filtered_events: int = 0
    unique_files: int = 0
    total_packages: int = 0
    packages_used: int = 0
    start_time: Optional[datetime] = None
    end_time: Optional[datetime] = None
    runtime_seconds: float = 0.0
    
    def to_dict(self) -> Dict:
        return {
            "total_events": self.total_events,
            "filtered_events": self.filtered_events,
            "unique_files": self.unique_files,
            "total_packages": self.total_packages,
            "packages_used": self.packages_used,
            "start_time": self.start_time.isoformat() if self.start_time else None,
            "end_time": self.end_time.isoformat() if self.end_time else None,
            "runtime_seconds": round(self.runtime_seconds, 3)
        }


# ============================================================================
# MAIN BUILDFILETRACKER CLASS
# ============================================================================

class BuildFileTracker:
    """
    Universal build file tracking system
    
    Examples:
        >>> tracker = BuildFileTracker()
        >>> tracker.register_package("openssl", "/path/to/openssl", version="3.0.0")
        >>> tracker.add_watch("/path/to/openssl")
        >>> tracker.start()
        >>> result = tracker.run_command(["make", "all"])
        >>> tracker.stop()
        >>> tracker.generate_report("report.json", ReportFormat.JSON)
    """
    
    def __init__(self,
                 backend: TrackingBackend = TrackingBackend.AUTO,
                 watch_paths: Optional[List[str]] = None,
                 exclude_patterns: Optional[List[str]] = None,
                 auto_detect_packages: bool = True):
        """
        Initialize BuildFileTracker
        
        Args:
            backend: Tracking backend to use
            watch_paths: Paths to monitor
            exclude_patterns: Glob patterns to exclude
            auto_detect_packages: Automatically detect packages
        """
        self.backend = backend
        self.active_backend = None
        self.watch_paths = [Path(p).resolve() for p in (watch_paths or [])]
        self.exclude_patterns = exclude_patterns or [
            "*.pyc", "__pycache__/*", ".git/*", ".svn/*",
            "*.swp", "*.tmp", "/tmp/*", "/proc/*", "/sys/*", "/dev/*"
        ]
        self.auto_detect = auto_detect_packages
        
        # Data structures
        self.packages: Dict[str, PackageInfo] = {}
        self.file_events: List[FileEvent] = []
        self.file_access_map: Dict[str, Set[str]] = {}  # filepath -> access types
        self.extension_filters: Optional[Set[str]] = None
        
        # State
        self.running = False
        self.stats = TrackingStatistics()
        self._lock = threading.Lock()
        
        # Callbacks
        self.event_callback: Optional[Callable[[FileEvent], None]] = None
        self.package_callback: Optional[Callable[[PackageInfo], None]] = None
        
        # Backend-specific
        self._preload_lib_path: Optional[str] = None
        self._temp_files: List[str] = []
        
        # Find preload library
        self._find_preload_library()
    
    def _find_preload_library(self):
        """Find the LD_PRELOAD library"""
        possible_paths = [
            Path(__file__).parent.parent.parent / "src" / "libfiletracker.so",
            Path(__file__).parent.parent / "lib" / "libfiletracker.so",
            Path("/usr/local/lib/libfiletracker.so"),
            Path("/usr/lib/libfiletracker.so"),
        ]
        
        for path in possible_paths:
            if path.exists():
                self._preload_lib_path = str(path)
                break
    
    def _select_backend(self) -> TrackingBackend:
        """Select best available backend"""
        if self.backend != TrackingBackend.AUTO:
            return self.backend
        
        # Check for Linux inotify
        if sys.platform.startswith('linux'):
            return TrackingBackend.INOTIFY
        
        # Check for preload library
        if self._preload_lib_path:
            return TrackingBackend.PRELOAD
        
        # Fall back to strace
        if self._check_strace_available():
            return TrackingBackend.STRACE
        
        raise RuntimeError("No suitable tracking backend available")
    
    def _check_strace_available(self) -> bool:
        """Check if strace is available"""
        try:
            subprocess.run(["strace", "--version"], 
                          capture_output=True, check=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
    
    # ========================================================================
    # PACKAGE MANAGEMENT
    # ========================================================================
    
    def register_package(self,
                        name: str,
                        root_path: str,
                        version: str = "unknown",
                        license: str = "unknown",
                        homepage: str = "",
                        supplier: str = "") -> None:
        """
        Register a package for tracking
        
        Args:
            name: Package name
            root_path: Root directory of package
            version: Package version
            license: SPDX license identifier
            homepage: Package homepage URL
            supplier: Package supplier/vendor
        """
        root = Path(root_path).resolve()
        
        if not root.exists():
            raise ValueError(f"Package root path does not exist: {root_path}")
        
        # Count total files
        total_files = 0
        if root.is_dir():
            total_files = sum(1 for _ in root.rglob("*") if _.is_file())
        
        # Calculate checksum
        checksum = self._calculate_package_checksum(root)
        
        pkg = PackageInfo(
            name=name,
            version=version,
            license=license,
            homepage=homepage,
            supplier=supplier,
            root_path=str(root),
            total_files=total_files,
            checksum=checksum
        )
        
        with self._lock:
            self.packages[name] = pkg
            self.stats.total_packages = len(self.packages)
    
    def _calculate_package_checksum(self, root_path: Path) -> str:
        """Calculate SHA256 checksum of package"""
        if not root_path.is_dir():
            return ""
        
        hasher = hashlib.sha256()
        try:
            for file in sorted(root_path.rglob("*")):
                if file.is_file():
                    hasher.update(str(file.relative_to(root_path)).encode())
        except Exception:
            pass
        return hasher.hexdigest()[:16]
    
    def auto_detect_packages(self, search_root: str) -> int:
        """
        Auto-detect packages in directory
        
        Args:
            search_root: Root directory to search
            
        Returns:
            Number of packages detected
        """
        root = Path(search_root).resolve()
        detected = 0
        
        # Look for common package indicators
        patterns = {
            "package.json": ("node_modules", "npm", None),
            "Cargo.toml": ("target", "cargo", None),
            "setup.py": ("", "pip", None),
            "CMakeLists.txt": ("", "cmake", None),
            "Makefile": ("", "make", None),
        }
        
        for pattern, (subdir, pkg_type, _) in patterns.items():
            for file in root.rglob(pattern):
                pkg_root = file.parent / subdir if subdir else file.parent
                if pkg_root.exists() and pkg_root.is_dir():
                    pkg_name = f"{pkg_root.name}-{pkg_type}"
                    if pkg_name not in self.packages:
                        self.register_package(pkg_name, str(pkg_root))
                        detected += 1
        
        return detected
    
    def get_package(self, name: str) -> Optional[PackageInfo]:
        """Get package information"""
        return self.packages.get(name)
    
    def get_all_packages(self) -> List[PackageInfo]:
        """Get all registered packages"""
        return list(self.packages.values())
    
    # ========================================================================
   # WATCH MANAGEMENT
    # ========================================================================
    
    def add_watch(self, path: str, recursive: bool = True) -> None:
        """Add a path to watch"""
        resolved = Path(path).resolve()
        if resolved not in self.watch_paths:
            self.watch_paths.append(resolved)
    
    def add_exclude(self, pattern: str) -> None:
        """Add exclude pattern"""
        if pattern not in self.exclude_patterns:
            self.exclude_patterns.append(pattern)
    
    def set_extension_filter(self, extensions: List[str]) -> None:
        """Filter by file extensions"""
        self.extension_filters = set(ext if ext.startswith('.') else f'.{ext}' 
                                     for ext in extensions)
    
    def _should_track(self, filepath: str) -> bool:
        """Check if file should be tracked"""
        path = Path(filepath)
        
        # Check exclude patterns
        for pattern in self.exclude_patterns:
            if fnmatch.fnmatch(str(path), pattern):
                with self._lock:
                    self.stats.filtered_events += 1
                return False
        
        # Check extension filter
        if self.extension_filters and path.suffix not in self.extension_filters:
            with self._lock:
                self.stats.filtered_events += 1
            return False
        
        # Check if in watch paths
        if self.watch_paths:
            for watch_path in self.watch_paths:
                try:
                    path.relative_to(watch_path)
                    return True
                except ValueError:
                    continue
            return False
        
        return True
    
    # ========================================================================
    # TRACKING CONTROL
    # ========================================================================
    
    def start(self) -> None:
        """Start tracking file accesses"""
        if self.running:
            raise RuntimeError("Tracking already running")
        
        self.active_backend = self._select_backend()
        self.stats.start_time = datetime.now()
        self.running = True
        
        print(f"📡 BuildFileTracker started (backend: {self.active_backend.value})")
    
    def stop(self) -> None:
        """Stop tracking file accesses"""
        if not self.running:
            return
        
        self.running = False
        self.stats.end_time = datetime.now()
        
        if self.stats.start_time:
            self.stats.runtime_seconds = (
                self.stats.end_time - self.stats.start_time
            ).total_seconds()
        
        # Update final statistics
        with self._lock:
            self.stats.unique_files = len(self.file_access_map)
            self.stats.packages_used = sum(
                1 for pkg in self.packages.values() if pkg.used_files
            )
        
        print(f"🛑 BuildFileTracker stopped ({self.stats.runtime_seconds:.2f}s)")
    
    # ========================================================================
    # FILE ACCESS RECORDING
    # ========================================================================
    
    def record_access(self,
                     filepath: str,
                     access_type: str = "read",
                     pid: int = 0,
                     process_name: str = "") -> None:
        """Record a file access event"""
        if not self._should_track(filepath):
            return
        
        try:
            resolved = str(Path(filepath).resolve())
        except Exception:
            resolved = filepath
        
        # Find package
        package_name = self._find_package_for_file(resolved)
        
        # Create event
        event = FileEvent(
            filepath=resolved,
            timestamp=datetime.now(),
            access_type=access_type,
            pid=pid or os.getpid(),
            process_name=process_name,
            package_name=package_name,
            uid=os.getuid() if hasattr(os, 'getuid') else 0
        )
        
        with self._lock:
            self.file_events.append(event)
            self.stats.total_events += 1
            
            # Update access map
            if resolved not in self.file_access_map:
                self.file_access_map[resolved] = set()
            self.file_access_map[resolved].add(access_type)
            
            # Update package used files
            if package_name and package_name in self.packages:
                self.packages[package_name].used_files.add(resolved)
        
        # Invoke callback
        if self.event_callback:
            try:
                self.event_callback(event)
            except Exception as e:
                print(f"Warning: Event callback failed: {e}")
    
    def _find_package_for_file(self, filepath: str) -> Optional[str]:
        """Find which package a file belongs to"""
        # Find longest matching package root
        best_match = None
        best_len = 0
        
        for name, pkg in self.packages.items():
            if filepath.startswith(pkg.root_path):
                if len(pkg.root_path) > best_len:
                    best_match = name
                    best_len = len(pkg.root_path)
        
        return best_match
    
    # Continue in next section...

    # ========================================================================
    # BUILD COMMAND EXECUTION
    # ========================================================================
    
    def run_command(self, 
                   command: List[str],
                   cwd: Optional[str] = None,
                   env: Optional[Dict[str, str]] = None) -> subprocess.CompletedProcess:
        """
        Run a build command with tracking
        
        Args:
            command: Command and arguments
            cwd: Working directory
            env: Environment variables
            
        Returns:
            CompletedProcess result
        """
        if not self.running:
            self.start()
            auto_started = True
        else:
            auto_started = False
        
        try:
            if self.active_backend == TrackingBackend.PRELOAD:
                result = self._run_with_preload(command, cwd, env)
            elif self.active_backend == TrackingBackend.STRACE:
                result = self._run_with_strace(command, cwd, env)
            else:
                # For inotify/fanotify, just run command normally
                result = subprocess.run(command, cwd=cwd, env=env,
                                       capture_output=True, text=True)
        finally:
            if auto_started:
                self.stop()
        
        return result
    
    def _run_with_preload(self,
                         command: List[str],
                         cwd: Optional[str],
                         env: Optional[Dict[str, str]]) -> subprocess.CompletedProcess:
        """Run command with LD_PRELOAD tracking"""
        if not self._preload_lib_path:
            raise RuntimeError("LD_PRELOAD library not found")
        
        # Create temp file for trace output
        trace_file = tempfile.NamedTemporaryFile(
            mode='w', suffix='.jsonl', delete=False
        )
        trace_file.close()
        self._temp_files.append(trace_file.name)
        
        # Setup environment
        run_env = os.environ.copy()
        if env:
            run_env.update(env)
        
        run_env['LD_PRELOAD'] = self._preload_lib_path
        run_env['FILE_TRACKER_JSON'] = trace_file.name
        
        if self.watch_paths:
            run_env['FILETRACKER_WATCH_PATHS'] = ':'.join(
                str(p) for p in self.watch_paths
            )
        
        # Run command
        result = subprocess.run(command, cwd=cwd, env=run_env,
                               capture_output=True, text=True)
        
        # Parse trace output
        self._parse_preload_trace(trace_file.name)
        
        return result
    
    def _run_with_strace(self,
                        command: List[str],
                        cwd: Optional[str],
                        env: Optional[Dict[str, str]]) -> subprocess.CompletedProcess:
        """Run command with strace tracking"""
        import re
        
        # Create temp file for strace output
        trace_file = tempfile.NamedTemporaryFile(
            mode='w', suffix='.strace', delete=False
        )
        trace_file.close()
        self._temp_files.append(trace_file.name)
        
        # Build strace command
        strace_cmd = [
            'strace', '-f', '-e', 'trace=open,openat,stat,access',
            '-o', trace_file.name
        ] + command
        
        # Run command
        result = subprocess.run(strace_cmd, cwd=cwd, env=env,
                               capture_output=True, text=True)
        
        # Parse strace output
        open_pattern = re.compile(
            r'open(?:at)?\([^"]*"([^"]+)".*\)\s*=\s*(\d+|-1)'
        )
        
        with open(trace_file.name, 'r') as f:
            for line in f:
                match = open_pattern.search(line)
                if match:
                    filepath = match.group(1)
                    if match.group(2) != '-1':  # Successful open
                        self.record_access(filepath)
        
        return result
    
    def _parse_preload_trace(self, trace_path: str) -> None:
        """Parse JSONL trace file from preload library"""
        try:
            with open(trace_path, 'r') as f:
                for line in f:
                    try:
                        event = json.loads(line.strip())
                        self.record_access(
                            filepath=event.get('path', ''),
                            access_type=event.get('access', 'read'),
                            pid=event.get('pid', 0)
                        )
                    except json.JSONDecodeError:
                        continue
        except FileNotFoundError:
            pass
    
    # ========================================================================
    # REPORTING
    # ========================================================================
    
    def generate_report(self, output_path: str, format: ReportFormat = ReportFormat.JSON) -> None:
        """Generate report in specified format
        
        Args:
            output_path: Path to output file
            format: Report format (JSON, CSV, XML, XLSX, SPDX, CycloneDX, Markdown, HTML)
        """
        from buildfiletracker.reporters import ReportGenerator
        
        generator = ReportGenerator(self)
        generator.generate(output_path, format)
    
    def generate_all_reports(self, output_dir: str, base_name: str = "report") -> None:
        """Generate reports in all supported formats
        
        Args:
            output_dir: Directory for output files
            base_name: Base filename (default: "report")
        """
        import os
        os.makedirs(output_dir, exist_ok=True)
        
        formats = {
            ReportFormat.JSON: f"{base_name}.json",
            ReportFormat.CSV: f"{base_name}.csv",
            ReportFormat.XML: f"{base_name}.xml",
            ReportFormat.XLSX: f"{base_name}.xlsx",
            ReportFormat.SPDX_JSON: f"{base_name}_spdx.json",
            ReportFormat.SPDX_TAGVALUE: f"{base_name}_spdx.spdx",
            ReportFormat.CYCLONEDX_JSON: f"{base_name}_cyclonedx.json",
            ReportFormat.CYCLONEDX_XML: f"{base_name}_cyclonedx.xml",
            ReportFormat.MARKDOWN: f"{base_name}.md",
            ReportFormat.HTML: f"{base_name}.html",
        }
        
        for format, filename in formats.items():
            try:
                output_path = os.path.join(output_dir, filename)
                self.generate_report(output_path, format)
            except Exception as e:
                print(f"⚠️  Failed to generate {filename}: {e}")
    
    def print_summary(self) -> None:
        """Print a quick summary to console"""
        print("\n" + "="*60)
        print("BuildFileTracker Summary")
        print("="*60)
        print(f"Total Packages: {self.stats.total_packages}")
        print(f"Packages Used: {self.stats.packages_used}")
        print(f"Files Tracked: {self.stats.unique_files}")
        print(f"Total Events: {self.stats.total_events}")
        print(f"Duration: {self.stats.runtime_seconds:.2f}s")
        print("\nTop Packages by Usage:")
        
        for pkg in sorted(self.get_all_packages(), 
                         key=lambda p: p.usage_percentage, reverse=True)[:10]:
            print(f"  {pkg.name:30} {pkg.usage_percentage:6.2f}% ({pkg.used_files_count}/{pkg.total_files})")
        
        print("="*60 + "\n")
