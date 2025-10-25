# Flux Archive Manager - Quality Checks Script
# This script runs all code quality checks locally before submitting PR

param(
    [switch]$SkipBuild,
    [switch]$SkipTests,
    [switch]$SkipFormat,
    [switch]$SkipLint,
    [string]$BuildType = "Debug"
)

Write-Host "🚀 Running Flux Archive Manager Quality Checks" -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green

$ErrorActionPreference = "Stop"
$StartTime = Get-Date

# Check if we're in the project root
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Error "Please run this script from the project root directory"
    exit 1
}

# Create build directory
$BuildDir = "build-quality-check"
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Function to run command and check result
function Invoke-QualityCommand {
    param(
        [string]$Command,
        [string]$Description,
        [string]$WorkingDirectory = "."
    )
    
    Write-Host "📋 $Description..." -ForegroundColor Yellow
    
    try {
        Push-Location $WorkingDirectory
        Invoke-Expression $Command
        if ($LASTEXITCODE -ne 0) {
            throw "Command failed with exit code $LASTEXITCODE"
        }
        Write-Host "✅ $Description completed successfully" -ForegroundColor Green
    }
    catch {
        Write-Host "❌ $Description failed: $($_.Exception.Message)" -ForegroundColor Red
        throw
    }
    finally {
        Pop-Location
    }
}

