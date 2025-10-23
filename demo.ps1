# Flux Archive Manager Demo Script
# Demonstrates current functionality and build process

param(
    [switch]$BuildOnly,
    [switch]$TestOnly,
    [switch]$ShowGUI,
    [switch]$Clean
)

Write-Host "🚀 Flux Archive Manager Demo" -ForegroundColor Cyan
Write-Host "=================================" -ForegroundColor Cyan

# Check if we're in the right directory
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "❌ Error: Please run this script from the project root directory" -ForegroundColor Red
    exit 1
}

# Clean build directory if requested
if ($Clean) {
    Write-Host "🧹 Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path "build") {
        Remove-Item -Recurse -Force "build"
    }
    Write-Host "✅ Build directory cleaned" -ForegroundColor Green
}

# Build the project
if (-not $TestOnly) {
    Write-Host "`n📦 Building Flux Archive Manager..." -ForegroundColor Yellow
    
    # Configure with CMake
    Write-Host "Configuring with CMake..." -ForegroundColor Gray
    $configResult = & cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Ninja" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ CMake configuration failed:" -ForegroundColor Red
        Write-Host $configResult -ForegroundColor Red
        Write-Host "`n💡 Make sure you have CMake and Qt6 installed:" -ForegroundColor Yellow
        Write-Host "   winget install Kitware.CMake" -ForegroundColor Gray
        Write-Host "   winget install Qt.Qt" -ForegroundColor Gray
        exit 1
    }
    
    # Build the project
    Write-Host "Building project..." -ForegroundColor Gray
    $buildResult = & cmake --build build --config Release --parallel 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Build failed:" -ForegroundColor Red
        Write-Host $buildResult -ForegroundColor Red
        exit 1
    }
    
    Write-Host "✅ Build completed successfully!" -ForegroundColor Green
}

if ($BuildOnly) {
    Write-Host "`n🎉 Build complete! You can now run:" -ForegroundColor Cyan
    Write-Host "   .\build\flux-cli\flux-cli.exe --help" -ForegroundColor Gray
    Write-Host "   .\build\flux-gui\flux-gui.exe" -ForegroundColor Gray
    exit 0
}

# Test CLI functionality
Write-Host "`n🖥️ Testing CLI functionality..." -ForegroundColor Yellow

if (Test-Path "build\flux-cli\flux-cli.exe") {
    Write-Host "CLI Help:" -ForegroundColor Gray
    & ".\build\flux-cli\flux-cli.exe" --help
    
    Write-Host "`nCLI Version:" -ForegroundColor Gray
    & ".\build\flux-cli\flux-cli.exe" --version 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Version command not yet implemented" -ForegroundColor Yellow
    }
} else {
    Write-Host "❌ CLI executable not found" -ForegroundColor Red
}

# Test GUI functionality
if ($ShowGUI -and (Test-Path "build\flux-gui\flux-gui.exe")) {
    Write-Host "`n🖼️ Launching GUI application..." -ForegroundColor Yellow
    Write-Host "GUI window should open. Close it to continue the demo." -ForegroundColor Gray
    
    # Launch GUI and wait for it to close
    Start-Process -FilePath ".\build\flux-gui\flux-gui.exe" -Wait
    Write-Host "✅ GUI application closed" -ForegroundColor Green
} elseif (-not $ShowGUI) {
    Write-Host "`n🖼️ GUI application available at: .\build\flux-gui\flux-gui.exe" -ForegroundColor Yellow
    Write-Host "   Use -ShowGUI parameter to launch it automatically" -ForegroundColor Gray
}

# Run tests if available
Write-Host "`n🧪 Running tests..." -ForegroundColor Yellow
if (Test-Path "build") {
    Push-Location "build"
    $testResult = & ctest --output-on-failure --parallel 2 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ All tests passed!" -ForegroundColor Green
    } else {
        Write-Host "⚠️ Some tests failed or no tests found:" -ForegroundColor Yellow
        Write-Host $testResult -ForegroundColor Gray
    }
    Pop-Location
}

# Show project status
Write-Host "`n📊 Project Status Summary:" -ForegroundColor Cyan
Write-Host "=========================" -ForegroundColor Cyan

$statusItems = @(
    @{ Name = "Build System"; Status = "✅ Working"; Details = "CMake + Ninja" },
    @{ Name = "CLI Application"; Status = "🚧 Partial"; Details = "Framework ready, needs core implementation" },
    @{ Name = "GUI Application"; Status = "🚧 Partial"; Details = "UI framework ready, needs core integration" },
    @{ Name = "Core Library"; Status = "🚧 Partial"; Details = "Architecture ready, needs third-party libraries" },
    @{ Name = "Archive Formats"; Status = "⏳ Planned"; Details = "ZIP, TAR, 7Z interfaces designed" },
    @{ Name = "CI/CD Pipeline"; Status = "✅ Working"; Details = "GitHub Actions with multi-platform support" },
    @{ Name = "Documentation"; Status = "✅ Complete"; Details = "Comprehensive project documentation" }
)

foreach ($item in $statusItems) {
    Write-Host ("  {0,-20} {1,-12} {2}" -f $item.Name, $item.Status, $item.Details) -ForegroundColor White
}

Write-Host "`n🎯 Next Steps:" -ForegroundColor Cyan
Write-Host "=============" -ForegroundColor Cyan
Write-Host "1. Integrate third-party libraries (libzip, libarchive, 7-Zip SDK)" -ForegroundColor Yellow
Write-Host "2. Implement core archive operations" -ForegroundColor Yellow
Write-Host "3. Connect GUI to working core functionality" -ForegroundColor Yellow
Write-Host "4. Add comprehensive unit tests" -ForegroundColor Yellow
Write-Host "5. Performance optimization and benchmarking" -ForegroundColor Yellow

Write-Host "`n📚 Documentation:" -ForegroundColor Cyan
Write-Host "=================" -ForegroundColor Cyan
Write-Host "  README.md                    - Project overview and setup" -ForegroundColor Gray
Write-Host "  PROJECT_STATUS.md            - Detailed project status" -ForegroundColor Gray
Write-Host "  docs/CORE_FEATURE_MATRIX.md  - Feature implementation matrix" -ForegroundColor Gray
Write-Host "  BUILD_GUIDE.md               - Comprehensive build instructions" -ForegroundColor Gray

Write-Host "`n🔗 Repository:" -ForegroundColor Cyan
Write-Host "=============" -ForegroundColor Cyan
Write-Host "  https://github.com/xiaokanchengyang/Flux-cpp" -ForegroundColor Blue

Write-Host "`n🎉 Demo completed!" -ForegroundColor Green
Write-Host "The project has a solid foundation and is ready for core functionality implementation." -ForegroundColor White
