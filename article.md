# CPP-PIC: A Modern C++ Approach to Zero-Dependency, Position-Independent Code Generation

## Intoduction

Shellcode, in security research and malware analysis, is a small, self-contained sequence of machine instructions that can be injected into memory and executed from an arbitrary location. It must operate without relying on external components such as DLLs, runtime initialization routines or fixed stack layouts. Because of these strict constraints, shellcode is traditionally written in assembly language, which provides precise control over instructions, registers, and memory access.
While assembly ensures fully dependency-free and position-independent execution, it quickly becomes impractical as complexity grows due to its low-level nature and limited expressiveness. High-level languages like C greatly improve readability, maintainability, and development speed, but standard C and C++ compilation models introduce significant challenges for shellcode development. Modern compilers typically generate binaries that depend on runtime libraries, import tables, relocation information, and read-only data sections. These dependencies violate the core requirement of shellcode—execution independent of any fixed memory layout or external support—and, moreover, these problems do not admit simple or universally effective solutions.
As a result, code produced by conventional toolchains cannot be used as standalone shellcode without substantial modification or manual restructuring.

## Motivation
A long time ago, in a corner of the darknet, two users debated which programming language was better. One argued that assembly provides almost complete control over execution, while the other claimed that C, as a higher-level language, is a better choice for implementing complex systems, since writing something like a TLS client in assembly is impractical.
That debate ended with kick out ASM coder from that forum.

With this work, I would like to add my two cents to that debate, even though much time has passed, by arguing that it is possible to leverage modern C++23 without compromising the strict execution guarantees required for shellcode.



## Common Problems Encountered
1. Implicit reliance on `.rdata` for constant data
2. Mandatory relocation processing
3. Dependence on C runtime (CRT) initialization
4. Static exposure of imported APIs
5. Inability to execute reliably from arbitrary memory locations

**1.1. Traditinal Solutions**

* **Problem:** When writing shellcode in C, only the `.text` section is available after compilation, so global constants and string literals cannot be used because they are placed in `.rdata` or `.data`.

* **Solution:** Minimize usage of construction that will cause generation of the data in `.rdata` or `.data`, with moving string literals to stack. To create stack‑based strings, represent the string as a character array stored in a local variable. This solution also obfuscates strings:

    ```
    // "example.exe"
    char path[] = {'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'e', 'x', 'e', '\0'};
    ```

    and in the case of wide character strings, the notation is as follows:

    ```
    // L"example.exe"
    wchar_t path[] = { L'e', L'x', L'a', L'm', L'p', L'l', L'e', L'.', L'e', L'x', L'e', L'\0' };
    ```

* **Why This Approach Is Not Suitable**

    This approach is not universal because it relies on compiler‑specific behavior and assumptions about stack layout. Compiler optimizations may modify or eliminate stack variables, breaking the intended execution. In addition, manually embedding constants and strings increases shellcode size, making it easier to detect and difficult to scale. Hand‑embedding large arrays is inefficient and does not guarantee that the data will remain on the stack. For large strings or when compiler optimizations are enabled, the compiler may place the data in other sections instead of stack-if string size is large enough, compiler may place the data in `.rdata` section. This approach also makes the code less readable and harder to maintain. 


* **Problem:** C‑generated shellcode relies on loader‑handled relocations that are not applied in a loaderless execution environment, preventing reliable execution from arbitrary memory.

