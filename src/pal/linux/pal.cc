#include "pal.h"
#include "syscall.h"
#include "system.h"

// Linux process exit implementation
NO_RETURN VOID ExitProcess(USIZE code)
{
    System::Call(SYS_EXIT, code);
    __builtin_unreachable();
}
