#!/usr/bin/env python3
"""
buildfiletracker - Universal build file access monitoring tool

Usage:
    buildfiletracker track [options] -- <command> [args...]
    buildfiletracker report [options] <input> <output>
    buildfiletracker analyze [options] <input>
    buildfiletracker version

Commands:
    track       Track a build command and generate reports
    report      Convert existing tracking data to different format
    analyze     Analyze tracking data and print insights
    version     Show version information

Examples:
    # Track a build with auto-detected packages
    buildfiletracker track --auto-detect --output report.json -- make all

    # Track with explicit packages
    buildfiletracker track \
        --package openssl:/usr/openssl:3.0.0:Apache-2.0 \
        --package zlib:/usr/lib/zlib:1.2.11:Zlib \
        --format json,spdx,html \
        --output build_report \
        -- cmake --build .

    # Generate HTML report from JSON
    buildfiletracker report --format html report.json report.html

    # Analyze existing report
    buildfiletracker analyze --optimize --compliance report.json

For more information: https://github.com/yourusername/buildfiletracker
"""

import sys
import os
import argparse
import json
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent / 'python'))

from buildfiletracker import (
    BuildFileTracker,
    TrackingBackend,
    ReportFormat,
    PackageInfo
)


def main():
    parser = argparse.ArgumentParser(
        description='Universal build file access monitoring tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Command to execute')
    
    # ========================================================================
    # TRACK COMMAND
    # ========================================================================
    track_parser = subparsers.add_parser('track', help='Track a build command')
    
    track_parser.add_argument(
        '--backend',
        choices=['auto', 'preload', 'inotify', 'fanotify', 'strace'],
        default='auto',
        help='Tracking backend to use (default: auto)'
    )
    
    track_parser.add_argument(
        '--package', '-p',
        action='append',
        dest='packages',
        metavar='NAME:PATH[:VERSION[:LICENSE[:HOMEPAGE[:SUPPLIER]]]]',
        help='Register a package (can be repeated)'
    )
    
    track_parser.add_argument(
        '--auto-detect',
        action='store_true',
        help='Auto-detect packages from common directories'
    )
    
    track_parser.add_argument(
        '--watch', '-w',
        action='append',
        dest='watch_paths',
        help='Add path to watch (can be repeated)'
    )
    
    track_parser.add_argument(
        '--extension', '-e',
        action='append',
        dest='extensions',
        help='Filter by file extension (can be repeated)'
    )
    
    track_parser.add_argument(
        '--exclude', '-x',
        action='append',
        dest='excludes',
        help='Exclude pattern (can be repeated)'
    )
    
    track_parser.add_argument(
        '--format', '-f',
        default='json',
        help='Output format(s), comma-separated: json,csv,xml,xlsx,spdx,cyclonedx,md,html'
    )
    
    track_parser.add_argument(
        '--output', '-o',
        default='report',
        help='Output file base name (default: report)'
    )
    
    track_parser.add_argument(
        '--output-dir',
        default='.',
        help='Output directory (default: current directory)'
    )
    
    track_parser.add_argument(
        '--quiet', '-q',
        action='store_true',
        help='Suppress output'
    )
    
    track_parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Verbose output'
    )
    
    track_parser.add_argument(
        'build_command',
        nargs=argparse.REMAINDER,
        help='Build command to track (after --)'
    )
    
    # ========================================================================
    # REPORT COMMAND
    # ========================================================================
    report_parser = subparsers.add_parser('report', help='Convert report format')
    
    report_parser.add_argument(
        'input',
        help='Input JSON file'
    )
    
    report_parser.add_argument(
        'output',
        help='Output file'
    )
    
    report_parser.add_argument(
        '--format', '-f',
        choices=['json', 'csv', 'xml', 'xlsx', 'spdx', 'cyclonedx', 'md', 'html'],
        help='Output format (auto-detected from extension if not specified)'
    )
    
    # ========================================================================
    # ANALYZE COMMAND
    # ========================================================================
    analyze_parser = subparsers.add_parser('analyze', help='Analyze tracking data')
    
    analyze_parser.add_argument(
        'input',
        help='Input JSON file'
    )
    
    analyze_parser.add_argument(
        '--optimize',
        action='store_true',
        help='Show optimization recommendations'
    )
    
    analyze_parser.add_argument(
        '--compliance',
        action='store_true',
        help='Show license compliance issues'
    )
    
    analyze_parser.add_argument(
        '--threshold',
        type=float,
        default=20.0,
        help='Usage threshold for optimization warnings (default: 20%%)'
    )
    
    # ========================================================================
    # VERSION COMMAND
    # ========================================================================
    version_parser = subparsers.add_parser('version', help='Show version')
    
    # Parse arguments
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    # Execute command
    if args.command == 'track':
        return cmd_track(args)
    elif args.command == 'report':
        return cmd_report(args)
    elif args.command == 'analyze':
        return cmd_analyze(args)
    elif args.command == 'version':
        return cmd_version(args)
    
    return 0


