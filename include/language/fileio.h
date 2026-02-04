/**
 * fileio.h - File I/O Functions for PIL (via PLATFORM)
 *
 * Provides file system access through the Platform Abstraction Layer.
 * Functions use handle-based file management with a fixed-size pool.
 *
 * Position-independent, no .rdata dependencies.
 *
 * Part of RAL (Runtime Abstraction Layer).
 *
 * USAGE:
 *   PIL::FilePool pool;
 *   PIL::State L;
 *   PIL::OpenFileIO(L, &pool);  // Registers file functions
 *   L.DoString("var f = fopen(\"test.txt\", \"w\"); fwrite(f, \"Hello\"); fclose(f);");
 *
 * FUNCTIONS:
 *   fopen(path, mode)         - Open file ("r"=read, "w"=write, "a"=append)
 *   fclose(handle)            - Close file
 *   fread(handle [, size])    - Read from file (max 255 bytes per call)
 *   freadline(handle)         - Read a line from file
 *   fwrite(handle, data)      - Write to file
 *   fexists(path)             - Check if file exists
 *   fdelete(path)             - Delete file
 *   fsize(handle)             - Get file size
 *   fseek(handle, offset, origin) - Set file position (0=start, 1=current, 2=end)
 *   ftell(handle)             - Get current file position
 *   mkdir(path)               - Create directory
 *   rmdir(path)               - Remove directory
 */

#pragma once

#include "value.h"  // includes State forward declaration
#include "file_system.h"
#include "embedded_function_pointer.h"

namespace PIL
{

// ============================================================================
// FILE POOL CONFIGURATION
// ============================================================================

constexpr USIZE MAX_FILE_HANDLES = 16;  // Maximum simultaneously open files

// ============================================================================
// FILE POOL - Manages open file handles
// ============================================================================

/**
 * FilePool - Fixed-size pool for managing open file handles.
 * File handles are returned as numbers (0-15) to script code.
 * Stored in State's user data pointer (no static variables for PIC safety).
 */
class FilePool
{
private:
    File m_files[MAX_FILE_HANDLES];
    BOOL m_inUse[MAX_FILE_HANDLES];

public:
    FilePool() noexcept
    {
        for (USIZE i = 0; i < MAX_FILE_HANDLES; i++)
        {
            m_inUse[i] = FALSE;
        }
    }

    ~FilePool() noexcept
    {
        CloseAll();
    }

    // Allocate a file handle, returns -1 if pool exhausted
    NOINLINE INT32 Alloc() noexcept
    {
        for (USIZE i = 0; i < MAX_FILE_HANDLES; i++)
        {
            if (!m_inUse[i])
            {
                m_inUse[i] = TRUE;
                return (INT32)i;
            }
        }
        return -1;
    }

    // Get file by handle
    FORCE_INLINE File* Get(INT32 handle) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MAX_FILE_HANDLES || !m_inUse[handle])
        {
            return nullptr;
        }
        return &m_files[handle];
    }

    // Set file at handle (takes ownership via move)
    NOINLINE BOOL Set(INT32 handle, File&& file) noexcept
    {
        if (handle < 0 || (USIZE)handle >= MAX_FILE_HANDLES || !m_inUse[handle])
        {
            return FALSE;
        }
        m_files[handle] = static_cast<File&&>(file);
        return TRUE;
    }

    // Free a file handle
    NOINLINE void Free(INT32 handle) noexcept
    {
        if (handle >= 0 && (USIZE)handle < MAX_FILE_HANDLES && m_inUse[handle])
        {
            m_files[handle].Close();
            m_inUse[handle] = FALSE;
        }
    }

    // Close all open files
    NOINLINE void CloseAll() noexcept
    {
        for (USIZE i = 0; i < MAX_FILE_HANDLES; i++)
        {
            if (m_inUse[i])
            {
                m_files[i].Close();
                m_inUse[i] = FALSE;
            }
        }
    }

    // Check if handle is valid
    FORCE_INLINE BOOL IsValid(INT32 handle) const noexcept
    {
        return handle >= 0 && (USIZE)handle < MAX_FILE_HANDLES && m_inUse[handle];
    }
};

// ============================================================================
// HELPER TO GET FILE POOL FROM STATE
// ============================================================================

FORCE_INLINE FilePool* GetFilePool(FunctionContext& ctx) noexcept
{
    return ctx.state ? static_cast<FilePool*>(ctx.state->GetUserData()) : nullptr;
}

// ============================================================================
// PATH CONVERSION HELPER
// ============================================================================

