/**
 * @file system.h
 * @brief Linux System::Call dispatcher.
 *
 * @details Architecture-selection header that includes the correct arch-specific
 * System::Call implementation for the current build target. Supported architectures
 * are x86_64, i386, AArch64, and ARMv7-A.
 */
#pragma once

#if defined(ARCHITECTURE_X86_64)
#include "platform/common/linux/system.x86_64.h"
#elif defined(ARCHITECTURE_I386)
#include "platform/common/linux/system.i386.h"
#elif defined(ARCHITECTURE_AARCH64)
#include "platform/common/linux/system.aarch64.h"
#elif defined(ARCHITECTURE_ARMV7A)
#include "platform/common/linux/system.armv7a.h"
#else
#error "Unsupported architecture"
#endif
