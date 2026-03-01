#pragma once

#include "core/types/primitives.h"

// =============================================================================
// Linux Syscall Numbers (Architecture-specific)
// =============================================================================

#if defined(ARCHITECTURE_X86_64)

// File I/O
constexpr USIZE SYS_READ = 0;
constexpr USIZE SYS_WRITE = 1;
constexpr USIZE SYS_OPEN = 2;
constexpr USIZE SYS_CLOSE = 3;
constexpr USIZE SYS_LSEEK = 8;
constexpr USIZE SYS_OPENAT = 257;

// File operations
constexpr USIZE SYS_STAT = 4;
constexpr USIZE SYS_FSTAT = 5;
constexpr USIZE SYS_UNLINK = 87;

// Directory operations
constexpr USIZE SYS_MKDIR = 83;
constexpr USIZE SYS_RMDIR = 84;
constexpr USIZE SYS_GETDENTS64 = 217;

// Memory operations
constexpr USIZE SYS_MMAP = 9;
constexpr USIZE SYS_MUNMAP = 11;

// Socket operations
constexpr USIZE SYS_SOCKET = 41;
constexpr USIZE SYS_CONNECT = 42;
constexpr USIZE SYS_SENDTO = 44;
constexpr USIZE SYS_RECVFROM = 45;
constexpr USIZE SYS_SHUTDOWN = 48;
constexpr USIZE SYS_BIND = 49;
constexpr USIZE SYS_SETSOCKOPT = 54;
constexpr USIZE SYS_GETSOCKOPT = 55;
constexpr USIZE SYS_PPOLL = 271;
constexpr USIZE SYS_FCNTL = 72;

// Time operations
constexpr USIZE SYS_CLOCK_GETTIME = 228;

// Random operations
constexpr USIZE SYS_GETRANDOM = 318;

// Process operations
constexpr USIZE SYS_EXIT = 60;
constexpr USIZE SYS_EXIT_GROUP = 231;
constexpr USIZE SYS_FORK = 57;
constexpr USIZE SYS_EXECVE = 59;
constexpr USIZE SYS_DUP2 = 33;
constexpr USIZE SYS_WAIT4 = 61;
constexpr USIZE SYS_SETSID = 112;
constexpr USIZE SYS_PIPE = 22;

#elif defined(ARCHITECTURE_I386)

// File I/O
constexpr USIZE SYS_READ = 3;
constexpr USIZE SYS_WRITE = 4;
constexpr USIZE SYS_OPEN = 5;
constexpr USIZE SYS_CLOSE = 6;
constexpr USIZE SYS_LSEEK = 19;
constexpr USIZE SYS_OPENAT = 295;

// File operations
constexpr USIZE SYS_STAT = 106;
constexpr USIZE SYS_FSTAT = 108;
constexpr USIZE SYS_UNLINK = 10;

// Directory operations
constexpr USIZE SYS_MKDIR = 39;
constexpr USIZE SYS_RMDIR = 40;
constexpr USIZE SYS_GETDENTS64 = 220;

// Memory operations
constexpr USIZE SYS_MMAP2 = 192;
constexpr USIZE SYS_MUNMAP = 91;

// Socket operations (i386 uses socketcall multiplexer)
constexpr USIZE SYS_SOCKETCALL = 102;
constexpr INT32 SOCKOP_SOCKET = 1;
constexpr INT32 SOCKOP_BIND = 2;
constexpr INT32 SOCKOP_CONNECT = 3;
constexpr INT32 SOCKOP_SEND = 9;
constexpr INT32 SOCKOP_RECV = 10;
constexpr INT32 SOCKOP_SENDTO = 11;
constexpr INT32 SOCKOP_RECVFROM = 12;
constexpr INT32 SOCKOP_SHUTDOWN = 13;
constexpr INT32 SOCKOP_SETSOCKOPT = 14;
constexpr INT32 SOCKOP_GETSOCKOPT = 15;

// Misc
constexpr USIZE SYS_PPOLL = 309;
constexpr USIZE SYS_FCNTL64 = 221;

// Time operations
constexpr USIZE SYS_CLOCK_GETTIME = 265;

// Random operations
constexpr USIZE SYS_GETRANDOM = 355;

// Process operations
constexpr USIZE SYS_EXIT = 1;
constexpr USIZE SYS_EXIT_GROUP = 252;
constexpr USIZE SYS_FORK = 2;
constexpr USIZE SYS_EXECVE = 11;
constexpr USIZE SYS_DUP2 = 63;
constexpr USIZE SYS_WAIT4 = 114;
constexpr USIZE SYS_SETSID = 66;
constexpr USIZE SYS_PIPE = 42;

