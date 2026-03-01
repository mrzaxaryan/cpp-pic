#include "platform/io/console.h"
#include "platform/os/linux/syscall.h"
#include "platform/os/linux/system.h"

UINT32 Console::Write(Span<const CHAR> text)
{
	SSIZE result = System::Call(SYS_WRITE, STDOUT_FILENO, (USIZE)text.Data(), text.Size());
	return (result >= 0) ? (UINT32)result : 0;
}
