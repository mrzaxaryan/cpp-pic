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

        RunScriptTestFileIO(allPassed, L"tests/language/scripts/fileio/file_open_close.pil"_embed,    L"File open/close"_embed);
        RunScriptTestFileIO(allPassed, L"tests/language/scripts/fileio/file_write_read.pil"_embed,    L"File write/read"_embed);
        RunScriptTestFileIO(allPassed, L"tests/language/scripts/fileio/file_exists.pil"_embed,        L"File exists"_embed);
        RunScriptTestFileIO(allPassed, L"tests/language/scripts/fileio/file_delete.pil"_embed,        L"File delete"_embed);
        RunScriptTestFileIO(allPassed, L"tests/language/scripts/fileio/file_size_seek_tell.pil"_embed,L"File size/seek/tell"_embed);
        RunScriptTestFileIO(allPassed, L"tests/language/scripts/fileio/file_append.pil"_embed,        L"File append mode"_embed);
        RunScriptTestFileIO(allPassed, L"tests/language/scripts/fileio/file_readline.pil"_embed,      L"File readline"_embed);
        RunScriptTestFileIO(allPassed, L"tests/language/scripts/fileio/directory_ops.pil"_embed,      L"Directory mkdir/rmdir"_embed);
        RunScriptTestFileIO(allPassed, L"tests/language/scripts/fileio/file_errors.pil"_embed,        L"File error handling"_embed);
        RunScriptTestFileIO(allPassed, L"tests/language/scripts/fileio/multiple_files.pil"_embed,     L"Multiple files"_embed);

        if (allPassed)
            LOG_INFO("All File I/O Tests passed!");
        else
            LOG_ERROR("Some File I/O Tests failed!");

        return allPassed;
    }
};
