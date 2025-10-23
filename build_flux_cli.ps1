# Flux CLI 构建脚本
param(
    [string]$BuildType = "Release",
    [string]$Generator = "",
    [switch]$Clean = $false,
    [switch]$Verbose = $false,
    [switch]$Test = $false
)

Write-Host "=== Flux CLI 构建脚本 ===" -ForegroundColor Cyan

# 检查 CMake
try {
    $cmakeVersion = cmake --version | Select-Object -First 1
    Write-Host "✓ $cmakeVersion" -ForegroundColor Green
} catch {
    Write-Host "❌ 错误: 未找到 CMake" -ForegroundColor Red
    Write-Host "请安装 CMake 3.22 或更高版本" -ForegroundColor Yellow
    exit 1
}

# 检查编译器
$compiler = ""
if ($env:CXX) {
    $compiler = $env:CXX
    Write-Host "✓ 使用编译器: $compiler" -ForegroundColor Green
} else {
    Write-Host "⚠️  未设置 CXX 环境变量，将使用系统默认编译器" -ForegroundColor Yellow
}

# 进入项目根目录
$originalPath = Get-Location
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $projectRoot

try {
    # 首先构建 flux-core
    Write-Host "🔧 构建 flux-core..." -ForegroundColor Yellow
    
    if (!(Test-Path "flux-core/build")) {
        New-Item -ItemType Directory -Path "flux-core/build" | Out-Null
    }
    
    Set-Location "flux-core/build"
    
    if ($Clean -and (Test-Path "*")) {
        Write-Host "🧹 清理 flux-core 构建目录..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force *
    }
    
    # 配置 flux-core
    $coreArgs = @("..", "-DCMAKE_BUILD_TYPE=$BuildType")
    if ($Generator) {
        $coreArgs += "-G", $Generator
    }
    
    & cmake @coreArgs
    if ($LASTEXITCODE -ne 0) {
        throw "flux-core CMake 配置失败"
    }
    
    # 构建 flux-core
    cmake --build . --config $BuildType
    if ($LASTEXITCODE -ne 0) {
        throw "flux-core 构建失败"
    }
    
    Write-Host "✅ flux-core 构建完成" -ForegroundColor Green
    
    # 返回项目根目录
    Set-Location $projectRoot
    
    # 构建 flux-cli
    Write-Host "🔧 构建 flux-cli..." -ForegroundColor Yellow
    
    if (!(Test-Path "flux-cli/build")) {
        New-Item -ItemType Directory -Path "flux-cli/build" | Out-Null
    }
    
    Set-Location "flux-cli/build"
    
    if ($Clean -and (Test-Path "*")) {
        Write-Host "🧹 清理 flux-cli 构建目录..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force *
    }
    
    # 配置 flux-cli
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
        throw "flux-cli CMake 配置失败"
    }
    
    # 构建 flux-cli
    $buildArgs = @("--build", ".", "--config", $BuildType)
    if ($Verbose) {
        $buildArgs += "--verbose"
    }
    
    & cmake @buildArgs
    if ($LASTEXITCODE -ne 0) {
        throw "flux-cli 构建失败"
    }
    
    Write-Host "✅ flux-cli 构建完成!" -ForegroundColor Green
    
    # 查找可执行文件
    $exePath = Get-ChildItem -Recurse -Name "flux.exe", "flux" | Select-Object -First 1
    if ($exePath) {
        $fullExePath = Join-Path (Get-Location) $exePath
        Write-Host "📱 可执行文件: $fullExePath" -ForegroundColor Cyan
        
        # 测试运行
        if ($Test) {
            Write-Host "🧪 运行测试..." -ForegroundColor Yellow
            
            Write-Host "测试版本信息:" -ForegroundColor Gray
            & ".\$exePath" --version
            
            Write-Host "`n测试帮助信息:" -ForegroundColor Gray
            & ".\$exePath" --help
            
            Write-Host "✅ 基本测试通过" -ForegroundColor Green
        } else {
            # 询问是否运行
            $run = Read-Host "是否运行 flux CLI? (y/N)"
            if ($run -eq "y" -or $run -eq "Y") {
                Write-Host "🚀 启动 Flux CLI..." -ForegroundColor Green
                & ".\$exePath" --help
            }
        }
    } else {
        Write-Host "⚠️  未找到可执行文件" -ForegroundColor Yellow
    }
    
    Write-Host "`n🎉 构建完成!" -ForegroundColor Green
    Write-Host "💡 使用方法:" -ForegroundColor Cyan
    Write-Host "   flux pack input1 input2 -o output.zip" -ForegroundColor White
    Write-Host "   flux extract archive.zip -o output_dir" -ForegroundColor White
    Write-Host "   flux inspect archive.zip --tree" -ForegroundColor White
    
} catch {
    Write-Host "❌ 构建失败: $_" -ForegroundColor Red
    exit 1
} finally {
    Set-Location $originalPath
}

Write-Host "✨ 完成!" -ForegroundColor Green

