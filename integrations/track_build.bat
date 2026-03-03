@echo off
REM track_build.bat - Cross-platform build file tracker wrapper for Windows
REM Usage: track_build.bat make all
REM Usage: track_build.bat cmake --build .

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
if not defined FILE_TRACKER_JSON set FILE_TRACKER_JSON=build_tracking.json
if not defined FILE_TRACKER_CSV set FILE_TRACKER_CSV=build_tracking.csv

echo.
echo BuildFileTracker - Build Wrapper for Windows
echo Build command: %*
echo Output JSON: %FILE_TRACKER_JSON%
echo Output CSV: %FILE_TRACKER_CSV%
echo ---
echo.

REM Check if C library exists (not typical on Windows native, but check anyway)
if exist "%SCRIPT_DIR%..\src\libfiletracker.dll" (
    echo Using C-based tracker
    REM Note: Requires separate Windows implementation
    echo Warning: Native Windows C tracker not yet available
) else (
    echo Using Python-based tracker
    python "%SCRIPT_DIR%..\python\tracker.py" -o %FILE_TRACKER_JSON% %*
    set RESULT=!ERRORLEVEL!
    
    if !RESULT! equ 0 (
        echo.
        echo ✓ Tracking completed
        echo Reports generated:
        echo   - %FILE_TRACKER_JSON%
        if exist %FILE_TRACKER_CSV% echo   - %FILE_TRACKER_CSV%
    ) else (
        echo.
        echo ✗ Tracking or build failed (exit code: !RESULT!)
    )
    
    exit /b !RESULT!
)

endlocal