* **Solution:** Preform the relocation manually:

    At runtime, the shellcode can determine its own position in memory and perform the loader’s work manually. In this approach, constants and strings may reside in sections such as `.rdata`, which are then merged into the `.text` section using a linker script. During execution, relocation entries are processed explicitly to fix up absolute addresses. 

    ```
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

    And then preform relocation like this:

    ```
    CHAR *string = "Hello, World!";
    CHAR *relocatedString = string + (SSIZE)startAddress;

    WCHAR *wideString = L"Hello, World!";
    WCHAR *relocatedWideString = (WCHAR*)((CHAR*)wideString + (SSIZE)startAddress);
    ```
    

* **Why This Approach Is Not Suitable**

    This method adds extra code and complexity, depends on unstable compiler behavior, and can easily break under optimization. As a result, it is unreliable and does not scale well for real‑world shellcode.

* **Problem:** Using floating‑point arithmetic in C‑generated shellcode introduces additional issues, as floating‑point constants are typically emitted into read‑only data sections such as `.rdata`. In a loaderless execution environment, these sections are not available, causing generated code to reference invalid memory. Consequently, floating‑point operations cannot be relied upon for safe, position‑independent shellcode execution.

* **Solution:** The well-known solution is to represent floating‑point values using their hexadecimal (IEEE‑754) representation and then cast this value to a double at runtime:

    ```   
    uint64 f = 0x3426328629; // IEEE‑754 representation
    double d  = *((double*)&f); 
    ```

    Alternative solution is taking string or integer and convert it into double at runtime, which also has its specific problems:

    ```
    // two ways: with string or with integer.
    todouble("1.2353");
    todouble(1,232342);
    ```    

* **Why This Approach Is Not Suitable**

    While this really avoids embedding floating‑point literals, it increases code size and complexity, which is not suitable in this case of work.

* **Problem:** Using function pointers in C‑generated shellcode introduces relocation dependencies, as function addresses are normally resolved by the loader. In a loaderless execution environment, these relocations are not applied, causing indirect function calls to reference invalid addresses and break execution from arbitrary memory locations.

* **Solution:**
    The traditional approach is to perform manual relocation at runtime. Problems of that approach were discussed above.


* **Problem:** Performing arithmetic with 64-bit integers on a 32-bit system, or with floating-point numbers, can cause issues because the compiler expects certain helper routines to be present.
* **Solution:** The common way to solve this problem is to implement that helper routines manually.

## What We Offer
Within this work, we present fully alternative approaches to solve these problems. We introduce CPP‑PIC, a C++23 runtime designed to achieve fully position‑independent execution by eliminating dependencies on `.rdata`, the C runtime (CRT), and other loader‑managed components. CPP-PIC provides a fully position-independence for
shellcode, code injection and embedded system capable of executing correctly from
arbitrary memory locations.

### CPP-PIC Design Goals

CPP-PIC is designed around the following goals:

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

6. **Multi-architecture support**  
   Compatibility across x86, x64, and ARM architectures. While the current implementation is Windows-oriented, we designed it to make supporting other operating systems as straightforward as possible by replacing Windows‑specific low‑level interfaces with their platform‑appropriate equivalents. A Linux implementation is currently in progress.
   
7. **Full Optimization support**
   Supports all LLVM optimization levels, allowing builds from unoptimized `(-O0)` to maximum optimization or performance size(`-Oz` or `-03`).


### Solutions We Offer
* **Solution: Compile-Time String Decomposition**

    CPP-PIC replaces conventional string literals with compile-time decomposed representations.
    Using C++23 features such as user-defined literals, variadic templates, and fold expressions,
    strings are decomposed into individual characters at compile - time:

    ```
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

    Usage :
    ``` 
    auto msg = "Hello, World!"_embed; // Embedded in code, not .rdata
    ```
     

    Assembly Output:
    ```
    movw $0x48, (%rdi) ; 'H'
    movw $0x65, 2(%rdi) ; 'e'
    movw $0x6C, 4(%rdi) ; 'l'
    movw $0x6C, 6(%rdi) ; 'l'
    movw $0x6F, 8(%rdi) ; 'o'
    movw $0x48, (%rdi) ; 'H'
    movw $0x65, 2(%rdi) ; 'e'
    movw $0x6C, 4(%rdi) ; 'l'
    movw $0x6C, 6(%rdi) ; 'l'
    movw $0x6F, 8(%rdi) ; 'o'
    ```

    As a result, string data exists only transiently and never appears in static data sections.

