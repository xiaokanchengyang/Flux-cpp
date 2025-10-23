#!/bin/bash
# Flux Archive Manager Demo Script
# Demonstrates current functionality and build process

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
GRAY='\033[0;37m'
NC='\033[0m' # No Color

# Parse command line arguments
BUILD_ONLY=false
TEST_ONLY=false
SHOW_GUI=false
CLEAN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        --test-only)
            TEST_ONLY=true
            shift
            ;;
        --show-gui)
            SHOW_GUI=true
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --build-only    Only build the project"
            echo "  --test-only     Only run tests (skip build)"
            echo "  --show-gui      Launch GUI application"
            echo "  --clean         Clean build directory first"
            echo "  -h, --help      Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo -e "${CYAN}🚀 Flux Archive Manager Demo${NC}"
echo -e "${CYAN}=================================${NC}"

# Check if we're in the right directory
if [[ ! -f "CMakeLists.txt" ]]; then
    echo -e "${RED}❌ Error: Please run this script from the project root directory${NC}"
    exit 1
fi

# Clean build directory if requested
if [[ "$CLEAN" == true ]]; then
    echo -e "${YELLOW}🧹 Cleaning build directory...${NC}"
    rm -rf build
    echo -e "${GREEN}✅ Build directory cleaned${NC}"
fi

# Build the project
if [[ "$TEST_ONLY" != true ]]; then
    echo -e "\n${YELLOW}📦 Building Flux Archive Manager...${NC}"
    
    # Check for required tools
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}❌ CMake not found. Please install CMake first.${NC}"
        echo -e "${YELLOW}💡 Installation suggestions:${NC}"
        echo -e "${GRAY}   Ubuntu/Debian: sudo apt install cmake ninja-build${NC}"
        echo -e "${GRAY}   macOS: brew install cmake ninja${NC}"
        exit 1
    fi
    
    # Configure with CMake
    echo -e "${GRAY}Configuring with CMake...${NC}"
    if ! cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Ninja" > /dev/null 2>&1; then
        echo -e "${RED}❌ CMake configuration failed${NC}"
        echo -e "${YELLOW}💡 Make sure you have Qt6 and other dependencies installed:${NC}"
        echo -e "${GRAY}   Ubuntu/Debian: sudo apt install qt6-base-dev qt6-tools-dev${NC}"
        echo -e "${GRAY}   macOS: brew install qt6${NC}"
        exit 1
    fi
    
    # Build the project
    echo -e "${GRAY}Building project...${NC}"
    if ! cmake --build build --config Release --parallel > /dev/null 2>&1; then
        echo -e "${RED}❌ Build failed${NC}"
        echo -e "${YELLOW}💡 Try building with verbose output:${NC}"
        echo -e "${GRAY}   cmake --build build --config Release --parallel --verbose${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✅ Build completed successfully!${NC}"
fi

if [[ "$BUILD_ONLY" == true ]]; then
    echo -e "\n${CYAN}🎉 Build complete! You can now run:${NC}"
    echo -e "${GRAY}   ./build/flux-cli/flux-cli --help${NC}"
    echo -e "${GRAY}   ./build/flux-gui/flux-gui${NC}"
    exit 0
fi

# Test CLI functionality
echo -e "\n${YELLOW}🖥️ Testing CLI functionality...${NC}"

if [[ -f "build/flux-cli/flux-cli" ]]; then
    echo -e "${GRAY}CLI Help:${NC}"
    ./build/flux-cli/flux-cli --help
    
    echo -e "\n${GRAY}CLI Version:${NC}"
    if ! ./build/flux-cli/flux-cli --version 2>/dev/null; then
        echo -e "${YELLOW}Version command not yet implemented${NC}"
    fi
else
    echo -e "${RED}❌ CLI executable not found${NC}"
fi

# Test GUI functionality
if [[ "$SHOW_GUI" == true && -f "build/flux-gui/flux-gui" ]]; then
    echo -e "\n${YELLOW}🖼️ Launching GUI application...${NC}"
    echo -e "${GRAY}GUI window should open. Close it to continue the demo.${NC}"
    
    # Check if we have a display
    if [[ -z "$DISPLAY" && -z "$WAYLAND_DISPLAY" ]]; then
        echo -e "${YELLOW}⚠️ No display detected. Skipping GUI launch.${NC}"
    else
        # Launch GUI and wait for it to close
        ./build/flux-gui/flux-gui &
        GUI_PID=$!
        wait $GUI_PID
        echo -e "${GREEN}✅ GUI application closed${NC}"
    fi
elif [[ "$SHOW_GUI" != true ]]; then
    echo -e "\n${YELLOW}🖼️ GUI application available at: ./build/flux-gui/flux-gui${NC}"
    echo -e "${GRAY}   Use --show-gui parameter to launch it automatically${NC}"
fi

# Run tests if available
echo -e "\n${YELLOW}🧪 Running tests...${NC}"
if [[ -d "build" ]]; then
    cd build
    if ctest --output-on-failure --parallel 2 > /dev/null 2>&1; then
        echo -e "${GREEN}✅ All tests passed!${NC}"
    else
        echo -e "${YELLOW}⚠️ Some tests failed or no tests found${NC}"
    fi
    cd ..
fi

# Show project status
echo -e "\n${CYAN}📊 Project Status Summary:${NC}"
echo -e "${CYAN}=========================${NC}"

declare -a status_items=(
    "Build System|✅ Working|CMake + Ninja"
    "CLI Application|🚧 Partial|Framework ready, needs core implementation"
    "GUI Application|🚧 Partial|UI framework ready, needs core integration"
    "Core Library|🚧 Partial|Architecture ready, needs third-party libraries"
    "Archive Formats|⏳ Planned|ZIP, TAR, 7Z interfaces designed"
    "CI/CD Pipeline|✅ Working|GitHub Actions with multi-platform support"
    "Documentation|✅ Complete|Comprehensive project documentation"
)

for item in "${status_items[@]}"; do
    IFS='|' read -r name status details <<< "$item"
    printf "  %-20s %-12s %s\n" "$name" "$status" "$details"
done

echo -e "\n${CYAN}🎯 Next Steps:${NC}"
echo -e "${CYAN}=============${NC}"
echo -e "${YELLOW}1. Integrate third-party libraries (libzip, libarchive, 7-Zip SDK)${NC}"
echo -e "${YELLOW}2. Implement core archive operations${NC}"
echo -e "${YELLOW}3. Connect GUI to working core functionality${NC}"
echo -e "${YELLOW}4. Add comprehensive unit tests${NC}"
echo -e "${YELLOW}5. Performance optimization and benchmarking${NC}"

echo -e "\n${CYAN}📚 Documentation:${NC}"
echo -e "${CYAN}=================${NC}"
echo -e "${GRAY}  README.md                    - Project overview and setup${NC}"
echo -e "${GRAY}  PROJECT_STATUS.md            - Detailed project status${NC}"
echo -e "${GRAY}  docs/CORE_FEATURE_MATRIX.md  - Feature implementation matrix${NC}"
echo -e "${GRAY}  BUILD_GUIDE.md               - Comprehensive build instructions${NC}"

echo -e "\n${CYAN}🔗 Repository:${NC}"
echo -e "${CYAN}=============${NC}"
echo -e "${BLUE}  https://github.com/xiaokanchengyang/Flux-cpp${NC}"

echo -e "\n${GREEN}🎉 Demo completed!${NC}"
echo -e "${WHITE}The project has a solid foundation and is ready for core functionality implementation.${NC}"
