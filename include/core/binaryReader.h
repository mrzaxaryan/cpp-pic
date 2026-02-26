#include "primitives.h"
#include "memory.h"

class BinaryReader {
private:
    PVOID address;
    USIZE offset;
    USIZE maxSize;

public:
    BinaryReader(PVOID address, USIZE offset, USIZE maxSize)
        : address(address), offset(offset), maxSize(maxSize) {}

    template<typename T>
    T Read() {
        if (offset + sizeof(T) > maxSize) {
            return T{};   // safer than return 0
        }

        T value;
        Memory::Copy(&value, (PCHAR)address + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    USIZE ReadBytes(PCHAR buffer, USIZE size) {
        if (offset + size > maxSize) {
            return 0;
        }

        Memory::Copy(buffer, (PCHAR)address + offset, size);
        offset += size;
        return size;
    }
};