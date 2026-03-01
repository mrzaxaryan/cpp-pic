#include "peb.h"
#include "platform.h"
#include "djb2.h"
#include "pe.h"

// Returns the current process's PEB pointer
PPEB GetCurrentPEB(VOID)
{
    PPEB peb;
#if defined(PLATFORM_WINDOWS_X86_64)

    __asm__("movq %%gs:%1, %0" : "=r"(peb) : "m"(*(PUINT64)(0x60)));

#elif defined(PLATFORM_WINDOWS_I386)

    __asm__("movl %%fs:%1, %0" : "=r"(peb) : "m"(*(PUINT32)(0x30)));

#elif defined(PLATFORM_WINDOWS_ARMV7A)

    __asm__("ldr %0, [r9, %1]" : "=r"(peb) : "i"(0x30));

#elif defined(PLATFORM_WINDOWS_AARCH64)

    __asm__("ldr %0, [x18, #%1]" : "=r"(peb) : "i"(0x60));

#else
#error Unsupported platform
#endif
    return peb;
}

// Get the base address of a module by its name
PVOID GetModuleHandleFromPEB(UINT64 moduleNameHash)
{
    PPEB peb = GetCurrentPEB();
    PLIST_ENTRY list = &peb->LoaderData->InMemoryOrderModuleList;
    PLIST_ENTRY entry = list->Flink;

    while (entry != list)
    {
        PLDR_DATA_TABLE_ENTRY module = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderModuleList);

        if (module->BaseDllName.Buffer != nullptr && Djb2::Hash(module->BaseDllName.Buffer) == moduleNameHash)
            return module->DllBase;

        entry = entry->Flink;
    }

    return nullptr;
}

PVOID ResolveExportAddressFromPebModule(UINT64 moduleNameHash, UINT64 functionNameHash)
{
    // Resolve the module handle
    PVOID moduleBase = GetModuleHandleFromPEB(moduleNameHash);
    // Validate the module handle
    if (moduleBase == nullptr)
        return nullptr;
    // Resolve the function address
    PVOID functionAddress = GetExportAddress(moduleBase, functionNameHash);
    return functionAddress;
}