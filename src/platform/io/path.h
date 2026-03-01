#pragma once
#include "primitives.h"
#include "string.h"
#include "platform.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_UEFI)
constexpr CHAR PATH_SEPARATOR = '\\';
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
constexpr CHAR PATH_SEPARATOR = '/';
#else
#error "Path separator not defined for this platform"
#endif

// Path class for handling file paths
class Path
{
public:
    /// @brief Combine two paths into one, ensuring proper path separators
    /// @tparam TChar Character type (CHAR or WCHAR)
    /// @param path1 First path to combine
    /// @param path2 Second path to combine
    /// @param out Output buffer to write combined path into
    /// @return Number of characters written (excluding null terminator), 0 on overflow

    template <TCHAR TChar>
    static USIZE Combine(const TChar *path1, const TChar *path2, Span<TChar> out)
    {
        USIZE len1 = String::Length(path1);
        USIZE len2 = String::Length(path2);
        BOOL needSep = len1 > 0 && path1[len1 - 1] != (TChar)PATH_SEPARATOR;
        USIZE totalLen = len1 + (needSep ? 1 : 0) + len2;

        if (out.Size() == 0 || totalLen >= out.Size())
            return 0;

        for (USIZE i = 0; i < len1; ++i)
            out[i] = path1[i];

        USIZE pos = len1;
        if (needSep)
            out[pos++] = (TChar)PATH_SEPARATOR;

        for (USIZE i = 0; i < len2; ++i)
            out[pos + i] = path2[i];

        out[totalLen] = (TChar)0;
        return totalLen;
    }

    /// @brief Get the file name from a full path
    /// @tparam TChar Character type (CHAR or WCHAR)
    /// @param fullPath The full file path
    /// @param out Output buffer to write file name into
    /// @return Number of characters written (excluding null terminator), 0 on overflow

    template <TCHAR TChar>
    static USIZE GetFileName(const TChar *fullPath, Span<TChar> out)
    {
        USIZE len = String::Length(fullPath);
        SSIZE lastSeparator = -1;

        for (USIZE i = 0; i < len; ++i)
        {
            if (fullPath[i] == (TChar)PATH_SEPARATOR)
                lastSeparator = (SSIZE)i;
        }

        const TChar *name = (lastSeparator == -1) ? fullPath : (fullPath + lastSeparator + 1);
        USIZE nameLen = (lastSeparator == -1) ? len : (len - (USIZE)lastSeparator - 1);

        if (out.Size() == 0 || nameLen >= out.Size())
            return 0;

        for (USIZE i = 0; i < nameLen; ++i)
            out[i] = name[i];
        out[nameLen] = (TChar)0;
        return nameLen;
    }

    /// @brief Get the file extension from a file name
    /// @tparam TChar Character type (CHAR or WCHAR)
    /// @param fileName File name to extract the extension from
    /// @param out Output buffer to write extension into (empty string if no extension)
    /// @return Number of characters written (excluding null terminator), 0 on overflow

    template <TCHAR TChar>
    static USIZE GetExtension(const TChar *fileName, Span<TChar> out)
    {
        if (out.Size() == 0)
            return 0;

        USIZE len = String::Length(fileName);
        SSIZE lastDot = -1;

        for (USIZE i = 0; i < len; ++i)
        {
            if (fileName[i] == (TChar)'.')
                lastDot = (SSIZE)i;
        }

        if (lastDot == -1 || (USIZE)lastDot == len - 1)
        {
            out[0] = (TChar)0;
            return 0;
        }

        USIZE extLen = len - (USIZE)lastDot - 1;
        if (extLen >= out.Size())
            return 0;

        for (USIZE i = 0; i < extLen; ++i)
            out[i] = fileName[(USIZE)lastDot + 1 + i];
        out[extLen] = (TChar)0;
        return extLen;
    }

    /// @brief Get the directory name from a full path
    /// @tparam TChar Character type (CHAR or WCHAR)
    /// @param fullPath Full file path to extract the directory name from
    /// @param out Output buffer to write directory name into (empty string if no directory)
    /// @return Number of characters written (excluding null terminator), 0 on overflow

    template <TCHAR TChar>
    static USIZE GetDirectoryName(const TChar *fullPath, Span<TChar> out)
    {
        if (out.Size() == 0)
            return 0;

        USIZE len = String::Length(fullPath);
        SSIZE lastSeparator = -1;

        for (USIZE i = 0; i < len; ++i)
        {
            if (fullPath[i] == (TChar)PATH_SEPARATOR)
                lastSeparator = (SSIZE)i;
        }

        if (lastSeparator == -1)
        {
            out[0] = (TChar)0;
            return 0;
        }

        USIZE dirLen = (USIZE)lastSeparator;
        if (dirLen >= out.Size())
            return 0;

        for (USIZE i = 0; i < dirLen; ++i)
            out[i] = fullPath[i];
        out[dirLen] = (TChar)0;
        return dirLen;
    }

    /// @brief Check if a path is rooted
    /// @tparam TChar Character type (CHAR or WCHAR)
    /// @param path Path to check
    /// @return true if the path is rooted, false otherwise

    template <TCHAR TChar>
    static BOOL IsPathRooted(const TChar *path)
    {
        // On Windows, a path is rooted if it starts with a drive letter or a backslash
        if (path == nullptr || path[0] == (TChar)0)
            return false;
#if defined(PLATFORM_WINDOWS)
        return (((path[0] >= (TChar)'A' && path[0] <= (TChar)'Z') || (path[0] >= (TChar)'a' && path[0] <= (TChar)'z')) && path[1] == (TChar)':') || (path[0] == (TChar)'\\');
#else
        return path[0] == (TChar)'/';
#endif
    }

    /// @brief Normalize a path by replacing all separators with the platform-specific separator
    /// @param path Source path to normalize
    /// @param out Output buffer to write normalized path into
    /// @return Number of characters written (excluding null terminator), 0 on overflow or null input

    static USIZE NormalizePath(PCWCHAR path, Span<WCHAR> out)
    {
        if (path == nullptr || out.Size() == 0)
            return 0;

        USIZE len = String::Length(path);
        if (len >= out.Size())
            return 0;

        for (USIZE i = 0; i < len; ++i)
        {
            if (path[i] == L'/' || path[i] == L'\\')
                out[i] = (WCHAR)PATH_SEPARATOR;
            else
                out[i] = path[i];
        }
        out[len] = 0;
        return len;
    }
};
