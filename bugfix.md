# Bug Analysis: macOS x86_64 SIGSEGV at -O1

**Note:** This crash is macOS x86_64 and macOS aarch64 only. Windows (i386, x86_64, aarch64), Linux (i386, x86_64, armv7a, aarch64), and UEFI (x86_64, aarch64) all build and pass tests successfully across all optimization levels.

## CI Failure Summary

**Workflow:** `build-macos-x86_64.yml` (macOS 15.7.4, Clang 21.1.8, `macos-15-intel` runner)
**Build preset:** `macos-x86_64-release` (includes `-flto=full`, `-fomit-frame-pointer`, `-fno-exceptions`)
**Failing optimization level:** `-O1`
**Symptom:** Segmentation fault (signal 11), exit code 139

### Crash Location

The binary crashes during **Socket test 5 (TestIpConversion)** in `tests/socket_tests.h`.

Last successful output:
```
IP conversion successful: 1.1.1.1 -> 0x01010101
```

The crash occurs somewhere between line 153 (above log message) and line 189 (next log message: "IPv6 conversion successful"). The code in this range is purely computational — no network I/O, no syscalls — just `IPAddress::FromString()` calls on invalid and IPv6 strings.

---

## What's Tested and Passing

| Platform | Arch | Opt Level | Status |
|----------|------|-----------|--------|
| macOS | x86_64 | O0 | ALL TESTS PASSED (Mach-O + PIC) |
| macOS | x86_64 | O1 | SIGSEGV during Socket tests |
| macOS | x86_64 | O2-Og | NOT TESTED (pipeline aborted after O1 failure) |
| macOS | aarch64 | O0-Og | Assumed passing (RDX/X1 clobbering fix in `daad480`, timeval fix in `e8ca1f4`) |
| Windows | all | all | Assumed passing (CI badges) |
| Linux | all | all | Assumed passing (CI badges) |

### Tests that pass at O0 before the crash point at O1

All CORE and most RAL test suites pass at O0:
DOUBLE, String, ArrayStorage, StringFormatter, DJB2, Base64, Memory, Random, SHA, ECC,
and Socket tests 1-4 (socket creation, connection, HTTP request, multiple connections).

**At O1**, Socket tests 1-4 also pass successfully. The crash first manifests in test 5 (IP address conversion), which suggests the issue is cumulative (stack pressure, register allocation, etc.) rather than an immediate crash on the first operation.

---

## What's Left to Find Out

### Unknown #1: Exact crash instruction

We don't have a stack trace or crash address. Need to reproduce with:
- Debug symbols (`-g` alongside `-O1`)
- AddressSanitizer (`-fsanitize=address`) to detect memory corruption
- Or at minimum, `lldb` or `coredump` analysis

### Unknown #2: Whether O2+ also crash

The CI pipeline uses `continue` on failure per-opt-level, but the script structure means O2-Og were not reached. We don't know if the bug is O1-specific or affects all optimization levels.

### Unknown #3: Whether the `COMPILER_RUNTIME` removal caused this

Commit `3a37294` removed `COMPILER_RUNTIME` (`__attribute__((noinline, used, optnone))`) from `memset`, `memcpy`, `memcmp`. This is the highest-probability root cause (see analysis below), but hasn't been confirmed with a revert test.

### Unknown #4: Whether the EMBEDDED_STRING destructor constraint is sufficient under LTO

The `"+m"` asm constraint in the destructor was verified on macOS aarch64 but not specifically on x86_64 with `-flto=full -O1`.

---

## Root Cause Hypotheses (ranked by probability)

### Hypothesis 1: memset/memcpy infinite recursion (HIGH probability)

**The theory:** Commit `3a37294` removed `COMPILER_RUNTIME` (which includes `optnone`) from `memset`, `memcpy`, `memcmp` in `src/core/memory.cc`. Without `optnone`, the compiler at O1+ can recognize the byte-by-byte loop inside `memset`:

```cpp
// src/core/memory.cc — now compiled with full O1 optimization
extern "C" PVOID memset(PVOID dest, INT32 ch, USIZE count)
{
    PCHAR p = (PCHAR)dest;
    CHAR byte = (CHAR)ch;
    for (USIZE i = 0; i < count; i++)
        p[i] = byte;     // <-- LoopIdiomRecognize sees this as "memset"
    return dest;
}
```

