#pragma once

#include "compiler.h"

#define TRUE true
#define FALSE false
#define NULL nullptr

typedef void VOID, *PVOID, **PPVOID;
typedef const void *PCVOID, **PPCVOID;

typedef signed char INT8, *PINT8;
typedef unsigned char UINT8, *PUINT8, **PPUINT8;

typedef signed short INT16, *PINT16;
typedef unsigned short UINT16, *PUINT16;

typedef signed int INT32, *PINT32;
typedef unsigned int UINT32, *PUINT32, **PPUINT32;

typedef char CHAR, *PCHAR, **PPCHAR;
typedef unsigned char UCHAR, *PUCHAR;
typedef const CHAR *PCCHAR;

typedef float FLOAT, *PFLOAT;

typedef wchar_t WCHAR, *PWCHAR, **PPWCHAR;
typedef const WCHAR *PCWCHAR;

typedef bool BOOL, *PBOOL, **PPBOOL;

#if defined(PLATFORM_WINDOWS)
    // Windows: Use long long for 64-bit, int for 32-bit
    #if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_AARCH64)
    typedef unsigned long long USIZE, *PUSIZE;
    typedef signed long long SSIZE, *PSSIZE;
    #else
    typedef unsigned int USIZE, *PUSIZE;
    typedef signed int SSIZE, *PSSIZE;
    #endif
#elif defined(PLATFORM_LINUX)
    // Linux: size_t is unsigned long on all architectures
    #if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_AARCH64)
    typedef unsigned long USIZE, *PUSIZE;
    typedef signed long SSIZE, *PSSIZE;
    #else
    typedef unsigned int USIZE, *PUSIZE;
    typedef signed int SSIZE, *PSSIZE;
    #endif
#else
#error "Unsupported platform for USIZE and SSIZE definitions"
#endif

typedef __builtin_va_list VA_LIST;
#define VA_START(ap, v) __builtin_va_start(ap, v)
#define VA_ARG(ap, t) __builtin_va_arg(ap, t)
#define VA_END(ap) __builtin_va_end(ap)

#if defined(PLATFORM_WINDOWS_I386)
#define STDCALL __attribute__((stdcall))
#elif defined(PLATFORM_WINDOWS_X86_64)
#define STDCALL __attribute__((ms_abi))
#elif defined(PLATFORM_WINDOWS_ARMV7A)
#define STDCALL
#elif defined(PLATFORM_WINDOWS_AARCH64)
#define STDCALL
#elif defined(PLATFORM_LINUX)
#define STDCALL  // Linux uses System V ABI, no special calling convention needed
#else
#define STDCALL  // Default: no special calling convention
#endif

