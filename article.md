# NOSTDLIB-RUNTIME: A Modern C++ Approach to Zero-Dependency, Position-Independent Code Generation

## Introduction

Shellcode, in security research and malware analysis, is a small, self-contained sequence of machine instructions that can be injected into memory and executed from an arbitrary location. It must operate without relying on external components such as DLLs, runtime initialization routines, or fixed stack layouts. Because of these strict constraints, shellcode is traditionally written in assembly language, which provides precise control over instructions, registers, and memory access.
While assembly ensures fully dependency-free and position-independent execution, it quickly becomes impractical as complexity grows due to its low-level nature and limited expressiveness. High-level languages like C offer improved readability, maintainability, and development speed, but standard C and C++ compilation models introduce significant challenges for shellcode development. Modern compilers typically generate binaries that depend on runtime libraries, import tables, relocation information, and read-only data sections. These dependencies violate the core requirement of shellcode—execution independent of any fixed memory layout or external support—and these problems do not admit simple or universally effective solutions.
As a result, code produced by conventional toolchains cannot be used as standalone shellcode without substantial modification or manual restructuring.

## Motivation
A long time ago, in a corner of the darknet, two users debated which programming language was better. One argued that assembly provides almost complete control over execution, while the other claimed that C, as a higher-level language, is a better choice for implementing complex systems, since writing something like a TLS client in assembly is impractical.
That debate ended with the assembly coder being kicked out of that forum.

With this work, we would like to add our two cents to that debate by arguing that it is possible to leverage modern C++23 without compromising the strict execution guarantees required for shellcode.

## Common Problems and Solutions

When writing shellcode in C/C++, developers face several fundamental challenges.This section examines each of these problems, outlines the traditional approaches used to address them, explains the limitations of those approaches, and demonstrates how NOSTDLIB-RUNTIME provides a robust solution.

### Problem 1: String literals in .rdata and other relocation Dependencies

C-generated shellcode relies on loader-handled relocations that are not applied in a loaderless execution environment, preventing reliable execution from arbitrary memory.

#### Traditional Approach

**Option 1:** Use a custom shellcode loader.

This aproach is not that easy to implement. Main difficulty is saving .reloc section in shellcode. 

**Option 2:** Minimize usage of constructs that cause generation of data in `.rdata` or `.data` sections. In case of string it can be done by moving string literals onto the stack. Stack-based strings can be created by representing the string as a character array stored in a local variable. This solution also obfuscates strings:

```cpp
// "example.exe"
char path[] = {'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'e', 'x', 'e', '\0'};
```

and in the case of wide character strings, the notation is as follows:

```cpp
// L"example.exe"
wchar_t path[] = { L'e', L'x', L'a', L'm', L'p', L'l', L'e', L'.', L'e', L'x', L'e', L'\0' };
```

**Alternative:** Manually assign each character to an array element on the stack, one by one:

```cpp
char path[12];
path[0] = 'e';
path[1] = 'x';
path[2] = 'a';
path[3] = 'm';
path[4] = 'p';
path[5] = 'l';
path[6] = 'e';
path[7] = '.';
path[8] = 'e';
path[9] = 'x';
path[10] = 'e';
path[11] = '\0';
```

**Option 3:** Perform the relocation manually at runtime. The shellcode determines its own position in memory and performs the loader's work manually. Constants and strings may reside in sections such as `.rdata`, which are then merged into the `.text` section using `/MERGE:.rdata=.text` with link.exe or a custom [linker script](linker_script.txt) for lld/ld. During execution, relocation entries are processed explicitly to fix up absolute addresses:
```cpp
PCHAR GetInstructionAddress(VOID)
{
    return __builtin_return_address(0);
}

PCHAR ReversePatternSearch(PCHAR rip, const CHAR *pattern, UINT32 len)
{
    PCHAR p = rip;
    while (1)
    {
        UINT32 i = 0;
        for (; i < len; i++)
        {
            if (p[i] != pattern[i])
                break;
        }
        if (i == len)
            return p; // found match
        p--;    // move backward
    }
}

ENTRYPOINT INT32 _start(VOID)
{
#if defined(PLATFORM_WINDOWS_I386)

    PCHAR currentAddress = GetInstructionAddress(); // Get the return address of the caller function
    UINT16 functionPrologue = 0x8955; // i386 function prologue: push ebp; mov ebp, esp
        // Scan backward for function prologue
    PCHAR startAddress = ReversePatternSearch(currentAddress, (PCHAR)&functionPrologue, sizeof(functionPrologue));

#endif
```

