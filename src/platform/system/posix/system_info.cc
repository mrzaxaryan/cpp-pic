#include "platform/system/system_info.h"
#include "platform/system/machine_id.h"
#include "platform/system/environment.h"
#include "platform/fs/file.h"
#include "platform/console/logger.h"
#include "core/memory/memory.h"
#include "core/string/string.h"

VOID GetSystemInfo(SystemInfo *info)
{
	Memory::Zero(info, sizeof(SystemInfo));

	// Machine UUID from /etc/machine-id or boot_id
	auto uuidResult = GetMachineUUID();
	if (uuidResult.IsOk())
		info->MachineUUID = uuidResult.Value();
	else
		LOG_ERROR("Failed to retrieve machine UUID");

	// Hostname from HOSTNAME environment variable
	auto hostnameVar = "HOSTNAME"_embed;
	USIZE len = Environment::GetVariable((PCCHAR)hostnameVar, Span<CHAR>(info->Hostname, 255));

	// Fallback: read /etc/hostname
	if (len == 0)
	{
		auto path = L"/etc/hostname"_embed;
		auto openResult = File::Open((const WCHAR *)path, File::ModeRead);
		if (openResult)
		{
			File &file = openResult.Value();
			auto readResult = file.Read(Span<UINT8>((UINT8 *)info->Hostname, 255));
			if (readResult && readResult.Value() > 0)
			{
				// Trim trailing newline
				len = readResult.Value();
				if (len > 0 && info->Hostname[len - 1] == '\n')
					info->Hostname[len - 1] = '\0';
			}
		}
	}
	
	if (info->Hostname[0] == '\0')
	{
		auto fallback = "unknown"_embed;
		StringUtils::CopyEmbed(fallback, Span<CHAR>(info->Hostname, 255));
	}

	// CPU architecture (compile-time)
#if defined(ARCHITECTURE_X86_64)
	auto arch = "x86_64"_embed;
#elif defined(ARCHITECTURE_I386)
	auto arch = "i386"_embed;
#elif defined(ARCHITECTURE_AARCH64)
	auto arch = "aarch64"_embed;
#elif defined(ARCHITECTURE_ARMV7A)
	auto arch = "armv7a"_embed;
#elif defined(ARCHITECTURE_RISCV64)
	auto arch = "riscv64"_embed;
#elif defined(ARCHITECTURE_RISCV32)
	auto arch = "riscv32"_embed;
#elif defined(ARCHITECTURE_MIPS64)
	auto arch = "mips64"_embed;
#else
	auto arch = "unknown"_embed;
#endif
	StringUtils::CopyEmbed(arch, Span<CHAR>(info->Architecture, 31));

	// OS platform (compile-time)
#if defined(PLATFORM_LINUX)
	auto platform = "linux"_embed;
#elif defined(PLATFORM_MACOS)
	auto platform = "macos"_embed;
#elif defined(PLATFORM_ANDROID)
	auto platform = "android"_embed;
#elif defined(PLATFORM_IOS)
	auto platform = "ios"_embed;
#elif defined(PLATFORM_FREEBSD)
	auto platform = "freebsd"_embed;
#elif defined(PLATFORM_SOLARIS)
	auto platform = "solaris"_embed;
#else
	auto platform = "unknown"_embed;
#endif
	StringUtils::CopyEmbed(platform, Span<CHAR>(info->Platform, 31));
}
