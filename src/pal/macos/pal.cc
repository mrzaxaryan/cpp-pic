#include "pal.h"
#include "syscall.h"

// macOS syscall number for exit
constexpr USIZE SYS_EXIT = 1;

// macOS process exit implementation
NO_RETURN VOID ExitProcess(USIZE code)
{
    Syscall::syscall1(SYS_EXIT, code);
    __builtin_unreachable();
}