LLVM's `LoopIdiomRecognize` pass transforms this loop into a call to `llvm.memset` intrinsic, which lowers to a call to `memset` — creating **infinite recursion**.

**Why it didn't crash earlier:** With `-flto=full` (used in release builds), the LTO optimizer processes all translation units together. The interaction between `-fno-builtin` and LTO's LoopIdiomRecognize might be different from per-TU compilation. Additionally, smaller memset calls (e.g., zeroing 8-16 bytes for small EMBEDDED_STRINGs) might be unrolled instead of transformed, so the recursion only triggers for larger buffers.

The crash during `TestIpConversion` could be because this function has many local variables and EMBEDDED_STRING temporaries, leading to larger `Memory::Zero()` / `memset()` calls that trigger the loop-to-memset transformation.

**Evidence:**
- `COMPILER_RUNTIME` was explicitly added in `adc11e3` (implying it was needed)
- Then removed in `3a37294` (the removal may have been premature)
- The crash is at O1 (first opt level where LoopIdiomRecognize runs), not at O0
- SIGSEGV from stack overflow (infinite recursion) matches the symptom

### Hypothesis 2: EMBEDDED_STRING stack slot reuse under LTO (MEDIUM probability)

**The theory:** The `"+m"` constraint in the EMBEDDED_STRING destructor might not be fully respected by the LTO optimizer. At O1 with `-flto=full`, the optimizer has whole-program visibility and might reorder stores or reuse stack slots across inlined function boundaries.

In `TestIpConversion`, there are 5 EMBEDDED_STRING variables + many more temporaries created by LOG_INFO macro expansion (each creates 3+ wide EMBEDDED_STRING temporaries in `TimestampedLogOutput`). If the optimizer reuses the stack slot of one EMBEDDED_STRING for another before the destructor runs, the data pointer becomes stale.

**Evidence:**
- Three commits (`f98d4f1`, `40c77a7`, `3622a59`) were needed to get the destructor constraint right
- The crash is only at O1+, consistent with optimizer reordering
- The crash happens in a function with high EMBEDDED_STRING density

### Hypothesis 3: Syscall RDX clobbering edge case (LOW probability)

**The theory:** The macOS x86_64 syscall wrappers correctly handle RDX clobbering (0-2 arg overloads clobber RDX, 3+ arg overloads use `"+r"`). However, there might be an edge case where the compiler's register allocator at O1 makes assumptions about register liveness that conflict with the syscall clobber specification.

**Evidence against:** The syscall wrappers were already fixed in `daad480` and have been verified on macOS aarch64 across all opt levels. The x86_64 wrappers follow the same pattern.

---

## Minimal Reproduction Tests

Build each test as a standalone program using `macos-x86_64-release` preset with `-DOPTIMIZATION_LEVEL=O1`.

### Repro 1: memset infinite recursion detector

Add to `tests/socket_tests.h` (or a new test file) to test before the crash point:

```cpp
// Minimal test: does Memory::Zero work at O1?
// If memset has infinite recursion, this will SIGSEGV immediately.
static BOOL TestMemsetNotRecursive()
{
    CHAR buffer[256];
    Memory::Zero(buffer, sizeof(buffer));  // Calls memset(buffer, 0, 256)

    // Verify it actually zeroed
    for (USIZE i = 0; i < 256; i++)
    {
        if (buffer[i] != 0)
            return FALSE;
    }
    return TRUE;
}
```

If this crashes, **Hypothesis 1 is confirmed**. Fix: restore `COMPILER_RUNTIME` on memory functions.

### Repro 2: EMBEDDED_STRING stack density test

```cpp
// Minimal test: many EMBEDDED_STRING temporaries in one function
// Tests whether the optimizer reuses stack slots prematurely.
static BOOL TestEmbeddedStringDensity()
{
    auto s1 = "first string here"_embed;
    LOG_INFO("s1 = %s", (PCCHAR)s1);

    auto s2 = "second string is different"_embed;
    LOG_INFO("s2 = %s", (PCCHAR)s2);

    auto s3 = "third string for testing"_embed;
    LOG_INFO("s3 = %s", (PCCHAR)s3);

    auto s4 = "fourth string value"_embed;
    LOG_INFO("s4 = %s", (PCCHAR)s4);

    auto s5 = "fifth and final string"_embed;
    LOG_INFO("s5 = %s", (PCCHAR)s5);

    // All strings should still be valid here (destructors haven't run yet)
    LOG_INFO("All: %s %s %s %s %s", (PCCHAR)s1, (PCCHAR)s2, (PCCHAR)s3, (PCCHAR)s4, (PCCHAR)s5);

    return TRUE;
}
```

