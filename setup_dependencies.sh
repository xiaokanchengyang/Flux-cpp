#!/bin/bash

# Flux Archive Manager - Linux/macOS 依赖安装脚本
# 此脚本将安装所需的第三方库

set -e

VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
TRIPLET="x64-linux"

# 检测操作系统
if [[ "$OSTYPE" == "darwin"* ]]; then
    TRIPLET="x64-osx"
    echo "检测到 macOS"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    TRIPLET="x64-linux"
    echo "检测到 Linux"
else
    echo "不支持的操作系统: $OSTYPE"
    exit 1
fi

echo "=== Flux Archive Manager - 依赖安装脚本 ==="
echo "目标平台: $TRIPLET"
echo "vcpkg 路径: $VCPKG_ROOT"

# 检查必需工具
check_command() {
    if ! command -v $1 &> /dev/null; then
        echo "✗ 错误: $1 未安装"
        return 1
    else
        echo "✓ $1 已安装"
        return 0
    fi
}

echo "检查必需工具..."
check_command git || exit 1
check_command cmake || exit 1

if [[ "$OSTYPE" == "darwin"* ]]; then
    check_command clang || exit 1
else
    check_command gcc || check_command clang || exit 1
fi

# 安装系统依赖
echo "安装系统依赖..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS - 使用 Homebrew
    if command -v brew &> /dev/null; then
        echo "使用 Homebrew 安装依赖..."
        brew update
        brew install libzip zlib xz zstd fmt spdlog
    else
        echo "警告: 未检测到 Homebrew，将使用 vcpkg 安装所有依赖"
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux - 检测包管理器
    if command -v apt-get &> /dev/null; then
        echo "使用 apt 安装依赖..."
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            libzip-dev \
            zlib1g-dev \
            liblzma-dev \
            libzstd-dev \
            libfmt-dev \
            libspdlog-dev \
            pkg-config
    elif command -v yum &> /dev/null; then
        echo "使用 yum 安装依赖..."
        sudo yum install -y \
            gcc-c++ \
            libzip-devel \
            zlib-devel \
            xz-devel \
            libzstd-devel \
            fmt-devel \
            spdlog-devel \
            pkgconfig
    elif command -v pacman &> /dev/null; then
        echo "使用 pacman 安装依赖..."
        sudo pacman -S --noconfirm \
            base-devel \
            libzip \
            zlib \
            xz \
            zstd \
            fmt \
            spdlog \
            pkgconf
    else
        echo "警告: 未检测到支持的包管理器，将使用 vcpkg 安装所有依赖"
    fi
fi

# 安装或更新 vcpkg（作为备用方案）
if [ -d "$VCPKG_ROOT" ]; then
    echo "✓ vcpkg 目录已存在，正在更新..."
    cd "$VCPKG_ROOT"
    git pull
else
    echo "正在克隆 vcpkg..."
    git clone https://github.com/Microsoft/vcpkg.git "$VCPKG_ROOT"
    cd "$VCPKG_ROOT"
fi

# 构建 vcpkg
echo "正在构建 vcpkg..."
./bootstrap-vcpkg.sh

# 设置环境变量
echo "设置环境变量..."
export VCPKG_ROOT="$VCPKG_ROOT"
export CMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

# 添加到 shell 配置文件
SHELL_CONFIG=""
if [ -n "$ZSH_VERSION" ]; then
    SHELL_CONFIG="$HOME/.zshrc"
elif [ -n "$BASH_VERSION" ]; then
    SHELL_CONFIG="$HOME/.bashrc"
fi

if [ -n "$SHELL_CONFIG" ]; then
    echo "添加环境变量到 $SHELL_CONFIG..."
    echo "" >> "$SHELL_CONFIG"
    echo "# Flux Archive Manager - vcpkg" >> "$SHELL_CONFIG"
    echo "export VCPKG_ROOT=\"$VCPKG_ROOT\"" >> "$SHELL_CONFIG"
    echo "export CMAKE_TOOLCHAIN_FILE=\"$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake\"" >> "$SHELL_CONFIG"
fi

echo ""
echo "=== 安装完成 ==="
echo "vcpkg 根目录: $VCPKG_ROOT"
echo "CMake 工具链文件: $CMAKE_TOOLCHAIN_FILE"
echo ""
echo "现在可以使用以下命令构建项目:"
echo "mkdir build && cd build"
echo "cmake .. -DCMAKE_TOOLCHAIN_FILE=\"$CMAKE_TOOLCHAIN_FILE\""
echo "cmake --build . --config Release"
echo ""
echo "注意: 请重启终端或运行 'source $SHELL_CONFIG' 以使环境变量生效"

