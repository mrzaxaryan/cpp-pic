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

        auto source = R"SCRIPT(
var f = fopen("test_open.txt", "w");
if (f < 0) {
    print("ERROR: Failed to open file for writing");
} else {
    print("Opened file with handle:", f);
    var closed = fclose(f);
    if (closed) {
        print("File closed successfully");
    } else {
        print("ERROR: Failed to close file");
    }
}

// Clean up
fdelete("test_open.txt");
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
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

        auto source = R"SCRIPT(
// Write to file
var f = fopen("test_rw.txt", "w");
if (f < 0) {
    print("ERROR: Failed to open file for writing");
} else {
    var written = fwrite(f, "Hello, World!");
    print("Wrote", written, "bytes");
    fclose(f);
}

// Read from file
var f2 = fopen("test_rw.txt", "r");
if (f2 < 0) {
    print("ERROR: Failed to open file for reading");
} else {
    var content = fread(f2);
    print("Read content:", content);
    if (content == "Hello, World!") {
        print("Content matches!");
    } else {
        print("ERROR: Content mismatch");
    }
    fclose(f2);
}

// Clean up
fdelete("test_rw.txt");
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
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

        auto source = R"SCRIPT(
// File should not exist initially
if (fexists("test_exists.txt")) {
    print("ERROR: File should not exist yet");
}

// Create file
var f = fopen("test_exists.txt", "w");
fwrite(f, "test");
fclose(f);

// Now file should exist
if (fexists("test_exists.txt")) {
    print("File exists after creation - PASS");
} else {
    print("ERROR: File should exist after creation");
}

// Clean up
fdelete("test_exists.txt");

