## PR Brief Description (Keep title concise)

**Related issue**: #(issue-number) or `None`

**Change type**:
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation
- [ ] CI / Build
- [ ] Refactoring
- [ ] Other (specify)

---

### Detailed Change Description
Please describe in detail what this PR does, why it's needed, and key implementation approaches (note any algorithm/protocol/format changes).

### How to Test (Required)
List reproduction steps and verification methods (commands, expected output, data/fixtures used):
1. `cmake -S . -B build && cmake --build build`
2. `ctest --test-dir build --output-on-failure`
3. Other manual steps...

### CI / Dependencies / Compatibility
- CI passing: `[]` Local, `[]` CI
- Dependencies added/removed: List them
- Platform compatibility (Linux/Windows/macOS)

### Security and Privacy Impact
Does this involve passwords, keys, plaintext sensitive data, network security, etc.: If yes, explain handling details.

### Rollback Strategy / Compatibility Notes
If rollback is needed or affects existing users, document recovery steps or compatibility approach.

---

### Checklist (PR Author Confirmation)
- [ ] Followed project code style (`.clang-format`)
- [ ] Ran and passed local unit tests
- [ ] CI workflow passed or has reasonable explanation
- [ ] Documentation/README/CHANGELOG updated (if needed)
- [ ] If API changed, updated compatibility notes in README or docs

@maintainers PTAL