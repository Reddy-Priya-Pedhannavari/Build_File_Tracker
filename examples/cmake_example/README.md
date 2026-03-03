# Example CMake Project with BuildFileTracker

This example demonstrates how to use BuildFileTracker with a CMake-based project.

## Project Structure

```
.
├── CMakeLists.txt          # Main CMake configuration
├── src/                    # Source files
│   └── main.c
├── external/               # Simulated external package
│   ├── package_a/
│   │   ├── used_file.c
│   │   ├── used_file.h
│   │   ├── unused_file.c
│   │   └── unused_file.h
│   └── package_b/
│       ├── helper.c
│       └── helper.h
└── README.md
```

## Usage

### 1. Build the example project normally

```bash
mkdir build
cd build
cmake ..
make
```

### 2. Build with file tracking

```bash
# Using the FileTracker.cmake integration
cd build
cmake -DFILETRACKER_LIB=/path/to/libfiletracker.so ..
make build_with_tracking
```

### 3. Generate reports

```bash
python3 ../../python/report_generator.py build/file_tracking_reports/file_access_*.json -f all
```

This will show that only `used_file.c`, `used_file.h`, and `helper.c` from the external packages were actually accessed during the build.
