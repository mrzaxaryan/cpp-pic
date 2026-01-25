#include "pal.h"
#include "primitives.h"

PVOID GetInstructionAddress(VOID)
{
    return __builtin_return_address(0);
}

PCHAR ReversePatternSearch(PCHAR ip, const CHAR *pattern, UINT32 len)
{
    PCHAR p = ip;
    while (1)
    {
        UINT32 i = 0;
        for (; i < len; i++)
        {
            if (p[i] != pattern[i])
                break;
        }
        if (i == len)
            return p; // found match
        p--;          // move backward
    }
}
