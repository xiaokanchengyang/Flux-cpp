# Contributing to Flux Archive Manager

Thank you for your interest in contributing to Flux! This document provides guidelines and information for contributors.

## 🚀 Quick Start

1. **Fork** the repository
2. **Clone** your fork: `git clone https://github.com/your-username/Flux-cpp.git`
3. **Create** a feature branch: `git checkout -b feature/amazing-feature`
4. **Make** your changes
5. **Test** your changes: `cmake --build build && ctest --test-dir build`
6. **Commit** your changes: `git commit -m 'Add amazing feature'`
7. **Push** to your branch: `git push origin feature/amazing-feature`
8. **Open** a Pull Request

## 📋 Development Setup

### Prerequisites

- **CMake** 3.25 or later
- **Qt6** 6.5.4 or later
- **C++23** compatible compiler:
  - GCC 13+ or Clang 16+ (Linux/macOS)
  - MSVC 2022 17.8+ (Windows)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/xiaokanchengyang/Flux-cpp.git
cd Flux-cpp

# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure
```

### Development Dependencies

Optional tools for development:
- **clang-format** for code formatting
- **clang-tidy** for static analysis
- **cppcheck** for additional static analysis
- **Qt Creator** or **Visual Studio Code** for IDE support

## 🎯 Code Standards

### C++ Guidelines

- **Modern C++**: Use C++23 features where appropriate
- **Const correctness**: Mark functions and variables `const` when possible
- **RAII**: Use smart pointers and automatic resource management
- **Error handling**: Prefer `std::expected` over exceptions for recoverable errors
- **Performance**: Use `string_view`, `constexpr`, and move semantics appropriately

### Code Style

We use **clang-format** with the following key rules:
- **Indentation**: 4 spaces (no tabs)
- **Line length**: 100 characters maximum
- **Braces**: Allman style for functions/classes, same-line for control flow
- **Naming**:
  - Classes: `PascalCase` (e.g., `ArchiveManager`)
  - Functions/variables: `camelCase` (e.g., `extractArchive`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_BUFFER_SIZE`)
  - Private members: `m_` prefix (e.g., `m_fileName`)

### Example Code Style

```cpp
class ArchiveManager {
public:
    explicit ArchiveManager(std::string_view archivePath);
    
    std::expected<void, std::string> extractTo(const std::filesystem::path& destination) const;
    
private:
    std::string m_archivePath;
    mutable std::mutex m_mutex;
};

std::expected<void, std::string> ArchiveManager::extractTo(
    const std::filesystem::path& destination) const 
{
    if (!std::filesystem::exists(m_archivePath)) {
        return std::unexpected("Archive file not found");
    }
    
    // Implementation...
    return {};
}
```

## 🧪 Testing Guidelines

### Unit Tests

- Write tests for all new functionality
- Use **GoogleTest** framework
- Aim for >85% code coverage
- Test both success and failure cases

### Test Structure

```cpp
#include <gtest/gtest.h>
#include "flux-core/archive.h"

class ArchiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test fixtures
    }
    
    void TearDown() override {
        // Cleanup
    }
};

TEST_F(ArchiveTest, ExtractValidArchive) {
    // Arrange
    ArchiveManager manager("test.zip");
    
    // Act
    auto result = manager.extractTo("/tmp/test");
    
    // Assert
    EXPECT_TRUE(result.has_value());
}
```

## 📝 Commit Guidelines

### Commit Message Format

```
type(scope): brief description

Detailed explanation of the change (optional)

Fixes #123
```

### Types

- **feat**: New feature
- **fix**: Bug fix
- **docs**: Documentation changes
- **style**: Code style changes (formatting, etc.)
- **refactor**: Code refactoring
- **test**: Adding or updating tests
- **chore**: Build system, dependencies, etc.

### Examples

```
feat(core): add support for 7z archives

Implement 7z compression and extraction using libarchive.
Includes unit tests and performance benchmarks.

Fixes #45
```

```
fix(gui): resolve memory leak in resource manager

The ResourceManager was not properly releasing cached icons.
Added proper cleanup in destructor and RAII patterns.

Fixes #67
```

## 🐛 Bug Reports

When reporting bugs, please include:

1. **Environment**: OS, compiler version, Qt version
2. **Steps to reproduce**: Minimal example
3. **Expected behavior**: What should happen
4. **Actual behavior**: What actually happens
5. **Logs/output**: Any error messages or stack traces

Use our [bug report template](.github/ISSUE_TEMPLATE/bug_report.md).

## 💡 Feature Requests

For new features, please:

1. **Check existing issues** to avoid duplicates
2. **Describe the use case** and motivation
3. **Propose a solution** if you have ideas
4. **Consider backwards compatibility**

Use our [feature request template](.github/ISSUE_TEMPLATE/feature_request.md).

## 🔍 Code Review Process

### Pull Request Requirements

- [ ] **Builds successfully** on all platforms (CI must pass)
- [ ] **Tests pass** and coverage is maintained
- [ ] **Code follows style guidelines**
- [ ] **Documentation updated** if needed
- [ ] **Commit messages follow guidelines**
- [ ] **No merge conflicts**

### Review Checklist

Reviewers will check:
- Code quality and adherence to standards
- Test coverage and quality
- Performance implications
- Security considerations
- Documentation completeness
- Backwards compatibility

## 🏗️ Architecture Guidelines

### Module Structure

```
flux-core/          # Core archive functionality
├── include/        # Public headers
├── src/           # Implementation
└── tests/         # Unit tests

flux-gui/          # Qt-based GUI application
├── src/           # GUI implementation
├── resources/     # UI resources, icons
└── tests/         # GUI tests

flux-cli/          # Command-line interface
├── src/           # CLI implementation
└── tests/         # CLI tests
```

### Dependencies

- **Core module**: Minimal dependencies, no Qt
- **GUI module**: Qt6 Widgets, depends on core
- **CLI module**: CLI11 for argument parsing, depends on core

### Error Handling Strategy

- **Core library**: Use `std::expected<T, std::string>` for recoverable errors
- **Applications**: Convert to appropriate UI feedback (dialogs, console output)
- **Exceptions**: Only for programming errors and unrecoverable situations

## 🚀 Performance Guidelines

- **Benchmark critical paths** using Google Benchmark
- **Profile memory usage** with Valgrind or similar tools
- **Optimize for common use cases** (large archives, many small files)
- **Consider multi-threading** for I/O intensive operations
- **Use appropriate data structures** (prefer `std::vector` over `std::list`)

## 📚 Documentation

### Code Documentation

- **Public APIs**: Full Doxygen documentation
- **Complex algorithms**: Inline comments explaining approach
- **Non-obvious code**: Comments explaining "why", not "what"

### User Documentation

- **README**: Keep updated with new features
- **Examples**: Provide working code samples
- **Tutorials**: Step-by-step guides for common tasks

## 🤝 Community

### Communication

- **GitHub Issues**: Bug reports, feature requests
- **GitHub Discussions**: General questions, ideas
- **Pull Requests**: Code contributions

### Code of Conduct

We follow the [Contributor Covenant](https://www.contributor-covenant.org/). Please be respectful and inclusive in all interactions.

## 📄 License

By contributing to Flux, you agree that your contributions will be licensed under the MIT License.

---

Thank you for contributing to Flux! 🎉
