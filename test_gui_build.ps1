# Flux GUI 测试构建脚本
param(
    [string]$BuildType = "Debug",
    [string]$QtPath = "",
    [switch]$Clean = $false
)

Write-Host "=== Flux GUI 测试构建脚本 ===" -ForegroundColor Cyan

# 检查 Qt 安装
if ([string]::IsNullOrEmpty($QtPath)) {
    $QtPath = $env:Qt6_DIR
    if ([string]::IsNullOrEmpty($QtPath)) {
        Write-Host "❌ 错误: 未找到 Qt6 安装路径" -ForegroundColor Red
        Write-Host "请设置 Qt6_DIR 环境变量或使用 -QtPath 参数" -ForegroundColor Yellow
        exit 1
    }
}

Write-Host "Qt6 路径: $QtPath" -ForegroundColor Green

# 检查 CMake
try {
    $cmakeVersion = cmake --version | Select-Object -First 1
    Write-Host "✓ $cmakeVersion" -ForegroundColor Green
} catch {
    Write-Host "❌ 错误: 未找到 CMake" -ForegroundColor Red
    exit 1
}

# 进入 flux-gui 目录
$originalPath = Get-Location
Set-Location "flux-gui"

try {
    # 清理构建目录
    if ($Clean -and (Test-Path "build")) {
        Write-Host "🧹 清理构建目录..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force "build"
    }

    # 创建构建目录
    if (!(Test-Path "build")) {
        New-Item -ItemType Directory -Name "build" | Out-Null
    }

    Set-Location "build"

    # 配置项目
    Write-Host "🔧 配置项目..." -ForegroundColor Yellow
    $cmakeArgs = @(
        "..",
        "-DCMAKE_BUILD_TYPE=$BuildType",
        "-DQt6_DIR=$QtPath"
    )

    if ($env:CMAKE_TOOLCHAIN_FILE) {
        $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$env:CMAKE_TOOLCHAIN_FILE"
    }

    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake 配置失败"
    }

    # 构建项目
    Write-Host "🔨 构建项目..." -ForegroundColor Yellow
    cmake --build . --config $BuildType
    if ($LASTEXITCODE -ne 0) {
        throw "构建失败"
    }

    Write-Host "✅ 构建成功!" -ForegroundColor Green
    
    # 显示可执行文件位置
    $exePath = Get-ChildItem -Recurse -Name "FluxGUI.exe" | Select-Object -First 1
    if ($exePath) {
        Write-Host "📱 可执行文件: $exePath" -ForegroundColor Cyan
        
        # 询问是否运行
        $run = Read-Host "是否运行应用程序? (y/N)"
        if ($run -eq "y" -or $run -eq "Y") {
            Write-Host "🚀 启动 Flux GUI..." -ForegroundColor Green
            & ".\$exePath"
        }
    }

} catch {
    Write-Host "❌ 构建失败: $_" -ForegroundColor Red
    exit 1
} finally {
    Set-Location $originalPath
}

Write-Host "✨ 完成!" -ForegroundColor Green

