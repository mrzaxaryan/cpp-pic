#pragma once
#include "primitives.h"
#include "string.h"
#include "platform.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_UEFI)
const char PATH_SEPARATOR = '\\';
#elif defined(PLATFORM_LINUX)
const char PATH_SEPARATOR = '/';
#else
#error "Path separator not defined for this platform"
#endif

class Path
{
public:
    template <TCHAR TChar>
    static TChar *Combine(const TChar *path1, const TChar *path2)
    {
        // Calculate lengths of both paths
        INT32 len1 = String::Length(path1);
        INT32 len2 = String::Length(path2);

        // Allocate buffer for combined path (+2 for possible separator and null terminator)
        TChar *combined = new TChar[len1 + len2 + 2];

        // Copy first path
        for (INT32 i = 0; i < len1; ++i)
        {
            combined[i] = path1[i];
        }

        // Add separator if needed
        if (len1 > 0 && combined[len1 - 1] != PATH_SEPARATOR)
        {
            combined[len1] = PATH_SEPARATOR;
            len1++;
        }

        // Copy second path
        for (INT32 i = 0; i < len2; ++i)
        {
            combined[len1 + i] = path2[i];
        }

        // Null terminate
        combined[len1 + len2] = (TChar)0;

        return combined;
    }

    template <TCHAR TChar>
    static TChar *GetFileName(const TChar *fullPath)
    {
        INT32 len = String::Length(fullPath);
        INT32 lastSeparator = -1;

        // Find the last occurrence of the path separator
        for (INT32 i = 0; i < len; ++i)
        {
            if (fullPath[i] == PATH_SEPARATOR)
            {
                lastSeparator = i;
            }
        }

        // If no separator found, return the full path as file name
        if (lastSeparator == -1)
        {
            TChar *copy = new TChar[len + 1];
            return String::Copy(copy, fullPath);
        }

        // Calculate file name length
        INT32 fileNameLen = len - lastSeparator - 1;

        // Allocate buffer for file name
        TChar *fileName = new TChar[fileNameLen + 1];

        // Copy file name
        for (INT32 i = 0; i < fileNameLen; ++i)
        {
            fileName[i] = fullPath[lastSeparator + 1 + i];
        }

        // Null terminate
        fileName[fileNameLen] = (TChar)0;

        return fileName;
    }

    template <TCHAR TChar>
    static TChar *GetExtension(const TChar *fileName)
    {
        INT32 len = String::Length(fileName);
        INT32 lastDot = -1;

        // Find the last occurrence of the dot character
        for (INT32 i = 0; i < len; ++i)
        {
            if (fileName[i] == (TChar)'.')
            {
                lastDot = i;
            }
        }

        // If no dot found, return empty string
        if (lastDot == -1 || lastDot == len - 1)
        {
            TChar *emptyExt = new TChar[1];
            emptyExt[0] = (TChar)0;
            return emptyExt;
        }

        // Calculate extension length
        INT32 extLen = len - lastDot - 1;

        // Allocate buffer for extension
        TChar *extension = new TChar[extLen + 1];

        // Copy extension
        for (INT32 i = 0; i < extLen; ++i)
        {
            extension[i] = fileName[lastDot + 1 + i];
        }

        // Null terminate
        extension[extLen] = (TChar)0;

        return extension;
    }

    template <TCHAR TChar>
    static TChar *GetDirectoryName(const TChar *fullPath)
    {
        INT32 len = String::Length(fullPath);
        INT32 lastSeparator = -1;

        // Find the last occurrence of the path separator
        for (INT32 i = 0; i < len; ++i)
        {
            if (fullPath[i] == PATH_SEPARATOR)
            {
                lastSeparator = i;
            }
        }

        // If no separator found, return empty string
        if (lastSeparator == -1)
        {
            TChar *emptyDir = new TChar[1];
            emptyDir[0] = (TChar)0;
            return emptyDir;
        }

        // Allocate buffer for directory name
        TChar *directoryName = new TChar[lastSeparator + 1];

        // Copy directory name
        for (INT32 i = 0; i < lastSeparator; ++i)
        {
            directoryName[i] = fullPath[i];
        }

        // Null terminate
        directoryName[lastSeparator] = (TChar)0;

        return directoryName;
    }

    template <TCHAR TChar>
    static BOOL IsPathRooted(TChar *path)
    {
        // On Windows, a path is rooted if it starts with a drive letter or a backslash
        if (path == NULL || path[0] == (TChar)0)
            return FALSE;
#ifdef _WIN32
        return (((path[0] >= (TChar)'A' && path[0] <= (TChar)'Z') || (path[0] >= (TChar)'a' && path[0] <= (TChar)'z')) && path[1] == (TChar)':') || (path[0] == (TChar)'\\');
#else
        return path[0] == (TChar)'/';
#endif
    }

    static PWCHAR NormalizePath(PCWCHAR path)
    {
        if (path == NULL)
            return NULL;

        INT32 len = String::Length(path);
        PWCHAR normalized = new WCHAR[len + 1];

        for (INT32 i = 0; i < len; ++i)
        {
            if (path[i] == L'/' || path[i] == L'\\')
            {
                normalized[i] = PATH_SEPARATOR;
            }
            else
            {
                normalized[i] = path[i];
            }
        }
        normalized[len] = 0;

        return normalized;
    }
};
