#include "platform/windows/system.h"
#include "platform.h"
#include "peb.h"
#include "djb2.h"

#if !defined(ARCHITECTURE_I386)
// Swap two entries — field by field, prevents compiler memcpy optimization
static inline VOID SwapEntry(ZW_ENTRY* a, ZW_ENTRY* b)
{
    UINT64  tmpHash    = a->nameHash;
    UINT32 tmpRva     = a->rva;
    PVOID  tmpSyscall = a->syscallAddress;
    a->nameHash       = b->nameHash;
    a->rva            = b->rva;
    a->syscallAddress = b->syscallAddress;
    b->nameHash       = tmpHash;
    b->rva            = tmpRva;
    b->syscallAddress = tmpSyscall;
}

static VOID SortEntriesByRva(ZW_ENTRY* entries, UINT32 count)
{
    // Insertion sort — swap-based to avoid struct copies
    for (UINT32 i = 1; i < count; i++)
    {
        UINT32 j = i;
        while (j > 0 && entries[j - 1].rva > entries[j].rva)
        {
            SwapEntry(&entries[j], &entries[j - 1]);
            j--;
        }
    }
}
#endif

static UINT32 BuildTable(SYSCALL_TABLE* table)
{
    table->count       = 0;

    PVOID ntdllBase = GetModuleHandleFromPEB(Djb2::HashCompileTime(L"ntdll.dll"));

    if (!ntdllBase)
        return 0;

    UINT8* base = (UINT8*)ntdllBase;

    UINT32 e_lfanew = *(UINT32*)(base + 0x3C);
    UINT8* ntHeaders = base + e_lfanew;

#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_AARCH64)
    UINT32 exportDirRva  = *(UINT32*)(ntHeaders + 0x88);
    UINT32 exportDirSize = *(UINT32*)(ntHeaders + 0x8C);
#elif defined(ARCHITECTURE_I386)
    UINT32 exportDirRva  = *(UINT32*)(ntHeaders + 0x78);
    UINT32 exportDirSize = *(UINT32*)(ntHeaders + 0x7C);
#else
#error Unsupported architecture
#endif

    if (exportDirRva == 0)
        return 0;

    UINT8* exportDir = base + exportDirRva;

    UINT32  numberOfNames      = *(UINT32*)(exportDir + 0x18);
    UINT32  addressOfFunctions = *(UINT32*)(exportDir + 0x1C);
    UINT32  addressOfNames     = *(UINT32*)(exportDir + 0x20);
    UINT32  addressOfOrdinals  = *(UINT32*)(exportDir + 0x24);

    UINT32* funcRvaTable = (UINT32*)(base + addressOfFunctions);
    UINT32* nameRvaTable = (UINT32*)(base + addressOfNames);
    UINT16* ordinalTable = (UINT16*)(base + addressOfOrdinals);

    UINT32 count = 0;

    for (UINT32 i = 0; i < numberOfNames && count < SYSCALL_MAX_ENTRIES; i++)
    {
        const CHAR* name = (const CHAR*)(base + nameRvaTable[i]);

        // Zw as short 0x775A ('Z''w') to quickly filter non-syscall exports
        if (*(UINT16*)name != 0x775A)
            continue;

        UINT16 ordinal = ordinalTable[i];
        UINT32 funcRva = funcRvaTable[ordinal];

        // Skip forwarded exports
        if (funcRva >= exportDirRva && funcRva < (exportDirRva + exportDirSize))
            continue;

#if defined(ARCHITECTURE_X86_64)
        // Each x64 stub contains an inline syscall;ret gadget (0F 05 C3)
        UINT8* funcAddr    = base + funcRva;
        PVOID  gadgetAddr  = nullptr;
        for (UINT32 k = 0; k < 30; k++)
        {
            if (funcAddr[k] == 0x0F && funcAddr[k + 1] == 0x05 && funcAddr[k + 2] == 0xC3)
            {
                gadgetAddr = (PVOID)(funcAddr + k);
                break;
            }
        }
#elif defined(ARCHITECTURE_I386)
        // i386 stubs: B8 [SSN:4] BA [addr:4] {FF12|FFD2} C2 [cleanup:2]
        // Read SSN and gadget address directly from the stub bytes.
        // Native 32-bit: FF 12 = call [edx] (deref SharedUserData pointer to KiFastSystemCall)
        // WoW64:         FF D2 = call *edx  (direct trampoline address)
        UINT8* funcAddr = base + funcRva;
        if (funcAddr[0] != 0xB8 || funcAddr[5] != 0xBA)
            continue;

        UINT32 stubSsn  = *(UINT32*)(funcAddr + 1);
        PVOID  rawAddr  = *(PVOID*)(funcAddr + 6);
        PVOID  gadgetAddr = nullptr;

        if (funcAddr[10] == 0xFF && funcAddr[11] == 0x12)
            gadgetAddr = *(PVOID*)rawAddr;   // native: dereference pointer to KiFastSystemCall
        else if (funcAddr[10] == 0xFF && funcAddr[11] == 0xD2)
            gadgetAddr = rawAddr;            // WoW64: direct trampoline address
        else
            continue;

        // Repurpose rva field to store the actual stub SSN
        funcRva = stubSsn;

#elif defined(ARCHITECTURE_AARCH64)
        // AArch64 Call wrappers use svc #0 directly; gadget address unused
        PVOID gadgetAddr = nullptr;
#endif

        table->entries[count].nameHash       = Djb2::Hash(name);
        table->entries[count].rva            = funcRva;
        table->entries[count].syscallAddress = gadgetAddr;
        count++;
    }

    table->count = count;
#if !defined(ARCHITECTURE_I386)
    // Sort by RVA to derive SSN from position (x86_64/aarch64 only).
    // On i386, the SSN is read directly from stub bytes and stored in the rva field.
    SortEntriesByRva(table->entries, table->count);
#endif
    return table->count;
}

SYSCALL_ENTRY System::ResolveSyscallEntry(UINT64 functionNameHash)
{
    SYSCALL_ENTRY result;
    result.ssn            = SYSCALL_SSN_INVALID;
    result.syscallAddress = nullptr;

    SYSCALL_TABLE table;
    if (BuildTable(&table) == 0)
        return result;

    for (UINT32 i = 0; i < table.count; i++)
    {
        if (table.entries[i].nameHash == functionNameHash)
        {
#if !defined(ARCHITECTURE_AARCH64)
            if (!table.entries[i].syscallAddress)
                return result;
#endif
#if defined(ARCHITECTURE_I386)
            // On i386, the actual SSN is stored in the rva field (read from stub bytes)
            result.ssn            = (INT32)table.entries[i].rva;
#else
            // On x86_64/aarch64, SSN = sorted position index
            result.ssn            = (INT32)i;
#endif
            result.syscallAddress = table.entries[i].syscallAddress;
            return result;
        }
    }

    return result;
}