/**
 * Convert narrow string (CHAR) to wide string (WCHAR).
 * Simple ASCII-only conversion for file paths.
 */
NOINLINE USIZE NarrowToWide(const CHAR* narrow, USIZE narrowLen, WCHAR* wide, USIZE wideSize) noexcept
{
    USIZE i = 0;
    while (i < narrowLen && i < wideSize - 1)
    {
        wide[i] = (WCHAR)(UINT8)narrow[i];
        i++;
    }
    wide[i] = L'\0';
    return i;
}

// ============================================================================
// FILE I/O FUNCTIONS
// ============================================================================

/**
 * fopen(path, mode) - Open a file
 *
 * Modes:
 *   "r"  - Read only (file must exist)
 *   "w"  - Write only (creates/truncates file)
 *   "a"  - Append (creates file if doesn't exist)
 *   "rb" - Read binary
 *   "wb" - Write binary
 *   "ab" - Append binary
 *
 * Returns: file handle (number) or -1 on error
 */
NOINLINE Value FileIO_Open(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsString(0) || !ctx.IsString(1))
    {
        return Value::Number(-1);
    }

    FilePool* pool = GetFilePool(ctx);
    if (!pool)
    {
        return Value::Number(-1);
    }

    // Get path and mode
    const CHAR* path = ctx.ToString(0);
    USIZE pathLen = ctx.ToStringLength(0);
    const CHAR* mode = ctx.ToString(1);
    USIZE modeLen = ctx.ToStringLength(1);

    // Convert path to wide string
    WCHAR widePath[256];
    NarrowToWide(path, pathLen, widePath, 256);

    // Parse mode string
    INT32 flags = 0;
    BOOL hasRead = FALSE;
    BOOL hasWrite = FALSE;
    BOOL hasAppend = FALSE;
    BOOL hasBinary = FALSE;

    for (USIZE i = 0; i < modeLen; i++)
    {
        if (mode[i] == 'r') hasRead = TRUE;
        else if (mode[i] == 'w') hasWrite = TRUE;
        else if (mode[i] == 'a') hasAppend = TRUE;
        else if (mode[i] == 'b') hasBinary = TRUE;
    }

    if (hasRead)
    {
        flags = FileSystem::FS_READ;
    }
    else if (hasWrite)
    {
        flags = FileSystem::FS_WRITE | FileSystem::FS_CREATE | FileSystem::FS_TRUNCATE;
    }
    else if (hasAppend)
    {
        flags = FileSystem::FS_CREATE | FileSystem::FS_APPEND;
    }
    else
    {
        return Value::Number(-1);  // Invalid mode
    }

    if (hasBinary)
    {
        flags |= FileSystem::FS_BINARY;
    }

    // Allocate handle
    INT32 handle = pool->Alloc();
    if (handle < 0)
    {
        return Value::Number(-1);  // Pool exhausted
    }

    // Open file
    File file = FileSystem::Open(widePath, flags);
    if (!file.IsValid())
    {
        pool->Free(handle);
        return Value::Number(-1);
    }

    // Store in pool
    pool->Set(handle, static_cast<File&&>(file));
    return Value::Number(handle);
}

/**
 * fclose(handle) - Close a file
 *
 * Returns: true on success, false on error
 */
NOINLINE Value FileIO_Close(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Bool(FALSE);
    }

    FilePool* pool = GetFilePool(ctx);
    if (!pool)
    {
        return Value::Bool(FALSE);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    if (!pool->IsValid(handle))
    {
        return Value::Bool(FALSE);
    }

    pool->Free(handle);
    return Value::Bool(TRUE);
}

/**
 * fread(handle [, size]) - Read from file
 *
 * If size is not provided, reads up to 255 bytes.
 * Due to string size limits (MAX_STRING_VALUE=256), max read is 255 bytes.
 *
 * Returns: string with data read, or empty string on error/EOF
 */
NOINLINE Value FileIO_Read(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgsMin(1) || !ctx.IsNumber(0))
    {
        return Value::String(""_embed, 0);
    }

    FilePool* pool = GetFilePool(ctx);
    if (!pool)
    {
        return Value::String(""_embed, 0);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    File* file = pool->Get(handle);
    if (!file || !file->IsValid())
    {
        return Value::String(""_embed, 0);
    }

    // Determine read size
    UINT32 readSize = 255;  // Default max (leave room for null terminator)
    if (ctx.GetArgCount() >= 2 && ctx.IsNumber(1))
    {
        INT64 requested = ctx.ToNumber(1);
        if (requested <= 0)
        {
            return Value::String(""_embed, 0);
        }
        if (requested < 255)
        {
            readSize = (UINT32)requested;
        }
    }

    // Read data
    CHAR buffer[256];
    UINT32 bytesRead = file->Read(buffer, readSize);
    if (bytesRead == 0)
    {
        return Value::String(""_embed, 0);
    }

    buffer[bytesRead] = '\0';
    return Value::String(buffer, bytesRead);
}

