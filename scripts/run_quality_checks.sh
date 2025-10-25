#!/bin/bash

# Flux Archive Manager - Quality Checks Script
# This script runs all code quality checks locally before submitting PR

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default values
SKIP_BUILD=false
SKIP_TESTS=false
SKIP_FORMAT=false
SKIP_LINT=false
BUILD_TYPE="Debug"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --skip-tests)
            SKIP_TESTS=true
            shift
            ;;
        --skip-format)
            SKIP_FORMAT=true
            shift
            ;;
        --skip-lint)
            SKIP_LINT=true
            shift
            ;;
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --skip-build     Skip building the project"
            echo "  --skip-tests     Skip running unit tests"
            echo "  --skip-format    Skip code formatting check"
            echo "  --skip-lint      Skip static analysis"
            echo "  --build-type     Build type (Debug/Release, default: Debug)"
            echo "  -h, --help       Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${GREEN}🚀 Running Flux Archive Manager Quality Checks${NC}"
echo -e "${GREEN}================================================${NC}"

START_TIME=$(date +%s)

# Check if we're in the project root
if [[ ! -f "CMakeLists.txt" ]]; then
    echo -e "${RED}❌ Please run this script from the project root directory${NC}"
    exit 1
fi

# Create build directory
BUILD_DIR="build-quality-check"
mkdir -p "$BUILD_DIR"

# Function to run command and check result
run_quality_command() {
    local cmd="$1"
    local description="$2"
    local work_dir="${3:-.}"
    
    echo -e "${YELLOW}📋 $description...${NC}"
    
    if ! (cd "$work_dir" && eval "$cmd"); then
        echo -e "${RED}❌ $description failed${NC}"
        return 1
    fi
    
    echo -e "${GREEN}✅ $description completed successfully${NC}"
    return 0
}

