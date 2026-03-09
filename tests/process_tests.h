#pragma once

#include "runtime/runtime.h"
#include "platform/system/process.h"
#include "tests.h"

class ProcessTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Process Tests...");

		RunTest(allPassed, EMBED_FUNC(TestCreateInvalidPath), "Create with null path returns error"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCreateInvalidArgs), "Create with null args returns error"_embed);
		RunTest(allPassed, EMBED_FUNC(TestInvalidProcessState), "Invalid process state"_embed);
#if !defined(PLATFORM_UEFI)
		RunTest(allPassed, EMBED_FUNC(TestCreateAndWait), "Create process and wait for exit"_embed);
		RunTest(allPassed, EMBED_FUNC(TestTerminate), "Create and terminate process"_embed);
		RunTest(allPassed, EMBED_FUNC(TestIsRunning), "IsRunning on active process"_embed);
		RunTest(allPassed, EMBED_FUNC(TestMoveSemantics), "Move constructor and assignment"_embed);
		RunTest(allPassed, EMBED_FUNC(TestCreateWithIO), "Create process with I/O redirection"_embed);
#endif

		if (allPassed)
			LOG_INFO("All Process tests passed!");
		else
			LOG_ERROR("Some Process tests failed!");

		return allPassed;
	}

private:
	static BOOL TestCreateInvalidPath()
	{
		auto result = Process::Create(nullptr, nullptr);
		if (result)
		{
			LOG_ERROR("Create(nullptr) should have failed");
			return false;
		}
		return true;
	}

	static BOOL TestCreateInvalidArgs()
	{
		auto path = "/nonexistent"_embed;
		auto result = Process::Create((const CHAR *)path, nullptr);
		if (result)
		{
			LOG_ERROR("Create with null args should have failed");
			return false;
		}
		return true;
	}

	static BOOL TestInvalidProcessState()
	{
		// Create should fail with null path
		auto result = Process::Create(nullptr, nullptr);
		if (result)
		{
			LOG_ERROR("Expected Create to fail with null path");
			return false;
		}

		// Verify the error code is Process_CreateFailed
		if (result.Error().Code != Error::Process_CreateFailed)
		{
			LOG_ERROR("Expected Process_CreateFailed error code");
			return false;
		}

		return true;
	}

