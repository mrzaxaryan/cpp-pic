param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$FilePath
)

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public static class PICLoader
{
    #region Constants
    private const uint HASH_SEED = 77777;
    private const uint NT_ALLOCATE_VIRTUAL_MEMORY_HASH = 3580609816;

    private const uint PAGE_EXECUTE_READWRITE = 0x40;
    private const uint MEM_COMMIT = 0x1000;
    private const uint MEM_RESERVE = 0x2000;

    private const int DOS_HEADER_E_LFANEW_OFFSET = 0x3C;
    private const int EXPORT_DIR_NUM_NAMES_OFFSET = 0x18;
    private const int EXPORT_DIR_FUNCTIONS_OFFSET = 0x1C;
    private const int EXPORT_DIR_NAMES_OFFSET = 0x20;
    private const int EXPORT_DIR_ORDINALS_OFFSET = 0x24;
    #endregion

    #region Structures
    [StructLayout(LayoutKind.Sequential)]
    private struct Parameters
    {
        public uint InjectorVersion;
        public uint InjectorCompilationType;
        public IntPtr DllPath;
    }

    private struct ArchitectureOffsets
    {
        public int PebOffset;
        public int LdrOffset;
        public int InMemoryOrderOffset;
        public int DllBaseOffset;
        public int ArbUserPtrOffset;
        public int OptionalHeaderOffset;
    }
    #endregion

    #region Delegates
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate IntPtr GetTEBDelegate();

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate int NtAllocateVirtualMemory(
        IntPtr ProcessHandle,
        ref IntPtr BaseAddress,
        IntPtr ZeroBits,
        ref UIntPtr RegionSize,
        uint AllocationType,
        uint Protect
    );

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate int entry_t(ulong injectorVersion);
    #endregion

    #region P/Invoke
    [DllImport("kernel32.dll", EntryPoint = "VirtualProtect", SetLastError = true)]
    private static extern bool ChangeMemoryProtection(
        IntPtr lpAddress,
        UIntPtr dwSize,
        uint flNewProtect,
        out uint lpflOldProtect
    );
    #endregion

    #region Static Fields
    private static readonly NtAllocateVirtualMemory s_ntAllocateVirtualMemory;
    #endregion

    #region Static Constructor
    static PICLoader()
    {
        IntPtr tebAddress = GetTEBAddress();
        ArchitectureOffsets offsets = GetArchitectureOffsets();
        IntPtr ntdllHandle = GetNtdllHandle(tebAddress, offsets);
        IntPtr ntAllocateVirtualMemoryPtr = GetProcAddressByHash(ntdllHandle, NT_ALLOCATE_VIRTUAL_MEMORY_HASH);

        s_ntAllocateVirtualMemory = (NtAllocateVirtualMemory)
            Marshal.GetDelegateForFunctionPointer(ntAllocateVirtualMemoryPtr, typeof(NtAllocateVirtualMemory));
    }
    #endregion

    #region Public Methods
    public static void Load(byte[] bytes)
    {
        IntPtr tebAddress = GetTEBAddress();
        ArchitectureOffsets offsets = GetArchitectureOffsets();

        IntPtr pMemory = AllocateExecutableMemory(bytes);
        IntPtr parametersMemory = CreateParameters(tebAddress, offsets.ArbUserPtrOffset);

        ExecutePIC(pMemory);

        Marshal.FreeHGlobal(parametersMemory);
    }
    #endregion

    #region Private Helper Methods
    private static bool IsArm64()
    {
        string arch = Environment.GetEnvironmentVariable("PROCESSOR_ARCHITECTURE");
        return arch != null && arch.Equals("ARM64", StringComparison.OrdinalIgnoreCase);
    }

    private static ArchitectureOffsets GetArchitectureOffsets()
    {
        bool isArm64 = IsArm64();
        bool is64Bit = IntPtr.Size == 8;

        return new ArchitectureOffsets
        {
            PebOffset = isArm64 ? 0x60 : (is64Bit ? 0x60 : 0x30),
            LdrOffset = isArm64 ? 0x18 : (is64Bit ? 0x18 : 0x0C),
            InMemoryOrderOffset = isArm64 ? 0x20 : (is64Bit ? 0x20 : 0x14),
            DllBaseOffset = isArm64 ? 32 : (is64Bit ? 32 : 16),
            ArbUserPtrOffset = isArm64 ? 0x50 : (is64Bit ? 0x50 : 0x28),
            OptionalHeaderOffset = is64Bit ? 0x88 : 0x78
        };
    }

    private static byte[] GetTEBAssemblyCode()
    {
        if (IsArm64())
        {
            return new byte[] { 0xE0, 0x03, 0x12, 0xAA, 0xC0, 0x03, 0x5F, 0xD6 };
        }
        else if (IntPtr.Size == 8)
        {
            return new byte[] { 0x65, 0x48, 0xA1, 0x30, 0, 0, 0, 0, 0, 0, 0, 0xC3 };
        }
        else
        {
            return new byte[] { 0x64, 0xA1, 0x18, 0, 0, 0, 0xC3 };
        }
    }

    private static IntPtr GetTEBAddress()
    {
        byte[] asmCode = GetTEBAssemblyCode();
        IntPtr codePtr = Marshal.AllocHGlobal(asmCode.Length);

        try
        {
            Marshal.Copy(asmCode, 0, codePtr, asmCode.Length);

            uint oldProtect;
            ChangeMemoryProtection(codePtr, (UIntPtr)asmCode.Length, PAGE_EXECUTE_READWRITE, out oldProtect);

            GetTEBDelegate getTEB = (GetTEBDelegate)Marshal.GetDelegateForFunctionPointer(
                codePtr,
                typeof(GetTEBDelegate)
            );
            IntPtr tebAddress = getTEB();

            ChangeMemoryProtection(codePtr, (UIntPtr)asmCode.Length, oldProtect, out oldProtect);

            return tebAddress;
        }
        finally
        {
            Marshal.FreeHGlobal(codePtr);
        }
    }

    private static IntPtr GetNtdllHandle(IntPtr tebAddress, ArchitectureOffsets offsets)
    {
        IntPtr pebAddress = Marshal.ReadIntPtr(AddOffset(tebAddress, offsets.PebOffset));
        IntPtr loaderDataAddress = Marshal.ReadIntPtr(AddOffset(pebAddress, offsets.LdrOffset));
        IntPtr inMemoryOrderModuleList = AddOffset(loaderDataAddress, offsets.InMemoryOrderOffset);

        IntPtr firstEntry = Marshal.ReadIntPtr(inMemoryOrderModuleList);
        IntPtr secondEntry = Marshal.ReadIntPtr(firstEntry);

        return Marshal.ReadIntPtr(AddOffset(secondEntry, offsets.DllBaseOffset));
    }

    private static IntPtr AllocateExecutableMemory(byte[] bytes)
    {
        IntPtr pMemory = IntPtr.Zero;
        UIntPtr regionSize = (UIntPtr)bytes.Length;

        s_ntAllocateVirtualMemory(
            new IntPtr(-1),
            ref pMemory,
            IntPtr.Zero,
            ref regionSize,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        );

        Marshal.Copy(bytes, 0, pMemory, bytes.Length);
        return pMemory;
    }

    private static IntPtr CreateParameters(IntPtr tebAddress, int arbUserPtrOffset)
    {
        IntPtr parametersMemory = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(Parameters)));
        Parameters parameters = new Parameters
        {
            InjectorVersion = uint.MaxValue,
            InjectorCompilationType = 100,
            DllPath = IntPtr.Zero
        };

        Marshal.StructureToPtr(parameters, parametersMemory, false);
        Marshal.WriteIntPtr(tebAddress, arbUserPtrOffset, parametersMemory);

        return parametersMemory;
    }

    private static void ExecutePIC(IntPtr pMemory)
    {
        entry_t entry = (entry_t)Marshal.GetDelegateForFunctionPointer(pMemory, typeof(entry_t));
        entry(uint.MaxValue);
    }

    private static IntPtr AddOffset(IntPtr ptr, int offset)
    {
        return new IntPtr((IntPtr.Size == 8 ? ptr.ToInt64() : ptr.ToInt32()) + offset);
    }

    private static IntPtr GetProcAddressByHash(IntPtr hModule, uint targetHash)
    {
        int e_lfanew = Marshal.ReadInt32(AddOffset(hModule, DOS_HEADER_E_LFANEW_OFFSET));

        ArchitectureOffsets offsets = GetArchitectureOffsets();
        int exportTableRVA = Marshal.ReadInt32(AddOffset(hModule, e_lfanew + offsets.OptionalHeaderOffset));
        IntPtr exportTable = AddOffset(hModule, exportTableRVA);

        int numberOfNames = Marshal.ReadInt32(AddOffset(exportTable, EXPORT_DIR_NUM_NAMES_OFFSET));
        int addressOfFunctions = Marshal.ReadInt32(AddOffset(exportTable, EXPORT_DIR_FUNCTIONS_OFFSET));
        int addressOfNames = Marshal.ReadInt32(AddOffset(exportTable, EXPORT_DIR_NAMES_OFFSET));
        int addressOfNameOrdinals = Marshal.ReadInt32(AddOffset(exportTable, EXPORT_DIR_ORDINALS_OFFSET));

        IntPtr namesPtr = AddOffset(hModule, addressOfNames);
        IntPtr ordinalsPtr = AddOffset(hModule, addressOfNameOrdinals);
        IntPtr functionsPtr = AddOffset(hModule, addressOfFunctions);

        for (int i = 0; i < numberOfNames; i++)
        {
            string functionName = GetExportedFunctionName(hModule, namesPtr, i);
            uint currentHash = CalculateHash(functionName);

            if (currentHash == targetHash)
            {
                return GetFunctionAddress(hModule, ordinalsPtr, functionsPtr, i);
            }
        }

        return IntPtr.Zero;
    }

    private static string GetExportedFunctionName(IntPtr hModule, IntPtr namesPtr, int index)
    {
        IntPtr nameRVA = AddOffset(namesPtr, index * 4);
        IntPtr namePtr = AddOffset(hModule, Marshal.ReadInt32(nameRVA));
        return Marshal.PtrToStringAnsi(namePtr);
    }

    private static uint CalculateHash(string input)
    {
        uint hash = HASH_SEED;

        foreach (char c in input)
        {
            char lowerChar = (c >= 65 && c <= 90) ? (char)(c + 32) : c;
            hash = ((hash << 5) + hash) + lowerChar;
        }

        return hash;
    }

    private static IntPtr GetFunctionAddress(IntPtr hModule, IntPtr ordinalsPtr, IntPtr functionsPtr, int index)
    {
        IntPtr ordinalPtr = AddOffset(ordinalsPtr, index * 2);
        ushort ordinal = (ushort)Marshal.ReadInt16(ordinalPtr);

        IntPtr functionRVA = AddOffset(functionsPtr, ordinal * 4);
        uint functionOffset = (uint)Marshal.ReadInt32(functionRVA);

        return AddOffset(hModule, (int)functionOffset);
    }
    #endregion
}
"@

# Validate and normalize path
$fullPath = (Resolve-Path -LiteralPath $FilePath).Path
if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
    throw "File not found: $fullPath"
}

# Read file bytes and load PIC
[byte[]]$bytes = [System.IO.File]::ReadAllBytes($fullPath)
Write-Host "Loading PIC from: $fullPath"

[PICLoader]::Load($bytes)