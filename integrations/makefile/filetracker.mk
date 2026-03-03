# BuildFileTracker Makefile Integration
# Include this in your Makefile to enable file tracking

# Configuration
FILETRACKER_LIB ?= $(shell pwd)/path/to/libfiletracker.so
FILETRACKER_OUTPUT_DIR ?= ./build_reports
FILETRACKER_TIMESTAMP := $(shell date +%Y%m%d_%H%M%S)

# Check if tracker library exists
ifeq ($(wildcard $(FILETRACKER_LIB)),)
    $(warning BuildFileTracker library not found at $(FILETRACKER_LIB))
    FILETRACKER_ENABLED := 0
else
    FILETRACKER_ENABLED := 1
endif

# Export environment variables when tracking is enabled
ifeq ($(FILETRACKER_ENABLED),1)
    export LD_PRELOAD := $(FILETRACKER_LIB)
    export FILE_TRACKER_JSON := $(FILETRACKER_OUTPUT_DIR)/file_access_$(FILETRACKER_TIMESTAMP).json
    export FILE_TRACKER_CSV := $(FILETRACKER_OUTPUT_DIR)/file_access_$(FILETRACKER_TIMESTAMP).csv
    
    $(info BuildFileTracker enabled)
    $(info   Library: $(FILETRACKER_LIB))
    $(info   Output: $(FILETRACKER_OUTPUT_DIR))
endif

# Create output directory
$(shell mkdir -p $(FILETRACKER_OUTPUT_DIR))

# Add a target to build with tracking
.PHONY: build-with-tracking
build-with-tracking:
	@echo "Building with file tracking..."
	@mkdir -p $(FILETRACKER_OUTPUT_DIR)
	LD_PRELOAD=$(FILETRACKER_LIB) \
	FILE_TRACKER_JSON=$(FILETRACKER_OUTPUT_DIR)/file_access_$(FILETRACKER_TIMESTAMP).json \
	FILE_TRACKER_CSV=$(FILETRACKER_OUTPUT_DIR)/file_access_$(FILETRACKER_TIMESTAMP).csv \
	$(MAKE) all
	@echo "Build complete. Reports in $(FILETRACKER_OUTPUT_DIR)"

# Add a target to generate all report formats
.PHONY: generate-reports
generate-reports:
	@echo "Generating reports..."
	@python3 python/report_generator.py $(FILE_TRACKER_JSON) -f all
	@echo "Reports generated"
