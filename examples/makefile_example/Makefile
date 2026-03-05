# Include BuildFileTracker integration
include ../../integrations/makefile/filetracker.mk

# Compiler settings
CC = gcc
CFLAGS = -Wall -Ilib/libmath -Ilib/libutil
LDFLAGS =

# Directories
SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files
SRCS = $(SRC_DIR)/main.c \
       $(LIB_DIR)/libmath/math_ops.c \
       $(LIB_DIR)/libutil/utils.c

# Object files
OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))

# Target executable
TARGET = $(BUILD_DIR)/example_app

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(LIB_DIR)/libmath/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(LIB_DIR)/libutil/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(FILETRACKER_OUTPUT_DIR)
