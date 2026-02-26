#include "primitives.h"
#include "memory.h"

class BinaryWriter{
    private:
        PVOID address;
        USIZE offset;
        USIZE maxSize;
    public:
        BinaryWriter(PVOID address, USIZE offset, USIZE maxSize) : address(address), offset(offset), maxSize(maxSize) {}
        template<typename T>
        PVOID Write(T value){
            if(offset + sizeof(T) > maxSize){
                return nullptr;
            }

            *(T*)((PCHAR)address + offset) = value;
            offset += sizeof(T);
            return address;
        }

        PVOID WriteBytes(PCHAR data, USIZE size){
            if(offset + size > maxSize){
                return nullptr;
            }

            Memory::Copy((PCHAR)address + offset, data, size);
            offset += size;
            return address;
        }
};
