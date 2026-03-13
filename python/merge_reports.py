#!/usr/bin/env python3
"""
merge_reports.py - Process the JSONL report produced by libfiletracker.so

The tracker now writes all entries to a SINGLE shared file in JSONL format
(one JSON object per line). This script reads it, deduplicates by filepath,
sums access counts, and writes a clean JSON report.

Usage:
    python merge_reports.py <jsonl_path> [output.json] [options]

    <jsonl_path>      Path passed as FILE_TRACKER_JSON to the build
    output.json       Optional output path (default: <jsonl_path>.report.json)

Options:
    --filter-dir DIR  Only keep entries whose filepath starts with DIR
    --ext EXT[,EXT]   Comma-separated extensions to keep (without dot)
                      Example: --ext c,h,cpp,hpp,py,mk,cmake,bb,bbclass
    --abs-only        Drop entries whose filepath is not absolute
"""

import sys
import os
import json
import argparse


def _keep(fp: str, filter_dir, ext_set, abs_only: bool) -> bool:
    if not fp:
        return False
    if abs_only and not fp.startswith("/"):
        return False
    if filter_dir and not fp.startswith(filter_dir):
        return False
    if ext_set:
        _, dot_ext = os.path.splitext(fp)
        if dot_ext.lstrip(".").lower() not in ext_set:
            return False
    return True


