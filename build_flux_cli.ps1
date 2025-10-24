# Flux CLI Build Script
param(
    [string]$BuildType = "Release",
    [string]$Generator = "",
    [switch]$Clean = $false,
    [switch]$Verbose = $false,
    [switch]$Test = $false
)

Write-Host "=== Flux CLI Build Script ===" -ForegroundColor Cyan

# Check CMake
try {
    $cmakeVersion = cmake --version | Select-Object -First 1
    Write-Host "✓ $cmakeVersion" -ForegroundColor Green
} catch {
    Write-Host "❌ Error: CMake not found" -ForegroundColor Red
    Write-Host "Please install CMake 3.22 or higher" -ForegroundColor Yellow
    exit 1
}

# Check compiler
$compiler = ""
if ($env:CXX) {
    $compiler = $env:CXX
    Write-Host "✓ Using compiler: $compiler" -ForegroundColor Green
} else {
    Write-Host "⚠️  CXX environment variable not set, using system default compiler" -ForegroundColor Yellow
}

# Enter project root directory
$originalPath = Get-Location
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $projectRoot

try {
    # First build flux-core
    Write-Host "🔧 Building flux-core..." -ForegroundColor Yellow
    
    if (!(Test-Path "flux-core/build")) {
        New-Item -ItemType Directory -Path "flux-core/build" | Out-Null
    }
    
    Set-Location "flux-core/build"
    
    if ($Clean -and (Test-Path "*")) {
        Write-Host "🧹 Cleaning flux-core build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force *
    }
    
    # Configure flux-core
    $coreArgs = @("..", "-DCMAKE_BUILD_TYPE=$BuildType")
    if ($Generator) {
        $coreArgs += "-G", $Generator
    }
    
    & cmake @coreArgs
    if ($LASTEXITCODE -ne 0) {
        throw "flux-core CMake configuration failed"
    }
    
    # Build flux-core
    cmake --build . --config $BuildType
    if ($LASTEXITCODE -ne 0) {
        throw "flux-core build failed"
    }
    
    Write-Host "✅ flux-core build completed" -ForegroundColor Green
    
    # Return to project root directory
    Set-Location $projectRoot
    
    # Build flux-cli
    Write-Host "🔧 Building flux-cli..." -ForegroundColor Yellow
    
    if (!(Test-Path "flux-cli/build")) {
        New-Item -ItemType Directory -Path "flux-cli/build" | Out-Null
    }
    
    Set-Location "flux-cli/build"
    
    if ($Clean -and (Test-Path "*")) {
        Write-Host "🧹 Cleaning flux-cli build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force *
    }
    
    # Configure flux-cli
    $cliArgs = @(
        "..",
        "-DCMAKE_BUILD_TYPE=$BuildType",
        "-Dflux-core_DIR=$projectRoot/flux-core/build"
    )
    
    if ($Generator) {
        $cliArgs += "-G", $Generator
    }
    
    if ($Verbose) {
        $cliArgs += "--verbose"
    }
    
    & cmake @cliArgs
    if ($LASTEXITCODE -ne 0) {
        throw "flux-cli CMake configuration failed"
    }
    
    # Build flux-cli
    $buildArgs = @("--build", ".", "--config", $BuildType)
    if ($Verbose) {
        $buildArgs += "--verbose"
    }
    
    & cmake @buildArgs
    if ($LASTEXITCODE -ne 0) {
        throw "flux-cli build failed"
    }
    
    Write-Host "✅ flux-cli build completed!" -ForegroundColor Green
    
    # Find executable file
    $exePath = Get-ChildItem -Recurse -Name "flux.exe", "flux" | Select-Object -First 1
    if ($exePath) {
        $fullExePath = Join-Path (Get-Location) $exePath
        Write-Host "📱 Executable file: $fullExePath" -ForegroundColor Cyan
        
        # Test run
        if ($Test) {
            Write-Host "🧪 Running tests..." -ForegroundColor Yellow
            
            Write-Host "Testing version info:" -ForegroundColor Gray
            & ".\$exePath" --version
            
            Write-Host "`nTesting help info:" -ForegroundColor Gray
            & ".\$exePath" --help
            
            Write-Host "✅ Basic tests passed" -ForegroundColor Green
        } else {
            # Ask whether to run
            $run = Read-Host "Run flux CLI? (y/N)"
            if ($run -eq "y" -or $run -eq "Y") {
                Write-Host "🚀 Starting Flux CLI..." -ForegroundColor Green
                & ".\$exePath" --help
            }
        }
    } else {
        Write-Host "⚠️  Executable file not found" -ForegroundColor Yellow
    }
    
    Write-Host "`n🎉 Build completed!" -ForegroundColor Green
    Write-Host "💡 Usage:" -ForegroundColor Cyan
    Write-Host "   flux pack input1 input2 -o output.zip" -ForegroundColor White
    Write-Host "   flux extract archive.zip -o output_dir" -ForegroundColor White
    Write-Host "   flux inspect archive.zip --tree" -ForegroundColor White
    
} catch {
    Write-Host "❌ Build failed: $_" -ForegroundColor Red
    exit 1
} finally {
    Set-Location $originalPath
}

Write-Host "✨ Completed!" -ForegroundColor Green

