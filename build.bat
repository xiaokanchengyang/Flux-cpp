@echo off
REM Flux Cross-Platform Build Script for Windows
REM This script builds the Flux archive manager on Windows

setlocal enabledelayedexpansion

REM Default values
set BUILD_TYPE=Release
set BUILD_DIR=build
set CLEAN_BUILD=false
set VERBOSE=false
set JOBS=%NUMBER_OF_PROCESSORS%
set GENERATOR=

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :args_done
if "%~1"=="-h" goto :show_help
if "%~1"=="--help" goto :show_help
if "%~1"=="-d" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if "%~1"=="-c" (
    set CLEAN_BUILD=true
    shift
    goto :parse_args
)
if "%~1"=="--clean" (
    set CLEAN_BUILD=true
    shift
    goto :parse_args
)
if "%~1"=="-v" (
    set VERBOSE=true
    shift
    goto :parse_args
)
if "%~1"=="--verbose" (
    set VERBOSE=true
    shift
    goto :parse_args
)
if "%~1"=="-j" (
    set JOBS=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--jobs" (
    set JOBS=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--build-dir" (
    set BUILD_DIR=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--generator" (
    set GENERATOR=%~2
    shift
    shift
    goto :parse_args
)
echo Error: Unknown option %~1
exit /b 1

:show_help
echo Flux Cross-Platform Build Script for Windows
echo.
echo Usage: %~nx0 [OPTIONS]
echo.
echo Options:
echo   -h, --help              Show this help message
echo   -d, --debug             Build in Debug mode (default: Release)
echo   -c, --clean             Clean build directory before building
echo   -v, --verbose           Enable verbose output
echo   -j, --jobs N            Number of parallel jobs (default: auto-detect)
echo   --build-dir DIR         Build directory (default: build)
echo   --generator GEN         CMake generator (default: auto-detect)
echo.
echo Examples:
echo   %~nx0                      # Build in Release mode
echo   %~nx0 -d -c               # Clean Debug build
echo   %~nx0 -j 8 -v             # Verbose build with 8 jobs
exit /b 0

:args_done

echo [INFO] Building Flux Archive Manager for Windows
echo [INFO] Build type: %BUILD_TYPE%
echo [INFO] Build directory: %BUILD_DIR%
echo [INFO] Parallel jobs: %JOBS%

REM Check for required tools
echo [INFO] Checking required tools...

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake is required but not found
    echo [ERROR] Please install CMake and add it to PATH
    exit /b 1
)

REM Auto-detect generator if not specified
if "%GENERATOR%"=="" (
    where msbuild >nul 2>&1
    if not errorlevel 1 (
        set GENERATOR=Visual Studio 17 2022
        echo [INFO] Using Visual Studio 2022 generator
    ) else (
        where ninja >nul 2>&1
        if not errorlevel 1 (
            set GENERATOR=Ninja
            echo [INFO] Using Ninja generator
        ) else (
            set GENERATOR=MinGW Makefiles
            echo [INFO] Using MinGW Makefiles generator
        )
    )
)

REM Check for Qt6
where qmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Qt6 is required but not found
    echo [ERROR] Please install Qt6 and add it to PATH
    exit /b 1
)

REM Clean build directory if requested
if "%CLEAN_BUILD%"=="true" (
    echo [INFO] Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure with CMake
echo [INFO] Configuring with CMake...
set CMAKE_ARGS=-DCMAKE_BUILD_TYPE=%BUILD_TYPE%

if "%VERBOSE%"=="true" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_VERBOSE_MAKEFILE=ON
)

cmake -G "%GENERATOR%" %CMAKE_ARGS% ..
if errorlevel 1 (
    echo [ERROR] CMake configuration failed
    exit /b 1
)

REM Build
echo [INFO] Building...
if "%GENERATOR%"=="Visual Studio 17 2022" (
    cmake --build . --config %BUILD_TYPE% --parallel %JOBS%
) else (
    cmake --build . --parallel %JOBS%
)

if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

echo [SUCCESS] Build completed successfully!

REM Show build results
echo [INFO] Build results:
if exist "flux-gui\%BUILD_TYPE%\FluxGUI.exe" (
    echo   GUI application: flux-gui\%BUILD_TYPE%\FluxGUI.exe
) else if exist "flux-gui\FluxGUI.exe" (
    echo   GUI application: flux-gui\FluxGUI.exe
)

if exist "flux-cli\%BUILD_TYPE%\flux-cli.exe" (
    echo   CLI application: flux-cli\%BUILD_TYPE%\flux-cli.exe
) else if exist "flux-cli\flux-cli.exe" (
    echo   CLI application: flux-cli\flux-cli.exe
)

REM Offer to install
echo.
set /p INSTALL="Do you want to install Flux? (y/N): "
if /i "%INSTALL%"=="y" (
    echo [INFO] Installing...
    cmake --install . --config %BUILD_TYPE%
    if errorlevel 1 (
        echo [ERROR] Installation failed
        exit /b 1
    )
    echo [SUCCESS] Installation completed!
) else (
    echo [INFO] You can install later by running: cmake --install . --config %BUILD_TYPE%
)

echo [SUCCESS] All done! 🎉
cd ..
exit /b 0












