# BuildFileTracker Yocto Integration
# Add this to your Yocto build configuration

FILETRACKER_LIB ?= "${TOPDIR}/../buildfiletracker/src/libfiletracker.so"
FILETRACKER_OUTPUT_DIR ?= "${TMPDIR}/file_tracking_reports"

# Enable file tracking for a recipe
# Usage: inherit filetracker
FILETRACKER_ENABLED ?= "1"

python filetracker_init() {
    import os
    import time
    
    if d.getVar('FILETRACKER_ENABLED') == '1':
        tracker_lib = d.getVar('FILETRACKER_LIB')
        output_dir = d.getVar('FILETRACKER_OUTPUT_DIR')
        
        if tracker_lib and os.path.exists(tracker_lib):
            # Create output directory
            os.makedirs(output_dir, exist_ok=True)
            
            # Set environment variables
            timestamp = time.strftime('%Y%m%d_%H%M%S')
            recipe_name = d.getVar('PN')
            
            d.setVar('LD_PRELOAD', tracker_lib)
            d.setVar('FILE_TRACKER_JSON', 
                    f"{output_dir}/file_access_{recipe_name}_{timestamp}.json")
            d.setVar('FILE_TRACKER_CSV', 
                    f"{output_dir}/file_access_{recipe_name}_{timestamp}.csv")
            
            bb.note(f"BuildFileTracker enabled for {recipe_name}")
        else:
            bb.warn("BuildFileTracker library not found, file tracking disabled")
}

# Add to bitbake configuration
addhandler filetracker_init
filetracker_init[eventmask] = "bb.event.ConfigParsed"

# Export environment variables for build tasks
export LD_PRELOAD
export FILE_TRACKER_JSON
export FILE_TRACKER_CSV