/**
 * freadline(handle) - Read a line from file
 *
 * Reads until newline (\n) or EOF.
 * The newline is NOT included in the returned string.
 *
 * Returns: string with line content, or nil on EOF
 */
NOINLINE Value FileIO_ReadLine(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Nil();
    }

    FilePool* pool = GetFilePool(ctx);
    if (!pool)
    {
        return Value::Nil();
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    File* file = pool->Get(handle);
    if (!file || !file->IsValid())
    {
        return Value::Nil();
    }

    CHAR buffer[256];
    USIZE pos = 0;

    // Read character by character until newline or EOF
    while (pos < 255)
    {
        CHAR ch;
        UINT32 bytesRead = file->Read(&ch, 1);
        if (bytesRead == 0)
        {
            // EOF
            if (pos == 0)
            {
                return Value::Nil();  // No data read at all
            }
            break;
        }

        if (ch == '\n')
        {
            break;  // Don't include newline
        }

        if (ch == '\r')
        {
            // Handle \r\n by peeking at next char
            USIZE currentOffset = file->GetOffset();
            CHAR nextCh;
            if (file->Read(&nextCh, 1) > 0 && nextCh != '\n')
            {
                // Not \r\n, restore position
                file->SetOffset(currentOffset);
            }
            break;
        }

        buffer[pos++] = ch;
    }

    buffer[pos] = '\0';
    return Value::String(buffer, pos);
}

/**
 * fwrite(handle, data) - Write to file
 *
 * Returns: number of bytes written, or -1 on error
 */
NOINLINE Value FileIO_Write(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(2) || !ctx.IsNumber(0) || !ctx.IsString(1))
    {
        return Value::Number(-1);
    }

    FilePool* pool = GetFilePool(ctx);
    if (!pool)
    {
        return Value::Number(-1);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    File* file = pool->Get(handle);
    if (!file || !file->IsValid())
    {
        return Value::Number(-1);
    }

    const CHAR* data = ctx.ToString(1);
    USIZE dataLen = ctx.ToStringLength(1);

    UINT32 bytesWritten = file->Write(data, dataLen);
    return Value::Number((INT64)bytesWritten);
}

/**
 * fexists(path) - Check if file exists
 *
 * Returns: true if file exists, false otherwise
 */
NOINLINE Value FileIO_Exists(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::Bool(FALSE);
    }

    const CHAR* path = ctx.ToString(0);
    USIZE pathLen = ctx.ToStringLength(0);

    WCHAR widePath[256];
    NarrowToWide(path, pathLen, widePath, 256);

    return Value::Bool(FileSystem::Exists(widePath));
}

/**
 * fdelete(path) - Delete a file
 *
 * Returns: true on success, false on error
 */
NOINLINE Value FileIO_Delete(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::Bool(FALSE);
    }

    const CHAR* path = ctx.ToString(0);
    USIZE pathLen = ctx.ToStringLength(0);

    WCHAR widePath[256];
    NarrowToWide(path, pathLen, widePath, 256);

    return Value::Bool(FileSystem::Delete(widePath));
}

/**
 * fsize(handle) - Get file size
 *
 * Returns: file size in bytes, or -1 on error
 */
NOINLINE Value FileIO_Size(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Number(-1);
    }

    FilePool* pool = GetFilePool(ctx);
    if (!pool)
    {
        return Value::Number(-1);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    File* file = pool->Get(handle);
    if (!file || !file->IsValid())
    {
        return Value::Number(-1);
    }

    return Value::Number((INT64)file->GetSize());
}

/**
 * fseek(handle, offset, origin) - Set file position
 *
 * Origin:
 *   0 - From start of file
 *   1 - From current position
 *   2 - From end of file
 *
 * Returns: true on success, false on error
 */
