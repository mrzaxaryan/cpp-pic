#include "pal.h"
#include "system.h"

// Linux syscall numbers for this file
#if defined(ARCHITECTURE_X86_64)
constexpr USIZE SYS_EXIT = 60;
#elif defined(ARCHITECTURE_I386)
constexpr USIZE SYS_EXIT = 1;
#elif defined(ARCHITECTURE_AARCH64)
constexpr USIZE SYS_EXIT = 93;
#elif defined(ARCHITECTURE_ARMV7A)
constexpr USIZE SYS_EXIT = 1;
#endif

// Linux process exit implementation
NO_RETURN VOID ExitProcess(USIZE code)
{
    System::Call(SYS_EXIT, code);
    __builtin_unreachable();
}
