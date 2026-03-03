#include "runtime/runtime.h"
#include "tests.h"
#include "core/types/uuid.h"

class UUIDTests
{
private:
    static BOOL TestToString(){

        auto bytes = MakeEmbedArray((const UINT8[]){
            0x12, 0x34, 0x56, 0x78,
            0x9a, 0xbc,
            0xde, 0xf0,
            0x12, 0x34,
            0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
        });

        UUID uuid(bytes);
        CHAR buffer[37] = {0};
        uuid.ToString(&uuid, buffer);
        return Memory::Compare(buffer, "12345678-9abc-def0-1234-56789abcdef0"_embed, 36) == 0;
    }
    
    static BOOL TestFromString(){
        const char* str = "12345678-9abc-def0-1234-56789abcdef0"_embed;

        UUID uuid = UUID::FromString(str);
        auto expected = MakeEmbedArray((const UINT8[]){
            0x12, 0x34, 0x56, 0x78,
            0x9a, 0xbc,
            0xde, 0xf0,
            0x12, 0x34,
            0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
        });
        

        return Memory::Compare(expected, &uuid, sizeof(UUID)) == 0;
    }

    public:
        static BOOL RunAll()
        {
            BOOL allPassed = true;

            LOG_INFO("Running UUID Tests...");

            RunTest(allPassed, EMBED_FUNC(TestToString), "UUID ToString"_embed);
            RunTest(allPassed, EMBED_FUNC(TestFromString), "UUID FromString"_embed);
            if (allPassed) LOG_INFO("All UUID tests passed!");
            else LOG_ERROR("Some UUID tests failed!");

            return allPassed;
        }   
};