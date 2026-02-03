#pragma once

#include "test_framework.h"

// ============================================================================
// FILE I/O TESTS
// ============================================================================

class FileIOTests
{
public:
    static BOOL RunAll()
    {
        BOOL allPassed = TRUE;
        LOG_INFO("Running File I/O Tests...");

        RUN_SCRIPT_TEST_FILEIO(allPassed, L"tests/language/scripts/fileio/file_open_close.pil"_embed,    "File open/close");
        RUN_SCRIPT_TEST_FILEIO(allPassed, L"tests/language/scripts/fileio/file_write_read.pil"_embed,    "File write/read");
        RUN_SCRIPT_TEST_FILEIO(allPassed, L"tests/language/scripts/fileio/file_exists.pil"_embed,        "File exists");
        RUN_SCRIPT_TEST_FILEIO(allPassed, L"tests/language/scripts/fileio/file_delete.pil"_embed,        "File delete");
        RUN_SCRIPT_TEST_FILEIO(allPassed, L"tests/language/scripts/fileio/file_size_seek_tell.pil"_embed,"File size/seek/tell");
        RUN_SCRIPT_TEST_FILEIO(allPassed, L"tests/language/scripts/fileio/file_append.pil"_embed,        "File append mode");
        RUN_SCRIPT_TEST_FILEIO(allPassed, L"tests/language/scripts/fileio/file_readline.pil"_embed,      "File readline");
        RUN_SCRIPT_TEST_FILEIO(allPassed, L"tests/language/scripts/fileio/directory_ops.pil"_embed,      "Directory mkdir/rmdir");
        RUN_SCRIPT_TEST_FILEIO(allPassed, L"tests/language/scripts/fileio/file_errors.pil"_embed,        "File error handling");
        RUN_SCRIPT_TEST_FILEIO(allPassed, L"tests/language/scripts/fileio/multiple_files.pil"_embed,     "Multiple files");

        if (allPassed)
            LOG_INFO("All File I/O Tests passed!");
        else
            LOG_ERROR("Some File I/O Tests failed!");

        return allPassed;
    }
};