#elif defined(ARCHITECTURE_AARCH64)

// aarch64 uses *at syscalls - AT_FDCWD (-100) means use current working dir
constexpr SSIZE AT_FDCWD = -100;
constexpr INT32 AT_REMOVEDIR = 0x200;

// File I/O
constexpr USIZE SYS_READ = 63;
constexpr USIZE SYS_WRITE = 64;
constexpr USIZE SYS_OPENAT = 56;
constexpr USIZE SYS_CLOSE = 57;
constexpr USIZE SYS_LSEEK = 62;

// File operations
constexpr USIZE SYS_FSTATAT = 79;
constexpr USIZE SYS_FSTAT = 80;
constexpr USIZE SYS_UNLINKAT = 35;

// Directory operations
constexpr USIZE SYS_MKDIRAT = 34;
constexpr USIZE SYS_GETDENTS64 = 61;

// Memory operations
constexpr USIZE SYS_MMAP = 222;
constexpr USIZE SYS_MUNMAP = 215;

// Socket operations
constexpr USIZE SYS_SOCKET = 198;
constexpr USIZE SYS_BIND = 200;
constexpr USIZE SYS_CONNECT = 203;
constexpr USIZE SYS_SENDTO = 206;
constexpr USIZE SYS_RECVFROM = 207;
constexpr USIZE SYS_SHUTDOWN = 210;
constexpr USIZE SYS_SETSOCKOPT = 208;
constexpr USIZE SYS_GETSOCKOPT = 209;
constexpr USIZE SYS_PPOLL = 73;
constexpr USIZE SYS_FCNTL = 25;

// Time operations
constexpr USIZE SYS_CLOCK_GETTIME = 113;

// Random operations
constexpr USIZE SYS_GETRANDOM = 278;

// Process operations
constexpr USIZE SYS_EXIT = 93;
constexpr USIZE SYS_EXIT_GROUP = 94;
constexpr USIZE SYS_CLONE = 220;  // aarch64 uses clone instead of fork
constexpr USIZE SYS_EXECVE = 221;
constexpr USIZE SYS_DUP3 = 24;    // aarch64 uses dup3 instead of dup2
constexpr USIZE SYS_WAIT4 = 260;
constexpr USIZE SYS_SETSID = 157;
constexpr USIZE SYS_PIPE2 = 59;   // aarch64 uses pipe2 instead of pipe

#elif defined(ARCHITECTURE_ARMV7A)

// File I/O
constexpr USIZE SYS_READ = 3;
constexpr USIZE SYS_WRITE = 4;
constexpr USIZE SYS_OPEN = 5;
constexpr USIZE SYS_CLOSE = 6;
constexpr USIZE SYS_LSEEK = 19;
constexpr USIZE SYS_OPENAT = 322;

// File operations
constexpr USIZE SYS_STAT = 106;
constexpr USIZE SYS_FSTAT = 108;
constexpr USIZE SYS_UNLINK = 10;

// Directory operations
constexpr USIZE SYS_MKDIR = 39;
constexpr USIZE SYS_RMDIR = 40;
constexpr USIZE SYS_GETDENTS64 = 217;  // ARM EABI uses 217, not 220

// Memory operations
constexpr USIZE SYS_MMAP2 = 192;
constexpr USIZE SYS_MUNMAP = 91;

// Socket operations
constexpr USIZE SYS_SOCKET = 281;
constexpr USIZE SYS_BIND = 282;
constexpr USIZE SYS_CONNECT = 283;
constexpr USIZE SYS_SENDTO = 290;
constexpr USIZE SYS_RECVFROM = 292;
constexpr USIZE SYS_SHUTDOWN = 293;
constexpr USIZE SYS_SETSOCKOPT = 294;
constexpr USIZE SYS_GETSOCKOPT = 295;
constexpr USIZE SYS_PPOLL = 336;
constexpr USIZE SYS_FCNTL64 = 221;

// Legacy socketcall support (for compatibility)
constexpr USIZE SYS_SOCKETCALL = 102;
constexpr INT32 SOCKOP_SOCKET_ARM = 1;
constexpr INT32 SOCKOP_BIND_ARM = 2;
constexpr INT32 SOCKOP_CONNECT_ARM = 3;
constexpr INT32 SOCKOP_SEND_ARM = 9;
constexpr INT32 SOCKOP_RECV_ARM = 10;

// Time operations
constexpr USIZE SYS_CLOCK_GETTIME = 263;

// Random operations
constexpr USIZE SYS_GETRANDOM = 384;

