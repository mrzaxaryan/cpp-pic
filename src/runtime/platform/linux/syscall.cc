/**
 * syscall.cc - Linux System Call Wrapper Implementations
 *
 * Implements Syscall class methods declared in syscall.h.
 * Each method invokes __syscall() with the appropriate syscall number.
 *
 * Architecture-specific __syscall() implementations:
 *   - platform.linux.x86_64.cc  (syscall instruction)
 *   - platform.linux.i386.cc    (int 0x80)
 *   - platform.linux.aarch64.cc (svc #0)
 *   - platform.linux.armv7a.cc  (svc 0)
 *
 * Syscall number sources:
 *   x86_64:  /usr/include/asm/unistd_64.h
 *   i386:    /usr/include/asm/unistd_32.h
 *   aarch64: /usr/include/asm-generic/unistd.h
 *   armv7a:  /usr/include/arm-linux-gnueabi/asm/unistd.h
 */

#include "linux/syscall.h"

/* Variadic syscall helpers - pad unused args with 0 */
#define syscall1(n)                   __syscall(n, 0, 0, 0, 0, 0, 0)
#define syscall2(n, a)                __syscall(n, (USIZE)(a), 0, 0, 0, 0, 0)
#define syscall3(n, a, b)             __syscall(n, (USIZE)(a), (USIZE)(b), 0, 0, 0, 0)
#define syscall4(n, a, b, c)          __syscall(n, (USIZE)(a), (USIZE)(b), (USIZE)(c), 0, 0, 0)
#define syscall5(n, a, b, c, d)       __syscall(n, (USIZE)(a), (USIZE)(b), (USIZE)(c), (USIZE)(d), 0, 0)
#define syscall6(n, a, b, c, d, e)    __syscall(n, (USIZE)(a), (USIZE)(b), (USIZE)(c), (USIZE)(d), (USIZE)(e), 0)
#define syscall7(n, a, b, c, d, e, f) __syscall(n, (USIZE)(a), (USIZE)(b), (USIZE)(c), (USIZE)(d), (USIZE)(e), (USIZE)(f))

/* Argument counting for __syscall_raw() dispatch */
#define __SC_NARGS(_1, _2, _3, _4, _5, _6, _7, N, ...) N
#define __SC_CAT(a, b) a##b
#define __SC_SELECT(N) __SC_CAT(syscall, N)
#define __syscall_raw(...) __SC_SELECT(__SC_NARGS(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1))(__VA_ARGS__)

/* x86_64 syscall numbers */
#if defined(PLATFORM_LINUX_X86_64)
#define SYS_write  1
#define SYS_mmap   9
#define SYS_munmap 11
#define SYS_exit   60

/* AArch64 syscall numbers */
#elif defined(PLATFORM_LINUX_AARCH64)
#define SYS_write  64
#define SYS_mmap   222
#define SYS_munmap 215
#define SYS_exit   93

/* i386 syscall numbers (uses mmap2, offset in 4KB pages) */
#elif defined(PLATFORM_LINUX_I386)
#define SYS_write  4
#define SYS_mmap2  192
#define SYS_munmap 91
#define SYS_exit   1

/* ARMv7-A syscall numbers (uses mmap2, offset in 4KB pages) */
#elif defined(PLATFORM_LINUX_ARMV7A)
#define SYS_write  4
#define SYS_mmap2  192
#define SYS_munmap 91
#define SYS_exit   1

#endif

SSIZE Syscall::Write(INT32 fd, PCVOID buf, USIZE count)
{
    return __syscall_raw(SYS_write, fd, buf, count);
}

SSIZE Syscall::Exit(INT32 status)
{
    return __syscall_raw(SYS_exit, status);
}

/* 32-bit platforms use mmap2 (offset in 4KB pages instead of bytes) */
#if defined(PLATFORM_LINUX_I386) || defined(PLATFORM_LINUX_ARMV7A)
PVOID Syscall::Mmap(PVOID addr, USIZE length, INT32 prot, INT32 flags, INT32 fd, SSIZE offset)
{
    return (PVOID)__syscall_raw(SYS_mmap2, addr, length, prot, flags, fd, offset);
}
#else
PVOID Syscall::Mmap(PVOID addr, USIZE length, INT32 prot, INT32 flags, INT32 fd, SSIZE offset)
{
    return (PVOID)__syscall_raw(SYS_mmap, addr, length, prot, flags, fd, offset);
}
#endif

SSIZE Syscall::Munmap(PVOID addr, USIZE length)
{
    return __syscall_raw(SYS_munmap, addr, length);
}
