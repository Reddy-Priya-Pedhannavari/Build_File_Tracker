# BuildFileTracker CMake Integration
# Include this file in your CMakeLists.txt to enable file tracking

cmake_minimum_required(VERSION 3.10)

# Find the BuildFileTracker library
find_library(FILETRACKER_LIB
    NAMES libfiletracker.so filetracker
    HINTS 
        ${CMAKE_CURRENT_LIST_DIR}/../src
        /usr/local/lib
        /usr/lib
    DOC "BuildFileTracker library"
)

if(FILETRACKER_LIB)
    message(STATUS "BuildFileTracker library found: ${FILETRACKER_LIB}")
    
    # Set default output directory
    if(NOT DEFINED FILETRACKER_OUTPUT_DIR)
        set(FILETRACKER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/file_tracking_reports")
    endif()
    
    # Create output directory
    file(MAKE_DIRECTORY ${FILETRACKER_OUTPUT_DIR})
    
    # Get timestamp
    string(TIMESTAMP FILETRACKER_TIMESTAMP "%Y%m%d_%H%M%S")
    
    # Set environment variables
    set(ENV{FILE_TRACKER_JSON} "${FILETRACKER_OUTPUT_DIR}/file_access_${FILETRACKER_TIMESTAMP}.json")
    set(ENV{FILE_TRACKER_CSV} "${FILETRACKER_OUTPUT_DIR}/file_access_${FILETRACKER_TIMESTAMP}.csv")
    
    # Function to enable file tracking for a target
    function(enable_file_tracking TARGET_NAME)
        if(UNIX AND NOT APPLE)
            set_target_properties(${TARGET_NAME} PROPERTIES
                LINK_FLAGS "-Wl,--no-as-needed -ldl"
            )
            
            # Set LD_PRELOAD for running the target
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "File tracking enabled for ${TARGET_NAME}"
                COMMAND ${CMAKE_COMMAND} -E echo "Run with: LD_PRELOAD=${FILETRACKER_LIB} ./${TARGET_NAME}"
            )
        endif()
    endfunction()
    
    # Add a custom target to run build with tracking
    add_custom_target(build_with_tracking
        COMMAND ${CMAKE_COMMAND} -E env 
            LD_PRELOAD=${FILETRACKER_LIB}
            FILE_TRACKER_JSON=${FILETRACKER_OUTPUT_DIR}/file_access_${FILETRACKER_TIMESTAMP}.json
            FILE_TRACKER_CSV=${FILETRACKER_OUTPUT_DIR}/file_access_${FILETRACKER_TIMESTAMP}.csv
            ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building with file access tracking..."
    )
    
    message(STATUS "BuildFileTracker integration enabled")
    message(STATUS "  Output directory: ${FILETRACKER_OUTPUT_DIR}")
    message(STATUS "  Use 'make build_with_tracking' to build with file tracking")
    
else()
    message(WARNING "BuildFileTracker library not found. File tracking disabled.")
endif()
