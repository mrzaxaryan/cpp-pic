#pragma once

#include "pil/pil.h"
#include "pil/fileio.h"
#include "tests.h"

// ============================================================================
// FILE I/O TESTS CLASS
// ============================================================================

class FileIOTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;

        LOG_INFO("Running File I/O Tests...");

        RUN_TEST(allPassed, TestFileOpenClose, "File open/close");
        RUN_TEST(allPassed, TestFileWriteRead, "File write/read");
        RUN_TEST(allPassed, TestFileExists, "File exists");
        RUN_TEST(allPassed, TestFileDelete, "File delete");
        RUN_TEST(allPassed, TestFileSizeSeekTell, "File size/seek/tell");
        RUN_TEST(allPassed, TestFileAppend, "File append mode");
        RUN_TEST(allPassed, TestFileReadLine, "File readline");
        RUN_TEST(allPassed, TestDirectoryOperations, "Directory mkdir/rmdir");
        RUN_TEST(allPassed, TestFileErrors, "File error handling");
        RUN_TEST(allPassed, TestMultipleFiles, "Multiple files");

        if (allPassed)
            LOG_INFO("All File I/O tests passed!");
        else
            LOG_ERROR("Some File I/O tests failed!");

        return allPassed;
    }

private:
    // Test file open and close
    static BOOL TestFileOpenClose()
    {
        script::FilePool pool;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenFileIO(*L, &pool);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/fileio/file_open_close.pil");
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test file write and read
    static BOOL TestFileWriteRead()
    {
        script::FilePool pool;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenFileIO(*L, &pool);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/fileio/file_write_read.pil");
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test file exists
    static BOOL TestFileExists()
    {
        script::FilePool pool;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenFileIO(*L, &pool);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/fileio/file_exists.pil");
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test file delete
    static BOOL TestFileDelete()
    {
        script::FilePool pool;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenFileIO(*L, &pool);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/fileio/file_delete.pil");
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test file size, seek, and tell
    static BOOL TestFileSizeSeekTell()
    {
        script::FilePool pool;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenFileIO(*L, &pool);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/fileio/file_size_seek_tell.pil");
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test file append mode
    static BOOL TestFileAppend()
    {
        script::FilePool pool;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenFileIO(*L, &pool);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/fileio/file_append.pil");
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test file readline
    static BOOL TestFileReadLine()
    {
        script::FilePool pool;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenFileIO(*L, &pool);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/fileio/file_readline.pil");
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test directory operations
    static BOOL TestDirectoryOperations()
    {
        script::FilePool pool;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenFileIO(*L, &pool);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/fileio/directory_ops.pil");
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test error handling
    static BOOL TestFileErrors()
    {
        script::FilePool pool;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenFileIO(*L, &pool);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/fileio/file_errors.pil");
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }

    // Test multiple files
    static BOOL TestMultipleFiles()
    {
        script::FilePool pool;
        script::State* L = CreateScriptState();
        script::OpenStdLib(*L);
        script::OpenFileIO(*L, &pool);

        BOOL result = RunScriptFile(L, L"tests/pil/scripts/fileio/multiple_files.pil");
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }
};
