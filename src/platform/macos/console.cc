#include "console.h"
#include "syscall.h"
#include "system.h"

// Write ANSI/ASCII string to console (straightforward)
UINT32 Console::Write(const CHAR *text, USIZE length)
{
	SSIZE result = System::Call(SYS_WRITE, STDOUT_FILENO, (USIZE)text, length);
	return (result >= 0) ? (UINT32)result : 0;
}