Then, perform relocation like this:

```cpp
CHAR *string = "Hello, World!";
CHAR *relocatedString = string + (SSIZE)startAddress;

WCHAR *wideString = L"Hello, World!";
WCHAR *relocatedWideString = (WCHAR*)((CHAR*)wideString + (SSIZE)startAddress);
```
**Note**: In this example the solution is shown for string literals but same aproach works in other cases (f.e. function pointers).

Similar to strings, constant arrays-such as lookup tables, binary blobs, are placed into `.rdata` by the compiler, making them inaccessible in a loaderless execution environment. And traditional approaches are the same to apply.

#### Why Traditional Approaches Fail

These approaches are not universal, as they rely on compiler-specific behavior and assumptions about stack layout. Modern compilers are sophisticated enough to recognize these patterns; when optimizations are enabled, the compiler may consolidate individual character assignments, place the string data in `.rdata`, and replace the code with a single `memcpy` call. This defeats the purpose of the technique and reintroduces the same `.rdata` dependency the approach was meant to avoid. Additionally, manually embedding constants and strings increases shellcode size, making it easier to detect and difficult to scale. These approaches also make the code less readable and harder to maintain.

#### NOSTDLIB-RUNTIME Solution: No Relocations Needed

By eliminating all `.rdata` dependencies through compile-time embedding of strings, arrays, floating-point constants-and using pure relative addressing for function pointers, NOSTDLIB-RUNTIME produces code that requires no relocations. The resulting binary is inherently position-independent without any runtime fixups.
To achieve this, NOSTDLIB-RUNTIME replaces conventional string literals with compile-time–decomposed representations. Leveraging C++23 features—such as user-defined literals, variadic templates, and fold expressions—string contents are expanded into individual character operations entirely at compile time:



```cpp
template <typename TChar, TChar... Chars>
class EMBEDDED_STRING
{
    TChar data[sizeof...(Chars) + 1];
    NOINLINE DISABLE_OPTIMIZATION EMBEDDED_STRING()
    {
        USIZE i = 0;
        ((data[i++] = Chars), ...); // Fold expression
        data[i] = 0;
    }
};
```

Usage:
```cpp
auto msg = "Hello, World!"_embed; // Embedded in code, not .rdata
```

Assembly Output:
```asm
movw $0x48, (%rdi)  ; 'H'
movw $0x65, 2(%rdi) ; 'e'
movw $0x6C, 4(%rdi) ; 'l'
movw $0x6C, 6(%rdi) ; 'l'
movw $0x6F, 8(%rdi) ; 'o'
```
As a result, string data exists only transiently in registers or on the stack and never appears in static data sections, fully eliminating loader dependencies and relocation requirements.
In case of working with constant arrays, array elements are packed into machine-word-sized integers at compile time and unpacked at runtime:

```cpp
template <typename TChar, USIZE N>
class EMBEDDED_ARRAY
{
    alignas(USIZE) USIZE words[WordCount]{};

    consteval EMBEDDED_ARRAY(const TChar (&src)[N]) {
        for (USIZE i = 0; i < N; ++i) {
            // Pack each element byte-by-byte into words
            for (USIZE b = 0; b < sizeof(TChar); ++b)
                SetByte(i * sizeof(TChar) + b, (src[i] >> (b * 8)) & 0xFF);
        }
    }

    TChar operator[](USIZE index) const {
        // Unpack element from words at runtime
    }
};
```

Usage:
```cpp
constexpr UINT32 lookup[] = {0x12345678, 0xABCDEF00};
auto embedded = MakeEmbedArray(lookup);
UINT32 value = embedded[0]; // Unpacked at runtime
```

### Problem 2: Floating-Point Constants

Using floating-point arithmetic in C-generated shellcode introduces additional issues, as floating-point constants are typically emitted into read-only data sections such as `.rdata`. In a loaderless execution environment, these sections are not available, causing generated code to reference invalid memory. Despite its similarity to the previous problem, the approaches and technical details differ markedly.

#### Traditional Approach

Represent floating-point values using their hexadecimal (IEEE-754) representation and cast to double at runtime:

```cpp
UINT64 f = 0x3426328629; // IEEE-754 representation
double d = *((double*)&f);
```

Or convert from string/integer at runtime:

```cpp
// Two ways: with string or with integer
toDouble("1.2353");
toDouble(1, 232342);
```

#### Why Traditional Approaches Fail

While this avoids embedding floating-point literals, it increases code size and complexity, which is not suitable for this type of work.

#### NOSTDLIB-RUNTIME Solution: Floating-Point Constant Embedding

