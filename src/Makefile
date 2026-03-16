CC = gcc
CFLAGS = -Wall -fPIC -shared -O2 -pthread
LDFLAGS = -ldl

TARGET    = libfiletracker.so
TARGET32  = libfiletracker32.so
SOURCES   = file_tracker.c file_interceptor.c
HEADERS   = file_tracker.h

# Default: build 64-bit; also attempt 32-bit if gcc-multilib is available
all: $(TARGET) try32

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# 32-bit variant — silences "wrong ELF class" errors from 32-bit host tools
# Requires: sudo apt-get install gcc-multilib  (Ubuntu/Debian)
$(TARGET32): $(SOURCES) $(HEADERS)
	$(CC) -m32 $(CFLAGS) $(SOURCES) -o $(TARGET32) $(LDFLAGS)

# Try to build 32-bit; don't fail the whole build if multilib isn't installed
try32:
	@$(CC) -m32 $(CFLAGS) $(SOURCES) -o $(TARGET32) $(LDFLAGS) 2>/dev/null \
		&& echo "✅ 32-bit library built: $(TARGET32)" \
		|| echo "ℹ️  Skipping 32-bit build (install gcc-multilib to enable)"

clean:
	rm -f $(TARGET) $(TARGET32) *.o

install: $(TARGET)
	cp $(TARGET) /usr/local/lib/
	@if [ -f $(TARGET32) ]; then cp $(TARGET32) /usr/local/lib/; fi
	ldconfig

.PHONY: all try32 clean install
