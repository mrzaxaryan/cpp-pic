/**
 * @file screen.cc
 * @brief POSIX Screen Implementation (Linux/FreeBSD)
 *
 * @details Implements screen device enumeration and capture via the Linux
 * framebuffer device interface (/dev/fb0..fb7). Uses ioctl with
 * FBIOGET_VSCREENINFO and FBIOGET_FSCREENINFO to query display
 * parameters, and mmap with MAP_SHARED to read pixel data directly
 * from the framebuffer memory.
 *
 * The framebuffer API is shared between Linux and FreeBSD (via the
 * linuxkpi DRM compatibility layer on FreeBSD 13+).
 *
 * @note Framebuffer devices are limited to one display per /dev/fbN.
 * Multi-monitor setups with separate framebuffers are enumerated as
 * separate devices. Virtual desktop position is not available;
 * ScreenDevice::Left stores the framebuffer index for Capture().
 *
 * @see Linux framebuffer API (linux/fb.h)
 *      https://www.kernel.org/doc/html/latest/fb/api.html
 */

#include "runtime/display/screen.h"
#include "core/memory/memory.h"

#if defined(PLATFORM_LINUX)
#include "platform/common/linux/syscall.h"
#include "platform/common/linux/system.h"
#elif defined(PLATFORM_FREEBSD)
#include "platform/common/freebsd/syscall.h"
#include "platform/common/freebsd/system.h"
#endif

// =============================================================================
// Framebuffer ioctl constants
// =============================================================================

/// @brief Get variable screen info (resolution, pixel format)
constexpr USIZE FBIOGET_VSCREENINFO = 0x4600;

/// @brief Get fixed screen info (line length, memory size)
constexpr USIZE FBIOGET_FSCREENINFO = 0x4602;

// =============================================================================
// Framebuffer kernel structures
// =============================================================================

/// @brief Color component bitfield descriptor
struct FbBitfield
{
	UINT32 Offset;   ///< Bit position of the least significant bit
	UINT32 Length;   ///< Number of bits in this component
	UINT32 MsbRight; ///< MSB is rightmost (!=0) or leftmost (==0)
};

/// @brief Variable screen information (resolution, pixel format, virtual size)
struct FbVarScreeninfo
{
	UINT32 Xres;         ///< Visible horizontal resolution in pixels
	UINT32 Yres;         ///< Visible vertical resolution in pixels
	UINT32 XresVirtual;  ///< Virtual horizontal resolution
	UINT32 YresVirtual;  ///< Virtual vertical resolution
	UINT32 Xoffset;      ///< Horizontal offset into virtual resolution
	UINT32 Yoffset;      ///< Vertical offset into virtual resolution
	UINT32 BitsPerPixel; ///< Bits per pixel (16, 24, or 32)
	UINT32 Grayscale;    ///< Non-zero for grayscale displays
	FbBitfield Red;      ///< Red component bitfield
	FbBitfield Green;    ///< Green component bitfield
	FbBitfield Blue;     ///< Blue component bitfield
	FbBitfield Transp;   ///< Transparency component bitfield
	UINT32 Nonstd;       ///< Non-standard pixel format flag
	UINT32 Activate;     ///< Activation flag
	UINT32 HeightMm;     ///< Height of picture in mm
	UINT32 WidthMm;      ///< Width of picture in mm
	UINT32 AccelFlags;   ///< Obsolete acceleration flags
	UINT32 Pixclock;     ///< Pixel clock in picoseconds
	UINT32 LeftMargin;   ///< Time from sync to picture (horizontal)
	UINT32 RightMargin;  ///< Time from picture to sync (horizontal)
	UINT32 UpperMargin;  ///< Time from sync to picture (vertical)
	UINT32 LowerMargin;  ///< Time from picture to sync (vertical)
	UINT32 HsyncLen;     ///< Horizontal sync length
	UINT32 VsyncLen;     ///< Vertical sync length
	UINT32 Sync;         ///< Sync type flags
	UINT32 Vmode;        ///< Video mode flags
	UINT32 Rotate;       ///< Rotation angle (0, 90, 180, 270)
	UINT32 Colorspace;   ///< Colorspace for FOURCC-based modes
	UINT32 Reserved[4];  ///< Reserved for future use
};