We solve this issue for double values; applying the same technique to float values is straightforward. Floating-point values are converted at compile time into IEEE-754 bit patterns and injected directly into registers as immediate operands:

```cpp
struct EMBEDDED_DOUBLE
{
    consteval explicit EMBEDDED_DOUBLE(double v) {
        bits = __builtin_bit_cast(unsigned long long, v);
    }

    operator double() const {
        return __builtin_bit_cast(double, bits);
    }
};
```

Usage:
```cpp
auto pi = 3.14159_embed; // IEEE-754 as immediate value
```

Assembly Output:
```asm
movabsq $0x400921f9f01b866e, %rax ; Pi as 64-bit immediate
```

This eliminates all floating-point constants from `.rdata` and avoids implicit compiler-generated helpers.

### Problem 3: Function Pointers

Using function pointers in C-generated shellcode introduces relocation dependencies, as function addresses are normally resolved by the loader. In a loaderless execution environment, these relocations are not applied, causing indirect function calls to reference invalid addresses.

#### Traditional Approach

Perform manual relocation at runtime, as discussed above.

#### Why Traditional Approaches Fail

The same issues remain: increased complexity, fragility, and sensitivity to compiler optimizations.

#### NOSTDLIB-RUNTIME Solution: Function Pointer Embedding

We introduce the `EMBED_FUNC` macro, which uses inline assembly to compute pure relative offsets without relying on absolute addresses. The target architecture is selected at compile time using CMake-defined macros, ensuring correct code generation without relocation dependencies. The implementation is located [here](include/bal/types/embedded/embedded_function_pointer.h).

### Problem 4: 64-bit Arithmetic on 32-bit Systems

Performing arithmetic with 64-bit integers on a 32-bit system, or with floating-point numbers, can cause issues because the compiler expects certain helper routines to be present.

#### Traditional Approach

Implement those helper routines manually.

#### Why Traditional Approaches Fail

Manual implementations are error-prone and may not cover all edge cases the compiler expects.

#### NOSTDLIB-RUNTIME Solution: 64-bit Arithmetic on 32-bit Systems

We define custom `UINT64` and `INT64` classes that store values as two 32-bit words (high and low). All operations are decomposed into 32-bit arithmetic with manual carry handling:

- **Multiplication**: Uses 16-bit partial products to avoid overflow, accumulating results with carry propagation across four 16-bit result segments
- **Division**: Implements bit-by-bit long division, extracting one quotient bit per iteration from most-significant to least-significant
- **Shifts**: Combines shifts across the word boundary with proper carry handling

```cpp
// 64-bit multiplication using only 32-bit operations
// Split into 16-bit parts: (a3,a2,a1,a0) * (b3,b2,b1,b0)
UINT32 p0 = a0 * b0;  // bits [0:31]
UINT32 p1 = a1 * b0;  // bits [16:47]
UINT32 p2 = a0 * b1;  // bits [16:47]
// ... accumulate with carry propagation
```

This eliminates the need for compiler-expected helper routines and guarantees no `.rdata` generation.

### Problem 5: CRT and Runtime Dependencies

Standard C/C++ programs depend on the C runtime (CRT) for initialization, memory management, and various helper functions. Shellcode cannot assume the presence of these.

#### Traditional Approach

Manually implement required CRT functions and avoid using features that depend on runtime initialization.

#### Why Traditional Approaches Fail

This is tedious, incomplete, and doesn't address the fundamental issue of static API imports remaining visible to analysis tools.

#### NOSTDLIB-RUNTIME Solution: Runtime Independence

NOSTDLIB-RUNTIME achieves complete independence from the C runtime (CRT) and standard libraries by providing fully custom implementations for essential services such as memory management, string manipulation, formatted output, and runtime initialization. Instead of relying on CRT startup code, NOSTDLIB-RUNTIME defines a custom entry point, enabling execution without loader-managed runtime setup.

Interaction with Windows system functionality is performed through low-level native interfaces. The runtime traverses the Process Environment Block (PEB) to locate loaded modules and parses PE export tables to resolve function addresses using hash-based lookup. By avoiding import tables, string-based API resolution, and `GetProcAddress` calls, NOSTDLIB-RUNTIME minimizes static analysis visibility and enables execution in constrained or adversarial environments.

### Problem 6: Type Conversions

Type conversions between integers and floating-point values can cause the compiler to emit hidden constants or helper routines.

#### Traditional Approach

Avoid type conversions or implement manual conversion functions.

