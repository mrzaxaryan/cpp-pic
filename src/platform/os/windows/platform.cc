#include "platform.h"
#include "ntdll.h"

NO_RETURN VOID ExitProcess(USIZE code)
{
    (void)NTDLL::ZwTerminateProcess(NTDLL::NtCurrentProcess(), (NTSTATUS)(code));
    __builtin_unreachable();
}
