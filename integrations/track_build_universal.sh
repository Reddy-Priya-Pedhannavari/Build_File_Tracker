#!/bin/bash
# track_build.sh - Cross-platform build file tracker wrapper for Linux/macOS
# Usage: ./track_build.sh make all
# Usage: ./track_build.sh cmake --build .

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_JSON="${FILE_TRACKER_JSON:-./build_tracking.json}"
OUTPUT_CSV="${FILE_TRACKER_CSV:-./build_tracking.csv}"

# Color output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}BuildFileTracker - Build Wrapper${NC}"
echo "Build command: $@"
echo "Output JSON: $OUTPUT_JSON"
echo "Output CSV: $OUTPUT_CSV"
echo "---"

# Check if C library is available
if [ -f "$SCRIPT_DIR/../src/libfiletracker.so" ] || [ -f "$SCRIPT_DIR/../src/libfiletracker.dylib" ]; then
    echo -e "${GREEN}Using C-based tracker (LD_PRELOAD)${NC}"
    
    # Find the library
    if [ -f "$SCRIPT_DIR/../src/libfiletracker.so" ]; then
        LIB_PATH="$SCRIPT_DIR/../src/libfiletracker.so"
    else
        LIB_PATH="$SCRIPT_DIR/../src/libfiletracker.dylib"
    fi
    
    # Set environment and run
    export LD_PRELOAD="$LIB_PATH"
    export FILE_TRACKER_JSON="$OUTPUT_JSON"
    export FILE_TRACKER_CSV="$OUTPUT_CSV"
    
    "$@"
    RESULT=$?
    
    unset LD_PRELOAD
    unset FILE_TRACKER_JSON
    unset FILE_TRACKER_CSV
else
    echo -e "${YELLOW}C library not found. Using Python tracker.${NC}"
    echo "Build with: cd src && make"
    
    python3 "$SCRIPT_DIR/../python/tracker.py" -o "$OUTPUT_JSON" "$@"
    RESULT=$?
fi

if [ $RESULT -eq 0 ]; then
    echo -e "${GREEN}✓ Tracking completed${NC}"
    echo "Reports: $OUTPUT_JSON, $OUTPUT_CSV"
else
    echo -e "${YELLOW}Build or tracking had issues (exit code: $RESULT)${NC}"
fi

exit $RESULT