#### NOSTDLIB-RUNTIME Solution: Pure Integer-Based Conversions

All type conversions are implemented using explicit bitwise and integer operations, preventing the compiler from emitting hidden constants or helper routines:

```cpp
// Extracts integer value from IEEE-754 without FPU instructions
INT64 d_to_i64(const DOUBLE& d)
{
    UINT64 bits = d.bits;
    int exponent = ((bits >> 52) & 0x7FF) - 1023;
    UINT64 mantissa = (bits & 0xFFFFFFFFFFFFF) | 0x10000000000000;
    // ... bit shifting logic ...
}
```

## NOSTDLIB-RUNTIME Architecture Overview

Within this work, we present NOSTDLIB-RUNTIME, a C++23 runtime designed to achieve fully position-independent execution by eliminating dependencies on `.rdata`, the C runtime (CRT), and other loader-managed components. NOSTDLIB-RUNTIME provides full position-independence for shellcode, code injection, and embedded systems, enabling execution from arbitrary memory locations.

### Design Goals

NOSTDLIB-RUNTIME is designed around the following goals:

1. **True position independence**
   Execution must not depend on fixed load addresses or loader-handled relocations.

2. **Elimination of `.rdata` dependencies**
   No string literals or floating-point constants stored in read-only data sections.

3. **No CRT or standard library reliance**
   A completely standalone runtime with no dependency on the C runtime or standard libraries.

4. **No static imports**
   All Windows functionality is resolved dynamically at runtime.

5. **Modern C++ expressiveness**
   Support for C++23 language features without requiring runtime initialization.

6. **Multi-architecture and multi-platform support**
   Compatibility across x86, x64, and ARM architectures on both Windows and Linux. The platform abstraction layer cleanly separates OS-specific code, making it straightforward to add support for additional operating systems by implementing the appropriate low-level interfaces.

7. **Full optimization support**
   Supports all LLVM optimization levels, allowing builds from unoptimized (`-O0`) to maximum optimization (`-O3`) or size optimization (`-Oz`).

### Three-Layer Architecture

NOSTDLIB-RUNTIME is built on a clean three-layer abstraction that separates concerns and enables multi-platform support:

```
┌─────────────────────────────────────────────────────────────┐
│  RAL (Runtime Abstraction Layer)                            │
│  High-level features: Cryptography, Networking, TLS 1.3     │
├─────────────────────────────────────────────────────────────┤
│  PAL (Platform Abstraction Layer)                           │
│  OS-specific: Windows PEB/NTAPI, Linux syscalls             │
├─────────────────────────────────────────────────────────────┤
│  BAL (Base Abstraction Layer)                               │
│  Platform-independent: Types, Memory, Strings, Algorithms   │
└─────────────────────────────────────────────────────────────┘
```

**BAL (Base Abstraction Layer)** provides platform-independent primitives:
- Embedded types (`EMBEDDED_STRING`, `EMBEDDED_DOUBLE`, `EMBEDDED_ARRAY`)
- Numeric types (`UINT64`, `INT64`, `DOUBLE`) with guaranteed no `.rdata` generation
- Memory operations, string utilities, and formatting
- Algorithms (DJB2 hashing, Base64, random number generation)

**PAL (Platform Abstraction Layer)** handles OS and hardware specifics:
- Windows: PEB walking, PE parsing, NTAPI-based operations
- Linux: Direct syscall interface without libc
- Console I/O, file system, networking, memory allocation

**RAL (Runtime Abstraction Layer)** provides high-level application features:
- Cryptography: SHA-256/384/512, HMAC, ChaCha20-Poly1305, ECC
- Networking: DNS resolution, HTTP client, WebSocket, TLS 1.3

### Entry Point Placement

We would also like to highlight a challenge faced during development. Ensuring that the shellcode entry point was placed at the very beginning of the `.text` section proved to be challenging, yet crucial for this architecture. After extensive research, we discovered a solution:

For MSVC:
```
# Custom entry point (no CRT)
/Entry:_start

# Function ordering for MSVC
/ORDER:@orderfile.txt
```

For Clang, we used custom linker scripts with section ordering directives to achieve the same result.

## Build System

### Critical Compiler Flags

Achieving true position-independence requires specific compiler flags that prevent the compiler from generating `.rdata` dependencies:

