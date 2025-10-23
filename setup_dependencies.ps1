# Flux Archive Manager - Windows 依赖安装脚本
# 此脚本将安装 vcpkg 并配置所需的第三方库

param(
    [string]$VcpkgRoot = "C:\vcpkg",
    [string]$Triplet = "x64-windows"
)

Write-Host "=== Flux Archive Manager - 依赖安装脚本 ===" -ForegroundColor Green
Write-Host "目标平台: $Triplet" -ForegroundColor Yellow
Write-Host "vcpkg 路径: $VcpkgRoot" -ForegroundColor Yellow

# 检查是否已安装 Git
try {
    $gitVersion = git --version
    Write-Host "✓ Git 已安装: $gitVersion" -ForegroundColor Green
} catch {
    Write-Host "✗ 错误: 需要安装 Git" -ForegroundColor Red
    Write-Host "请从 https://git-scm.com/ 下载并安装 Git" -ForegroundColor Yellow
    exit 1
}

# 检查是否已安装 Visual Studio 或 Build Tools
$vsInstallations = @()
if (Test-Path "C:\Program Files\Microsoft Visual Studio\2022") {
    $vsInstallations += "Visual Studio 2022"
}
if (Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2019") {
    $vsInstallations += "Visual Studio 2019"
}

if ($vsInstallations.Count -eq 0) {
    Write-Host "✗ 警告: 未检测到 Visual Studio" -ForegroundColor Yellow
    Write-Host "请确保已安装 Visual Studio 2019 或 2022 (包含 C++ 工具)" -ForegroundColor Yellow
} else {
    Write-Host "✓ 检测到: $($vsInstallations -join ', ')" -ForegroundColor Green
}

# 安装或更新 vcpkg
if (Test-Path $VcpkgRoot) {
    Write-Host "✓ vcpkg 目录已存在，正在更新..." -ForegroundColor Yellow
    Push-Location $VcpkgRoot
    git pull
    Pop-Location
} else {
    Write-Host "正在克隆 vcpkg..." -ForegroundColor Yellow
    git clone https://github.com/Microsoft/vcpkg.git $VcpkgRoot
}

# 构建 vcpkg
Write-Host "正在构建 vcpkg..." -ForegroundColor Yellow
Push-Location $VcpkgRoot
& .\bootstrap-vcpkg.bat
if ($LASTEXITCODE -ne 0) {
    Write-Host "✗ vcpkg 构建失败" -ForegroundColor Red
    Pop-Location
    exit 1
}

# 集成 vcpkg 到 Visual Studio
Write-Host "正在集成 vcpkg 到 Visual Studio..." -ForegroundColor Yellow
& .\vcpkg.exe integrate install

# 安装依赖包
$packages = @(
    "libzip",
    "zlib", 
    "liblzma",
    "zstd",
    "fmt",
    "spdlog"
)

Write-Host "正在安装依赖包..." -ForegroundColor Yellow
foreach ($package in $packages) {
    Write-Host "安装 $package..." -ForegroundColor Cyan
    & .\vcpkg.exe install "${package}:${Triplet}"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "✗ 安装 $package 失败" -ForegroundColor Red
    } else {
        Write-Host "✓ $package 安装成功" -ForegroundColor Green
    }
}

Pop-Location

# 设置环境变量
Write-Host "设置环境变量..." -ForegroundColor Yellow
$env:VCPKG_ROOT = $VcpkgRoot
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", $VcpkgRoot, "User")

# 创建 CMake 工具链文件路径
$toolchainFile = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
[Environment]::SetEnvironmentVariable("CMAKE_TOOLCHAIN_FILE", $toolchainFile, "User")

Write-Host ""
Write-Host "=== 安装完成 ===" -ForegroundColor Green
Write-Host "vcpkg 根目录: $VcpkgRoot" -ForegroundColor Yellow
Write-Host "CMake 工具链文件: $toolchainFile" -ForegroundColor Yellow
Write-Host ""
Write-Host "现在可以使用以下命令构建项目:" -ForegroundColor Cyan
Write-Host "mkdir build && cd build" -ForegroundColor White
Write-Host "cmake .. -DCMAKE_TOOLCHAIN_FILE=`"$toolchainFile`"" -ForegroundColor White
Write-Host "cmake --build . --config Release" -ForegroundColor White
Write-Host ""
Write-Host "注意: 请重启终端以使环境变量生效" -ForegroundColor Yellow

