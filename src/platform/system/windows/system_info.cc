#include "platform/system/system_info.h"
#include "platform/system/machine_id.h"
#include "platform/system/environment.h"
#include "platform/console/logger.h"
#include "core/memory/memory.h"
#include "core/string/string.h"

VOID GetSystemInfo(SystemInfo *info)
{
	Memory::Zero(info, sizeof(SystemInfo));

	// Machine UUID from SMBIOS
	auto uuidResult = GetMachineUUID();
	if (uuidResult.IsOk())
		info->MachineUUID = uuidResult.Value();
	else
		LOG_ERROR("Failed to retrieve machine UUID");

	// Hostname from COMPUTERNAME environment variable
	auto hostnameVar = "COMPUTERNAME"_embed;
	USIZE len = Environment::GetVariable((PCCHAR)hostnameVar, Span<CHAR>(info->Hostname, 255));
	if (len == 0)
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
#else
	auto arch = "unknown"_embed;
#endif
	StringUtils::CopyEmbed(arch, Span<CHAR>(info->Architecture, 31));

	// OS platform (compile-time)
	auto platform = "windows"_embed;
	StringUtils::CopyEmbed(platform, Span<CHAR>(info->Platform, 31));
}
