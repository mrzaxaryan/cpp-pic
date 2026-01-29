#pragma once

#if defined(PLATFORM_WINDOWS_X86_64)
#define ARCHITECTURE_X86_64
#elif defined(PLATFORM_WINDOWS_I386)
#define ARCHITECTURE_I386
#elif defined(PLATFORM_WINDOWS_AARCH64)
#define ARCHITECTURE_AARCH64
#elif defined(PLATFORM_WINDOWS_ARMV7A)
#define ARCHITECTURE_ARMV7A
#else
#error "Unsupported architecture"
#endif