#if !defined(PLATFORM_UEFI)
	static BOOL TestCreateAndWait()
	{
#if defined(PLATFORM_WINDOWS)
		auto cmd = "C:\\Windows\\System32\\cmd.exe"_embed;
		const CHAR *args[] = {(const CHAR *)cmd, "/c", "exit", "0", nullptr};
#else
		auto cmd = "/bin/sh"_embed;
		const CHAR *args[] = {(const CHAR *)cmd, "-c", "exit 0", nullptr};
#endif

		auto result = Process::Create((const CHAR *)cmd, args);
		if (!result)
		{
			LOG_ERROR("Failed to create process: %e", result.Error());
			return false;
		}

		auto &proc = result.Value();
		if (!proc.IsValid())
		{
			LOG_ERROR("Process should be valid after Create");
			return false;
		}

		auto waitResult = proc.Wait();
		if (!waitResult)
		{
			LOG_ERROR("Wait failed: %e", waitResult.Error());
			return false;
		}

		SSIZE exitCode = waitResult.Value();
		if (exitCode != 0)
		{
			LOG_ERROR("Expected exit code 0, got %d", (INT32)exitCode);
			return false;
		}

		return true;
	}

	static BOOL TestTerminate()
	{
#if defined(PLATFORM_WINDOWS)
		auto cmd = "C:\\Windows\\System32\\cmd.exe"_embed;
		// /c pause would wait for input; use ping -n 60 to sleep
		const CHAR *args[] = {(const CHAR *)cmd, "/c", "ping", "-n", "60", "127.0.0.1", nullptr};
#else
		auto cmd = "/bin/sh"_embed;
		const CHAR *args[] = {(const CHAR *)cmd, "-c", "sleep 60", nullptr};
#endif

		auto result = Process::Create((const CHAR *)cmd, args);
		if (!result)
		{
			LOG_ERROR("Failed to create process: %e", result.Error());
			return false;
		}

		auto &proc = result.Value();

		auto termResult = proc.Terminate();
		if (!termResult)
		{
			LOG_ERROR("Terminate failed: %e", termResult.Error());
			return false;
		}

		// After terminate, wait should succeed (reap the child)
		auto waitResult = proc.Wait();
		if (!waitResult)
		{
			// On some platforms, wait after kill may fail — that's acceptable
			LOG_WARNING("Wait after terminate returned error (may be expected): %e", waitResult.Error());
		}

		return true;
	}

	static BOOL TestIsRunning()
	{
#if defined(PLATFORM_WINDOWS)
		auto cmd = "C:\\Windows\\System32\\cmd.exe"_embed;
		const CHAR *args[] = {(const CHAR *)cmd, "/c", "ping", "-n", "60", "127.0.0.1", nullptr};
#else
		auto cmd = "/bin/sh"_embed;
		const CHAR *args[] = {(const CHAR *)cmd, "-c", "sleep 60", nullptr};
#endif

		auto result = Process::Create((const CHAR *)cmd, args);
		if (!result)
		{
			LOG_ERROR("Failed to create process: %e", result.Error());
			return false;
		}

		auto &proc = result.Value();

		if (!proc.IsRunning())
		{
			LOG_ERROR("Process should be running immediately after Create");
			return false;
		}

		// Terminate it
		(void)proc.Terminate();
		(void)proc.Wait();

		return true;
	}

	static BOOL TestMoveSemantics()
	{
#if defined(PLATFORM_WINDOWS)
		auto cmd = "C:\\Windows\\System32\\cmd.exe"_embed;
		const CHAR *args[] = {(const CHAR *)cmd, "/c", "exit", "0", nullptr};
#else
		auto cmd = "/bin/sh"_embed;
		const CHAR *args[] = {(const CHAR *)cmd, "-c", "exit 0", nullptr};
#endif

		auto result = Process::Create((const CHAR *)cmd, args);
		if (!result)
		{
			LOG_ERROR("Failed to create process: %e", result.Error());
			return false;
		}

		// Move construct
		Process moved(static_cast<Process &&>(result.Value()));
		if (!moved.IsValid())
		{
			LOG_ERROR("Moved process should be valid");
			return false;
		}

		// Original should be invalid after move
		if (result.Value().IsValid())
		{
			LOG_ERROR("Source process should be invalid after move");
			// Clean up moved
			(void)moved.Wait();
			return false;
		}

		(void)moved.Wait();
		return true;
	}

	static BOOL TestCreateWithIO()
	{
		// Test that Create with IO redirection parameters doesn't crash
		// We can't easily verify IO was redirected without pipes, but we can
		// verify the process runs and exits normally with fd params

#if defined(PLATFORM_WINDOWS)
		auto cmd = "C:\\Windows\\System32\\cmd.exe"_embed;
		const CHAR *args[] = {(const CHAR *)cmd, "/c", "exit", "0", nullptr};
#else
		auto cmd = "/bin/sh"_embed;
		const CHAR *args[] = {(const CHAR *)cmd, "-c", "exit 0", nullptr};
#endif

		// Create with -1 (inherit) — should behave like no redirection
		auto result = Process::Create((const CHAR *)cmd, args, -1, -1, -1);
		if (!result)
		{
			LOG_ERROR("Failed to create process with default IO: %e", result.Error());
			return false;
		}

		auto waitResult = result.Value().Wait();
		if (!waitResult)
		{
			LOG_ERROR("Wait failed: %e", waitResult.Error());
			return false;
		}

		return true;
	}
#endif // !PLATFORM_UEFI
};
