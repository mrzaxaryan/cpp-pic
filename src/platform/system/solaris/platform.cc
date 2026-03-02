#include "platform/platform.h"
#include "platform/common/solaris/syscall.h"
#include "platform/common/solaris/system.h"

// Solaris process exit implementation
NO_RETURN VOID ExitProcess(USIZE code)
{
	System::Call(SYS_EXIT, code);
	__builtin_unreachable();
}
