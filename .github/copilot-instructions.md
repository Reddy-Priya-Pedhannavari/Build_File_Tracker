# BuildFileTracker - Workspace Instructions

## Project Overview
BuildFileTracker is a universal build file access monitoring library that tracks which files from external packages are actually used during builds. It works across different build systems (CMake, Yocto, OTP, etc.) and generates detailed reports.

## Project Structure
- `src/` - Core C library for file interception
- `python/` - Python utilities for report generation
- `integrations/` - Build system integration helpers
- `examples/` - Example projects demonstrating usage
- `docs/` - Documentation

## Key Technologies
- C (for LD_PRELOAD interception)
- Python (for report processing)
- Multiple output formats: JSON, CSV, XML, XLSX

## Development Guidelines
- Keep the interception library lightweight and efficient
- Ensure thread-safe operations
- Support multiple platforms (Linux primary, with Windows/macOS considerations)
- Generate human-readable reports
- Provide easy integration mechanisms
