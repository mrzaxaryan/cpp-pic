#include "platform/windows/system.h"
#include "platform.h"
#include "peb.h"
#include "djb2.h"

SYSCALL_ENTRY System::ResolveSyscallEntry(UINT64 functionNameHash)
{
    SYSCALL_ENTRY result;
    result.ssn            = SYSCALL_SSN_INVALID;
    result.syscallAddress = nullptr;

    PVOID ntdllBase = GetModuleHandleFromPEB(Djb2::HashCompileTime(L"ntdll.dll"));

    if (!ntdllBase)
        return result;

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
        return result;

    UINT8* exportDir = base + exportDirRva;

    UINT32  numberOfNames      = *(UINT32*)(exportDir + 0x18);
    UINT32  addressOfFunctions = *(UINT32*)(exportDir + 0x1C);
    UINT32  addressOfNames     = *(UINT32*)(exportDir + 0x20);
    UINT32  addressOfOrdinals  = *(UINT32*)(exportDir + 0x24);

    UINT32* funcRvaTable = (UINT32*)(base + addressOfFunctions);
    UINT32* nameRvaTable = (UINT32*)(base + addressOfNames);
    UINT16* ordinalTable = (UINT16*)(base + addressOfOrdinals);

    // --- Pass 1: Find the target function by hash ---
    UINT32 targetRva    = 0;
    PVOID  targetGadget = nullptr;

    for (UINT32 i = 0; i < numberOfNames; i++)
    {
        const CHAR* name = (const CHAR*)(base + nameRvaTable[i]);

        // 'Zw' prefix filter
        if (*(UINT16*)name != 0x775A)
            continue;

        if (Djb2::Hash(name) != functionNameHash)
            continue;

        UINT16 ordinal = ordinalTable[i];
        UINT32 funcRva = funcRvaTable[ordinal];

        // Skip forwarded exports
        if (funcRva >= exportDirRva && funcRva < (exportDirRva + exportDirSize))
            return result;

#if defined(ARCHITECTURE_X86_64)
        {
            // Each x64 stub contains an inline syscall;ret gadget (0F 05 C3)
            UINT8* funcAddr = base + funcRva;
            for (UINT32 k = 0; k < 30; k++)
            {
                if (funcAddr[k] == 0x0F && funcAddr[k + 1] == 0x05 && funcAddr[k + 2] == 0xC3)
                {
                    targetGadget = (PVOID)(funcAddr + k);
                    break;
                }
            }
            if (!targetGadget)
                return result;
            targetRva = funcRva;
        }
#elif defined(ARCHITECTURE_I386)
        {
            // i386 stubs: B8 [SSN:4] BA [addr:4] {FF12|FFD2} C2 [cleanup:2]
            UINT8* funcAddr = base + funcRva;
            if (funcAddr[0] != 0xB8 || funcAddr[5] != 0xBA)
                return result;

            PVOID rawAddr = *(PVOID*)(funcAddr + 6);

            if (funcAddr[10] == 0xFF && funcAddr[11] == 0x12)
                targetGadget = *(PVOID*)rawAddr;   // native: dereference pointer to KiFastSystemCall
            else if (funcAddr[10] == 0xFF && funcAddr[11] == 0xD2)
                targetGadget = rawAddr;            // WoW64: direct trampoline address
            else
                return result;

            // SSN is embedded directly in the stub — no pass 2 needed
            result.ssn            = (INT32)(*(UINT32*)(funcAddr + 1));
            result.syscallAddress = targetGadget;
            return result;
        }
#elif defined(ARCHITECTURE_AARCH64)
        // AArch64 stubs use svc #0 directly; no gadget needed
        targetRva = funcRva;
#endif

        break;
    }

    if (targetRva == 0)
        return result;

    // --- Pass 2: Count Zw* exports with lower RVA to derive SSN (x86_64/aarch64) ---
    UINT32 ssn = 0;
    for (UINT32 i = 0; i < numberOfNames; i++)
    {
        const CHAR* name = (const CHAR*)(base + nameRvaTable[i]);

        if (*(UINT16*)name != 0x775A)
            continue;

        UINT16 ordinal = ordinalTable[i];
        UINT32 funcRva = funcRvaTable[ordinal];

        // Skip forwarded exports
        if (funcRva >= exportDirRva && funcRva < (exportDirRva + exportDirSize))
            continue;

        if (funcRva < targetRva)
            ssn++;
    }

    result.ssn            = (INT32)ssn;
    result.syscallAddress = targetGadget;
    return result;
}
