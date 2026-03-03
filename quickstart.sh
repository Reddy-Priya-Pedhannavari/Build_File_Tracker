#!/bin/bash
# BuildFileTracker Quick Start Script
# This script sets up and demonstrates BuildFileTracker

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_quickstart"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}=====================================${NC}"
echo -e "${BLUE}BuildFileTracker Quick Start${NC}"
echo -e "${BLUE}=====================================${NC}"
echo ""

# Step 1: Build the library
echo -e "${GREEN}[1/5] Building tracker library...${NC}"
if [ ! -f "$SCRIPT_DIR/src/libfiletracker.so" ]; then
    cd "$SCRIPT_DIR/src"
    make
    echo -e "${GREEN}✓ Library built successfully${NC}"
else
    echo -e "${YELLOW}✓ Library already built${NC}"
fi
echo ""

# Step 2: Install Python dependencies
echo -e "${GREEN}[2/5] Installing Python dependencies...${NC}"
cd "$SCRIPT_DIR/python"
if command -v pip3 &> /dev/null; then
    pip3 install -r requirements.txt -q 2>&1 || echo "Note: Some dependencies may need manual installation"
    echo -e "${GREEN}✓ Python dependencies installed${NC}"
else
    echo -e "${YELLOW}⚠ pip3 not found, skipping Python dependencies${NC}"
fi
echo ""

# Step 3: Build an example
echo -e "${GREEN}[3/5] Building example project with tracking...${NC}"
cd "$SCRIPT_DIR/examples/cmake_example"
mkdir -p build
cd build

echo "  Configuring CMake..."
cmake .. > /dev/null 2>&1

echo "  Building with file tracking..."
export LD_PRELOAD="$SCRIPT_DIR/src/libfiletracker.so"
export FILE_TRACKER_JSON="$SCRIPT_DIR/example_report.json"
export FILE_TRACKER_CSV="$SCRIPT_DIR/example_report.csv"

make > /dev/null 2>&1

unset LD_PRELOAD

echo -e "${GREEN}✓ Example built with tracking enabled${NC}"
echo ""

# Step 4: Generate reports
echo -e "${GREEN}[4/5] Generating reports...${NC}"
cd "$SCRIPT_DIR"

if [ -f "example_report.json" ]; then
    python3 python/report_generator.py example_report.json -f all -o quickstart_report 2>&1 || true
    echo -e "${GREEN}✓ Reports generated${NC}"
else
    echo -e "${YELLOW}⚠ No report file generated${NC}"
fi
echo ""

# Step 5: Show summary
echo -e "${GREEN}[5/5] Summary${NC}"
if [ -f "quickstart_report_summary.txt" ]; then
    cat quickstart_report_summary.txt
else
    echo -e "${YELLOW}Summary not available${NC}"
fi
echo ""

echo -e "${BLUE}=====================================${NC}"
echo -e "${BLUE}Quick Start Complete!${NC}"
echo -e "${BLUE}=====================================${NC}"
echo ""
echo "Generated files:"
echo "  - example_report.json      (Raw JSON data)"
echo "  - example_report.csv       (CSV format)"
echo "  - quickstart_report.json   (Formatted JSON)"
echo "  - quickstart_report.csv    (CSV report)"
echo "  - quickstart_report.xml    (XML report)"
echo "  - quickstart_report_summary.txt (Summary)"
echo ""
echo "Next steps:"
echo "  1. Review the reports above"
echo "  2. Try tracking your own build:"
echo "     ./integrations/track_build.sh make all"
echo "  3. Read the User Guide: docs/USER_GUIDE.md"
echo "  4. Check examples: examples/"
echo ""
echo -e "${GREEN}Happy tracking!${NC}"
