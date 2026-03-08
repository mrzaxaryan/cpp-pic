---
name: New Platform / Architecture Support
about: Request or propose support for a new platform or CPU architecture
title: "[Platform] "
labels: platform
assignees: ''
---

## Platform / Architecture

- **OS:** (e.g., NetBSD, OpenBSD, QNX)
- **Architecture:** (e.g., ppc64, loongarch64, s390x)

## Syscall Interface

Describe the syscall interface for this platform:

- Syscall convention (registers, instruction)
- Error return convention (e.g., carry flag, negative return, errno)
- Key differences from existing supported platforms

## Implementation Plan

Outline what needs to be implemented:

- [ ] Syscall numbers and `System::Call` wrappers
- [ ] Platform result conversion (`result::From*`)
- [ ] Memory allocation (mmap/munmap equivalent)
- [ ] Console I/O
- [ ] File system operations
- [ ] Network (socket) support
- [ ] Entry point and linker script
- [ ] CMake presets and CI workflow
- [ ] Tests

## References

Links to relevant documentation, man pages, or ABI specifications.

## Additional Context

Any other relevant information, known challenges, or related issues.