// Process operations
constexpr USIZE SYS_EXIT = 1;
constexpr USIZE SYS_EXIT_GROUP = 248;
constexpr USIZE SYS_FORK = 2;
constexpr USIZE SYS_EXECVE = 11;
constexpr USIZE SYS_DUP2 = 63;
constexpr USIZE SYS_WAIT4 = 114;
constexpr USIZE SYS_SETSID = 66;
constexpr USIZE SYS_PIPE = 42;

#endif

// =============================================================================
// POSIX/Linux Constants
// =============================================================================

// Standard file descriptors
constexpr INT32 STDIN_FILENO = 0;
constexpr INT32 STDOUT_FILENO = 1;
constexpr INT32 STDERR_FILENO = 2;

// File open flags
constexpr INT32 O_RDONLY = 0x0000;
constexpr INT32 O_WRONLY = 0x0001;
constexpr INT32 O_RDWR = 0x0002;
constexpr INT32 O_CREAT = 0x0040;
constexpr INT32 O_TRUNC = 0x0200;
constexpr INT32 O_APPEND = 0x0400;
constexpr INT32 O_NONBLOCK = 0x0800;

#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_I386)
constexpr INT32 O_DIRECTORY = 0x10000;
#elif defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_ARMV7A)
constexpr INT32 O_DIRECTORY = 0x4000;
#endif

// lseek whence values
constexpr INT32 SEEK_SET = 0;
constexpr INT32 SEEK_CUR = 1;
constexpr INT32 SEEK_END = 2;

// File mode/permission bits
constexpr INT32 S_IRUSR = 0x0100;  // User read
constexpr INT32 S_IWUSR = 0x0080;  // User write
constexpr INT32 S_IXUSR = 0x0040;  // User execute
constexpr INT32 S_IRGRP = 0x0020;  // Group read
constexpr INT32 S_IWGRP = 0x0010;  // Group write
constexpr INT32 S_IXGRP = 0x0008;  // Group execute
constexpr INT32 S_IROTH = 0x0004;  // Others read
constexpr INT32 S_IWOTH = 0x0002;  // Others write
constexpr INT32 S_IXOTH = 0x0001;  // Others execute

// Directory entry types
constexpr UINT8 DT_UNKNOWN = 0;
constexpr UINT8 DT_FIFO = 1;
constexpr UINT8 DT_CHR = 2;
constexpr UINT8 DT_DIR = 4;
constexpr UINT8 DT_BLK = 6;
constexpr UINT8 DT_REG = 8;
constexpr UINT8 DT_LNK = 10;
constexpr UINT8 DT_SOCK = 12;

// Memory protection flags
constexpr INT32 PROT_READ = 0x01;
constexpr INT32 PROT_WRITE = 0x02;
constexpr INT32 PROT_EXEC = 0x04;

// Memory mapping flags
constexpr INT32 MAP_PRIVATE = 0x02;
constexpr INT32 MAP_ANONYMOUS = 0x20;
#define MAP_FAILED ((PVOID)(-1))

// Clock IDs
constexpr INT32 CLOCK_REALTIME = 0;
constexpr INT32 CLOCK_MONOTONIC = 1;

// Socket options
constexpr INT32 SOL_SOCKET = 1;
constexpr INT32 SO_ERROR = 4;
constexpr INT32 SO_RCVTIMEO = 20;
constexpr INT32 SO_SNDTIMEO = 21;
constexpr INT32 IPPROTO_TCP = 6;
constexpr INT32 TCP_NODELAY = 1;

// fcntl commands
constexpr INT32 F_GETFL = 3;
constexpr INT32 F_SETFL = 4;

// errno values
constexpr INT32 EINPROGRESS = 115;

// poll event flags
constexpr INT16 POLLOUT = 0x0004;
constexpr INT16 POLLERR = 0x0008;
constexpr INT16 POLLHUP = 0x0010;

// Invalid file descriptor
constexpr SSIZE INVALID_FD = -1;

// =============================================================================
// Linux Structures
// =============================================================================

// Linux dirent64 structure for directory iteration
struct linux_dirent64
{
	UINT64 d_ino;
	INT64 d_off;
	UINT16 d_reclen;
	UINT8 d_type;
	CHAR d_name[];
};

// Timespec structure
struct timespec
{
	SSIZE tv_sec;
	SSIZE tv_nsec;
};

// Timeval structure (for socket timeouts)
struct timeval
{
	SSIZE tv_sec;
	SSIZE tv_usec;
};

// pollfd structure (for poll/ppoll)
struct pollfd
{
	INT32 fd;
	INT16 events;
	INT16 revents;
};
