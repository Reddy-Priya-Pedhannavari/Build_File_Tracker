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

_bft_on() {
    local report_dir="${1:-/tmp/bft_reports}"
    mkdir -p "${report_dir}"

    export FILE_TRACKER_JSON="${report_dir}/reports.json"
    export FILE_TRACKER_FILTER_DIR="${report_dir%/*}"  # parent of report_dir

    # Use only the 64-bit library. The 32-bit variant (libfiletracker32.so)
    # is only needed if you specifically know your build spawns 32-bit host
    # tools. For most builds (Ramses, Yocto, CMake) 64-bit only is correct.
    export LD_PRELOAD="${_BFT_LIB64}"

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
