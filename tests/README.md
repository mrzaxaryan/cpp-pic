# Tests Directory

This directory contains test implementation files for the CPP-PIC project.

## Test Organization

All tests are header-based and located in `include/tests/`. Tests are executed from `src/start.cc` as part of the main entry point.

## Test Suites

1. **Djb2Tests** - Hash function validation
2. **MemoryTests** - Memory operations (Copy, Zero, Compare)
3. **StringTests** - String manipulation
4. **Uint64Tests** - 64-bit unsigned arithmetic
5. **Int64Tests** - 64-bit signed arithmetic
6. **DoubleTests** - IEEE-754 floating-point operations
7. **StringFormatterTests** - Printf-style formatting

## Running Tests

Tests automatically run when the executable is launched:

```powershell
# Run Windows x64 release build
.\build\windows\x86_64\release\output.exe

# Run Windows i386 debug build
.\build\windows\i386\debug\output.exe

# Run Windows ARM64 release build
.\build\windows\aarch64\release\output.exe
```

**Expected Output:**
```
CPP-PIC Runtime Starting...
Running DJB2 Tests... PASSED
Running Memory Tests... PASSED
Running String Tests... PASSED
Running UINT64 Tests... PASSED
Running INT64 Tests... PASSED
Running Double Tests... PASSED
Running StringFormatter Tests... PASSED
All tests passed!
```

## Test Coverage

### UINT64 Tests
- Construction (default, single-arg, two-arg, native unsigned long long)
- Arithmetic (addition, subtraction, multiplication, division, modulo)
- Bitwise operations (AND, OR, XOR, NOT)
- Shift operations (left/right shifts, cross-word shifts, edge cases)
- Comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`)
- Increment/Decrement (pre/post with carry/borrow)
- Overflow behavior

### INT64 Tests
- All UINT64 tests plus signed-specific behavior
- Negative number handling
- Sign extension in shifts
- Signed comparison operators

### StringFormatter Tests
- Integer formats (`%d`, `%u`, `%ld`)
- Hex formats (`%x`, `%X`, `%#x`)
- String formats (`%s`, `%ls`)
- Character format (`%c`)
- Float format (`%f`, `%.Nf`)
- Width/padding (right-align, left-align `%-`, zero-padding `%0`)
- Percent literal (`%%`)

### Double Tests
- IEEE-754 bit pattern operations
- Double to integer conversions
- Integer to double conversions
- Arithmetic operations

### Memory Tests
- Memory::Copy - Copy memory regions
- Memory::Zero - Zero out memory
- Memory::Compare - Compare memory regions

### String Tests
- String::Length - Calculate string length
- String::Copy - Copy strings
- String::Compare - Compare strings

### DJB2 Tests
- Hash function consistency
- Compile-time vs runtime hash matching
- Known hash values

## CI/CD Testing

GitHub Actions automatically tests all configurations:
- **Build Matrix**: i386, x86_64, aarch64 (Windows)
- **Test Execution**:
  - i386: Native execution via WoW64
  - x86_64: Native execution
  - aarch64: Self-hosted ARM64 runner
- **Validation**: Exit code 0 = all tests passed

## Adding New Tests

To add a new test suite:

1. Create a test header in `include/tests/`:

```cpp
#pragma once
#include "primitives.h"
#include "logger.h"

class MyFeatureTests {
public:
    static BOOL RunAll() {
        BOOL allPassed = TRUE;
        Logger::Info<WCHAR>(L"Running MyFeature Tests..."_embed);

        if (!TestSomething()) {
            allPassed = FALSE;
            Logger::Error<WCHAR>(L"  FAILED: Something"_embed);
        } else {
            Logger::Info<WCHAR>(L"  PASSED: Something"_embed);
        }

        return allPassed;
    }

private:
    static BOOL TestSomething() {
        // Test implementation
        return TRUE;
    }
};
```

2. Include and call from `src/start.cc`:

```cpp
#include "tests/my_feature_tests.h"
// ...
MyFeatureTests::RunAll();
```

## Future Enhancements

Potential improvements:
- Move test `.cc` files here (keep headers in `include/tests/`)
- Add integration tests
- Add benchmark tests
- Add platform-specific test cases
- Add automated regression testing

## References

- [Main README](../README.md) - Project overview
- [Architecture Guide](../docs/architecture.md) - System architecture
- [Platform Guide](../docs/platform_guide.md) - Windows implementation
