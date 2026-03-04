#include "platform/io/console.h"
#include "platform/platform.h"
#include "platform/common/windows/ntdll.h"
#include "platform/common/windows/peb.h"

UINT32 Console::Write(Span<const CHAR> text)
{
	PPEB peb = GetCurrentPEB();
	IO_STATUS_BLOCK ioStatusBlock;
	Memory::Zero(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));
	(void)NTDLL::ZwWriteFile(peb->ProcessParameters->StandardOutput, nullptr, nullptr, nullptr, &ioStatusBlock, (PVOID)text.Data(), text.Size(), nullptr, nullptr);
	return (UINT32)ioStatusBlock.Information;
}
