#include "pal.h"
#include "ntdll.h"
#include "peb.h"
#include "pe.h"

PVOID ResolveExportAddressFromPebModule(USIZE moduleNameHash, USIZE functionNameHash)
{
    // Resolve the module handle
    PVOID moduleBase = GetModuleHandleFromPEB(moduleNameHash);
    // Validate the module handle
    if (moduleBase == NULL)
        return NULL;
    // Resolve the function address
    PVOID functionAddress = GetExportAddress(moduleBase, functionNameHash);
    return functionAddress;
}

NO_RETURN VOID ExitProcess(USIZE code)
{
    NTDLL::ZwTerminateProcess(NTDLL::NtCurrentProcess(), (NTSTATUS)(code));
    __builtin_unreachable();
}