* **Solution: Floating-Point Constant Embedding** 
    We solve this issue for double values, and applying the same technique to float values is straightforward and does not introduce any additional complications, so no further attention is required for that case.
    Floating-point values are converted at compile time into IEEE-754 bit patterns and injected
    directly into registers as immediate operands. This eliminates all floating-point constants
    from `.rdata` and avoids implicit compiler-generated helpers.

    ```
    struct EMBEDDED_DOUBLE 
    {
        consteval explicit EMBEDDED_DOUBLE(double v) {
            bits = __builtin_bit_cast(unsigned long long, v);
        }

        operator double() const {
            return __builtin_bit_cast(double, bits);
        }
    };

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

    Usage :
    ```
    auto pi = 3.14159_embed; // IEEE-754 as immediate value
    ```


    Assembly Output:
    ```
    movabsq $0x400921f9f01b866e, %rax ; Pi as 64-bit immediate
    ```
    
* **Solution: Pure Integer-Based Conversions**

    All type conversions are implemented using explicit bitwise and integer operations, preventing the compiler from emitting hidden constants or helper routines.
       
    ```
    // Extracts integer value from IEEE-754 without FPU instructions
    INT64 operator(INT64)(const DOUBLE& d) 
    {
        UINT64 bits = d.bits;
        int exponent = ((bits >> 52) & 0x7FF) - 1023;
        UINT64 mantissa = (bits & 0xFFFFFFFFFFFFF) | 0x10000000000000;
        // ... bit shifting magic ...
    }

    // Extracts integer value from IEEE-754 without FPU instructions
    INT64 operator(INT64)(const DOUBLE& d) 
    {
        UINT64 bits = d.bits;
        int exponent = ((bits >> 52) & 0x7FF) - 1023;
            UINT64 mantissa = (bits & 0xFFFFFFFFFFFFF) | 0x10000000000000;
            // ... bit shifting magic ...
    }
    ```
    
* **Solution: Function Pointer Embedding**
    We introduce EMBED_FUNC macro, which uses inline assembly to compute pure relative offsets without relying on absolute addresses. The target architecture is selected at compile time using CMake‑defined macros, ensuring correct code generation without relocation dependencies. The implementation of that approach is placed in file `embedded_function_pointer.h`.

* **Solution: 64-bit Arithmetic on 32-bit Systems**  

    To perform 64-bit arithmetic on 32-bit systems, we manually defined a `uint64` class along with its arithmetic operations. This eliminates the need for compiler‑expected helper routines implementations.

* **Solution: Runtime Independence**

    CPP-PIC achieves complete independence from the C runtime (CRT) and standard libraries by providing fully custom implementations for essential services such as memory management, string manipulation, formatted output, and runtime initialization. Rather than relying on CRT startup code, CPP-PIC defines a custom entry point, enabling execution without loader-managed runtime setup.
    Interaction with Windows system functionality is performed through low-level native interfaces. The runtime traverses the Process Environment Block (PEB) to locate loaded modules and parses PE export tables to resolve function addresses using hash-based lookup. By avoiding import tables, string-based API resolution, and `GetProcAddress` calls, CPP-PIC minimizes static analysis visibility and enables execution in constrained or adversarial environments.

I would also like to bring your attention to a challenge we faced during the project improvement phase. Ensuring that the shellcode entry point was placed at the very beginning of the `.text` section proved to be challenging, yet crucial for this architecture. After many hours of research, we discovered a solution using a linker script, which worked well under GCC. To achieve the same result with Clang, we used the following configuration:

    
    # Merge .rdata into .text (CRITICAL for PIC)
    /MERGE:.rdata=.text

    # Custom entry point (no CRT)
    /Entry:_start

    # Funrepresentations MSVC
    /ORDER:@orderfile.txt
    
## Windows Implementation

CPP-PIC integrates deeply with Windows internals to provide a fully functional, standalone execution environment while maintaining position independence.
### Low-Level Native Interfaces

By completely eliminating static import tables and bypassing loader-dependent API resolution mechanisms such as `GetProcAddress`, CPP-PIC removes all dependencies on the operating system’s runtime initialization and dynamic linking processes. This ensures that all required function addresses are resolved internally at runtime, using hash-based lookups of exported symbols in loaded modules. As a result, the generated binaries are fully self-contained, do not rely on predefined memory locations, and can execute correctly from any arbitrary memory address without requiring relocation tables or loader-managed fixups.

### File System Support
A complete abstraction over `NTAPI` enables file and directory operations:
* File creation, reading, writing, deletion
* Directory creation, enumeration, deletion
* Path and attribute management

All file system operations are executed without relying on CRT or standard libraries.

### Console Output
Printf-style output is implemented natively within the runtime.This allows robust console output without runtime support.


### Cryptography and Networking
CPP-PIC provides a complete cryptographic and networking stack:
* Cryptography: `SHA-256/512`, `HMAC`, `ChaCha20`, `ECC`,` Base64` encoding/decoding
* Networking: `DNS resolution`, `HTTP client`, `WebSocket` connections, `TLS 1.3` with certificate verification

All functionality is implemented using low-level native interfaces to avoid external dependencies.

## Practical Use Cases
CPP-PIC is designed to support execution environments where traditional runtime assumptions do not hold. Its architecture makes it particularly suitable for the following domains:
- Shellcode and loaderless code execution
- Security research and malware analysis
- Embedded and low-level system programming
- Cross-architecture C++ development
- Environments without standard C runtime support

## Conclusion

CPP-PIC is not merely a library — it is a proof of concept that challenges long-held assumptions about C++, Windows binaries, and position-independent execution. By eliminating `.rdata`, CRT dependencies, relocations, and static API references, CPP-PIC enables a new class of C++ programs capable of running in environments where traditional C++ has never been viable. As demonstrated throughout this work, modern C++23 compile‑time features and carefully selected compiler intrinsics play a key role in achieving these guarantees, allowing expressive high‑level code while preserving strict low‑level control.

This project is intended for researchers, systems programmers, and security engineers who are willing to work beneath high-level abstractions and take full control of the machine. Any unauthorized or malicious use of this software is strictly prohibited and falls outside the scope of the project’s design goals.
