# Flux Archive Manager

<div align="center">

![Flux Logo](flux-gui/resources/icons/flux.png)

**A modern, cross-platform archive management solution built with C++23**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()

</div>

## 🚀 Overview

Flux Archive Manager is a comprehensive, modern archive management solution that provides both graphical and command-line interfaces for handling various archive formats. Built with cutting-edge C++23 features, it offers professional-grade functionality with an intuitive user experience.

## ✨ Features

### 🎯 **Core Functionality**
- **Multi-format Support**: ZIP, TAR, TAR.GZ, TAR.XZ, TAR.ZSTD, and 7-Zip archives
- **Real-time Progress Tracking**: Visual progress indicators with cancellation support
- **Integrity Verification**: Comprehensive archive validation and error detection
- **Smart Format Detection**: Automatic format detection from file headers and extensions
- **Partial Extraction**: Extract specific files or directories with pattern matching

### 🖥️ **Graphical Interface (Qt6)**
- **Modern UI Design**: Clean, intuitive interface with dark/light theme support
- **Drag & Drop Support**: Easy file management with visual feedback
- **Archive Explorer**: Browse archive contents with detailed metadata
- **Compression Wizard**: Step-by-step archive creation with preset configurations
- **Context Menus**: Right-click operations for quick access to functions
- **Multi-tab Interface**: Work with multiple archives simultaneously

### 💻 **Command Line Interface**
- **Powerful CLI**: Full-featured command-line tool for automation and scripting
- **Batch Operations**: Process multiple archives with single commands
- **JSON Output**: Machine-readable output for integration with other tools
- **Progress Indicators**: Terminal-based progress bars and status updates
- **Cross-platform**: Consistent behavior across Windows, Linux, and macOS

### 🔧 **Advanced Features**
- **Password Protection**: Support for encrypted archives (framework ready)
- **Compression Levels**: Configurable compression settings for optimal size/speed balance
- **Memory Efficient**: Streaming operations for handling large archives
- **Error Recovery**: Robust error handling with detailed diagnostic messages
- **Plugin Architecture**: Extensible design for future format support

## 📦 Installation

### Prerequisites

- **C++23 compatible compiler** (GCC 13+, Clang 16+, MSVC 2022+)
- **CMake 3.22+**
- **vcpkg** (for dependency management)
- **Qt6** (for GUI application)

### Dependencies

The project uses the following libraries:
- **libzip** - ZIP format support
- **libarchive** - TAR format support
- **zlib, liblzma, libzstd** - Compression algorithms
- **Qt6** - GUI framework
- **CLI11** - Command line parsing
- **spdlog** - High-performance logging
- **fmt** - String formatting

### Build Instructions

#### 1. Clone the Repository
```bash
git clone https://github.com/your-username/Flux-cpp.git
cd Flux-cpp
```

#### 2. Setup Dependencies
```bash
# Windows (PowerShell)
.\setup_dependencies.ps1

# Linux/macOS
./setup_dependencies.sh
```

#### 3. Build the Project
```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build . --config Release

# Install (optional)
cmake --install . --prefix /usr/local
```

#### 4. Quick Build Scripts
```bash
# Build CLI only
.\build_flux_cli.ps1

# Build GUI
.\build_gui.bat

# Build everything
.\build.bat  # Windows
./build.sh   # Linux/macOS
```

## 🎮 Usage

### Command Line Interface

#### Basic Operations
```bash
# Extract an archive
flux extract archive.zip -o /path/to/output

# Create an archive
flux pack -f zip -o archive.zip file1.txt file2.txt folder/

# Inspect archive contents
flux inspect archive.tar.gz --json

# Batch operations
flux batch --extract *.zip --output-dir ./extracted/
```

#### Advanced Usage
```bash
# Extract with progress and specific files
flux extract large-archive.tar.xz -o ./output --progress --pattern "*.cpp"

# Create compressed archive with custom settings
flux pack -f tar.xz -o backup.tar.xz --compression-level 6 ./project/

# Verify archive integrity
flux verify archive.zip --detailed
```

### Graphical Interface

1. **Launch the GUI**: Run `FluxGUI.exe` (Windows) or `flux-gui` (Linux/macOS)
2. **Open Archives**: Drag & drop archives or use File → Open
3. **Extract Files**: Select files and click Extract, or use context menu
4. **Create Archives**: Use the Compression tab to create new archives
5. **Browse Contents**: Double-click to navigate directories within archives

## 📁 Project Structure

```
Flux-cpp/
├── docs/                          # Documentation
│   ├── guides/                    # User guides and tutorials
│   ├── implementation/            # Technical implementation docs
│   └── summaries/                 # Project summaries and reports
├── flux-core/                     # Core library
│   ├── include/flux-core/         # Public headers
│   └── src/
│       ├── core/                  # Main functionality (flux.cpp, extractor.cpp, packer.cpp)
│       ├── utils/                 # Utilities (archive_utils.cpp, format_detector.cpp)
│       └── formats/               # Format-specific implementations
│           ├── extractors/        # Extraction implementations
│           └── packers/           # Packing implementations
├── flux-cli/                      # Command line application
│   └── src/
│       ├── application/           # Main CLI app (main.cpp, cli_app.cpp)
│       ├── commands/              # Command implementations
│       ├── utils/                 # CLI utilities
│       └── platform/              # Platform-specific code
├── flux-gui/                      # GUI application
│   ├── resources/                 # Icons, themes, configurations
│   └── src/
│       ├── application/           # Main GUI app (main.cpp)
│       ├── ui/
│       │   ├── windows/           # Main windows
│       │   ├── components/        # Reusable UI components
│       │   ├── widgets/           # Custom widgets
│       │   └── views/             # Application views
│       ├── core/                  # GUI core functionality
│       ├── models/                # Data models
│       └── utils/                 # GUI utilities
└── third-party/                   # Third-party dependencies
```

## 🛠️ Development

### Code Style
- **Modern C++23**: Uses latest language features including `std::expected`, `std::span`
- **RAII**: Proper resource management with smart pointers
- **Error Handling**: Comprehensive error handling with meaningful messages
- **Documentation**: Inline documentation and comprehensive README files

### Contributing
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Testing
```bash
# Run tests
cd build
ctest --output-on-failure

# Run specific tests
./flux-core/tests/test_archive_utils
```

## 📊 Performance

- **Memory Efficient**: Streaming operations minimize memory usage
- **Fast Processing**: Optimized algorithms for large archive handling
- **Parallel Operations**: Multi-threaded extraction and compression
- **Progress Tracking**: Real-time progress without performance impact

## 🔒 Security

- **Input Validation**: Comprehensive validation of archive contents
- **Path Traversal Protection**: Prevents directory traversal attacks
- **Memory Safety**: Modern C++ practices prevent buffer overflows
- **Error Handling**: Graceful handling of malformed archives

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **libzip** - ZIP format support
- **libarchive** - TAR format support  
- **Qt6** - Cross-platform GUI framework
- **CLI11** - Modern command line parsing
- **spdlog** - High-performance logging
- **vcpkg** - C++ package management

## 📞 Support

- **Documentation**: Check the [docs/](docs/) directory for detailed guides
- **Issues**: Report bugs and feature requests on GitHub Issues
- **Discussions**: Join community discussions on GitHub Discussions

---

<div align="center">

**Built with ❤️ using Modern C++23**

[Documentation](docs/) • [Contributing](docs/guides/CONTRIBUTING.md) • [License](LICENSE)

</div>