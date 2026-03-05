CC = gcc
CFLAGS = -Wall -fPIC -shared -O2 -pthread
LDFLAGS = -ldl

TARGET = libfiletracker.so
SOURCES = file_tracker.c file_interceptor.c
HEADERS = file_tracker.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o

install: $(TARGET)
	cp $(TARGET) /usr/local/lib/
	ldconfig

.PHONY: all clean install
