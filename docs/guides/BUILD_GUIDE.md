# Flux Archive Manager - 构建指南

本指南将帮助您在不同平台上构建 Flux Archive Manager。

## 系统要求

### 通用要求
- **CMake** 3.22 或更高版本
- **C++20** 兼容编译器
- **Git** (用于获取依赖)

### Windows
- **Visual Studio 2019/2022** 或 **Visual Studio Build Tools**
- **PowerShell** (用于运行安装脚本)

### Linux
- **GCC 10+** 或 **Clang 12+**
- **包管理器**: apt, yum, 或 pacman

### macOS
- **Xcode** 或 **Xcode Command Line Tools**
- **Homebrew** (推荐)

## 快速开始

### 1. 克隆项目
```bash
git clone <repository-url>
cd Flux-cpp
```

### 2. 安装依赖

#### Windows (PowerShell)
```powershell
# 以管理员身份运行 PowerShell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
.\setup_dependencies.ps1
```

#### Linux/macOS
```bash
./setup_dependencies.sh
```

### 3. 构建项目
```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE"
cmake --build . --config Release
```

## 详细安装步骤

### Windows 详细步骤

1. **安装 Visual Studio**
   - 下载 [Visual Studio Community 2022](https://visualstudio.microsoft.com/vs/community/)
   - 安装时选择 "C++ 桌面开发" 工作负载

2. **运行依赖安装脚本**
   ```powershell
   .\setup_dependencies.ps1
   ```
   
   脚本将自动：
   - 安装 vcpkg 包管理器
   - 安装所需的第三方库：
     - libzip (ZIP 支持)
     - zlib (GZIP 压缩)
     - liblzma (XZ 压缩)
     - zstd (ZSTD 压缩)
     - fmt (字符串格式化)
     - spdlog (日志记录)

3. **构建项目**
   ```powershell
   mkdir build
   cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
   cmake --build . --config Release
   ```

### Linux 详细步骤

1. **安装系统依赖**
   
   **Ubuntu/Debian:**
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential cmake git \
       libzip-dev zlib1g-dev liblzma-dev libzstd-dev \
       libfmt-dev libspdlog-dev pkg-config
   ```
   
   **CentOS/RHEL/Fedora:**
   ```bash
   sudo yum install -y gcc-c++ cmake git \
       libzip-devel zlib-devel xz-devel libzstd-devel \
       fmt-devel spdlog-devel pkgconfig
   ```
   
   **Arch Linux:**
   ```bash
   sudo pacman -S base-devel cmake git \
       libzip zlib xz zstd fmt spdlog pkgconf
   ```

2. **运行安装脚本（可选）**
   ```bash
   ./setup_dependencies.sh
   ```

3. **构建项目**
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

### macOS 详细步骤

1. **安装 Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

2. **安装 Homebrew**
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

3. **安装依赖**
   ```bash
   brew install cmake git libzip zlib xz zstd fmt spdlog
   ```

4. **构建项目**
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(sysctl -n hw.ncpu)
   ```

## 手动依赖管理

如果自动安装脚本失败，您可以手动安装依赖：

### 使用 vcpkg (推荐)

1. **安装 vcpkg**
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh  # Linux/macOS
   # 或
   .\bootstrap-vcpkg.bat  # Windows
   ```

2. **安装依赖包**
   ```bash
   ./vcpkg install libzip zlib liblzma zstd fmt spdlog
   ```

3. **配置 CMake**
   ```bash
   cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```

### 使用系统包管理器

大多数 Linux 发行版和 macOS (通过 Homebrew) 都提供了预编译的包。

## 构建选项

### CMake 选项
- `CMAKE_BUILD_TYPE`: 构建类型 (Debug, Release, RelWithDebInfo)
- `BUILD_TESTING`: 启用测试 (默认: ON)
- `BUILD_GUI`: 构建 GUI 应用 (默认: ON)

### 示例构建命令
```bash
# Release 构建
cmake .. -DCMAKE_BUILD_TYPE=Release

# Debug 构建
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 禁用 GUI
cmake .. -DBUILD_GUI=OFF

# 禁用测试
cmake .. -DBUILD_TESTING=OFF
```

## 故障排除

### 常见问题

1. **找不到第三方库**
   - 确保已正确安装所有依赖
   - 检查 `CMAKE_TOOLCHAIN_FILE` 是否正确设置
   - 尝试清理构建目录并重新构建

2. **编译错误**
   - 确保使用 C++20 兼容编译器
   - 检查编译器版本是否满足要求

3. **链接错误**
   - 确保所有依赖库都已正确安装
   - 检查库的版本兼容性

### 获取帮助

如果遇到问题，请：
1. 检查构建日志中的详细错误信息
2. 确认所有依赖都已正确安装
3. 查看项目的 Issues 页面
4. 提交新的 Issue 并包含详细的错误信息

## 验证安装

构建完成后，您可以运行以下命令验证安装：

```bash
# 运行 CLI 工具
./flux-cli --help

# 测试 ZIP 解压
./flux-cli extract test.zip -o output/

# 测试 ZIP 创建
./flux-cli pack file1.txt file2.txt -o test.zip
```

## 开发环境设置

### Visual Studio Code
推荐安装以下扩展：
- C/C++ Extension Pack
- CMake Tools
- GitLens

### CLion
CLion 原生支持 CMake 项目，只需打开项目根目录即可。

### Visual Studio
使用 "打开文件夹" 功能打开项目根目录，Visual Studio 会自动检测 CMake 项目。

