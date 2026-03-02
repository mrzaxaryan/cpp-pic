#include "platform/io/console.h"
#if defined(PLATFORM_LINUX)
#include "platform/common/linux/syscall.h"
#include "platform/common/linux/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/common/macos/syscall.h"
#include "platform/common/macos/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/common/solaris/syscall.h"
#include "platform/common/solaris/system.h"
#endif

UINT32 Console::Write(Span<const CHAR> text)
{
	SSIZE result = System::Call(SYS_WRITE, STDOUT_FILENO, (USIZE)text.Data(), text.Size());
	return (result >= 0) ? (UINT32)result : 0;
}
