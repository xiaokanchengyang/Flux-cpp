#!/bin/bash

# Flux Cross-Platform Build Script
# This script builds the Flux archive manager on Linux and macOS

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"
CLEAN_BUILD=false
VERBOSE=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            echo "Flux Cross-Platform Build Script"
            echo ""
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -h, --help              Show this help message"
            echo "  -d, --debug             Build in Debug mode (default: Release)"
            echo "  -c, --clean             Clean build directory before building"
            echo "  -v, --verbose           Enable verbose output"
            echo "  -j, --jobs N            Number of parallel jobs (default: auto-detect)"
            echo "  -p, --prefix PATH       Installation prefix (default: /usr/local)"
            echo "  --build-dir DIR         Build directory (default: build)"
            echo ""
            echo "Examples:"
            echo "  $0                      # Build in Release mode"
            echo "  $0 -d -c               # Clean Debug build"
            echo "  $0 -j 8 -v             # Verbose build with 8 jobs"
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -p|--prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Detect platform
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
else
    print_error "Unsupported platform: $OSTYPE"
    exit 1
fi

print_status "Building Flux Archive Manager for $PLATFORM"
print_status "Build type: $BUILD_TYPE"
print_status "Build directory: $BUILD_DIR"
print_status "Install prefix: $INSTALL_PREFIX"
print_status "Parallel jobs: $JOBS"

# Check for required tools
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        print_error "$1 is required but not installed"
        exit 1
    fi
}

print_status "Checking required tools..."
check_tool cmake
check_tool make

# Check for Qt6
if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
    print_error "Qt6 is required but not found"
    print_error "Please install Qt6 development packages"
    exit 1
fi

# Clean build directory if requested
if [[ "$CLEAN_BUILD" == true ]]; then
    print_status "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_status "Configuring with CMake..."
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
)

if [[ "$VERBOSE" == true ]]; then
    CMAKE_ARGS+=(-DCMAKE_VERBOSE_MAKEFILE=ON)
fi

# Platform-specific configuration
if [[ "$PLATFORM" == "macOS" ]]; then
    CMAKE_ARGS+=(
        -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
        -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
    )
fi

cmake "${CMAKE_ARGS[@]}" ..

# Build
print_status "Building..."
MAKE_ARGS=(-j "$JOBS")

if [[ "$VERBOSE" == true ]]; then
    MAKE_ARGS+=(VERBOSE=1)
fi

make "${MAKE_ARGS[@]}"

print_success "Build completed successfully!"

# Show build results
print_status "Build results:"
if [[ -f "flux-gui/FluxGUI" ]]; then
    echo "  GUI application: flux-gui/FluxGUI"
elif [[ -f "flux-gui/FluxGUI.app/Contents/MacOS/FluxGUI" ]]; then
    echo "  GUI application: flux-gui/FluxGUI.app"
fi

if [[ -f "flux-cli/flux-cli" ]]; then
    echo "  CLI application: flux-cli/flux-cli"
fi

# Offer to install
echo ""
read -p "Do you want to install Flux? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_status "Installing..."
    make install
    print_success "Installation completed!"
    
    if [[ "$PLATFORM" == "Linux" ]]; then
        print_status "Updating desktop database..."
        if command -v update-desktop-database &> /dev/null; then
            update-desktop-database ~/.local/share/applications/ 2>/dev/null || true
        fi
        
        if command -v update-mime-database &> /dev/null; then
            update-mime-database ~/.local/share/mime/ 2>/dev/null || true
        fi
    fi
else
    print_status "You can install later by running: make install"
fi

print_success "All done! 🎉"























