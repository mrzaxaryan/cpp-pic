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
		// Create root directory
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root"_embed)))
		{
			LOG_ERROR("Failed to create test_io_root");
			return false;
		}

		// Create first level directories
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1"_embed)))
		{
			LOG_ERROR("Failed to create level1_dir1");
			return false;
		}
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2"_embed)))
		{
			LOG_ERROR("Failed to create level1_dir2");
			return false;
		}
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir3"_embed)))
		{
			LOG_ERROR("Failed to create level1_dir3");
			return false;
		}

		// Create second level directories
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1"_embed)))
		{
			LOG_ERROR("Failed to create level2_dir1");
			return false;
		}
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir2"_embed)))
		{
			LOG_ERROR("Failed to create level2_dir2");
			return false;
		}
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir3"_embed)))
		{
			LOG_ERROR("Failed to create level2_dir3");
			return false;
		}
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir4"_embed)))
		{
			LOG_ERROR("Failed to create level2_dir4");
			return false;
		}
		if (!FileSystem::CreateDirectory(Path::NormalizePath(L"test_io_root\\level1_dir3\\level2_dir5"_embed)))
		{
			LOG_ERROR("Failed to create level2_dir5");
			return false;
		}

		// Verify all directories exist
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root"_embed)))
		{
			LOG_ERROR("test_io_root does not exist after creation");
			return false;
		}
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1"_embed)))
		{
			LOG_ERROR("level1_dir1 does not exist after creation");
			return false;
		}
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir2"_embed)))
		{
			LOG_ERROR("level1_dir2 does not exist after creation");
			return false;
		}
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir3"_embed)))
		{
			LOG_ERROR("level1_dir3 does not exist after creation");
			return false;
		}
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1"_embed)))
		{
			LOG_ERROR("level2_dir1 does not exist after creation");
			return false;
		}
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir2"_embed)))
		{
			LOG_ERROR("level2_dir2 does not exist after creation");
			return false;
		}
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir3"_embed)))
		{
			LOG_ERROR("level2_dir3 does not exist after creation");
			return false;
		}
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir4"_embed)))
		{
			LOG_ERROR("level2_dir4 does not exist after creation");
			return false;
		}
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir3\\level2_dir5"_embed)))
		{
			LOG_ERROR("level2_dir5 does not exist after creation");
			return false;
		}

		return true;
	}

	static BOOL TestCreateFilesInDirectories()
	{
		File rootFile = FileSystem::Open(Path::NormalizePath(L"test_io_root\\root_file.txt"_embed),
										 FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!rootFile.IsValid())
		{
			LOG_ERROR("Failed to create root_file.txt");
			return false;
		}
		rootFile.Close();

		File file1 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\file1.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file1.IsValid())
		{
			LOG_ERROR("Failed to create file1.txt");
			return false;
		}
		file1.Close();

		File file2 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir2\\file2.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file2.IsValid())
		{
			LOG_ERROR("Failed to create file2.txt");
			return false;
		}
		file2.Close();

		File file3 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir3\\file3.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file3.IsValid())
		{
			LOG_ERROR("Failed to create file3.txt");
			return false;
		}
		file3.Close();

		File file4 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1\\deep_file1.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file4.IsValid())
		{
			LOG_ERROR("Failed to create deep_file1.txt");
			return false;
		}
		file4.Close();

		File file5 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir2\\deep_file2.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file5.IsValid())
		{
			LOG_ERROR("Failed to create deep_file2.txt");
			return false;
		}
		file5.Close();

		File file6 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir3\\deep_file3.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file6.IsValid())
		{
			LOG_ERROR("Failed to create deep_file3.txt");
			return false;
		}
		file6.Close();

		File file7 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir4\\deep_file4.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file7.IsValid())
		{
			LOG_ERROR("Failed to create deep_file4.txt");
			return false;
		}
		file7.Close();

		File file8 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir3\\level2_dir5\\deep_file5.txt"_embed),
									  FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!file8.IsValid())
		{
			LOG_ERROR("Failed to create deep_file5.txt");
			return false;
		}
		file8.Close();

		File extra1 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\extra1.txt"_embed),
									   FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!extra1.IsValid())
		{
			LOG_ERROR("Failed to create extra1.txt");
			return false;
		}
		extra1.Close();

		File extra2 = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\extra2.txt"_embed),
									   FileSystem::FS_CREATE | FileSystem::FS_WRITE);
		if (!extra2.IsValid())
		{
			LOG_ERROR("Failed to create extra2.txt");
			return false;
		}
		extra2.Close();

		return true;
	}

	static BOOL TestWriteReadContent()
	{
		// Test 1: Simple text
		{
			File file = FileSystem::Open(Path::NormalizePath(L"test_io_root\\test_write_read.txt"_embed),
										 FileSystem::FS_CREATE | FileSystem::FS_WRITE | FileSystem::FS_TRUNCATE);
			if (!file.IsValid())
			{
				LOG_ERROR("Failed to open test_write_read.txt for writing");
				return false;
			}

			auto testData = "Hello, File System!"_embed;
			auto writeResult = file.Write((const CHAR *)testData, 20);
			if (!writeResult)
			{
				LOG_ERROR("Write to test_write_read.txt failed (error: %e)", writeResult.Error());
				return false;
			}
			if (writeResult.Value() != 20)
			{
				LOG_ERROR("Write to test_write_read.txt: expected 20 bytes, got %u", writeResult.Value());
				return false;
			}

			file.Close();

			// Read it back
			file = FileSystem::Open(Path::NormalizePath(L"test_io_root\\test_write_read.txt"_embed), FileSystem::FS_READ);
			if (!file.IsValid())
			{
				LOG_ERROR("Failed to open test_write_read.txt for reading");
				return false;
			}

			CHAR buffer[32];
			Memory::Zero(buffer, 32);
			auto readResult = file.Read(buffer, 20);
			if (!readResult)
			{
				LOG_ERROR("Read from test_write_read.txt failed (error: %e)", readResult.Error());
				return false;
			}
			if (readResult.Value() != 20)
			{
				LOG_ERROR("Read from test_write_read.txt: expected 20 bytes, got %u", readResult.Value());
				return false;
			}

			// Verify content
			for (INT32 i = 0; i < 20; i++)
			{
				if (buffer[i] != ((const CHAR *)testData)[i])
				{
					LOG_ERROR("Content mismatch at index %d", i);
					return false;
				}
			}

			file.Close();
		}

		// Test 2: Binary data
		{
			File file = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\binary_test.dat"_embed),
										 FileSystem::FS_CREATE | FileSystem::FS_WRITE | FileSystem::FS_TRUNCATE);
			if (!file.IsValid())
			{
				LOG_ERROR("Failed to open binary_test.dat for writing");
				return false;
			}

			UINT8 binaryData[256];
			for (INT32 i = 0; i < 256; i++)
			{
				binaryData[i] = (UINT8)i;
			}

			auto writeResult = file.Write(binaryData, 256);
			if (!writeResult)
			{
				LOG_ERROR("Binary write failed (error: %e)", writeResult.Error());
				return false;
			}
			if (writeResult.Value() != 256)
			{
				LOG_ERROR("Binary write: expected 256 bytes, got %u", writeResult.Value());
				return false;
			}

			file.Close();

			// Read it back
			file = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir1\\binary_test.dat"_embed), FileSystem::FS_READ);
			if (!file.IsValid())
			{
				LOG_ERROR("Failed to open binary_test.dat for reading");
				return false;
			}

			UINT8 readBuffer[256];
			Memory::Zero(readBuffer, 256);
			auto readResult = file.Read(readBuffer, 256);
			if (!readResult)
			{
				LOG_ERROR("Binary read failed (error: %e)", readResult.Error());
				return false;
			}
			if (readResult.Value() != 256)
			{
				LOG_ERROR("Binary read: expected 256 bytes, got %u", readResult.Value());
				return false;
			}

			// Verify content
			for (INT32 i = 0; i < 256; i++)
			{
				if (readBuffer[i] != (UINT8)i)
				{
					LOG_ERROR("Binary content mismatch at index %d: got %u", i, (UINT32)readBuffer[i]);
					return false;
				}
			}

			file.Close();
		}

		// Test 3: File offset operations
		{
			File file = FileSystem::Open(Path::NormalizePath(L"test_io_root\\level1_dir2\\offset_test.dat"_embed),
										 FileSystem::FS_CREATE | FileSystem::FS_WRITE | FileSystem::FS_TRUNCATE);
			if (!file.IsValid())
			{
				LOG_ERROR("Failed to open offset_test.dat for writing");
				return false;
			}

			auto data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"_embed;
			auto writeResult = file.Write((const CHAR *)data, 26);
			if (!writeResult)
			{
				LOG_ERROR("Offset test write failed (error: %e)", writeResult.Error());
				return false;
			}

			// Test SetOffset
			file.SetOffset(10);
			if (file.GetOffset() != 10)
			{
				LOG_ERROR("SetOffset(10): GetOffset() returned %u", (UINT32)file.GetOffset());
				return false;
			}

			// Test MoveOffset from current position
			file.MoveOffset(5, OffsetOrigin::Current);
			if (file.GetOffset() != 15)
			{
				LOG_ERROR("MoveOffset(5, Current): GetOffset() returned %u", (UINT32)file.GetOffset());
				return false;
			}

			// Test MoveOffset from start
			file.MoveOffset(0, OffsetOrigin::Start);
			if (file.GetOffset() != 0)
			{
				LOG_ERROR("MoveOffset(0, Start): GetOffset() returned %u", (UINT32)file.GetOffset());
				return false;
			}

			file.Close();
		}

		return true;
	}

	static BOOL TestFileExistence()
	{
		// Test existing files
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\root_file.txt"_embed)))
		{
			LOG_ERROR("root_file.txt should exist");
			return false;
		}
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1\\file1.txt"_embed)))
		{
			LOG_ERROR("file1.txt should exist");
			return false;
		}
		if (!FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1\\deep_file1.txt"_embed)))
		{
			LOG_ERROR("deep_file1.txt should exist");
			return false;
		}

		// Test non-existing files
		if (FileSystem::Exists(Path::NormalizePath(L"test_io_root\\nonexistent.txt"_embed)))
		{
			LOG_ERROR("nonexistent.txt should not exist");
			return false;
		}
		if (FileSystem::Exists(Path::NormalizePath(L"test_io_root\\level1_dir1\\missing.txt"_embed)))
		{
			LOG_ERROR("missing.txt should not exist");
			return false;
		}

		return true;
	}

	static BOOL TestDirectoryIteration()
	{
		DirectoryIterator rootIter;
		auto result = rootIter.Initialization(Path::NormalizePath(L""_embed));
		if (!result.IsOk())
		{
			LOG_ERROR("Failed to create DirectoryIterator for root");
			return false;
		}
		// Test iterating through a directory with multiple files
		DirectoryIterator iter;
		result = iter.Initialization(Path::NormalizePath(L"test_io_root\\level1_dir1"_embed));
		if (!result.IsOk())
		{
			LOG_ERROR("Failed to create DirectoryIterator for level1_dir1");
			return false;
		}

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
		{
			LOG_ERROR("Directory iteration: expected 4 files, got %d", fileCount);
			return false;
		}
		if (dirCount != 2)
		{
			LOG_ERROR("Directory iteration: expected 2 dirs, got %d", dirCount);
			return false;
		}

		return true;
	}

	static BOOL TestCleanup()
	{
		// Delete files first (from deepest to shallowest)

		// Second level files
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1\\deep_file1.txt"_embed)))
		{
			LOG_ERROR("Failed to delete deep_file1.txt");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir2\\deep_file2.txt"_embed)))
		{
			LOG_ERROR("Failed to delete deep_file2.txt");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir3\\deep_file3.txt"_embed)))
		{
			LOG_ERROR("Failed to delete deep_file3.txt");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir4\\deep_file4.txt"_embed)))
		{
			LOG_ERROR("Failed to delete deep_file4.txt");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir3\\level2_dir5\\deep_file5.txt"_embed)))
		{
			LOG_ERROR("Failed to delete deep_file5.txt");
			return false;
		}

		// First level files
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\file1.txt"_embed)))
		{
			LOG_ERROR("Failed to delete file1.txt");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\extra1.txt"_embed)))
		{
			LOG_ERROR("Failed to delete extra1.txt");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\extra2.txt"_embed)))
		{
			LOG_ERROR("Failed to delete extra2.txt");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir1\\binary_test.dat"_embed)))
		{
			LOG_ERROR("Failed to delete binary_test.dat");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir2\\file2.txt"_embed)))
		{
			LOG_ERROR("Failed to delete file2.txt");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir2\\offset_test.dat"_embed)))
		{
			LOG_ERROR("Failed to delete offset_test.dat");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\level1_dir3\\file3.txt"_embed)))
		{
			LOG_ERROR("Failed to delete file3.txt");
			return false;
		}

		// Root level files
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\root_file.txt"_embed)))
		{
			LOG_ERROR("Failed to delete root_file.txt");
			return false;
		}
		if (!FileSystem::Delete(Path::NormalizePath(L"test_io_root\\test_write_read.txt"_embed)))
		{
			LOG_ERROR("Failed to delete test_write_read.txt");
			return false;
		}

		// Delete directories (from deepest to shallowest)

		// Second level directories
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir1"_embed)))
		{
			LOG_ERROR("Failed to delete level2_dir1");
			return false;
		}
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1\\level2_dir2"_embed)))
		{
			LOG_ERROR("Failed to delete level2_dir2");
			return false;
		}
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir3"_embed)))
		{
			LOG_ERROR("Failed to delete level2_dir3");
			return false;
		}
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2\\level2_dir4"_embed)))
		{
			LOG_ERROR("Failed to delete level2_dir4");
			return false;
		}
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir3\\level2_dir5"_embed)))
		{
			LOG_ERROR("Failed to delete level2_dir5");
			return false;
		}

		// First level directories
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir1"_embed)))
		{
			LOG_ERROR("Failed to delete level1_dir1");
			return false;
		}
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir2"_embed)))
		{
			LOG_ERROR("Failed to delete level1_dir2");
			return false;
		}
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root\\level1_dir3"_embed)))
		{
			LOG_ERROR("Failed to delete level1_dir3");
			return false;
		}

		// Root directory
		if (!FileSystem::DeleteDirectory(Path::NormalizePath(L"test_io_root"_embed)))
		{
			LOG_ERROR("Failed to delete test_io_root");
			return false;
		}

		// Verify cleanup was successful
		if (FileSystem::Exists(Path::NormalizePath(L"test_io_root"_embed)))
		{
			LOG_ERROR("test_io_root still exists after cleanup");
			return false;
		}

		return true;
	}
};
