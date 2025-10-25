# Priority Task List for GitHub Issues

Copy each section below as individual GitHub issues with the suggested labels and priority levels.

## P0 (Immediate - Directly affects code review approval)

### 1. P0 — Add .clang-format and .editorconfig with formatting rules documentation

**Description:** Add new files providing sample rules (Google style / LLVM) and include "how to format locally" instructions (clang-format -i) in README.

**Labels:** enhancement, ci, good-first-issue

**Acceptance Criteria:**
- [ ] `.clang-format` file added with consistent style rules
- [ ] `.editorconfig` file added for cross-editor consistency  
- [ ] README updated with local formatting instructions
- [ ] All existing code formatted according to new rules

---

### 2. P0 — Create GitHub Actions CI: build + unit tests + clang-tidy + sanitizer

**Description:** Create `.github/workflows/ci.yml` (content provided above) ensuring every PR must pass CI.

**Labels:** ci, blocking, infrastructure

**Acceptance Criteria:**
- [ ] Multi-platform build matrix (Linux/Windows/macOS)
- [ ] Unit test execution with failure reporting
- [ ] Static analysis with clang-tidy integration
- [ ] Sanitizer builds (ASAN/UBSAN) for memory safety
- [ ] CI status checks required for PR merging

---

### 3. P0 — Add basic unit tests for core functionality (flux-core)

**Description:** Use GoogleTest/Catch2 to cover pack/unpack/inspect with 10 basic scenarios (empty files, single file, multiple files, subdirectories, symbolic links).

**Labels:** test, flux-core, priority-0

**Acceptance Criteria:**
- [ ] Test framework integration (GoogleTest recommended)
- [ ] Core API test coverage: pack, unpack, inspect operations
- [ ] Edge case testing: empty archives, corrupted files, permission issues
- [ ] CI integration with automatic test execution
- [ ] Test coverage reporting

---

### 4. P0 — Convert blocking UI synchronous I/O to asynchronous (example PR)

**Description:** Convert one obvious blocking UI operation (e.g., GUI performing large file packing on main thread) to use AsyncWorker pattern with progress display.

**Labels:** bug, ui, priority-0

**Acceptance Criteria:**
- [ ] Identify and document current blocking operations
- [ ] Implement AsyncWorker pattern for at least one major operation
- [ ] Add progress bar and cancellation support
- [ ] Ensure UI remains responsive during operations
- [ ] Add error handling and user feedback

---

## P1 (Short-term improvements - Quality and experience enhancement)

### 5. P1 — Introduce clang-tidy autofix and submit in batches

**Description:** Run clang-tidy -fix to resolve simple issues, split into multiple small PRs for easier review.

**Labels:** refactor, static-analysis

**Acceptance Criteria:**
- [ ] Run comprehensive clang-tidy analysis
- [ ] Group fixes by category (style, performance, modernization)
- [ ] Submit fixes in reviewable batches (max 50 files per PR)
- [ ] Document any suppressed warnings with justification
- [ ] Update CI to prevent regression

---

### 6. P1 — Convert file list to QAbstractItemModel + virtualization / lazy loading

**Description:** Optimize large file scenarios to avoid freezing when loading tens of thousands of files.

**Labels:** ui, performance

**Acceptance Criteria:**
- [ ] Replace current file list with QAbstractItemModel implementation
- [ ] Implement virtualization for large datasets
- [ ] Add lazy loading for file metadata
- [ ] Benchmark performance with 10k+ file archives
- [ ] Maintain existing UI functionality (selection, sorting, filtering)

---

### 7. P1 — Add preview panel (text/image/hex)

**Description:** Implement automatic encoding detection for text preview, image preview, and hex preview (large files read only first N KB).

**Labels:** feature, ux

**Acceptance Criteria:**
- [ ] Text preview with encoding auto-detection
- [ ] Image preview for common formats (PNG, JPG, GIF, etc.)
- [ ] Hex viewer for binary files with configurable byte limits
- [ ] Preview size limits to prevent memory issues
- [ ] Integration with file selection in main UI

---

## P2 (Medium/Long-term)

### 8. P2 — Plugin-based archive format support (Strategy/Plugin pattern)

**Description:** Abstract format implementations into interfaces, supporting third-party format plugins.

**Labels:** architecture, enhancement

**Acceptance Criteria:**
- [ ] Define plugin interface for archive formats
- [ ] Refactor existing formats to use plugin system
- [ ] Create plugin discovery and loading mechanism
- [ ] Document plugin development API
- [ ] Provide example plugin implementation

---

### 9. P2 — Fuzz testing and edge case regression

**Description:** Add fuzz testing pipeline (libFuzzer / AFL) for parser and archive loading logic to verify project stability with corrupted archives.

**Labels:** security, test

**Acceptance Criteria:**
- [ ] Integrate fuzzing framework (libFuzzer or AFL++)
- [ ] Create fuzz targets for all parsers
- [ ] Set up continuous fuzzing in CI
- [ ] Document and fix discovered vulnerabilities
- [ ] Add regression tests for found issues

---

### 10. P2 — Performance benchmarking & regression monitoring

**Description:** Add benchmark scripts (python/benchmark) and run periodic comparisons in CI (key paths: large file compression/parallel extraction).

**Labels:** performance, benchmark

**Acceptance Criteria:**
- [ ] Implement benchmark suite for critical operations
- [ ] Set up automated performance regression detection
- [ ] Create performance dashboard/reporting
- [ ] Document performance characteristics and limits
- [ ] Establish performance SLAs for key operations

---

## Implementation Notes

### Quick Setup Commands

```bash
# Install pre-commit hooks
pip install pre-commit
pre-commit install

# Format all code
find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | xargs clang-format -i

# Run local quality checks
./scripts/run_quality_checks.sh  # Linux/macOS
.\scripts\run_quality_checks.ps1  # Windows
```

### Priority Guidelines

- **P0 tasks** should be completed within 1-3 days and are blocking for code review approval
- **P1 tasks** improve code quality and user experience, target 1-2 weeks
- **P2 tasks** are architectural improvements for long-term maintainability

### Issue Creation Tips

1. Copy each task as a separate GitHub issue
2. Add suggested labels to help with organization
3. Link related issues using "Related to #X" or "Blocks #X"
4. Assign to appropriate team members based on expertise
5. Use milestones to track progress toward release goals