NOINLINE Value FileIO_Seek(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(3) || !ctx.IsNumber(0) || !ctx.IsNumber(1) || !ctx.IsNumber(2))
    {
        return Value::Bool(FALSE);
    }

    FilePool* pool = GetFilePool(ctx);
    if (!pool)
    {
        return Value::Bool(FALSE);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    File* file = pool->Get(handle);
    if (!file || !file->IsValid())
    {
        return Value::Bool(FALSE);
    }

    INT64 offset = ctx.ToNumber(1);
    INT64 origin = ctx.ToNumber(2);

    OffsetOrigin seekOrigin;
    switch (origin)
    {
        case 0: seekOrigin = OffsetOrigin::Start; break;
        case 1: seekOrigin = OffsetOrigin::Current; break;
        case 2: seekOrigin = OffsetOrigin::End; break;
        default: return Value::Bool(FALSE);
    }

    if (seekOrigin == OffsetOrigin::Start && offset >= 0)
    {
        file->SetOffset((USIZE)offset);
    }
    else
    {
        file->MoveOffset((SSIZE)offset, seekOrigin);
    }

    return Value::Bool(TRUE);
}

/**
 * ftell(handle) - Get current file position
 *
 * Returns: current offset in bytes, or -1 on error
 */
NOINLINE Value FileIO_Tell(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsNumber(0))
    {
        return Value::Number(-1);
    }

    FilePool* pool = GetFilePool(ctx);
    if (!pool)
    {
        return Value::Number(-1);
    }

    INT32 handle = (INT32)ctx.ToNumber(0);
    File* file = pool->Get(handle);
    if (!file || !file->IsValid())
    {
        return Value::Number(-1);
    }

    return Value::Number((INT64)file->GetOffset());
}

/**
 * mkdir(path) - Create a directory
 *
 * Returns: true on success, false on error
 */
NOINLINE Value FileIO_MkDir(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::Bool(FALSE);
    }

    const CHAR* path = ctx.ToString(0);
    USIZE pathLen = ctx.ToStringLength(0);

    WCHAR widePath[256];
    NarrowToWide(path, pathLen, widePath, 256);

    return Value::Bool(FileSystem::CreateDirectory(widePath));
}

/**
 * rmdir(path) - Remove a directory
 *
 * Returns: true on success, false on error
 */
NOINLINE Value FileIO_RmDir(FunctionContext& ctx) noexcept
{
    if (!ctx.CheckArgs(1) || !ctx.IsString(0))
    {
        return Value::Bool(FALSE);
    }

    const CHAR* path = ctx.ToString(0);
    USIZE pathLen = ctx.ToStringLength(0);

    WCHAR widePath[256];
    NarrowToWide(path, pathLen, widePath, 256);

    return Value::Bool(FileSystem::DeleteDirectory(widePath));
}

// ============================================================================
// OPEN FILE I/O LIBRARY
// ============================================================================

/**
 * Register all file I/O functions with a State.
 *
 * @param L The script State to register functions with
 * @param pool Pointer to FilePool instance (must outlive the State)
 *
 * IMPORTANT: The FilePool should have the same lifetime as the State.
 *
 * Functions registered:
 *   1. fopen     - Open file
 *   2. fclose    - Close file
 *   3. fread     - Read from file
 *   4. freadline - Read line from file
 *   5. fwrite    - Write to file
 *   6. fexists   - Check if file exists
 *   7. fdelete   - Delete file
 *   8. fsize     - Get file size
 *   9. fseek     - Set file position
 *  10. ftell     - Get file position
 *  11. mkdir     - Create directory
 *  12. rmdir     - Remove directory
 */
NOINLINE void OpenFileIO(State& L, FilePool* pool) noexcept
{
    // Store FilePool pointer in State's user data
    L.SetUserData(pool);
    L.Register("fopen"_embed, EMBED_FUNC(FileIO_Open));
    L.Register("fclose"_embed, EMBED_FUNC(FileIO_Close));
    L.Register("fread"_embed, EMBED_FUNC(FileIO_Read));
    L.Register("freadline"_embed, EMBED_FUNC(FileIO_ReadLine));
    L.Register("fwrite"_embed, EMBED_FUNC(FileIO_Write));
    L.Register("fexists"_embed, EMBED_FUNC(FileIO_Exists));
    L.Register("fdelete"_embed, EMBED_FUNC(FileIO_Delete));
    L.Register("fsize"_embed, EMBED_FUNC(FileIO_Size));
    L.Register("fseek"_embed, EMBED_FUNC(FileIO_Seek));
    L.Register("ftell"_embed, EMBED_FUNC(FileIO_Tell));
    L.Register("mkdir"_embed, EMBED_FUNC(FileIO_MkDir));
    L.Register("rmdir"_embed, EMBED_FUNC(FileIO_RmDir));
}

} // namespace PIL
