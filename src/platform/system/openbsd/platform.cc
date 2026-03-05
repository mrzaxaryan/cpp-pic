#include "platform/platform.h"
#include "platform/common/openbsd/syscall.h"
#include "platform/common/openbsd/system.h"

// OpenBSD process exit implementation
NO_RETURN VOID ExitProcess(USIZE code)
{
	System::Call(SYS_EXIT, code);
	__builtin_unreachable();
}
