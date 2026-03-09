#include "platform/system/machine_id.h"
#include "platform/kernel/uefi/efi_context.h"
#include "platform/kernel/uefi/efi_system_table.h"
#include "platform/console/logger.h"
#include "core/memory/memory.h"

// SMBIOS entry point structures (DMTF DSP0134)
#pragma pack(push, 1)

struct SMBIOS_ENTRY_POINT
{
	UINT8  AnchorString[4];     // "_SM_"
	UINT8  Checksum;
	UINT8  Length;
	UINT8  MajorVersion;
	UINT8  MinorVersion;
	UINT16 MaxStructureSize;
	UINT8  EntryPointRevision;
	UINT8  FormattedArea[5];
	UINT8  IntermediateAnchor[5]; // "_DMI_"
	UINT8  IntermediateChecksum;
	UINT16 TableLength;
	UINT32 TableAddress;
	UINT16 NumberOfStructures;
	UINT8  BCDRevision;
};

struct SMBIOS3_ENTRY_POINT
{
	UINT8  AnchorString[5];     // "_SM3_"
	UINT8  Checksum;
	UINT8  Length;
	UINT8  MajorVersion;
	UINT8  MinorVersion;
	UINT8  DocRevision;
	UINT8  EntryPointRevision;
	UINT8  Reserved;
	UINT32 TableMaximumSize;
	UINT64 TableAddress;
};

struct SMBIOS_HEADER
{
	UINT8  Type;
	UINT8  Length;
	UINT16 Handle;
};

struct SMBIOS_TYPE1
{
	SMBIOS_HEADER Header;
	UINT8  Manufacturer;
	UINT8  ProductName;
	UINT8  Version;
	UINT8  SerialNumber;
	UINT8  UUID[16];
};

#pragma pack(pop)

static Result<UUID, Error> ParseSmbiosTable(PUINT8 tableData, UINT32 tableLength)
{
	PUINT8 tableEnd = tableData + tableLength;

	while (tableData < tableEnd)
	{
		auto header = (SMBIOS_HEADER *)tableData;

		if (tableData + header->Length > tableEnd)
			break;

		if (header->Type == 1 && header->Length >= 0x19)
		{
			auto systemInfo = (SMBIOS_TYPE1 *)tableData;

			BOOL allZero = true;
			BOOL allFF = true;
			for (INT32 i = 0; i < 16; i++)
			{
				if (systemInfo->UUID[i] != 0x00) allZero = false;
				if (systemInfo->UUID[i] != 0xFF) allFF = false;
			}

			if (!allZero && !allFF)
			{
				// SMBIOS UUID is stored in mixed-endian format per spec.
				UINT8 uuidBytes[16];

				// TimeLow (4 bytes, little-endian -> big-endian)
				uuidBytes[0] = systemInfo->UUID[3];
				uuidBytes[1] = systemInfo->UUID[2];
				uuidBytes[2] = systemInfo->UUID[1];
				uuidBytes[3] = systemInfo->UUID[0];

				// TimeMid (2 bytes, little-endian -> big-endian)
				uuidBytes[4] = systemInfo->UUID[5];
				uuidBytes[5] = systemInfo->UUID[4];

				// TimeHiAndVersion (2 bytes, little-endian -> big-endian)
				uuidBytes[6] = systemInfo->UUID[7];
				uuidBytes[7] = systemInfo->UUID[6];

				// Remaining 8 bytes are already in big-endian order.
				for (INT32 i = 8; i < 16; i++)
					uuidBytes[i] = systemInfo->UUID[i];

				return Result<UUID, Error>::Ok(UUID(Span<const UINT8, 16>(uuidBytes)));
			}
		}

		// Skip past the formatted area to the string table.
		PUINT8 stringTable = tableData + header->Length;

		// The string table is terminated by a double null.
		while (stringTable < tableEnd - 1 && !(stringTable[0] == 0x00 && stringTable[1] == 0x00))
			stringTable++;

		tableData = stringTable + 2;
	}

	return Result<UUID, Error>::Err(Error(Error::None));
}

Result<UUID, Error> GetMachineUUID()
{
	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_SYSTEM_TABLE *st = ctx->SystemTable;

	// Try SMBIOS 3.0 first, then fall back to SMBIOS 2.x
	EFI_GUID smbios3Guid = SMBIOS3_TABLE_GUID;
	EFI_GUID smbiosGuid = SMBIOS_TABLE_GUID;

	for (USIZE i = 0; i < st->NumberOfTableEntries; i++)
	{
		EFI_CONFIGURATION_TABLE *entry = &st->ConfigurationTable[i];

		if (memcmp(&entry->VendorGuid, &smbios3Guid, sizeof(EFI_GUID)) == 0)
		{
			auto ep = (SMBIOS3_ENTRY_POINT *)entry->VendorTable;
			auto tableData = (PUINT8)(USIZE)ep->TableAddress;
			auto result = ParseSmbiosTable(tableData, ep->TableMaximumSize);
			if (result)
				return result;
		}
	}

	for (USIZE i = 0; i < st->NumberOfTableEntries; i++)
	{
		EFI_CONFIGURATION_TABLE *entry = &st->ConfigurationTable[i];

		if (memcmp(&entry->VendorGuid, &smbiosGuid, sizeof(EFI_GUID)) == 0)
		{
			auto ep = (SMBIOS_ENTRY_POINT *)entry->VendorTable;
			auto tableData = (PUINT8)(USIZE)ep->TableAddress;
			auto result = ParseSmbiosTable(tableData, ep->TableLength);
			if (result)
				return result;
		}
	}

	LOG_ERROR("SMBIOS table not found in EFI configuration tables");
	return Result<UUID, Error>::Err(Error(Error::None));
}
