#include "platform/io/console.h"
#if defined(PLATFORM_LINUX)
#include "platform/os/linux/common/syscall.h"
#include "platform/os/linux/common/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/os/macos/common/syscall.h"
#include "platform/os/macos/common/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/os/solaris/common/syscall.h"
#include "platform/os/solaris/common/system.h"
#endif

UINT32 Console::Write(Span<const CHAR> text)
{
	SSIZE result = System::Call(SYS_WRITE, STDOUT_FILENO, (USIZE)text.Data(), text.Size());
	return (result >= 0) ? (UINT32)result : 0;
}