If this crashes or produces garbled output, **Hypothesis 2 is confirmed**.

### Repro 3: IPAddress::FromString isolation

```cpp
// Minimal test: just IP parsing, no logging between parses
// If this passes but the full TestIpConversion crashes, the issue is in
// LOG_INFO interaction, not in IPAddress::FromString itself.
static BOOL TestIpParsingAlone()
{
    auto ip1 = "1.1.1.1"_embed;
    IPAddress r1 = IPAddress::FromString((PCCHAR)ip1);
    if (!r1.IsValid()) return FALSE;

    auto ip2 = "256.1.1.1"_embed;
    IPAddress r2 = IPAddress::FromString((PCCHAR)ip2);
    if (r2.IsValid()) return FALSE;  // Should be invalid

    auto ip3 = "192.168.1"_embed;
    IPAddress r3 = IPAddress::FromString((PCCHAR)ip3);
    if (r3.IsValid()) return FALSE;  // Should be invalid

    auto ip4 = "abc.def.ghi.jkl"_embed;
    IPAddress r4 = IPAddress::FromString((PCCHAR)ip4);
    if (r4.IsValid()) return FALSE;  // Should be invalid

    auto ip5 = "2001:db8::1"_embed;
    IPAddress r5 = IPAddress::FromString((PCCHAR)ip5);
    if (!r5.IsValid() || !r5.IsIPv6()) return FALSE;

    LOG_INFO("All IP parsing tests passed in isolation");
    return TRUE;
}
```

### Repro 4: memcpy in IPAddress context

```cpp
// Minimal test: exercise Memory::Copy with struct-sized copies
// IPAddress::FromString uses Memory::Copy for the final address construction
static BOOL TestMemcpySmallBuffers()
{
    UINT8 src[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    UINT8 dst[16];

    Memory::Zero(dst, 16);
    Memory::Copy(dst, src, 4);   // IPv4-sized copy
    if (dst[0] != 1 || dst[3] != 4) return FALSE;

    Memory::Zero(dst, 16);
    Memory::Copy(dst, src, 16);  // IPv6-sized copy
    if (dst[0] != 1 || dst[15] != 16) return FALSE;

    LOG_INFO("memcpy small buffer tests passed");
    return TRUE;
}
```

---

## Suggested Investigation Steps

### Step 1: Quick revert test

Restore `COMPILER_RUNTIME` on memory functions (revert commit `3a37294`) and re-run the macOS x86_64 CI. If O1 passes, Hypothesis 1 is confirmed.

```cpp
// src/core/memory.cc
extern "C" COMPILER_RUNTIME PVOID memset(PVOID dest, INT32 ch, USIZE count)
//          ^^^^^^^^^^^^^^^^ restore this
```

### Step 2: Disassembly check

If you have the O1 build artifact, check if memset was compiled to call itself:

```bash
llvm-objdump -d output | grep -A 20 '<memset>'
```

If you see a `callq` to `memset` within the `memset` function body, that confirms infinite recursion.

### Step 3: AddressSanitizer build

Build with ASan to get precise crash information:

```bash
cmake --preset macos-x86_64-release \
  -DOPTIMIZATION_LEVEL=O1 \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g"
```

Note: ASan adds its own memset/memcpy implementations, so if Hypothesis 1 is the issue, ASan might mask it. If ASan passes but non-ASan crashes, that's strong evidence for the memset recursion theory.

### Step 4: Test O2-Og independently

Modify the CI workflow to continue testing even after one opt level fails:

```yaml
# Change from:
if [ $? -ne 0 ]; then FAILED="$FAILED $opt-macho"; continue; fi
# To:
if [ $? -ne 0 ]; then FAILED="$FAILED $opt-macho"; fi
```

This reveals which other optimization levels are affected.

### Step 5: Check with `-fno-builtin-memset` explicitly