def cmd_track(args):
    """Track build command"""
    
    # Remove '--' from build command if present
    if args.build_command and args.build_command[0] == '--':
        args.build_command = args.build_command[1:]
    
    if not args.build_command:
        print("Error: No build command specified", file=sys.stderr)
        print("Usage: buildfiletracker track [options] -- <command> [args...]", file=sys.stderr)
        return 1
    
    if not args.quiet:
        print("🔍 BuildFileTracker - Tracking Build")
        print("=" * 60)
    
    # Map backend name
    backend_map = {
        'auto': TrackingBackend.AUTO,
        'preload': TrackingBackend.PRELOAD,
        'inotify': TrackingBackend.INOTIFY,
        'fanotify': TrackingBackend.FANOTIFY,
        'strace': TrackingBackend.STRACE,
    }
    
    # Create tracker
    tracker = BuildFileTracker(backend=backend_map[args.backend])
    
    if not args.quiet:
        print(f"Backend: {args.backend}")
    
    # Register packages
    if args.packages:
        if not args.quiet:
            print(f"\n📦 Registering {len(args.packages)} packages...")
        
        for pkg_spec in args.packages:
            parts = pkg_spec.split(':')
            if len(parts) < 2:
                print(f"Warning: Invalid package spec: {pkg_spec}", file=sys.stderr)
                continue
            
            name = parts[0]
            root_path = parts[1]
            version = parts[2] if len(parts) > 2 else "unknown"
            license = parts[3] if len(parts) > 3 else "unknown"
            homepage = parts[4] if len(parts) > 4 else None
            supplier = parts[5] if len(parts) > 5 else None
            
            tracker.register_package(
                name=name,
                root_path=root_path,
                version=version,
                license=license,
                homepage=homepage,
                supplier=supplier
            )
            
            if args.verbose:
                print(f"   ✓ {name} ({version}) at {root_path}")
    
    # Auto-detect packages
    if args.auto_detect:
        if not args.quiet:
            print("\n🔍 Auto-detecting packages...")
        
        detected = tracker.auto_detect_packages()
        
        if not args.quiet:
            print(f"   ✓ Detected {detected} packages")
    
    # Configure watch paths
    if args.watch_paths:
        for path in args.watch_paths:
            tracker.add_watch(path, recursive=True)
            if args.verbose:
                print(f"   Watching: {path}")
    
    # Configure filters
    if args.extensions:
        tracker.set_extension_filter(args.extensions)
        if args.verbose:
            print(f"   Extensions: {', '.join(args.extensions)}")
    
    if args.excludes:
        for pattern in args.excludes:
            tracker.add_exclude(pattern)
            if args.verbose:
                print(f"   Excluding: {pattern}")
    
    # Track build
    if not args.quiet:
        print(f"\n🔨 Building: {' '.join(args.build_command)}")
    
    try:
        tracker.start()
        result = tracker.run_command(args.build_command)
        tracker.stop()
        
        if result.returncode != 0:
            print(f"\n⚠️  Build failed with exit code {result.returncode}", file=sys.stderr)
            if result.stderr:
                print(result.stderr, file=sys.stderr)
    except Exception as e:
        print(f"\n❌ Error tracking build: {e}", file=sys.stderr)
        return 1
    
    # Get statistics
    stats = tracker.get_statistics()
    
    if not args.quiet:
        print(f"\n📊 Statistics:")
        print(f"   Packages: {stats.total_packages} total, {stats.packages_used} used")
        print(f"   Files: {stats.unique_files} tracked")
        print(f"   Events: {stats.total_events} total")
        print(f"   Runtime: {stats.runtime_seconds:.2f}s")
    
    # Generate reports
    output_dir = Path(args.output_dir)
    output_dir.mkdir(exist_ok=True, parents=True)
    
    formats = args.format.split(',')
    
    if not args.quiet:
        print(f"\n📝 Generating reports...")
    
    format_map = {
        'json': ReportFormat.JSON,
        'csv': ReportFormat.CSV,
        'xml': ReportFormat.XML,
        'xlsx': ReportFormat.XLSX,
        'spdx': ReportFormat.SPDX_JSON,
        'cyclonedx': ReportFormat.CYCLONEDX_JSON,
        'md': ReportFormat.MARKDOWN,
        'html': ReportFormat.HTML,
    }
    
    for fmt in formats:
        fmt = fmt.strip().lower()
        if fmt not in format_map:
            print(f"Warning: Unknown format '{fmt}'", file=sys.stderr)
            continue
        
        # Determine file extension
        ext_map = {
            'json': '.json',
            'csv': '.csv',
            'xml': '.xml',
            'xlsx': '.xlsx',
            'spdx': '_spdx.json',
            'cyclonedx': '_cyclonedx.json',
            'md': '.md',
            'html': '.html',
        }
        
        output_path = output_dir / f"{args.output}{ext_map[fmt]}"
        
        try:
            tracker.generate_report(str(output_path), format_map[fmt])
            if not args.quiet:
                print(f"   ✓ {output_path}")
        except Exception as e:
            print(f"   ✗ Failed to generate {fmt}: {e}", file=sys.stderr)
    
    if not args.quiet:
        print("\n✅ Complete!")
    
    return 0


