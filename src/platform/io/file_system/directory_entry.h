/**
 * @file directory_entry.h
 * @brief Directory entry structure for filesystem iteration
 * @details Defines the DirectoryEntry struct used by DirectoryIterator to
 * represent files and directories. Packed layout with no padding.
 * Part of the PLATFORM layer of the Position-Independent Runtime (PIR).
 */

#pragma once

#include "core/types/primitives.h"

#pragma pack(push, 1) // Ensure no padding in structures
// Directory entry structure
struct DirectoryEntry
{
	WCHAR Name[256];         // File or directory name
	UINT64 CreationTime;     // Filetime format
	UINT64 LastModifiedTime; // Filetime format
	UINT64 Size;             // Size in bytes
	UINT32 Type;             // Store DriveType (2=Removable, 3=Fixed, etc.)
	BOOL IsDirectory;        // Set if the entry is a directory
	BOOL IsDrive;            // Set if the entry represents a root (e.g., C:\)
	BOOL IsHidden;           // Flag for hidden files
	BOOL IsSystem;           // Flag for system files
	BOOL IsReadOnly;         // Flag for read-only files
};

#pragma pack(pop)
