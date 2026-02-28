#include "console.h"
#include "platform.h"
#include "ntdll.h"
#include "peb.h"

UINT32 Console::Write(const CHAR *text, USIZE length)
{
	PPEB peb = GetCurrentPEB();
	IO_STATUS_BLOCK ioStatusBlock;
	Memory::Zero(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));
	(void)NTDLL::ZwWriteFile(peb->ProcessParameters->StandardOutput, nullptr, nullptr, nullptr, &ioStatusBlock, (PVOID)text, length, nullptr, nullptr);
	return (UINT32)ioStatusBlock.Information;
}
