#!/usr/bin/env python3
"""
BuildFileTracker Analysis Tool
Provides advanced analysis of file access patterns
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Set
from collections import defaultdict

class FileAccessAnalyzer:
    """Analyze file access patterns from build tracking data"""
    
    def __init__(self, input_json: str):
        """Initialize with input JSON file"""
        self.input_file = input_json
        self.data = self._load_data()
    
    def _load_data(self) -> Dict:
        """Load data from JSON file"""
        with open(self.input_file, 'r') as f:
            return json.load(f)
    
    def get_package_dependencies(self) -> Dict[str, Set[str]]:
        """Identify dependencies between packages"""
        dependencies = defaultdict(set)
        files_by_package = defaultdict(list)
        
        # Group files by package
        for file_entry in self.data.get('accessed_files', []):
            pkg = file_entry.get('package', 'unknown')
            files_by_package[pkg].append(file_entry)
        
        return dict(files_by_package)
    
    def get_unused_file_types(self, package_path: str) -> Set[str]:
        """Identify file types in package directory that were never accessed"""
        all_files = set()
        accessed_files = set()
        
        # Scan package directory
        pkg_path = Path(package_path)
        if pkg_path.exists():
            all_files = {str(f) for f in pkg_path.rglob('*') if f.is_file()}
        
        # Get accessed files
        for file_entry in self.data.get('accessed_files', []):
            accessed_files.add(file_entry.get('filepath', ''))
        
        return all_files - accessed_files
    
    def get_most_accessed_files(self, top_n: int = 10) -> List[Dict]:
        """Get the most frequently accessed files"""
        files = self.data.get('accessed_files', [])
        sorted_files = sorted(files, key=lambda x: x.get('access_count', 0), reverse=True)
        return sorted_files[:top_n]
    
    def get_package_usage_stats(self) -> Dict[str, Dict]:
        """Calculate usage statistics per package"""
        stats = defaultdict(lambda: {
            'file_count': 0,
            'total_accesses': 0,
            'file_types': defaultdict(int)
        })
        
        for file_entry in self.data.get('accessed_files', []):
            pkg = file_entry.get('package', 'unknown')
            ftype = file_entry.get('file_type', 'unknown')
            count = file_entry.get('access_count', 0)
            
            stats[pkg]['file_count'] += 1
            stats[pkg]['total_accesses'] += count
            stats[pkg]['file_types'][ftype] += 1
        
        return dict(stats)
    
    def filter_by_package(self, package_name: str) -> List[Dict]:
        """Filter files by package name"""
        return [f for f in self.data.get('accessed_files', []) 
                if f.get('package') == package_name]
    
    def filter_by_extension(self, extension: str) -> List[Dict]:
        """Filter files by file extension"""
        if not extension.startswith('.'):
            extension = '.' + extension
        return [f for f in self.data.get('accessed_files', []) 
                if f.get('filepath', '').endswith(extension)]
    
    def generate_dependency_graph(self, output_file: str):
        """Generate a simple text-based dependency graph"""
        package_deps = self.get_package_dependencies()
        
        with open(output_file, 'w') as f:
            f.write("Package File Usage Graph\n")
            f.write("=" * 60 + "\n\n")
            
            for pkg, files in sorted(package_deps.items()):
                f.write(f"{pkg} ({len(files)} files)\n")
                
                # Group by file type
                files_by_type = defaultdict(list)
                for file_entry in files:
                    ftype = file_entry.get('file_type', 'unknown')
                    files_by_type[ftype].append(file_entry)
                
                for ftype, flist in sorted(files_by_type.items()):
                    f.write(f"  ├─ {ftype}: {len(flist)} files\n")
                
                f.write("\n")
        
        print(f"Dependency graph generated: {output_file}")


def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Analyze build file tracking data'
    )
    parser.add_argument('input', help='Input JSON file from tracker')
    parser.add_argument('-t', '--top', type=int, default=10,
                       help='Show top N most accessed files (default: 10)')
    parser.add_argument('-p', '--package', help='Filter by package name')
    parser.add_argument('-e', '--extension', help='Filter by file extension')
    parser.add_argument('-g', '--graph', help='Generate dependency graph to file')
    parser.add_argument('-s', '--stats', action='store_true',
                       help='Show package usage statistics')
    
    args = parser.parse_args()
    
    analyzer = FileAccessAnalyzer(args.input)
    
    if args.graph:
        analyzer.generate_dependency_graph(args.graph)
    
    if args.stats:
        stats = analyzer.get_package_usage_stats()
        print("\nPackage Usage Statistics:")
        print("=" * 80)
        for pkg, data in sorted(stats.items()):
            print(f"\n{pkg}:")
            print(f"  Files: {data['file_count']}")
            print(f"  Total Accesses: {data['total_accesses']}")
            print(f"  File Types: {dict(data['file_types'])}")
    
    if args.package:
        files = analyzer.filter_by_package(args.package)
        print(f"\nFiles from package '{args.package}':")
        print("=" * 80)
        for f in files:
            print(f"  {f['filepath']} (accessed {f['access_count']} times)")
    
    if args.extension:
        files = analyzer.filter_by_extension(args.extension)
        print(f"\nFiles with extension '{args.extension}':")
        print("=" * 80)
        for f in files:
            print(f"  {f['filepath']} (accessed {f['access_count']} times)")
    
    if not (args.graph or args.stats or args.package or args.extension):
        # Show top accessed files by default
        top_files = analyzer.get_most_accessed_files(args.top)
        print(f"\nTop {args.top} Most Accessed Files:")
        print("=" * 80)
        for i, f in enumerate(top_files, 1):
            print(f"{i:2d}. {f['filepath']}")
            print(f"    Package: {f['package']}, Type: {f['file_type']}, "
                  f"Accesses: {f['access_count']}")


if __name__ == '__main__':
    main()