/// @brief Fixed screen information (memory layout, line length)
struct FbFixScreeninfo
{
	CHAR Id[16];         ///< Identification string (e.g. "VESA VGA")
	USIZE SmemStart;     ///< Start of frame buffer memory (physical address)
	UINT32 SmemLen;      ///< Length of frame buffer memory in bytes
	UINT32 Type;         ///< Frame buffer type
	UINT32 TypeAux;      ///< Interleave for interleaved planes
	UINT32 Visual;       ///< Visual type (truecolor, pseudocolor, etc.)
	UINT16 Xpanstep;     ///< Zero if no hardware panning
	UINT16 Ypanstep;     ///< Zero if no hardware panning
	UINT16 Ywrapstep;    ///< Zero if no hardware ywrap
	UINT32 LineLength;   ///< Length of a line in bytes
	USIZE MmioStart;     ///< Start of memory-mapped I/O (physical address)
	UINT32 MmioLen;      ///< Length of memory-mapped I/O
	UINT32 Accel;        ///< Acceleration capabilities
	UINT16 Capabilities; ///< Feature flags
	UINT16 Reserved[2];  ///< Reserved for future use
};

// =============================================================================
// Internal helpers
// =============================================================================

/// @brief Open a framebuffer device by index (/dev/fb0../dev/fb7)
/// @param index Framebuffer device number (0-7)
/// @return File descriptor on success, negative errno on failure
static SSIZE OpenFramebuffer(UINT32 index)
{
	auto devFb = "/dev/fb"_embed;
	CHAR path[16];
	Memory::Copy(path, (const CHAR *)devFb, 8);
	path[7] = '0' + (CHAR)index;
	path[8] = '\0';

#if (defined(PLATFORM_LINUX) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32))) || \
	(defined(PLATFORM_FREEBSD) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64)))
	return System::Call(SYS_OPENAT, (USIZE)AT_FDCWD, (USIZE)path, (USIZE)O_RDONLY);
#else
	return System::Call(SYS_OPEN, (USIZE)path, (USIZE)O_RDONLY);
#endif
}

/// @brief Perform an ioctl syscall on a file descriptor
/// @param fd File descriptor
/// @param request Ioctl request code
/// @param arg Pointer to request-specific data
/// @return 0 on success, negative errno on failure
static SSIZE Ioctl(SSIZE fd, USIZE request, PVOID arg)
{
	return System::Call(SYS_IOCTL, (USIZE)fd, request, (USIZE)arg);
}

/// @brief Map framebuffer memory for reading
/// @param size Number of bytes to map
/// @param fd Framebuffer file descriptor
/// @return Mapped address, or nullptr on failure
static PVOID MmapFramebuffer(USIZE size, SSIZE fd)
{
	INT32 prot = PROT_READ;
	INT32 flags = MAP_SHARED;

#if defined(PLATFORM_LINUX) && (defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A) || defined(ARCHITECTURE_RISCV32))
	// 32-bit Linux uses mmap2 with page-shifted offset
	SSIZE result = System::Call(SYS_MMAP2, (USIZE)0, size,
		(USIZE)prot, (USIZE)flags, (USIZE)fd, (USIZE)0);
#elif defined(PLATFORM_FREEBSD) && defined(ARCHITECTURE_I386)
	// FreeBSD i386: mmap takes 64-bit off_t split across two 32-bit stack
	// slots. System::Call only pushes one slot, so use inline asm to push
	// all 7 argument slots + dummy return address = 32 bytes.
	SSIZE result;
	register USIZE r1 __asm__("ebx") = 0;           // addr
	register USIZE r2 __asm__("ecx") = size;         // len
	register USIZE r3 __asm__("edx") = (USIZE)prot;  // prot
	register USIZE r4 __asm__("esi") = (USIZE)flags;  // flags
	register USIZE r5 __asm__("edi") = (USIZE)fd;     // fd
	__asm__ volatile(
		"pushl $0\n"          // off_t pos high 32 bits = 0
		"pushl $0\n"          // off_t pos low 32 bits = 0
		"pushl %%edi\n"       // fd
		"pushl %%esi\n"       // flags
		"pushl %%edx\n"       // prot
		"pushl %%ecx\n"       // len
		"pushl %%ebx\n"       // addr
		"pushl $0\n"          // dummy return address
		"int $0x80\n"
		"jnc 1f\n"
		"negl %%eax\n"
		"1:\n"
		"addl $32, %%esp\n"
		: "=a"(result)
		: "a"((USIZE)SYS_MMAP), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5)
		: "memory", "cc"
	);
#else
	SSIZE result = System::Call(SYS_MMAP, (USIZE)0, size,
		(USIZE)prot, (USIZE)flags, (USIZE)fd, (USIZE)0);
#endif

	if (result < 0 && result >= -4095)
		return nullptr;

	return (PVOID)result;
}

/// @brief Extract an N-bit color component from a pixel value
/// @param pixel Raw pixel value
/// @param field Bitfield descriptor for the component
/// @return 8-bit color value
static UINT8 ExtractComponent(UINT32 pixel, const FbBitfield &field)
{
	if (field.Length == 0)
		return 0;

	UINT32 value = (pixel >> field.Offset) & ((1u << field.Length) - 1);

	// Scale to 8-bit
	if (field.Length < 8)
		value = (value * 255) / ((1u << field.Length) - 1);
	else if (field.Length > 8)
		value >>= (field.Length - 8);

	return (UINT8)value;
}

