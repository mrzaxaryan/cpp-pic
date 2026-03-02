#include "primitives.h"
#include "random.h"


class UUID {
    private:
        UINT8 data[16];
    public:
        UUID() { Memory::Zero(data, 16); }

        UUID(const UINT8* bytes){
           Memory::Copy(data, bytes, 16);
        }

        static UUID RandomUUID(){
            UUID uuid;
            Random rng; // your random generator

            for (INT32 i = 0; i < 16; i++){
                uuid.data[i] = (UINT8)rng.Get();
            }

            return uuid;
        }

        static UUID FromString(const char* str){
            UINT8 bytes[16] = {0};
            INT32 byteIndex = 0;
            INT32 count = 0;

            for(INT32 i = 0; str[i]!='\0'; i++){
                if(str[i] == '-') continue;
                if(byteIndex >= 16) break;

                UINT8 value = 0;
                CHAR c = str[i];
                if(c >= '0' && c <= '9') value = c - '0';
                else if(c >= 'a' && c <= 'f') value = c - 'a' + 10;
                else if(c >= 'A' && c <= 'F') value = c - 'A' + 10;
                else continue;

                if(count == 0){
                    bytes[byteIndex] = value << 4; // high nibble
                    count = 1;
                } else {
                    bytes[byteIndex] |= value; // low nibble
                    byteIndex++;
                    count = 0;
                }
            }

            return UUID(bytes);
        }
   
        // toString method to convert the UUID to a string representation
        VOID ToString(CHAR* buffer, USIZE bufferSize){
            if(bufferSize < 37) return;

            INT32 index = 0;
            const CHAR* hex = "0123456789abcdef"_embed;
            auto byteToHex = [&](UINT8 byte){
                buffer[index++] = hex[(byte >> 4) & 0xF];
                buffer[index++] = hex[byte & 0xF];
            };

            for(INT32 i =0; i < 16; i++){
                byteToHex(data[i]);
                if(i == 3 || i == 5 || i == 7 || i == 9) buffer[index++] = '-';
            }
            buffer[index] = '\0';
    }

        UINT64 GetMostSignificantBits(){
            UINT64 msb = 0;
            for(INT32 i = 0; i < 8; i++){
                msb = (msb << 8) | data[i];
            }
            return msb;
        }

        UINT64 GetLeastSignficantBits(){
            UINT64 lsb = 0;
            for(INT32 i = 8; i < 16; i++){
                lsb = (lsb << 8) | data[i];
            }
            return lsb;
        }
   
    };