def process(jsonl_path: str,
            filter_dir=None,
            ext_set=None,
            abs_only: bool = False) -> dict:
    merged: dict[str, dict] = {}
    total_lines = 0
    skipped = 0

    with open(jsonl_path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            total_lines += 1
            try:
                entry = json.loads(line)
            except json.JSONDecodeError as e:
                print(f"Warning: bad JSON line ({e}): {line[:80]}", file=sys.stderr)
                continue

            fp = entry.get("filepath", "")
            if not _keep(fp, filter_dir, ext_set, abs_only):
                skipped += 1
                continue

            if fp in merged:
                merged[fp]["access_count"] += entry.get("access_count", 1)
            else:
                merged[fp] = dict(entry)

    return {
        "report_type": "build_file_tracker",
        "total_raw_entries": total_lines,
        "skipped_by_filter": skipped,
        "total_unique_files": len(merged),
        "accessed_files": sorted(merged.values(), key=lambda e: e["filepath"])
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Process BuildFileTracker JSONL report into clean JSON",
    )
    parser.add_argument("jsonl_path", help="JSONL file (FILE_TRACKER_JSON value)")
    parser.add_argument("output", nargs="?", help="Output JSON file")
    parser.add_argument("--filter-dir", metavar="DIR",
                        help="Only keep files whose path starts with DIR")
    parser.add_argument("--ext", metavar="EXT,...",
                        help="Comma-separated extensions to keep (e.g. c,h,cpp,py)")
    parser.add_argument("--abs-only", action="store_true",
                        help="Drop entries with non-absolute paths")
    args = parser.parse_args()

    ext_set = None
    if args.ext:
        ext_set = {e.strip().lower().lstrip(".") for e in args.ext.split(",")}

    result = process(
        args.jsonl_path,
        filter_dir=args.filter_dir,
        ext_set=ext_set,
        abs_only=args.abs_only,
    )

    output = args.output or (args.jsonl_path + ".report.json")
    with open(output, "w") as f:
        json.dump(result, f, indent=2)

    print(f"Report written       : {output}")
    print(f"Unique files tracked : {result['total_unique_files']}")
    print(f"Total raw entries    : {result['total_raw_entries']}")
    if result["skipped_by_filter"]:
        print(f"Filtered out         : {result['skipped_by_filter']} entries")

Usage:
    python merge_reports.py <base_path> [output.json] [options]

    <base_path>       Base path used with FILE_TRACKER_JSON (e.g. /tmp/reports.json)
    output.json       Optional output path (default: <base_path>.merged.json)

Options:
    --filter-dir DIR  Only keep entries whose filepath starts with DIR.
                      Useful to restrict results to the build tree.
                      Example: --filter-dir /foss2/uie21749/__Priya__

    --ext EXT[,EXT]   Comma-separated list of extensions to keep (without leading dot).
                      Example: --ext c,h,cpp,hpp,py,mk,cmake,bb,bbclass,inc,conf

    --abs-only        Skip any entry whose filepath is not an absolute path
                      (i.e. does not start with '/').  This is usually already
                      enforced by the C library since v2, but can be used as a
                      safety net when processing older reports.

All files matching <base_path>.<PID> (or exactly <base_path> itself) are merged,
de-duplicating by filepath and summing access counts.
"""

import sys
import os
import glob
import json
import argparse


def _keep(fp: str, filter_dir: str | None, ext_set: set | None, abs_only: bool) -> bool:
    if not fp:
        return False
    if abs_only and not fp.startswith("/"):
        return False
    if filter_dir and not fp.startswith(filter_dir):
        return False
    if ext_set:
        _, dot_ext = os.path.splitext(fp)
        if dot_ext.lstrip(".").lower() not in ext_set:
            return False
    return True


def merge(base_path: str,
          filter_dir: str | None = None,
          ext_set: set | None = None,
          abs_only: bool = False) -> dict:
    # Accept both the base path itself (single file) and per-PID variants
    candidates = glob.glob(base_path + ".*") + ([base_path] if os.path.isfile(base_path) else [])
    candidates = sorted(set(candidates))

    if not candidates:
        print(f"No report files found matching: {base_path}.*", file=sys.stderr)
        sys.exit(1)

    merged: dict[str, dict] = {}
    loaded = 0
    skipped_filter = 0

    for fpath in candidates:
        try:
            with open(fpath) as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError) as e:
            print(f"Warning: skipping {fpath}: {e}", file=sys.stderr)
            continue
        loaded += 1

        for entry in data.get("accessed_files", []):
            fp = entry.get("filepath", "")
            if not _keep(fp, filter_dir, ext_set, abs_only):
                skipped_filter += 1
                continue
            if fp in merged:
                merged[fp]["access_count"] += entry.get("access_count", 1)
            else:
                merged[fp] = dict(entry)

    return {
        "report_type": "build_file_tracker",
        "merged_from_files": loaded,
        "skipped_by_filter": skipped_filter,
        "total_unique_files": len(merged),
        "accessed_files": sorted(merged.values(), key=lambda e: e["filepath"])
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Merge per-PID BuildFileTracker JSON reports",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__.split("All files")[1]
    )
    parser.add_argument("base_path", help="Base path (FILE_TRACKER_JSON value)")
    parser.add_argument("output", nargs="?", help="Output file (default: <base_path>.merged.json)")
    parser.add_argument("--filter-dir", metavar="DIR",
                        help="Only keep files whose path starts with DIR")
    parser.add_argument("--ext", metavar="EXT,...",
                        help="Comma-separated list of extensions to keep (e.g. c,h,cpp,py)")
    parser.add_argument("--abs-only", action="store_true",
                        help="Drop entries with non-absolute paths")
    args = parser.parse_args()

    ext_set = None
    if args.ext:
        ext_set = {e.strip().lower().lstrip(".") for e in args.ext.split(",")}

    result = merge(
        args.base_path,
        filter_dir=args.filter_dir,
        ext_set=ext_set,
        abs_only=args.abs_only,
    )

    output = args.output or (args.base_path + ".merged.json")
    with open(output, "w") as f:
        json.dump(result, f, indent=2)

    print(f"Merged {result['merged_from_files']} report file(s)  ->  {output}")
    print(f"Unique files tracked : {result['total_unique_files']}")
    if result["skipped_by_filter"]:
        print(f"Filtered out         : {result['skipped_by_filter']} entries")
