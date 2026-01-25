#include "pal.h"
#include "ntdll.h"
#include "peb.h"
#include "pe.h"

#if defined(PLATFORM_WINDOWS)

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

#if defined(PLATFORM_WINDOWS_I386)

// InitializeRuntime environment data for PIC-style rebasing
// Must be called from _start with a stack-allocated ENVIRONMENT_DATA struct
NOINLINE VOID InitializeRuntime(PENVIRONMENT_DATA envData)
{
    // Get the return address (points inside _start)
    PCHAR currentAddress = (PCHAR)__builtin_return_address(0);

    // i386 function prologue: push ebp; mov ebp, esp
    UINT16 functionPrologue = 0x8955;

    // Scan backward for function prologue to find _start's address
    PCHAR functionStart = ReversePatternSearch(currentAddress, (PCHAR)&functionPrologue, sizeof(functionPrologue));

    // Get the PEB and store envData pointer
    PPEB peb = GetCurrentPEB();
    peb->SubSystemData = (PVOID)envData;

    // Get loader data to find the EXE's entry point
    PPEB_LDR_DATA ldr = peb->LoaderData;
    PLIST_ENTRY list = &ldr->InMemoryOrderModuleList;
    PLIST_ENTRY flink = list->Flink;
    PLDR_DATA_TABLE_ENTRY entry = CONTAINING_RECORD(flink, LDR_DATA_TABLE_ENTRY, InMemoryOrderModuleList);
    USIZE EntryPoint = (USIZE)entry->EntryPoint;

    // Determine if we need to relocate (PIC blob vs normal EXE)
    envData->BaseAddress = functionStart;
    envData->ShouldRelocate = (EntryPoint != (USIZE)functionStart);
}

// Perform pointer relocation for position-independent code
PVOID PerformRelocation(PVOID p)
{
    PENVIRONMENT_DATA envData = GetEnvironmentData();

    if (envData->ShouldRelocate)
    {
        // Running as PIC blob - need to relocate
        return (PVOID)((USIZE)p + (USIZE)envData->BaseAddress - IMAGE_LINK_BASE);
    }
    else
    {
        // Running as normal EXE - pointer is already relocated
        return p;
    }
}

#endif // PLATFORM_WINDOWS_I386

#endif // PLATFORM_WINDOWS