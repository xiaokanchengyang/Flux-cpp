# Contributing to Flux Archive Manager

Thank you for your interest in contributing to Flux-cpp! Please follow this workflow to ensure smooth collaboration.

## Development Workflow

### 1. Setup
1. Fork the repository
2. Create a feature branch (`feat/xxx`) or bugfix branch (`fix/xxx`)
3. Set up development environment:
   ```bash
   # Install pre-commit hooks (recommended)
   pip install pre-commit
   pre-commit install
   ```

### 2. Local Development
Before committing, ensure your changes meet quality standards:

```bash
# Format code
find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | xargs clang-format -i

# Build project
cmake -S . -B build && cmake --build build

# Run tests
ctest --test-dir build --output-on-failure

# Run quality checks
./scripts/run_quality_checks.sh    # Linux/macOS
.\scripts\run_quality_checks.ps1   # Windows
```

### 3. Code Standards
- **Code Style**: Follow `.clang-format` configuration (Google style)
- **Testing**: Add/update unit tests for logic changes
- **Documentation**: Update README/docs for API changes
- **Commit Messages**: Use conventional commits format
- **No Warnings**: Code must compile without warnings

### 4. Pull Request Process
1. **Reference Issues**: Link to related issues in PR description
2. **Fill PR Template**: Complete all sections of the PR template
3. **Wait for CI**: Ensure all CI checks pass before requesting review
4. **Address Feedback**: Respond to review comments promptly

## Pull Request Requirements

### ✅ Before Submitting
- [ ] Code follows project style (`.clang-format`)
- [ ] All tests pass locally
- [ ] New features include unit tests
- [ ] Documentation updated if needed
- [ ] CI workflow passes

### ✅ PR Template Checklist
- [ ] Clear description of changes and motivation
- [ ] Testing instructions provided
- [ ] Security/compatibility impact assessed
- [ ] Rollback strategy documented (if applicable)

## Code Review Process

### For Contributors
- **Be Responsive**: Address review feedback within 48 hours
- **Ask Questions**: Don't hesitate to ask for clarification
- **Small PRs**: Keep changes focused and reviewable
- **Test Coverage**: Ensure adequate test coverage for new code

### For Reviewers
- **Be Constructive**: Provide specific, actionable feedback
- **Check CI**: Verify all automated checks pass
- **Test Locally**: Test complex changes in your environment
- **Security Focus**: Pay attention to security implications

## Development Guidelines

### Architecture Principles
- **Modern C++23**: Use latest language features appropriately
- **RAII**: Proper resource management with smart pointers
- **Thread Safety**: Document thread safety guarantees
- **Error Handling**: Use `std::expected` for error propagation
- **Performance**: Consider memory usage and algorithmic complexity

### Testing Strategy
- **Unit Tests**: Test individual components in isolation
- **Integration Tests**: Test component interactions
- **Edge Cases**: Test boundary conditions and error scenarios
- **Performance Tests**: Benchmark critical paths

### Documentation Standards
- **Code Comments**: Document complex algorithms and design decisions
- **API Documentation**: Document public interfaces thoroughly
- **User Documentation**: Update user-facing documentation
- **Architecture Docs**: Maintain high-level design documentation

## Getting Help

- **Issues**: Report bugs and request features via GitHub Issues
- **Discussions**: Ask questions in GitHub Discussions
- **Code Review**: Tag maintainers for review assistance
- **Documentation**: Check `docs/` directory for detailed guides

## Recognition

Contributors will be recognized in:
- **CONTRIBUTORS.md**: All contributors listed
- **Release Notes**: Significant contributions highlighted
- **GitHub**: Contributor badges and statistics

---

**Thank you for contributing to Flux Archive Manager!** 🚀

Your contributions help make this project better for everyone.