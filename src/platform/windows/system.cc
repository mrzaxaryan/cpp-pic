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

    UINT8* base      = (UINT8*)ntdllBase;
    UINT8* ntHeaders = base + *(UINT32*)(base + 0x3C);

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

    UINT32  numberOfNames = *(UINT32*)(exportDir + 0x18);
    UINT32* funcRvaTable  = (UINT32*)(base + *(UINT32*)(exportDir + 0x1C));
    UINT32* nameRvaTable  = (UINT32*)(base + *(UINT32*)(exportDir + 0x20));
    UINT16* ordinalTable  = (UINT16*)(base + *(UINT32*)(exportDir + 0x24));

    for (UINT32 i = 0; i < numberOfNames; i++)
    {
        const CHAR* name = (const CHAR*)(base + nameRvaTable[i]);

        if (*(UINT16*)name != 0x775A)
            continue;

        if (Djb2::Hash(name) != functionNameHash)
            continue;

        UINT32 funcRva = funcRvaTable[ordinalTable[i]];

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
                    result.syscallAddress = (PVOID)(funcAddr + k);
                    break;
                }
            }
            if (!result.syscallAddress)
                return result;
        }
#elif defined(ARCHITECTURE_AARCH64)
        {
            // ARM64 ntdll stubs: SVC #N; RET (each instruction is 4 bytes)
            // The syscall number is encoded in the SVC immediate, NOT in a register.
            // We find the SVC+RET pair and BLR to it (indirect syscall).
            // SVC encoding: 0xD4000001 | (imm16 << 5), mask 0xFFE0001F
            // RET encoding: 0xD65F03C0
            UINT32* instrs = (UINT32*)(base + funcRva);
            for (UINT32 k = 0; k < 8; k++)
            {
                if ((instrs[k] & 0xFFE0001F) == 0xD4000001 && instrs[k + 1] == 0xD65F03C0)
                {
                    result.syscallAddress = (PVOID)&instrs[k];
                    break;
                }
            }
            if (!result.syscallAddress)
                return result;
        }
#elif defined(ARCHITECTURE_I386)
        {
            // i386 stubs: B8 [SSN:4] BA [addr:4] {FF12|FFD2} C2 [cleanup:2]
            UINT8* funcAddr = base + funcRva;
            if (funcAddr[0] != 0xB8 || funcAddr[5] != 0xBA)
                return result;

            PVOID rawAddr = *(PVOID*)(funcAddr + 6);

            if (funcAddr[10] == 0xFF && funcAddr[11] == 0x12)
                result.syscallAddress = *(PVOID*)rawAddr;   // native: dereference pointer to KiFastSystemCall
            else if (funcAddr[10] == 0xFF && funcAddr[11] == 0xD2)
                result.syscallAddress = rawAddr;            // WoW64: direct trampoline address
            else
                return result;

            // SSN is embedded directly in the stub — no counting needed
            result.ssn = (INT32)(*(UINT32*)(funcAddr + 1));
            return result;
        }
#endif

        // Derive SSN by counting Zw* exports with lower RVA (x86_64/aarch64)
        result.ssn = 0;
        for (UINT32 j = 0; j < numberOfNames; j++)
        {
            const CHAR* n = (const CHAR*)(base + nameRvaTable[j]);
            if (*(UINT16*)n != 0x775A)
                continue;

            UINT32 rva = funcRvaTable[ordinalTable[j]];
            if (rva >= exportDirRva && rva < (exportDirRva + exportDirSize))
                continue;

            if (rva < funcRva)
                result.ssn++;
        }

        return result;
    }

    return result;
}