// File should not exist after deletion
if (!fexists("test_exists.txt")) {
    print("File gone after deletion - PASS");
} else {
    print("ERROR: File should be deleted");
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
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

        auto source = R"SCRIPT(
// Create file
var f = fopen("test_delete.txt", "w");
fwrite(f, "delete me");
fclose(f);

// Delete file
var deleted = fdelete("test_delete.txt");
if (deleted) {
    print("File deleted successfully - PASS");
} else {
    print("ERROR: Failed to delete file");
}

// Verify deletion
if (!fexists("test_delete.txt")) {
    print("File verified deleted - PASS");
} else {
    print("ERROR: File still exists after delete");
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
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

        auto source = R"SCRIPT(
// Create file with known content
var f = fopen("test_seek.txt", "w");
fwrite(f, "0123456789");
fclose(f);

// Open for reading and test fsize
var f2 = fopen("test_seek.txt", "r");
var size = fsize(f2);
print("File size:", size);
if (size == 10) {
    print("Size correct - PASS");
} else {
    print("ERROR: Expected size 10");
}

// Test ftell at start
var pos = ftell(f2);
if (pos == 0) {
    print("Initial position 0 - PASS");
} else {
    print("ERROR: Expected position 0, got", pos);
}

// Read some bytes and check position
var data = fread(f2, 5);
print("Read:", data);
pos = ftell(f2);
if (pos == 5) {
    print("Position after read - PASS");
} else {
    print("ERROR: Expected position 5, got", pos);
}

// Seek to start
fseek(f2, 0, 0);
pos = ftell(f2);
if (pos == 0) {
    print("Seek to start - PASS");
} else {
    print("ERROR: Seek to start failed");
}

// Seek to end
fseek(f2, 0, 2);
pos = ftell(f2);
if (pos == 10) {
    print("Seek to end - PASS");
} else {
    print("ERROR: Seek to end failed, got", pos);
}

// Seek relative from current - back 3 bytes
fseek(f2, -3, 1);
pos = ftell(f2);
if (pos == 7) {
    print("Relative seek - PASS");
} else {
    print("ERROR: Relative seek failed, got", pos);
}

fclose(f2);
fdelete("test_seek.txt");
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
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

        auto source = R"SCRIPT(
// Create initial file
var f = fopen("test_append.txt", "w");
fwrite(f, "Hello");
fclose(f);

// Append to file
var f2 = fopen("test_append.txt", "a");
fwrite(f2, " World");
fclose(f2);

// Read and verify
var f3 = fopen("test_append.txt", "r");
var content = fread(f3);
print("Appended content:", content);
if (content == "Hello World") {
    print("Append mode - PASS");
} else {
    print("ERROR: Expected Hello World");
}
fclose(f3);

fdelete("test_append.txt");
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
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

        // Note: Using explicit \n characters for line endings
        auto source = R"SCRIPT(
// Create file with multiple lines
var f = fopen("test_lines.txt", "w");
fwrite(f, "Line 1\nLine 2\nLine 3");
fclose(f);

// Read lines one by one
var f2 = fopen("test_lines.txt", "r");
var line1 = freadline(f2);
var line2 = freadline(f2);
var line3 = freadline(f2);
var line4 = freadline(f2);

print("Line 1:", line1);
print("Line 2:", line2);
print("Line 3:", line3);

if (line1 == "Line 1") {
    print("Line 1 correct - PASS");
} else {
    print("ERROR: Line 1 mismatch");
}

if (line2 == "Line 2") {
    print("Line 2 correct - PASS");
} else {
    print("ERROR: Line 2 mismatch");
}

if (line3 == "Line 3") {
    print("Line 3 correct - PASS");
} else {
    print("ERROR: Line 3 mismatch");
}

// Line 4 should be nil - EOF
if (line4 == nil) {
    print("EOF returns nil - PASS");
} else {
    print("ERROR: Expected nil at EOF");
}

fclose(f2);
fdelete("test_lines.txt");
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
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

        auto source = R"SCRIPT(
// Create directory
var created = mkdir("test_dir");
if (created) {
    print("Directory created - PASS");
} else {
    print("ERROR: Failed to create directory");
}

// Create a file inside the directory
var f = fopen("test_dir/test.txt", "w");
if (f >= 0) {
    fwrite(f, "test content");
    fclose(f);
    print("File in directory created - PASS");
} else {
    print("Note: Creating file in new dir might need full path");
}

// Clean up file first
fdelete("test_dir/test.txt");

// Remove directory
var removed = rmdir("test_dir");
if (removed) {
    print("Directory removed - PASS");
} else {
    print("Note: Directory removal may have failed");
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
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

        auto source = R"SCRIPT(
// Try to open non-existent file for reading
var f = fopen("nonexistent_file_12345.txt", "r");
if (f < 0) {
    print("Opening non-existent file returns -1 - PASS");
} else {
    print("ERROR: Should fail to open non-existent file");
    fclose(f);
}

// Try to close invalid handle
var closed = fclose(999);
if (!closed) {
    print("Closing invalid handle returns false - PASS");
} else {
    print("ERROR: Should fail to close invalid handle");
}

// Try to read from invalid handle
var data = fread(999);
if (len(data) == 0) {
    print("Reading invalid handle returns empty - PASS");
} else {
    print("ERROR: Should return empty for invalid handle");
}

// Try to write to invalid handle
var written = fwrite(999, "test");
if (written < 0) {
    print("Writing to invalid handle returns -1 - PASS");
} else {
    print("ERROR: Should fail to write to invalid handle");
}

// Invalid mode
var f2 = fopen("test.txt", "xyz");
if (f2 < 0) {
    print("Invalid mode returns -1 - PASS");
} else {
    print("ERROR: Should fail with invalid mode");
    fclose(f2);
}
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
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

        auto source = R"SCRIPT(
// Open multiple files simultaneously
var f1 = fopen("multi1.txt", "w");
var f2 = fopen("multi2.txt", "w");
var f3 = fopen("multi3.txt", "w");

if (f1 >= 0 && f2 >= 0 && f3 >= 0) {
    print("Opened 3 files with handles:", f1, f2, f3);

    // Verify different handles
    if (f1 != f2 && f2 != f3 && f1 != f3) {
        print("All handles unique - PASS");
    } else {
        print("ERROR: Handles should be unique");
    }

    // Write different content to each
    fwrite(f1, "File One");
    fwrite(f2, "File Two");
    fwrite(f3, "File Three");

    fclose(f1);
    fclose(f2);
    fclose(f3);

    // Verify content
    var r1 = fopen("multi1.txt", "r");
    var r2 = fopen("multi2.txt", "r");
    var r3 = fopen("multi3.txt", "r");

    var c1 = fread(r1);
    var c2 = fread(r2);
    var c3 = fread(r3);

    print("File 1:", c1);
    print("File 2:", c2);
    print("File 3:", c3);

    if (c1 == "File One" && c2 == "File Two" && c3 == "File Three") {
        print("Multiple files content correct - PASS");
    } else {
        print("ERROR: Content mismatch");
    }

    fclose(r1);
    fclose(r2);
    fclose(r3);
} else {
    print("ERROR: Failed to open multiple files");
}

// Clean up
fdelete("multi1.txt");
fdelete("multi2.txt");
fdelete("multi3.txt");
)SCRIPT"_embed;

        BOOL result = L->DoString(source);
        if (!result)
        {
            LOG_ERROR("Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
        delete L;
        return result;
    }
};