Even though `-fno-builtin` is set globally, try adding `-fno-builtin-memset -fno-builtin-memcpy -fno-builtin-memcmp` explicitly to see if it changes behavior. Some LLVM passes may not fully respect `-fno-builtin` under LTO.

---

## Relevant Commits

| Commit | Description | Relevance |
|--------|-------------|-----------|
| `3a37294` | Remove COMPILER_RUNTIME from memory functions | **Primary suspect** — removed optnone protection |
| `adc11e3` | Add COMPILER_RUNTIME to memory functions | Shows the attribute was intentionally added |
| `daad480` | Fix syscall RDX/X1 clobbering for macOS | Fixed aarch64, x86_64 wrappers look correct |
| `e8ca1f4` | Fix timeval tv_usec to INT32 for macOS | Fixed struct size mismatch |
| `3622a59` | Fix EMBEDDED_STRING destructor to "+m" | Latest destructor constraint |

## Relevant Files

| File | Role |
|------|------|
| `src/core/memory.cc` | memset/memcpy/memcmp definitions — no longer protected by optnone |
| `include/core/compiler.h` | COMPILER_RUNTIME macro definition |
| `include/platform/macos/system.h` | Syscall wrappers with RDX clobbering |
| `tests/socket_tests.h:133-192` | TestIpConversion — crash location |
| `src/network/ip_address.cc` | IPAddress::FromString — called during crash |
| `include/core/types/embedded/embedded_string.h` | EMBEDDED_STRING with "+m" destructor |
| `include/io/logger.h` | Logger creating EMBEDDED_STRING temporaries |
| `cmake/Common.cmake` | Build flags including -fno-builtin and -flto=full |

---

## Fix Applied

**Confirmed root cause: Hypothesis 1 — memset/memcpy infinite recursion.**

Restored `COMPILER_RUNTIME` attribute on all three memory functions in `src/core/memory.cc`:

```cpp
extern "C" COMPILER_RUNTIME PVOID memset(PVOID dest, INT32 ch, USIZE count)
extern "C" COMPILER_RUNTIME PVOID memcpy(PVOID dest, const VOID *src, USIZE count)
extern "C" COMPILER_RUNTIME INT32 memcmp(const VOID *ptr1, const VOID *ptr2, USIZE num)
```

### Why this fixes the crash

`COMPILER_RUNTIME` expands to `__attribute__((noinline, used, optnone))`. The `optnone` attribute prevents LLVM's `LoopIdiomRecognize` pass from running on these functions. Without it, at `-O1+` the optimizer recognizes the byte-by-byte loops as standard memory operations and transforms them into calls to `llvm.memset`/`llvm.memcpy` intrinsics, which lower to calls to `memset`/`memcpy` — the very same functions being defined — creating infinite recursion that overflows the stack (SIGSEGV).

### Why the attribute is essential and must not be removed again

These three functions (`memset`, `memcpy`, `memcmp`) are ABI-mandated symbols that the compiler's code generation backend may emit calls to at any time. The `optnone` attribute is not optional optimization guidance — it is a correctness requirement. Without it, the compiler is free to "optimize" the implementation into a recursive call to itself. The `-fno-builtin` flag is insufficient because LLVM's LTO pipeline (`-flto=full`) may not fully respect it across translation unit boundaries.

### Verification

- Windows x86_64 debug build: compiles cleanly, PIC verification passes
- All tests pass on Windows (DOUBLE, String, ArrayStorage, StringFormatter, DJB2, Base64, Memory, Random, SHA, ECC, Socket, TLS, DNS, WebSocket, FileSystem)
- **macOS CI result: FIX INSUFFICIENT** — crash persists at O1 with identical symptoms (same last output line, same SIGSEGV). The COMPILER_RUNTIME fix is kept as a correctness measure but is not the root cause of this crash. Investigation continues below.

---

## Continued Investigation (Post-COMPILER_RUNTIME fix)

### Deep Static Analysis of O1 Binary

Exhaustive disassembly analysis of the O1 Mach-O binary was performed. Key findings:

#### Binary layout (O1, `-flto=full -fomit-frame-pointer`)
- `start()` at 0x100017E70: frame 0xB28 + 6 pushes (~2960 bytes)
- `ConsoleCallback<wchar_t>` at 0x1000042A0: **NO frame, uses red zone** for UTF-8 syscall buffer
- `FormatWithArgs<wchar_t>` at 0x100002A40: frame 0x88 + 6 pushes (184 bytes)
- `TimestampedLogOutput<wchar_t, char const*>` at 0x10002E8B0: frame 0x98 + 6 pushes (200 bytes)
- `Logger::Info<wchar_t, char const*>` at 0x100032740: frame 0x20 + 3 pushes (56 bytes)
- `EMBEDDED_FUNCTION_POINTER::Get()` at 0x100002A20: `leaq ConsoleCallback(%rip), %rax; retq`

#### Hypotheses DISPROVEN by binary analysis:
1. **Register allocation bug in FromString** — all registers correctly assigned in disassembly
2. **FromString miscompilation** — verified all test inputs produce expected control flow
3. **Stack frame overflow** — 0xB18 highest offset within 0xB28 frame
4. **EMBEDDED_STRING constructor overflow** — 16 bytes written correctly
5. **SIMD alignment crash** — no SIMD instructions in FromString
6. **Logger::Info argument passing** — RSI correctly saved to RBX and forwarded
7. **TimestampedLogOutput RDX handling** — saves arg ref to R14 before DateTime::Now
8. **Argument forwarding/dereference** — `movq (%r14), %rax` correctly dereferences at 0x10002E9F5
9. **Format string corruption** — RBX preserves format pointer across all three Format calls
10. **Stack slot reuse** — "+m" destructor barrier prevents premature reuse
11. **Console callback function pointer** — EMBEDDED_FUNCTION_POINTER::Get() returns correct address
12. **Syscall RDX clobbering** — all overloads correctly handle RDX (0-2 arg: clobber, 3+: "+r")
13. **UTF-16 CodepointToUTF8** — inputIndex passed by reference, loop terminates correctly

#### Critical Discovery: First-Time Template Instantiation

Test 5 line 189 (`LOG_INFO("IPv6 conversion successful: %s", ...)`) is the **FIRST-EVER execution** of `TimestampedLogOutput<wchar_t, char const*>` (1-arg version) in the entire test suite.

- All prior `LOG_INFO` calls use the 0-arg instantiation (`Logger::Info<WCHAR>`) or the 2-arg version
- `LOG_ERROR` calls with `%s` exist but are on error paths NOT taken (tests pass)
- The specific 1-arg template instantiation has never been exercised before the crash point

The 2-arg version (`TimestampedLogOutput<wchar_t, char const*, unsigned int>`) at line 153 works correctly, suggesting the issue is specific to the 1-arg instantiation's codegen at O1.

#### Critical Discovery: CI Runner Environment

The `macos-15-intel` runner specified in `.github/workflows/build-macos-x86_64.yml` is **NOT a standard GitHub Actions runner**. GitHub's standard `macos-15` runner uses Apple Silicon (M-series). The `macos-15-intel` designation likely means Apple Silicon running x86_64 code via **Rosetta 2**.

This is significant because:
- Rosetta 2 translates x86_64 instructions to ARM64 on the fly
- The `syscall` instruction is translated to `svc` for the ARM64 kernel
- Red zone handling during the syscall translation may differ from native x86_64

#### ConsoleCallback Red Zone Usage

At O1, `ConsoleCallback<wchar_t>` (and likely `<char>`) compiles as a **leaf function with no stack frame**, storing the UTF-8 conversion buffer in the **red zone** (below RSP). The function flow is:

1. Store WCHAR `ch` below RSP (red zone)
2. Convert to UTF-8 in red zone buffer
3. Execute `syscall` instruction for `SYS_WRITE` with buffer pointer pointing into the red zone