```bash
-fno-jump-tables      # CRITICAL: Prevents switch statement jump tables in .rdata
-fno-exceptions       # No exception handling tables (.pdata/.xdata)
-fno-rtti             # No runtime type information (no typeinfo in .rdata)
-nostdlib             # No standard C/C++ libraries (no CRT linkage)
-fno-builtin          # Disable compiler built-in functions
-ffunction-sections   # Each function in own section (enables dead code elimination)
-fdata-sections       # Each data item in own section (enables garbage collection)
-fshort-wchar         # 2-byte wchar_t (Windows ABI compatibility)
```

The `-fno-jump-tables` flag is particularly critical—without it, `switch` statements generate jump tables stored in `.rdata`, breaking position-independence.

### Post-Build Verification

The build system automatically verifies that the final binary contains no separate data sections:

```cmake
# Verify no .rdata/.rodata/.data/.bss sections exist
string(REGEX MATCH "\\.rdata" RDATA_FOUND "${MAP_CONTENT}")
string(REGEX MATCH "\\.rodata" RODATA_FOUND "${MAP_CONTENT}")
string(REGEX MATCH "\\.data" DATA_FOUND "${MAP_CONTENT}")
string(REGEX MATCH "\\.bss" BSS_FOUND "${MAP_CONTENT}")

if(RDATA_FOUND OR RODATA_FOUND OR DATA_FOUND OR BSS_FOUND)
    message(FATAL_ERROR "Data section detected - breaks position-independence!")
endif()
```

This verification runs after every build, ensuring that code changes don't accidentally introduce `.rdata` dependencies.

## Windows Implementation

NOSTDLIB-RUNTIME integrates deeply with Windows internals to provide a fully functional, standalone execution environment while maintaining position independence.

### Low-Level Native Interfaces

By completely eliminating static import tables and bypassing loader-dependent API resolution mechanisms such as `GetProcAddress`, NOSTDLIB-RUNTIME removes all dependencies on the operating system's runtime initialization and dynamic linking processes. This ensures that all required function addresses are resolved internally at runtime, using hash-based lookups of exported symbols in loaded modules. As a result, the generated binaries are fully self-contained, do not rely on predefined memory locations, and can execute correctly from any arbitrary memory address without requiring relocation tables or loader-managed fixups.

### File System Support
A complete abstraction over `NTAPI` enables file and directory operations:
* File creation, reading, writing, deletion
* Directory creation, enumeration, deletion
* Path and attribute management

All file system operations are executed without relying on CRT or standard libraries.

### Console Output
Printf-style output is implemented natively within the runtime. This allows robust console output without runtime support.

### Cryptography and Networking
NOSTDLIB-RUNTIME provides a complete cryptographic and networking stack:
* Cryptography: SHA-256/512, HMAC, ChaCha20, ECC, Base64 encoding/decoding
* Networking: DNS resolution, HTTP client, WebSocket connections, TLS 1.3 with certificate verification

All functionality is implemented using low-level native interfaces to avoid external dependencies.

## Practical Use Cases

NOSTDLIB‑RUNTIME is designed for execution environments where traditional runtime assumptions do not apply. Its architecture makes it particularly suitable for the following domains:
- Shellcode and loaderless code execution
- Security research and malware analysis
- Embedded and low-level system programming
- Cross-architecture C++ development
- Environments without standard C runtime support

## To Do
This project is still a work in progress. Below is a list of remaining tasks and planned improvements. Any help or contributions are greatly appreciated.

- Support for additional platforms (macOS, FreeBSD)
- Windows direct syscall implementations (bypassing ntdll)
- Compile-time polymorphism

## Conclusion

NOSTDLIB-RUNTIME is not merely a library—it is a proof of concept that challenges long-held assumptions about C++, binary formats, and position-independent execution across multiple platforms. This project compiles into a PE file on Windows or an ELF file on Linux, supporting i386, x86_64, armv7a, and aarch64 architectures. The resulting binary can run both as a standalone executable and as shellcode after extracting the `.text` section. By eliminating `.rdata`, CRT dependencies, relocations, and static API references, NOSTDLIB-RUNTIME enables a new class of C++ programs capable of running in environments where traditional C++ has never been viable.

The platform abstraction layer demonstrates that the same high-level C++23 codebase can target fundamentally different operating systems—Windows with its PEB walking and NTAPI interfaces, and Linux with its direct syscall approach—while maintaining identical position-independence guarantees. As demonstrated throughout this work, modern C++23 compile-time features and carefully selected compiler intrinsics play a key role in achieving these guarantees, allowing expressive high-level code while preserving strict low-level control.

This project is intended for researchers, systems programmers, and security engineers who are willing to work beneath high-level abstractions and take full control of the machine. Any unauthorized or malicious use of this software is strictly prohibited and falls outside the scope of the project's design goals.