def cmd_report(args):
    """Convert report format"""
    
    print(f"📝 Converting report format...")
    
    # TODO: Load existing JSON and regenerate in different format
    print("Not yet implemented", file=sys.stderr)
    return 1


def cmd_analyze(args):
    """Analyze tracking data"""
    
    if not os.path.exists(args.input):
        print(f"Error: File not found: {args.input}", file=sys.stderr)
        return 1
    
    print("📊 Analyzing build tracking data")
    print("=" * 60)
    
    try:
        with open(args.input, 'r') as f:
            data = json.load(f)
    except Exception as e:
        print(f"Error loading file: {e}", file=sys.stderr)
        return 1
    
    # Display statistics
    stats = data.get('statistics', {})
    print("\n📈 Statistics:")
    for key, value in stats.items():
        print(f"   {key.replace('_', ' ').title()}: {value}")
    
    # Package analysis
    packages = data.get('packages', [])
    print(f"\n📦 Package Analysis ({len(packages)} packages):")
    
    # Sort by usage
    packages.sort(key=lambda p: p.get('usage_percentage', 0), reverse=True)
    
    for pkg in packages:
        name = pkg.get('name', 'unknown')
        usage = pkg.get('usage_percentage', 0)
        used = pkg.get('used_files_count', 0)
        total = pkg.get('total_files', 0)
        license = pkg.get('license', 'unknown')
        
        symbol = "✅" if usage > 50 else "⚠️" if usage > 20 else "❌"
        print(f"   {symbol} {name:30} {usage:6.2f}% ({used}/{total}) [{license}]")
    
    # Optimization recommendations
    if args.optimize:
        print(f"\n🔧 Optimization Recommendations:")
        
        low_usage = [p for p in packages 
                    if p.get('used_files_count', 0) > 0 
                    and p.get('usage_percentage', 0) < args.threshold]
        
        if low_usage:
            print(f"\n   Found {len(low_usage)} dependencies with < {args.threshold}% usage:\n")
            for pkg in low_usage:
                print(f"   ⚠️  {pkg['name']}: Only {pkg['usage_percentage']:.2f}% used")
                print(f"      Using {pkg['used_files_count']} of {pkg['total_files']} files")
                print(f"      Recommendation: Consider lighter alternative or extract needed files\n")
        else:
            print("   ✅ All dependencies have good usage rates")
    
    # License compliance
    if args.compliance:
        print(f"\n⚖️  License Compliance:")
        
        gpl_packages = [p for p in packages 
                       if 'GPL' in p.get('license', '').upper()
                       and p.get('used_files_count', 0) > 0]
        
        if gpl_packages:
            print(f"\n   ⚠️  Found {len(gpl_packages)} GPL-licensed dependencies in use:\n")
            for pkg in gpl_packages:
                print(f"   • {pkg['name']} ({pkg['license']})")
                print(f"     Used files: {pkg['used_files_count']}")
                if pkg.get('used_files'):
                    for f in list(pkg['used_files'])[:3]:
                        print(f"        - {Path(f).name}")
                    if len(pkg['used_files']) > 3:
                        print(f"        ... and {len(pkg['used_files']) - 3} more")
                print()
        else:
            print("   ✅ No GPL dependencies detected")
    
    print("=" * 60)
    return 0


def cmd_version(args):
    """Show version"""
    print("BuildFileTracker v2.0.0")
    print("Universal build file access monitoring library")
    print()
    print("Backends: LD_PRELOAD, inotify, fanotify, strace")
    print("Formats: JSON, CSV, XML, XLSX, SPDX, CycloneDX, Markdown, HTML")
    print()
    print("https://github.com/yourusername/buildfiletracker")
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n⚠️  Interrupted by user", file=sys.stderr)
        sys.exit(130)
    except Exception as e:
        print(f"\n❌ Fatal error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)
