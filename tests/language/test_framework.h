#pragma once

#include "pil.h"
#include "fileio.h"
#include "networkio.h"
#include "console.h"
#include "file_system.h"
#include "tests.h"

// ============================================================================
// SCRIPT LOADING UTILITIES
// ============================================================================

/**
 * LoadScript - Load a PIL script from a file with dynamic allocation
 * Reads until EOF with no size limit. Caller must delete[] the returned buffer.
 */
static inline CHAR* LoadScript(PCWCHAR path)
{
    File file = FileSystem::Open(path, FileSystem::FS_READ | FileSystem::FS_BINARY);
    if (!file.IsValid())
    {
        LOG_ERROR("Failed to open script file");
        return nullptr;
    }

    // Start with 4KB, grow as needed
    USIZE capacity = 4096;
    USIZE totalRead = 0;
    CHAR* buffer = new CHAR[capacity];

    while (true)
    {
        // Grow buffer if needed
        if (totalRead + 4096 >= capacity)
        {
            capacity *= 2;
            CHAR* newBuffer = new CHAR[capacity];
            for (USIZE i = 0; i < totalRead; i++)
                newBuffer[i] = buffer[i];
            delete[] buffer;
            buffer = newBuffer;
        }

        UINT32 bytesRead = file.Read(buffer + totalRead, 4096);
        if (bytesRead == 0)
            break;  // EOF reached

        totalRead += bytesRead;
    }

    file.Close();

    if (totalRead == 0)
    {
        LOG_ERROR("Script file is empty");
        delete[] buffer;
        return nullptr;
    }

    buffer[totalRead] = '\0';
    return buffer;
}

/**
 * RunScriptFile - Load and execute a PIL script file
 */
static inline BOOL RunScriptFile(PIL::State* L, PCWCHAR path)
{
    CHAR* source = LoadScript(path);
    if (source == nullptr)
        return FALSE;
    BOOL result = L->DoString(source);
    delete[] source;
    return result;
}

/**
 * RunScriptAndCheckResult - Execute a script and verify the 'result' global variable is TRUE
 */
static inline BOOL RunScriptAndCheckResult(PIL::State* L, PCWCHAR path)
{
    if (!RunScriptFile(L, path))
    {
        LOG_ERROR("    Script execution failed: %s", L->GetError());
        return FALSE;
    }

    PIL::Value resultValue;
    if (!L->GetGlobal("result"_embed, 6, resultValue))
    {
        LOG_ERROR("    Global 'result' variable not found");
        return FALSE;
    }

    if (!resultValue.IsBool())
    {
        LOG_ERROR("    Global 'result' is not a boolean");
        return FALSE;
    }

    if (!resultValue.boolValue)
    {
        LOG_ERROR("    Test assertion failed: result = false");
        return FALSE;
    }

    return TRUE;
}

// ============================================================================
// CONSOLE OUTPUT CALLBACK
// ============================================================================

static inline void ScriptConsoleOutput(const CHAR* str, USIZE len)
{
    Console::Write(str, len);
}

static inline PIL::State* CreateScriptState()
{
    PIL::State* L = new PIL::State();
    L->SetOutput(NULL);
    return L;
}

// ============================================================================
// TEST CONFIGURATION
// ============================================================================

/**
 * TestConfig - Configuration flags for test execution
 */
enum class TestConfig : UINT8
{
    NONE           = 0,
    OPEN_STDLIB    = 1 << 0,  // Call OpenStdLib
    OPEN_FILEIO    = 1 << 1,  // Call OpenFileIO (requires FilePool)
    OPEN_NETWORKIO = 1 << 2,  // Call OpenNetworkIO (requires NetworkContext)
    EXPECT_FAILURE = 1 << 3,  // Expect script to fail (negate result)
    CHECK_RESULT   = 1 << 4,  // Check 'result' global variable
    LOG_ERROR_INFO = 1 << 5,  // Log error info on expected failure
};

constexpr TestConfig operator|(TestConfig a, TestConfig b)
{
    return static_cast<TestConfig>(static_cast<UINT8>(a) | static_cast<UINT8>(b));
}

constexpr bool operator&(TestConfig a, TestConfig b)
{
    return (static_cast<UINT8>(a) & static_cast<UINT8>(b)) != 0;
}

// Common configurations
static constexpr TestConfig CFG_STDLIB = TestConfig::OPEN_STDLIB | TestConfig::CHECK_RESULT;
static constexpr TestConfig CFG_STDLIB_EXPECT_FAIL = TestConfig::OPEN_STDLIB | TestConfig::EXPECT_FAILURE | TestConfig::LOG_ERROR_INFO;
static constexpr TestConfig CFG_FILEIO = TestConfig::OPEN_STDLIB | TestConfig::OPEN_FILEIO;
static constexpr TestConfig CFG_NETWORKIO = TestConfig::OPEN_STDLIB | TestConfig::OPEN_NETWORKIO;

