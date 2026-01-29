#include "pal.h"
#include "syscall.h"

// Linux process exit implementation
NO_RETURN VOID ExitProcess(USIZE code)
{
    Syscall::syscall1(Syscall::SYS_EXIT, code);
    __builtin_unreachable();
}