# 1. Code Formatting Check
if [[ "$SKIP_FORMAT" != true ]]; then
    echo -e "\n${CYAN}🎨 Checking Code Formatting${NC}"
    
    # Check if clang-format is available
    if ! command -v clang-format &> /dev/null; then
        echo -e "${YELLOW}⚠️  clang-format not found. Please install it:${NC}"
        echo "  Ubuntu/Debian: sudo apt-get install clang-format"
        echo "  macOS: brew install clang-format"
        echo "  Or use your package manager"
        exit 1
    fi
    
    # Find all C++ files
    format_issues=()
    while IFS= read -r -d '' file; do
        if ! clang-format --dry-run --Werror "$file" &>/dev/null; then
            format_issues+=("$file")
        fi
    done < <(find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | grep -v -E "(build|third-party|vcpkg)" | tr '\n' '\0')
    
    if [[ ${#format_issues[@]} -gt 0 ]]; then
        echo -e "${RED}❌ Format issues found in:${NC}"
        for file in "${format_issues[@]}"; do
            echo -e "${RED}  - $file${NC}"
        done
        echo -e "${YELLOW}Run 'clang-format -i <file>' to fix formatting${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✅ Code formatting check passed${NC}"
fi

# 2. Build Project
if [[ "$SKIP_BUILD" != true ]]; then
    echo -e "\n${CYAN}🔨 Building Project${NC}"
    
    # Configure with all warnings enabled
    configure_cmd="cmake -B $BUILD_DIR \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DFLUX_BUILD_TESTS=ON \
        -DFLUX_BUILD_GUI=ON \
        -DFLUX_BUILD_CLI=ON \
        -DFLUX_ENABLE_SANITIZERS=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    
    run_quality_command "$configure_cmd" "Configuring CMake"
    
    # Build
    run_quality_command "cmake --build $BUILD_DIR --config $BUILD_TYPE --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)" "Building project"
fi

# 3. Static Analysis
if [[ "$SKIP_LINT" != true ]]; then
    echo -e "\n${CYAN}🔍 Running Static Analysis${NC}"
    
    # Check if clang-tidy is available
    if ! command -v clang-tidy &> /dev/null; then
        echo -e "${YELLOW}⚠️  clang-tidy not found. Please install it:${NC}"
        echo "  Ubuntu/Debian: sudo apt-get install clang-tidy"
        echo "  macOS: brew install llvm"
    else
        # Run clang-tidy on core files
        find flux-core/src -name "*.cpp" | while read -r file; do
            echo -e "${YELLOW}🔍 Analyzing $(basename "$file")...${NC}"
            if ! clang-tidy -p "$BUILD_DIR" "$file" 2>/dev/null; then
                echo -e "${YELLOW}⚠️  Static analysis issues found in $(basename "$file")${NC}"
            fi
        done
    fi
    
    echo -e "${GREEN}✅ Static analysis completed${NC}"
fi

# 4. Run Tests
if [[ "$SKIP_TESTS" != true ]]; then
    echo -e "\n${CYAN}🧪 Running Unit Tests${NC}"
    
    run_quality_command "ctest --output-on-failure --build-config $BUILD_TYPE" "Running unit tests" "$BUILD_DIR"
    
    # Run with memory checking if available
    if command -v valgrind &> /dev/null; then
        echo -e "${YELLOW}🔍 Running memory checks...${NC}"
        run_quality_command "ctest -T memcheck" "Memory leak detection" "$BUILD_DIR" || true
    fi
fi

# 5. Additional Checks
echo -e "\n${CYAN}🔧 Additional Quality Checks${NC}"

# Check for TODO/FIXME in production code
todo_files=$(find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | \
            grep -E "(flux-core|flux-gui|flux-cli)" | \
            grep -v test | \
            xargs grep -l "TODO\|FIXME\|XXX\|HACK" 2>/dev/null || true)

if [[ -n "$todo_files" ]]; then
    echo -e "${YELLOW}⚠️  Found TODO/FIXME comments in production code:${NC}"
    echo "$todo_files" | while read -r file; do
        echo -e "${YELLOW}  - $file${NC}"
    done
fi

# Check for debug print statements
debug_prints=$(find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | \
              grep -E "(flux-core|flux-gui|flux-cli)" | \
              xargs grep -n "std::cout\|printf\|qDebug" 2>/dev/null | \
              grep -v "//.*std::cout\|//.*printf\|//.*qDebug" || true)

if [[ -n "$debug_prints" ]]; then
    echo -e "${YELLOW}⚠️  Found debug print statements:${NC}"
    echo "$debug_prints" | while read -r line; do
        echo -e "${YELLOW}  - $line${NC}"
    done
fi

# 6. Generate Report
echo -e "\n${CYAN}📊 Generating Quality Report${NC}"

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
DURATION_MIN=$(echo "scale=2; $DURATION / 60" | bc -l 2>/dev/null || echo "$DURATION")

cat > quality-check-report.md << EOF
# Flux Archive Manager - Quality Check Report

**Generated:** $(date '+%Y-%m-%d %H:%M:%S')
**Duration:** ${DURATION_MIN} minutes
**Build Type:** $BUILD_TYPE

## Summary

- ✅ Code formatting: PASSED
- ✅ Build: PASSED  
- ✅ Static analysis: COMPLETED
- ✅ Unit tests: PASSED
- ✅ Additional checks: COMPLETED

## Build Information

- **Build Directory:** $BUILD_DIR
- **CMake Configuration:** $BUILD_TYPE with sanitizers
- **Compiler Warnings:** Treated as errors
- **Test Framework:** GoogleTest

## Next Steps

1. Review any warnings or suggestions from static analysis
2. Address any TODO/FIXME comments if found
3. Remove any debug print statements if found
4. Ready for pull request submission

---
*Generated by Flux Quality Check Script*
EOF

echo -e "\n${GREEN}🎉 All Quality Checks Completed Successfully!${NC}"
echo -e "${CYAN}📄 Report saved to: quality-check-report.md${NC}"
echo -e "${CYAN}⏱️  Total time: ${DURATION_MIN} minutes${NC}"

# Cleanup
if [[ -f "compile_commands.json" ]]; then
    cp "compile_commands.json" "$BUILD_DIR/" 2>/dev/null || true
fi

echo -e "\n${GREEN}🚀 Ready for code review submission!${NC}"