This is technically legal on native x86_64 (the kernel doesn't touch user stack during syscall). However, under Rosetta 2, the translation of `syscall` involves additional runtime state management that may clobber the red zone.

### Fixes Applied

#### Fix 1: IPv6 Double-Colon Parsing (correctness bug)

**File:** `src/network/ip_address.cc`

The `::` handler in `FromString()` did not process accumulated hex digits before recording the double-colon position, and did not reset `hexIndex`. For input "2001:db8::1":
- "db8" accumulated in hexBuffer (hexIndex=3)
- `::` encountered but hexIndex not reset
- "1" appended at hexIndex=3, making "db81"
- Parsed as single group 0xDB81 instead of separate groups 0x0DB8 and 0x0001

**Fix:** Process accumulated hex digits before the double colon, reset hexIndex, then record `doubleColonPos` after the potential `groupIndex` increment. Added `continue` statement for explicit control flow.

This is a correctness bug only — produces wrong IPv6 bytes but doesn't cause SIGSEGV (no out-of-bounds access).

#### Fix 2: Console::Write NOINLINE (speculative SIGSEGV fix)

**File:** `include/io/console.h`

Added `NOINLINE` attribute to both `Console::Write` overloads:
```cpp
static NOINLINE UINT32 Write(const CHAR *text, USIZE length);
static NOINLINE UINT32 Write(const WCHAR *text, USIZE length);
```

**Rationale:** At O1 with `-flto=full`, LTO inlines `Console::Write` into `ConsoleCallback`, making it a leaf function that uses the red zone for the syscall buffer. By marking Write as NOINLINE:
- ConsoleCallback must issue a `call` instruction → requires a proper stack frame
- The `ch` parameter is stored in ConsoleCallback's frame (not the red zone)
- The syscall happens inside Console::Write's own context
- Eliminates any potential red zone/Rosetta 2 syscall interaction

**Performance impact:** Negligible. Adds one `call`/`ret` pair per character written, dwarfed by the syscall overhead.

**Verification:** Windows x86_64 debug build compiles and all tests pass (DOUBLE, String, ArrayStorage, StringFormatter, DJB2, Base64, Memory, Random, SHA, ECC, Socket, TLS, DNS, WebSocket, FileSystem).

### What Remains Unknown

1. **Whether this fixes the macOS O1 crash** — Needs CI run to confirm
2. **Whether the runner is truly Rosetta 2** — `macos-15-intel` is not a documented standard GitHub runner
3. **Whether there's a Clang 21 miscompilation** specific to the 1-arg template instantiation at O1
4. **Whether O2-Og are also affected** — CI pipeline stops after first failure

#### Fix 3: CI Workflow `set -e` Bug (all platforms)

**Files:** All `.github/workflows/build-*.yml` files

With `set -e` active, a bare command like `"$TEST_BIN"` that exits via signal (e.g., SIGSEGV exit code 139) causes the shell to abort immediately — before the `if [ $? -ne 0 ]` check on the next line can execute. This meant:
- O1 crash killed the entire loop, O2-Og were never tested
- We couldn't determine if the bug was O1-specific or affected all optimization levels

**Fix:** Changed all bare test execution + separate exit check patterns from:
```bash
"$TEST_BIN"
if [ $? -ne 0 ]; then FAILED="$FAILED $opt-macho"; continue; fi
```
to:
```bash
if ! "$TEST_BIN"; then FAILED="$FAILED $opt-macho"; continue; fi
```

Under `set -e`, commands inside `if` conditions are exempt from early-abort behavior. Applied to all test binary executions (ELF, Mach-O, PIC loader) and cmake build commands across 8 workflow files.

#### Fix 4: Diagnostic Markers in TestIpConversion

**File:** `tests/socket_tests.h`

Added `LOG_INFO` markers between each major operation in `TestIpConversion` to narrow down the exact crash location via CI output:
- `[D1]` before `256.1.1.1` invalid IP test
- `[D2]` before `192.168.1` invalid IP test
- `[D3]` before `abc.def.ghi.jkl` invalid IP test
- `[D4]` before `2001:db8::1` IPv6 test

If CI shows `[D1]` but not `[D2]`, the crash is in the first invalid IP test. If all markers appear, the crash is in the final IPv6 block. This also tests whether adding LOG_INFO calls (which change the stack layout) moves or masks the bug — a positive result either way.

### Recommended Next Steps

1. Push changes and run macOS CI to test all fixes
2. Analyze which diagnostic markers appear before the crash
3. If crash persists, add `-g` to the O1 build for debug symbols and use `lldb` to get a stack trace
4. With CI fix, check whether O2-Og also crash or if this is O1-specific
5. Consider adding `NOINLINE` to `Logger::ConsoleCallback` as an additional measure
6. Test with `-mno-red-zone` flag (prevents any red zone usage) as a more aggressive workaround
