@echo off
echo Building Flux GUI Application...
echo.

REM Check if we're in the correct directory
if not exist "flux-gui" (
    echo Error: flux-gui directory not found!
    echo Please run this script from the project root directory.
    pause
    exit /b 1
)

REM Create build directory
if not exist "build-gui" (
    mkdir build-gui
    echo Created build-gui directory
)

cd build-gui

REM Configure with CMake
echo Configuring CMake...
cmake -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2022_64" ^
    ../flux-gui

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build the project
echo Building project...
cmake --build . --config Debug

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable should be in: build-gui\Debug\FluxGUI.exe
echo.

REM Ask if user wants to run the application
set /p choice="Do you want to run the application? (y/n): "
if /i "%choice%"=="y" (
    if exist "Debug\FluxGUI.exe" (
        echo Running FluxGUI...
        start Debug\FluxGUI.exe
    ) else (
        echo Executable not found at Debug\FluxGUI.exe
    )
)

pause
