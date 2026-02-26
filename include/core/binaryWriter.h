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
        static PVOID Write(BinaryWriter* writer, T value){
            if(writer->offset + sizeof(T) > writer->maxSize){
                return nullptr;
            }

            *(T*)((PCHAR)writer->address + writer->offset) = value;
            writer->offset += sizeof(T);
            return writer->address;
        }

        static PVOID WriteBytes(BinaryWriter* writer, PCHAR data, USIZE size){
            if(writer->offset + size > writer->maxSize){
                return nullptr;
            }

            Memory::Copy((PCHAR)writer->address + writer->offset, data, size);
            writer->offset += size;
            return writer->address;
        }
};