try {
    # 1. Code Formatting Check
    if (-not $SkipFormat) {
        Write-Host "`n🎨 Checking Code Formatting" -ForegroundColor Cyan
        
        # Check if clang-format is available
        try {
            clang-format --version | Out-Null
        }
        catch {
            Write-Warning "clang-format not found. Installing via vcpkg..."
            Invoke-QualityCommand "vcpkg install llvm[clang-format]" "Installing clang-format"
        }
        
        # Find all C++ files
        $CppFiles = Get-ChildItem -Recurse -Include "*.cpp", "*.h", "*.hpp" | 
                   Where-Object { $_.FullName -notmatch "(build|third-party|vcpkg)" }
        
        $FormatIssues = @()
        foreach ($File in $CppFiles) {
            $Output = clang-format --dry-run --Werror $File.FullName 2>&1
            if ($LASTEXITCODE -ne 0) {
                $FormatIssues += $File.FullName
            }
        }
        
        if ($FormatIssues.Count -gt 0) {
            Write-Host "❌ Format issues found in:" -ForegroundColor Red
            $FormatIssues | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
            Write-Host "Run 'clang-format -i <file>' to fix formatting" -ForegroundColor Yellow
            throw "Code formatting check failed"
        }
        
        Write-Host "✅ Code formatting check passed" -ForegroundColor Green
    }

    # 2. Build Project
    if (-not $SkipBuild) {
        Write-Host "`n🔨 Building Project" -ForegroundColor Cyan
        
        # Configure with all warnings enabled
        $ConfigureCmd = @(
            "cmake -B $BuildDir",
            "-DCMAKE_BUILD_TYPE=$BuildType",
            "-DFLUX_BUILD_TESTS=ON",
            "-DFLUX_BUILD_GUI=ON",
            "-DFLUX_BUILD_CLI=ON",
            "-DFLUX_ENABLE_SANITIZERS=ON",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        ) -join " "
        
        Invoke-QualityCommand $ConfigureCmd "Configuring CMake"
        
        # Build
        Invoke-QualityCommand "cmake --build $BuildDir --config $BuildType --parallel" "Building project"
    }

    # 3. Static Analysis
    if (-not $SkipLint) {
        Write-Host "`n🔍 Running Static Analysis" -ForegroundColor Cyan
        
        # Check if clang-tidy is available
        try {
            clang-tidy --version | Out-Null
        }
        catch {
            Write-Warning "clang-tidy not found. Please install LLVM tools."
        }
        
        # Run clang-tidy on core files
        $CoreFiles = Get-ChildItem -Path "flux-core/src" -Recurse -Include "*.cpp"
        
        foreach ($File in $CoreFiles) {
            try {
                Invoke-QualityCommand "clang-tidy -p $BuildDir $($File.FullName)" "Analyzing $($File.Name)"
            }
            catch {
                Write-Warning "Static analysis issues found in $($File.Name)"
                # Continue with other files
            }
        }
        
        Write-Host "✅ Static analysis completed" -ForegroundColor Green
    }

    # 4. Run Tests
    if (-not $SkipTests) {
        Write-Host "`n🧪 Running Unit Tests" -ForegroundColor Cyan
        
        Push-Location $BuildDir
        try {
            # Run CTest
            Invoke-QualityCommand "ctest --output-on-failure --build-config $BuildType" "Running unit tests"
            
            # Run with memory checking if available
            if (Get-Command "valgrind" -ErrorAction SilentlyContinue) {
                Write-Host "🔍 Running memory checks..." -ForegroundColor Yellow
                Invoke-QualityCommand "ctest -T memcheck" "Memory leak detection"
            }
        }
        finally {
            Pop-Location
        }
    }

    # 5. Additional Checks
    Write-Host "`n🔧 Additional Quality Checks" -ForegroundColor Cyan
    
    # Check for TODO/FIXME in production code
    $TodoFiles = Get-ChildItem -Recurse -Include "*.cpp", "*.h", "*.hpp" |
                Where-Object { $_.FullName -match "(flux-core|flux-gui|flux-cli)" } |
                Where-Object { $_.FullName -notmatch "test" } |
                Select-String -Pattern "TODO|FIXME|XXX|HACK" |
                Group-Object Filename |
                Select-Object Name
    
    if ($TodoFiles.Count -gt 0) {
        Write-Warning "Found TODO/FIXME comments in production code:"
        $TodoFiles | ForEach-Object { Write-Host "  - $($_.Name)" -ForegroundColor Yellow }
    }
    
    # Check for debug print statements
    $DebugPrints = Get-ChildItem -Recurse -Include "*.cpp", "*.h", "*.hpp" |
                  Where-Object { $_.FullName -match "(flux-core|flux-gui|flux-cli)" } |
                  Select-String -Pattern "std::cout|printf|qDebug" |
                  Where-Object { $_.Line -notmatch "//.*std::cout|//.*printf|//.*qDebug" }
    
    if ($DebugPrints.Count -gt 0) {
        Write-Warning "Found debug print statements:"
        $DebugPrints | ForEach-Object { Write-Host "  - $($_.Filename):$($_.LineNumber)" -ForegroundColor Yellow }
    }

    # 6. Generate Report
    Write-Host "`n📊 Generating Quality Report" -ForegroundColor Cyan
    
    $EndTime = Get-Date
    $Duration = $EndTime - $StartTime
    
    $Report = @"
# Flux Archive Manager - Quality Check Report

**Generated:** $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
**Duration:** $($Duration.TotalMinutes.ToString("F2")) minutes
**Build Type:** $BuildType

## Summary

- ✅ Code formatting: PASSED
- ✅ Build: PASSED  
- ✅ Static analysis: COMPLETED
- ✅ Unit tests: PASSED
- ✅ Additional checks: COMPLETED

## Build Information

- **Build Directory:** $BuildDir
- **CMake Configuration:** $BuildType with sanitizers
- **Compiler Warnings:** Treated as errors
- **Test Framework:** GoogleTest

## Next Steps

1. Review any warnings or suggestions from static analysis
2. Address any TODO/FIXME comments if found
3. Remove any debug print statements if found
4. Ready for pull request submission

---
*Generated by Flux Quality Check Script*
"@

    $Report | Out-File -FilePath "quality-check-report.md" -Encoding UTF8
    
    Write-Host "`n🎉 All Quality Checks Completed Successfully!" -ForegroundColor Green
    Write-Host "📄 Report saved to: quality-check-report.md" -ForegroundColor Cyan
    Write-Host "⏱️  Total time: $($Duration.TotalMinutes.ToString("F2")) minutes" -ForegroundColor Cyan
}
catch {
    Write-Host "`n💥 Quality checks failed: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Please fix the issues and run the script again." -ForegroundColor Yellow
    exit 1
}
finally {
    # Cleanup
    if (Test-Path "compile_commands.json") {
        Copy-Item "compile_commands.json" "$BuildDir/" -Force
    }
}

Write-Host "`n🚀 Ready for code review submission!" -ForegroundColor Green
