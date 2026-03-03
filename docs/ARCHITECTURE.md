# Architecture Overview

This document provides a high-level map of the BuildFileTracker project, showing entry points, core components, integrations, examples, and how they relate across operating systems.

```mermaid
flowchart LR
    User((User))
    QuickStart["quickstart.sh (bash)"]
    CLI["buildfiletracker_cli.py (Python CLI)"]
    BuildLib["src/ (C library)\nMakefile / build commands"]
    TrackerLib["libfiletracker.so / .dll (built)\nLD_PRELOAD hooks"]
    Integrations["integrations/ helpers"]
    CMake["integrations/cmake/FileTracker.cmake"]
    MakefileInt["integrations/makefile/filetracker.mk"]
    PythonInt["integrations/python/buildfiletracker.py"]
    Yocto["integrations/yocto/filetracker.bbclass"]
    TrackScripts["integrations/track_build*.sh/.bat"]
    Examples["examples/ (CMake/make demos)"]
    Reports["python/report_generator.py & analyzer.py"]
    Output["report.json/csv/xml/xlsx/..." ]
    OS_Linux("Linux/macOS/WSL")
    OS_Win("Windows")

    User -->|runs| QuickStart
    User -->|runs| CLI
    QuickStart --> BuildLib
    QuickStart --> Examples
    CLI --> BuildLib
    BuildLib --> TrackerLib
    TrackerLib --> Integrations
    Integrations --> CMake
    Integrations --> MakefileInt
    Integrations --> PythonInt
    Integrations --> Yocto
    Integrations --> TrackScripts
    CLI --> TrackerLib
    CLI --> Reports
    Examples --> TrackerLib
    TrackerLib --> Reports
    Reports --> Output
    OS_Linux --> QuickStart
    OS_Linux --> TrackScripts
    OS_Linux --> TrackerLib
    OS_Win --> CLI
    OS_Win --> PythonInt
    OS_Win -.-> TrackerLib
    OS_Win --> TrackScripts
```

## Entry Points

1. **quickstart.sh** – shell script for a guided demo.
2. **buildfiletracker_cli.py** – Python command‑line interface for tracking, reporting, and analysis.

## Core Components

* **src/**: C library providing interception via LD_PRELOAD (or equivalent on Windows).
* **integrations/**: Helper modules and scripts for various build systems (CMake, Makefile, Yocto, Python).
* **examples/**: Demonstration projects used by quickstart and documentation.
* **python/**: Report generator and analysis tools.

## Workflow

1. build the tracker library (`make` in `src/`).
2. run a build command with the library preloaded (via wrapper or CLI).
3. tracker captures file accesses and writes raw JSON.
4. run Python report/analyzer tools to convert or inspect the data.
5. review outputs (JSON, CSV, XML, XLSX, summary text).

## Platform Notes

* Linux/macOS/WSL: full support with `LD_PRELOAD` and shell wrappers.
* Windows: limited native support; usually used via Python CLI or WSL.
