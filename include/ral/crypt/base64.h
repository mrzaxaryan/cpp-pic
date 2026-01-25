#pragma once

#include "primitives.h"

// Class to handle Base64 encoding and decoding
class Base64
{
public:
    static BOOL Encode(PCCHAR input, UINT32 inputSize, PCHAR output);  // Encodes input data to Base64 format
    static BOOL Decode(PCCHAR input, UINT32 inputSize, PCHAR output);  // Decodes Base64 formatted data to original format
    static UINT32 GetEncodeOutSize(UINT32 inputSize);                  // Calculates the output size needed for Base64 encoding
    static UINT32 GetDecodeOutSize(UINT32 inputSize);                  // Calculates the output size needed for Base64 decoding
};