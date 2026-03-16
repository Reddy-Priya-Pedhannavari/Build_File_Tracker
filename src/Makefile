CC = gcc
CFLAGS = -Wall -fPIC -shared -O2 -pthread
LDFLAGS = -ldl

TARGET    = libfiletracker.so
TARGET32  = libfiletracker32.so
SOURCES   = file_tracker.c file_interceptor.c
HEADERS   = file_tracker.h

# Directories for $LIB-token based LD_PRELOAD (no wrong-ELF-class errors)
DIR64     = lib/x86_64-linux-gnu
DIR32     = lib/i386-linux-gnu

# Default: build 64-bit; also attempt 32-bit if gcc-multilib is available
all: $(TARGET) try32 install-lib

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# 32-bit variant
$(TARGET32): $(SOURCES) $(HEADERS)
	$(CC) -m32 $(CFLAGS) $(SOURCES) -o $(TARGET32) $(LDFLAGS)

# Try 32-bit silently; skip without error if gcc-multilib not installed
try32:
	@$(CC) -m32 $(CFLAGS) $(SOURCES) -o $(TARGET32) $(LDFLAGS) 2>/dev/null \
		&& echo "✅ 32-bit library built: $(TARGET32)" \
		|| echo "ℹ️  Skipping 32-bit build (install gcc-multilib to enable)"

# Install both into $LIB-style subdirs so LD_PRELOAD='.../$LIB/libfiletracker.so'
# works for both 32 and 64-bit processes with zero wrong-ELF-class errors.
install-lib: $(TARGET)
	@mkdir -p $(DIR64)
	@cp $(TARGET) $(DIR64)/$(TARGET)
	@echo "✅ Installed 64-bit -> $(DIR64)/$(TARGET)"
	@if [ -f $(TARGET32) ]; then \
		mkdir -p $(DIR32); \
		cp $(TARGET32) $(DIR32)/$(TARGET); \
		echo "✅ Installed 32-bit -> $(DIR32)/$(TARGET)"; \
	fi

clean:
	rm -f $(TARGET) $(TARGET32) *.o
	rm -rf lib/

install: $(TARGET)
	cp $(TARGET) /usr/local/lib/
	@if [ -f $(TARGET32) ]; then cp $(TARGET32) /usr/local/lib/; fi
	ldconfig

.PHONY: all try32 install-lib clean install
