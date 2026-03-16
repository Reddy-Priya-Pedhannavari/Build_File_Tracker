#!/bin/bash
# setup_tracking.sh — Source this file to enable/disable BuildFileTracker
#
# Usage:
#   source setup_tracking.sh on  [report_dir]   # enable tracking
#   source setup_tracking.sh off                 # disable tracking
#   source setup_tracking.sh status             # show current state
#
# Default report dir: /tmp/bft_reports
# Example:
#   source /foss2/uie21749/__Priya__/Build_File_Tracker/setup_tracking.sh on \
#          /foss2/uie21749/__Priya__/Ramses_demo_build/tmp

_BFT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
_BFT_LIB64="${_BFT_DIR}/src/libfiletracker.so"
_BFT_LIB32="${_BFT_DIR}/src/libfiletracker32.so"
# $LIB-token path: linker expands $LIB per-process to the correct arch dir.
# 64-bit processes get lib/x86_64-linux-gnu/libfiletracker.so
# 32-bit processes get lib/i386-linux-gnu/libfiletracker.so
# This means zero wrong-ELF-class errors even in mixed 32/64-bit builds.
_BFT_LIB_TOKEN="${_BFT_DIR}/src/lib/\$LIB/libfiletracker.so"

_bft_on() {
    local report_dir="${1:-/tmp/bft_reports}"
    mkdir -p "${report_dir}"

    export FILE_TRACKER_JSON="${report_dir}/reports.json"
    export FILE_TRACKER_FILTER_DIR="${report_dir%/*}"  # parent of report_dir

    # Use the $LIB dynamic token so the linker picks the right arch automatically.
    # Works for both 32-bit and 64-bit processes with zero error messages.
    # Falls back to plain 64-bit .so if the $LIB subdirs haven't been built yet.
    if [ -d "${_BFT_DIR}/src/lib" ]; then
        export LD_PRELOAD="${_BFT_LIB_TOKEN}"
    else
        export LD_PRELOAD="${_BFT_LIB64}"
    fi

    echo "✅ Tracking ON"
    echo "   LD_PRELOAD         = ${LD_PRELOAD}"
    echo "   FILE_TRACKER_JSON  = ${FILE_TRACKER_JSON}"
    echo "   FILTER_DIR         = ${FILE_TRACKER_FILTER_DIR}"
}

_bft_off() {
    unset LD_PRELOAD
    unset FILE_TRACKER_JSON
    unset FILE_TRACKER_CSV
    unset FILE_TRACKER_FILTER_DIR
    echo "🔴 Tracking OFF — LD_PRELOAD cleared"
}

_bft_status() {
    if [ -n "${LD_PRELOAD}" ]; then
        echo "✅ Tracking ON"
        echo "   LD_PRELOAD        = ${LD_PRELOAD}"
        echo "   FILE_TRACKER_JSON = ${FILE_TRACKER_JSON}"
        echo "   FILTER_DIR        = ${FILE_TRACKER_FILTER_DIR}"
        if [ -f "${FILE_TRACKER_JSON}" ]; then
            echo "   Report lines so far: $(wc -l < "${FILE_TRACKER_JSON}")"
        fi
    else
        echo "🔴 Tracking OFF"
    fi
}

case "${1:-}" in
    on)     _bft_on "${2:-}" ;;
    off)    _bft_off ;;
    status) _bft_status ;;
    *)
        echo "Usage: source setup_tracking.sh on [report_dir]"
        echo "       source setup_tracking.sh off"
        echo "       source setup_tracking.sh status"
        ;;
esac