// ============================================================================
// SCRIPT TEST RUNNER
// ============================================================================

/**
 * RunScriptTestInline - Execute a script test with the given configuration
 */
static inline BOOL RunScriptTestInline(PCWCHAR path, TestConfig config,
                                        PIL::FilePool* filePool = nullptr,
                                        PIL::NetworkContext* netCtx = nullptr)
{
    PIL::State* L = CreateScriptState();

    // Open libraries based on configuration
    if (config & TestConfig::OPEN_STDLIB)
        PIL::OpenStdLib(*L);

    if (config & TestConfig::OPEN_FILEIO)
    {
        if (filePool)
            PIL::OpenFileIO(*L, filePool);
    }

    if (config & TestConfig::OPEN_NETWORKIO)
    {
        if (netCtx)
            PIL::OpenNetworkIO(*L, netCtx);
    }

    // Execute the test
    BOOL result;
    if (config & TestConfig::CHECK_RESULT)
    {
        result = RunScriptAndCheckResult(L, path);
    }
    else
    {
        result = RunScriptFile(L, path);
        if (!result && !(config & TestConfig::EXPECT_FAILURE))
        {
            LOG_ERROR("    Script error: %s at line %d", L->GetError(), L->GetErrorLine());
        }
    }

    // Handle expected failure
    if (config & TestConfig::EXPECT_FAILURE)
    {
        if (!result && (config & TestConfig::LOG_ERROR_INFO))
        {
            LOG_INFO("    Error detected: %s", L->GetError());
        }
        result = !result;  // Invert result for expected failures
    }

    delete L;
    return result;
}

// ============================================================================
// SCRIPT TEST FUNCTIONS
// ============================================================================

/**
 * RunScriptTest - Run a script test and log the result
 *
 * @param allPassedVar - Boolean variable to track overall test status
 * @param scriptPath   - Path to the script file (wide embedded string)
 * @param description  - Human-readable description of the test (wide embedded string)
 * @param config       - Test configuration flags
 * @return TRUE if test passed, FALSE otherwise
 */
inline BOOL RunScriptTest(BOOL& allPassedVar, PCWCHAR scriptPath, PCWCHAR description, TestConfig config)
{
    BOOL passed = RunScriptTestInline(scriptPath, config);
    if (passed)
        LOG_INFO("  PASSED: %ls", description);
    else
    {
        LOG_ERROR("  FAILED: %ls", description);
        allPassedVar = FALSE;
    }
    return passed;
}

/**
 * RunScriptTestFileIO - Run a script test with FileIO
 *
 * @param allPassedVar - Boolean variable to track overall test status
 * @param scriptPath   - Path to the script file (wide embedded string)
 * @param description  - Human-readable description of the test (wide embedded string)
 * @return TRUE if test passed, FALSE otherwise
 */
inline BOOL RunScriptTestFileIO(BOOL& allPassedVar, PCWCHAR scriptPath, PCWCHAR description)
{
    PIL::FilePool pool;
    BOOL passed = RunScriptTestInline(scriptPath, CFG_FILEIO, &pool, nullptr);
    if (passed)
        LOG_INFO("  PASSED: %ls", description);
    else
    {
        LOG_ERROR("  FAILED: %ls", description);
        allPassedVar = FALSE;
    }
    return passed;
}

/**
 * RunScriptTestNetworkIO - Run a script test with NetworkIO
 *
 * NOTE: NetworkContext is allocated on heap because it's very large (~32KB+)
 * due to inline storage for HttpClient and WebSocketClient objects.
 * Stack allocation causes overflow in -O0 builds.
 *
 * @param allPassedVar - Boolean variable to track overall test status
 * @param scriptPath   - Path to the script file (wide embedded string)
 * @param description  - Human-readable description of the test (wide embedded string)
 * @return TRUE if test passed, FALSE otherwise
 */
inline BOOL RunScriptTestNetworkIO(BOOL& allPassedVar, PCWCHAR scriptPath, PCWCHAR description)
{
    PIL::NetworkContext* netCtx = new PIL::NetworkContext();
    BOOL passed = RunScriptTestInline(scriptPath, CFG_NETWORKIO, nullptr, netCtx);
    delete netCtx;
    if (passed)
        LOG_INFO("  PASSED: %ls", description);
    else
    {
        LOG_ERROR("  FAILED: %ls", description);
        allPassedVar = FALSE;
    }
    return passed;
}
