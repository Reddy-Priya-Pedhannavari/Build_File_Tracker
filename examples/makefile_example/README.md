# Example Makefile Project with BuildFileTracker

This example demonstrates how to use BuildFileTracker with a traditional Makefile-based project.

## Project Structure

```
.
├── Makefile               # Main Makefile
├── src/                   # Source files
│   └── main.c
└── lib/                   # External libraries
    ├── libmath/           # Package A
    │   ├── math_ops.c
    │   ├── math_ops.h
    │   ├── unused_ops.c
    │   └── unused_ops.h
    └── libutil/           # Package B
        ├── utils.c
        └── utils.h
```

## Usage

### 1. Build the example normally

```bash
make
```

### 2. Build with file tracking

```bash
make build-with-tracking
```

Or use the helper script:

```bash
../../integrations/track_build.sh make all
```

### 3. Generate reports

```bash
python3 ../../python/report_generator.py build_reports/file_access_*.json -f all
```

The reports will show that only `math_ops.c`, `math_ops.h`, `utils.c`, and `utils.h` were accessed during the build, while `unused_ops.c` and `unused_ops.h` were not.
