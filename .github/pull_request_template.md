## Summary

<!-- Brief description of the changes (1-3 sentences) -->

## Type of Change

- [ ] Bug fix
- [ ] New feature
- [ ] New platform / architecture support
- [ ] Refactoring (no functional changes)
- [ ] Documentation
- [ ] Build system / CI
- [ ] Other:

## Affected Layer

- [ ] CORE
- [ ] PLATFORM
- [ ] RUNTIME
- [ ] Tests
- [ ] Build system / CMake

## Platforms Tested

<!-- List the platform-architecture presets you built and tested against -->

- [ ] `windows-x86_64-release`
- [ ] `linux-x86_64-release`
- [ ] `macos-aarch64-release`
- [ ] Other:

## Binary Size Impact

<!-- Required: Report .text section size before and after your change for at least one preset -->
<!-- Use: llvm-size build/release/<platform>/<arch>/output.exe -->

```
Preset: <platform>-<arch>-release
Before: exe XXXXX, bin XXXXX
After:  exe XXXXX, bin XXXXX
Diff:   +/- XXX B
```

## Checklist

- [ ] Build passes with no warnings for at least one preset
- [ ] Post-build PIC verification passes (no `.rdata`/`.rodata`/`.data`/`.bss` sections)
- [ ] All tests pass
- [ ] Code follows [naming conventions](CONTRIBUTING.md#naming-conventions) and [code style](CONTRIBUTING.md#code-style)
- [ ] Doxygen documentation added for new public APIs
- [ ] No STL, no exceptions, no RTTI
- [ ] `_embed` suffix used for all string/float/array literals
- [ ] `Result<T, Error>` used for all fallible functions
- [ ] Binary size diff reported above

## Additional Notes

<!-- Any context, design decisions, trade-offs, or related issues -->