// =============================================================================
// Screen::GetDevices
// =============================================================================

Result<ScreenDeviceList, Error> Screen::GetDevices()
{
	constexpr UINT32 maxDevices = 8;
	ScreenDevice tempDevices[maxDevices];
	UINT32 deviceCount = 0;

	for (UINT32 i = 0; i < maxDevices; i++)
	{
		SSIZE fd = OpenFramebuffer(i);
		if (fd < 0)
			continue;

		FbVarScreeninfo vinfo;
		Memory::Zero(&vinfo, sizeof(vinfo));

		SSIZE ret = Ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
		System::Call(SYS_CLOSE, (USIZE)fd);

		if (ret < 0)
			continue;

		if (vinfo.Xres == 0 || vinfo.Yres == 0)
			continue;

		tempDevices[deviceCount].Left = (INT32)i;  // framebuffer index
		tempDevices[deviceCount].Top = 0;
		tempDevices[deviceCount].Width = vinfo.Xres;
		tempDevices[deviceCount].Height = vinfo.Yres;
		tempDevices[deviceCount].Primary = (deviceCount == 0);
		deviceCount++;
	}

	if (deviceCount == 0)
		return Result<ScreenDeviceList, Error>::Err(Error(Error::Screen_GetDevicesFailed));

	ScreenDevice *devices = new ScreenDevice[deviceCount];
	if (devices == nullptr)
		return Result<ScreenDeviceList, Error>::Err(Error(Error::Screen_AllocFailed));

	Memory::Copy(devices, tempDevices, deviceCount * sizeof(ScreenDevice));

	ScreenDeviceList list;
	list.Devices = devices;
	list.Count = deviceCount;
	return Result<ScreenDeviceList, Error>::Ok(list);
}

// =============================================================================
// Screen::Capture
// =============================================================================

Result<void, Error> Screen::Capture(const ScreenDevice &device, Span<RGB> buffer)
{
	// Use Left field as framebuffer device index
	UINT32 fbIndex = (UINT32)device.Left;
	if (fbIndex > 7)
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));

	SSIZE fd = OpenFramebuffer(fbIndex);
	if (fd < 0)
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));

	// Query screen parameters
	FbVarScreeninfo vinfo;
	Memory::Zero(&vinfo, sizeof(vinfo));

	FbFixScreeninfo finfo;
	Memory::Zero(&finfo, sizeof(finfo));

	if (Ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0 ||
		Ioctl(fd, FBIOGET_FSCREENINFO, &finfo) < 0)
	{
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	// Validate dimensions match the device
	if (vinfo.Xres != device.Width || vinfo.Yres != device.Height)
	{
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	UINT32 bytesPerPixel = vinfo.BitsPerPixel / 8;
	if (bytesPerPixel == 0 || bytesPerPixel > 4)
	{
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	// Map the framebuffer memory
	PVOID mapped = MmapFramebuffer((USIZE)finfo.SmemLen, fd);
	System::Call(SYS_CLOSE, (USIZE)fd);

	if (mapped == nullptr)
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));

	// Calculate pointer to the visible area
	UINT8 *fbBase = (UINT8 *)mapped;
	USIZE visibleOffset = (USIZE)vinfo.Yoffset * finfo.LineLength +
		(USIZE)vinfo.Xoffset * bytesPerPixel;

	PRGB rgbBuf = buffer.Data();
	UINT32 width = device.Width;
	UINT32 height = device.Height;

	// Convert framebuffer pixels to RGB
	for (UINT32 y = 0; y < height; y++)
	{
		UINT8 *row = fbBase + visibleOffset + (USIZE)y * finfo.LineLength;

		for (UINT32 x = 0; x < width; x++)
		{
			UINT32 pixel = 0;
			UINT8 *src = row + (USIZE)x * bytesPerPixel;

			// Read pixel bytes (little-endian)
			for (UINT32 b = 0; b < bytesPerPixel; b++)
				pixel |= (UINT32)src[b] << (b * 8);

			rgbBuf[y * width + x].Red = ExtractComponent(pixel, vinfo.Red);
			rgbBuf[y * width + x].Green = ExtractComponent(pixel, vinfo.Green);
			rgbBuf[y * width + x].Blue = ExtractComponent(pixel, vinfo.Blue);
		}
	}

	// Unmap framebuffer
	System::Call(SYS_MUNMAP, (USIZE)mapped, (USIZE)finfo.SmemLen);

	return Result<void, Error>::Ok();
}
