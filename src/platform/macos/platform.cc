#include "platform.h"
#include "syscall.h"
#include "system.h"

// macOS process exit implementation
NO_RETURN VOID ExitProcess(USIZE code)
{
	System::Call(SYS_EXIT, code);
	__builtin_unreachable();
}
