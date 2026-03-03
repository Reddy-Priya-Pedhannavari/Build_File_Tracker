#!/bin/bash
# BuildFileTracker wrapper script for any build system
# This script sets up LD_PRELOAD to track file access during builds

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TRACKER_LIB="${FILETRACKER_LIB:-$SCRIPT_DIR/../src/libfiletracker.so}"
OUTPUT_DIR="${FILETRACKER_OUTPUT_DIR:-./build_reports}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Set environment variables for the tracker
export LD_PRELOAD="$TRACKER_LIB"
export FILE_TRACKER_JSON="$OUTPUT_DIR/file_access_${TIMESTAMP}.json"
export FILE_TRACKER_CSV="$OUTPUT_DIR/file_access_${TIMESTAMP}.csv"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}BuildFileTracker - Universal Wrapper${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo -e "${GREEN}Tracker Library:${NC} $TRACKER_LIB"
echo -e "${GREEN}Output Directory:${NC} $OUTPUT_DIR"
echo -e "${GREEN}JSON Report:${NC} $FILE_TRACKER_JSON"
echo -e "${GREEN}CSV Report:${NC} $FILE_TRACKER_CSV"
echo ""
echo -e "${BLUE}Executing build command:${NC} $@"
echo -e "${BLUE}========================================${NC}"
echo ""

# Execute the build command
"$@"
BUILD_EXIT_CODE=$?

# Unset LD_PRELOAD
unset LD_PRELOAD

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Build completed with exit code: $BUILD_EXIT_CODE${NC}"
echo -e "${BLUE}========================================${NC}"

# Check if reports were generated
if [ -f "$FILE_TRACKER_JSON" ]; then
    echo -e "${GREEN}Reports generated successfully!${NC}"
    echo ""
    echo "To generate additional report formats, run:"
    echo "  python3 python/report_generator.py $FILE_TRACKER_JSON"
else
    echo "Warning: No reports were generated"
fi

exit $BUILD_EXIT_CODE
