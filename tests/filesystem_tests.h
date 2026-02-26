#pragma once

#include "runtime.h"
#include "tests.h"

class FileSystemTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running FileSystem Tests...");

		RunTest(allPassed, EMBED_FUNC(TestCreateNestedDirectories), L"Create nested directories"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCreateFilesInDirectories), L"Create files in directories"_embed);
		RunTest(allPassed, EMBED_FUNC(TestWriteReadContent), L"Write and read file content"_embed);
		RunTest(allPassed, EMBED_FUNC(TestFileExistence), L"File existence checks"_embed);
		RunTest(allPassed, EMBED_FUNC(TestDirectoryIteration), L"Directory iteration"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCleanup), L"Cleanup files and directories"_embed);

		if (allPassed)
			LOG_INFO("All FileSystem tests passed!");
		else
			LOG_ERROR("Some FileSystem tests failed!");

		return allPassed;
	}

private:
	static BOOL TestCreateNestedDirectories()
	{
		// Create a test root directory structure:
		// test_io_root/
		//   ├── level1_dir1/
		//   │   ├── level2_dir1/
		//   │   └── level2_dir2/
		//   ├── level1_dir2/
		//   │   ├── level2_dir3/
		//   │   └── level2_dir4/
		//   └── level1_dir3/
		//       └── level2_dir5/

		// Create root directory
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root"_embed)))
			return false;

		// Create first level directories
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1"_embed)))
			return false;
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2"_embed)))
			return false;
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir3"_embed)))
			return false;

		// Create second level directories (nested inside first level)
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1"_embed)))
			return false;
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir2"_embed)))
			return false;
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir3"_embed)))
			return false;
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir4"_embed)))
			return false;
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir3\\level2_dir5"_embed)))
			return false;

		// Verify all directories exist
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root"_embed)))
			return false;
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1"_embed)))
			return false;
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir2"_embed)))
			return false;
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir3"_embed)))
			return false;
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1"_embed)))
			return false;
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir2"_embed)))
			return false;
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir3"_embed)))
			return false;
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir4"_embed)))
			return false;
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir3\\level2_dir5"_embed)))
			return false;

		return true;
	}

	static BOOL TestCreateFilesInDirectories()
	{
		// Create files in various directories at different levels
		// Root level file
		File rootFile = FileSystem::Open(Path::NormalizePath(L"test_io_root\\root_file.txt"_embed),
										 FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!rootFile.IsValid())
			return false;
		rootFile.Close();

		// Files in first level directories
		File file1 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\file1.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file1.IsValid())
			return false;
		file1.Close();

		File file2 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir2\\file2.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file2.IsValid())
			return false;
		file2.Close();

		File file3 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir3\\file3.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file3.IsValid())
			return false;
		file3.Close();

		// Files in second level directories
		File file4 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1\\deep_file1.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file4.IsValid())
			return false;
		file4.Close();

		File file5 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir2\\deep_file2.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file5.IsValid())
			return false;
		file5.Close();

		File file6 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir3\\deep_file3.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file6.IsValid())
			return false;
		file6.Close();

		File file7 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir4\\deep_file4.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file7.IsValid())
			return false;
		file7.Close();

		File file8 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir3\\level2_dir5\\deep_file5.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file8.IsValid())
			return false;
		file8.Close();

		// Create multiple files in one directory
		File extra1 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\extra1.txt"_embed),
									   FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!extra1.IsValid())
			return false;
		extra1.Close();

		File extra2 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\extra2.txt"_embed),
									   FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!extra2.IsValid())
			return false;
		extra2.Close();

		return true;
	}

	static BOOL TestWriteReadContent()
	{
		// Test writing and reading various content patterns

		// Test 1: Simple text
		{
			File file = FileSystem::Open(Path::NormalizePath(L"test_io_root\\test_write_read.txt"_embed),
										 FileSystem::FS_CREATE | FileSystem::FS_WRITE | FileSystem::FS_TRUNCATE);
			if (!file.IsValid())
				return false;

			auto testData = "Hello, File System!"_embed;
			auto writeResult = file.Write((const CHAR *)testData, 20);
			if (!writeResult || writeResult.Value() != 20)
				return false;

			file.Close();

			// Read it back
			file = FileSystem::Open(Path::NormalizePath(L"test_io_root\\test_write_read.txt"_embed), FileSystem::FS_READ);
			if (!file.IsValid())
				return false;

			CHAR buffer[32];
			Memory::Zero(buffer, 32);
			auto readResult = file.Read(buffer, 20);
			if (!readResult || readResult.Value() != 20)
				return false;

			// Verify content
			for (INT32 i = 0; i < 20; i++)
			{
				if (buffer[i] != ((const CHAR *)testData)[i])
					return false;
			}

			file.Close();
		}

		// Test 2: Binary data
		{
			File file = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\binary_test.dat"_embed),
										 FileSystem::FS_CREATE | FileSystem::FS_WRITE | FileSystem::FS_TRUNCATE);
			if (!file.IsValid())
				return false;

			UINT8 binaryData[256];
			for (INT32 i = 0; i < 256; i++)
			{
				binaryData[i] = (UINT8)i;
			}

			auto writeResult = file.Write(binaryData, 256);
			if (!writeResult || writeResult.Value() != 256)
				return false;

			file.Close();

			// Read it back
			file = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\binary_test.dat"_embed), FileSystem::FS_READ);
			if (!file.IsValid())
				return false;

			UINT8 readBuffer[256];
			Memory::Zero(readBuffer, 256);
			auto readResult = file.Read(readBuffer, 256);
			if (!readResult || readResult.Value() != 256)
				return false;

			// Verify content
			for (INT32 i = 0; i < 256; i++)
			{
				if (readBuffer[i] != (UINT8)i)
					return false;
			}

			file.Close();
		}

		// Test 3: File offset operations
		{
			File file = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir2\\offset_test.dat"_embed),
										 FileSystem::FS_CREATE | FileSystem::FS_WRITE | FileSystem::FS_TRUNCATE);
			if (!file.IsValid())
				return false;

			auto data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"_embed;
			auto writeResult = file.Write((const CHAR *)data, 26);
			if (!writeResult)
				return false;

			// Test SetOffset
			file.SetOffset(10);
			if (file.GetOffset() != 10)
				return false;

			// Test MoveOffset from current position
			file.MoveOffset(5, OffsetOrigin::Current);
			if (file.GetOffset() != 15)
				return false;

			// Test MoveOffset from start
			file.MoveOffset(0, OffsetOrigin::Start);
			if (file.GetOffset() != 0)
				return false;

			file.Close();
		}

		return true;
	}

	static BOOL TestFileExistence()
	{
		// Test existing files
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\root_file.txt"_embed)))
			return false;
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1\\file1.txt"_embed)))
			return false;
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1\\deep_file1.txt"_embed)))
			return false;

		// Test non-existing files
		if (FileSystem::Exists(Path::NormalizePath(L"test_io_root\\nonexistent.txt"_embed)))
			return false;
		if (FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1\\missing.txt"_embed)))
			return false;

		return true;
	}

	static BOOL TestDirectoryIteration()
	{
		DirectoryIterator rootIter(Path::NormalizePath(L""_embed));
		if (!rootIter.IsValid())
		{
			LOG_ERROR("Failed to create DirectoryIterator for root");
			return false;
		}
		// Test iterating through a directory with multiple files
		DirectoryIterator iter(Path::NormalizePath(L"test_io_root\\level1_dir1"_embed));
		if (!iter.IsValid())
			return false;

		INT32 fileCount = 0;
		INT32 dirCount = 0;

		while (iter.Next())
		{
			const DirectoryEntry &entry = iter.Get();

			// Skip "." and ".." entries
			if (entry.name[0] == L'.' &&
				(entry.name[1] == L'\0' ||
				 (entry.name[1] == L'.' && entry.name[2] == L'\0')))
			{
				continue;
			}

			if (entry.isDirectory)
			{
				dirCount++;
			}
			else
			{
				fileCount++;
			}
		}

		// We created 2 subdirectories and 3 files in level1_dir1
		// (file1.txt, extra1.txt, extra2.txt, binary_test.dat = 4 files)
		// (level2_dir1, level2_dir2 = 2 directories)
		if (fileCount != 4)
			return false;
		if (dirCount != 2)
			return false;

		return true;
	}

	static BOOL TestCleanup()
	{
		// Delete files first (from deepest to shallowest)

		// Second level files
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1\\deep_file1.txt"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir2\\deep_file2.txt"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir3\\deep_file3.txt"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir4\\deep_file4.txt"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir3\\level2_dir5\\deep_file5.txt"_embed)))
			return false;

		// First level files
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\file1.txt"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\extra1.txt"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\extra2.txt"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\binary_test.dat"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir2\\file2.txt"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir2\\offset_test.dat"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir3\\file3.txt"_embed)))
			return false;

		// Root level files
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\root_file.txt"_embed)))
			return false;
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\test_write_read.txt"_embed)))
			return false;

		// Delete directories (from deepest to shallowest)

		// Second level directories
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1"_embed)))
			return false;
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir2"_embed)))
			return false;
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir3"_embed)))
			return false;
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir4"_embed)))
			return false;
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir3\\level2_dir5"_embed)))
			return false;

		// First level directories
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1"_embed)))
			return false;
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2"_embed)))
			return false;
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir3"_embed)))
			return false;

		// Root directory
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root"_embed)))
			return false;

		// Verify cleanup was successful
		if (FileSystem::Exists(Path::NormalizePath(L"test_io_root"_embed)))
			return false;

		return true;
	}
};