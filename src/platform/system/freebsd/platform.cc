#include "platform/platform.h"
#include "platform/common/freebsd/syscall.h"
#include "platform/common/freebsd/system.h"

// FreeBSD process exit implementation
NO_RETURN VOID ExitProcess(USIZE code)
{
	System::Call(SYS_EXIT, code);
	__builtin_unreachable();
}
