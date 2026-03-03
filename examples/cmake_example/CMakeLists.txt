cmake_minimum_required(VERSION 3.10)
project(BuildFileTrackerExample C)

set(CMAKE_C_STANDARD 11)

# Include BuildFileTracker integration
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../integrations/cmake")
include(FileTracker OPTIONAL)

# Add external packages as include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/external/package_a
    ${CMAKE_CURRENT_SOURCE_DIR}/external/package_b
)

# Create executable
add_executable(example_app
    src/main.c
    external/package_a/used_file.c
    external/package_b/helper.c
)

# Enable file tracking for this target
if(COMMAND enable_file_tracking)
    enable_file_tracking(example_app)
endif()
