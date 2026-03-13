/**
 * @file screen.cc
 * @brief POSIX Screen Implementation (Linux/Android/FreeBSD/Solaris)
 *
 * @details Implements screen device enumeration and capture via two backends:
 *
 * 1. DRM dumb buffers (/dev/dri/card0..card7): The preferred backend on
 *    modern Linux, Android, and FreeBSD. Enumerates connectors, encoders,
 *    and CRTCs via DRM mode-setting ioctls. Capture maps the active
 *    framebuffer via DRM_IOCTL_MODE_MAP_DUMB. Requires DRM master or
 *    CAP_SYS_ADMIN for framebuffer mapping. DRM devices are encoded in
 *    ScreenDevice as Left = -(cardIndex + 1), Top = crtcId.
 *
 * 2. Linux framebuffer (/dev/fb0..fb7): Legacy fallback when DRM is
 *    unavailable. Uses FBIOGET_VSCREENINFO and FBIOGET_FSCREENINFO ioctls
 *    with mmap to read pixel data. Shared between Linux, Android, and
 *    FreeBSD (via linuxkpi compatibility). On Android, /dev/graphics/fb0..fb7
 *    is tried when /dev/fb* is unavailable. ScreenDevice::Left stores the
 *    framebuffer index.
 *
 * GetDevices() tries DRM first; if no displays are found, falls back to
 * framebuffer. Capture() dispatches based on the Left field encoding.
 *
 * Solaris uses the SunOS framebuffer API (sys/fbio.h) with FBIOGTYPE
 * ioctl to query /dev/fb device parameters and mmap to read pixel data.
 * Only a single console framebuffer is supported.
 *
 * macOS and iOS have separate implementations (macos/screen.cc, ios/screen.cc).
 *
 * @see DRM KMS userspace API
 *      https://www.kernel.org/doc/html/latest/gpu/drm-uapi.html
 * @see Linux framebuffer API (linux/fb.h)
 *      https://www.kernel.org/doc/html/latest/fb/api.html
 * @see Solaris fbio(4I)
 *      https://docs.oracle.com/cd/E36784_01/html/E36884/fbio-7i.html
 */

#include "platform/screen/screen.h"
#include "core/memory/memory.h"

#if defined(PLATFORM_LINUX)
#include "platform/kernel/linux/syscall.h"
#include "platform/kernel/linux/system.h"
#elif defined(PLATFORM_ANDROID)
#include "platform/kernel/android/syscall.h"
#include "platform/kernel/android/system.h"
#elif defined(PLATFORM_FREEBSD)
#include "platform/kernel/freebsd/syscall.h"
#include "platform/kernel/freebsd/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/kernel/solaris/syscall.h"
#include "platform/kernel/solaris/system.h"
#endif

#if defined(PLATFORM_SOLARIS)

// =============================================================================
// Solaris framebuffer — FBIOGTYPE ioctl + mmap (/dev/fb)
// =============================================================================

/// @brief SunOS framebuffer type information (sys/fbio.h)
/// @see fbio(4I)
///      https://docs.oracle.com/cd/E36784_01/html/E36884/fbio-7i.html
struct FbType
{
	INT32 Type;	  ///< Frame buffer type (FBTYPE_* constant)
	INT32 Height; ///< Height in pixels
	INT32 Width;  ///< Width in pixels
	INT32 Depth;  ///< Bits per pixel
	INT32 CmSize; ///< Size of color map (entries)
	INT32 Size;	  ///< Total framebuffer memory in bytes
};

/// @brief Get framebuffer type information
/// @details FBIOGTYPE = (FIOC | 0) where FIOC = ('F' << 8)
constexpr USIZE FBIOGTYPE = 0x4600;

/// @brief Open the Solaris console framebuffer device (/dev/fb)
/// @return File descriptor on success, negative errno on failure
static SSIZE OpenFramebuffer()
{
	auto devFb = "/dev/fb";
	CHAR path[8];
	Memory::Copy(path, (const CHAR *)devFb, 8);

	return System::Call(SYS_OPENAT, (USIZE)AT_FDCWD, (USIZE)path, (USIZE)O_RDONLY);
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

#if defined(ARCHITECTURE_I386)
	// Solaris i386: mmap takes 64-bit off_t split across two 32-bit stack
	// slots. The 6-arg System::Call only pushes one slot for the offset.
	// Use inline asm to push all 7 argument slots + dummy return address.
	SSIZE result;
	register USIZE r1 __asm__("ebx") = 0;			 // addr
	register USIZE r2 __asm__("ecx") = size;		 // len
	register USIZE r3 __asm__("edx") = (USIZE)prot;	 // prot
	register USIZE r4 __asm__("esi") = (USIZE)flags; // flags
	register USIZE r5 __asm__("edi") = (USIZE)fd;	 // fd
	__asm__ volatile(
		"pushl $0\n"	// off_t pos high 32 bits = 0
		"pushl $0\n"	// off_t pos low 32 bits = 0
		"pushl %%edi\n" // fd
		"pushl %%esi\n" // flags
		"pushl %%edx\n" // prot
		"pushl %%ecx\n" // len
		"pushl %%ebx\n" // addr
		"pushl $0\n"	// dummy return address
		"int $0x91\n"
		"jnc 1f\n"
		"negl %%eax\n"
		"1:\n"
		"addl $32, %%esp\n"
		: "=a"(result)
		: "a"((USIZE)SYS_MMAP), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5)
		: "memory", "cc");
#else
	SSIZE result = System::Call(SYS_MMAP, (USIZE)0, size,
								(USIZE)prot, (USIZE)flags, (USIZE)fd, (USIZE)0);
#endif

	if (result < 0 && result >= -4095)
		return nullptr;

	return (PVOID)result;
}

// =============================================================================
// Screen::GetDevices (Solaris)
// =============================================================================

Result<ScreenDeviceList, Error> Screen::GetDevices()
{
	SSIZE fd = OpenFramebuffer();
	if (fd < 0)
		return Result<ScreenDeviceList, Error>::Err(Error(Error::Screen_GetDevicesFailed));

	FbType fbt;
	Memory::Zero(&fbt, sizeof(fbt));

	SSIZE ret = Ioctl(fd, FBIOGTYPE, &fbt);
	System::Call(SYS_CLOSE, (USIZE)fd);

	if (ret < 0)
		return Result<ScreenDeviceList, Error>::Err(Error(Error::Screen_GetDevicesFailed));

	if (fbt.Width <= 0 || fbt.Height <= 0)
		return Result<ScreenDeviceList, Error>::Err(Error(Error::Screen_GetDevicesFailed));

	ScreenDevice *devices = new ScreenDevice[1];
	if (devices == nullptr)
		return Result<ScreenDeviceList, Error>::Err(Error(Error::Screen_AllocFailed));

	devices[0].Left = 0;
	devices[0].Top = 0;
	devices[0].Width = (UINT32)fbt.Width;
	devices[0].Height = (UINT32)fbt.Height;
	devices[0].Primary = true;

	ScreenDeviceList list;
	list.Devices = devices;
	list.Count = 1;
	return Result<ScreenDeviceList, Error>::Ok(list);
}

// =============================================================================
// Screen::Capture (Solaris)
// =============================================================================

Result<void, Error> Screen::Capture(const ScreenDevice &device, Span<RGB> buffer)
{
	SSIZE fd = OpenFramebuffer();
	if (fd < 0)
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));

	FbType fbt;
	Memory::Zero(&fbt, sizeof(fbt));

	SSIZE ret = Ioctl(fd, FBIOGTYPE, &fbt);
	if (ret < 0 || fbt.Width <= 0 || fbt.Height <= 0)
	{
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	// Validate dimensions match the device
	if ((UINT32)fbt.Width != device.Width || (UINT32)fbt.Height != device.Height)
	{
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	UINT32 bytesPerPixel = (UINT32)fbt.Depth / 8;
	if (bytesPerPixel == 0 || bytesPerPixel > 4)
	{
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	// Use fb_size from FBIOGTYPE, fall back to computed size
	USIZE mapSize = (fbt.Size > 0) ? (USIZE)fbt.Size
								   : (USIZE)fbt.Width * (USIZE)fbt.Height * bytesPerPixel;

	PVOID mapped = MmapFramebuffer(mapSize, fd);
	System::Call(SYS_CLOSE, (USIZE)fd);

	if (mapped == nullptr)
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));

	// Convert framebuffer pixels to RGB
	// FBIOGTYPE does not report per-component bitfield offsets or line stride,
	// so assume standard little-endian layouts for common depths.
	UINT8 *fbBase = (UINT8 *)mapped;
	PRGB rgbBuf = buffer.Data();
	UINT32 width = device.Width;
	UINT32 height = device.Height;
	USIZE lineLength = (USIZE)width * bytesPerPixel;

	for (UINT32 y = 0; y < height; y++)
	{
		UINT8 *row = fbBase + (USIZE)y * lineLength;

		for (UINT32 x = 0; x < width; x++)
		{
			UINT8 *src = row + (USIZE)x * bytesPerPixel;

			if (bytesPerPixel == 4)
			{
				// 32bpp BGRA (standard x86/aarch64 framebuffer layout)
				rgbBuf[y * width + x].Red = src[2];
				rgbBuf[y * width + x].Green = src[1];
				rgbBuf[y * width + x].Blue = src[0];
			}
			else if (bytesPerPixel == 3)
			{
				// 24bpp BGR
				rgbBuf[y * width + x].Red = src[2];
				rgbBuf[y * width + x].Green = src[1];
				rgbBuf[y * width + x].Blue = src[0];
			}
			else if (bytesPerPixel == 2)
			{
				// 16bpp RGB565
				UINT16 pixel = (UINT16)src[0] | ((UINT16)src[1] << 8);
				rgbBuf[y * width + x].Red = (UINT8)(((pixel >> 11) & 0x1F) * 255 / 31);
				rgbBuf[y * width + x].Green = (UINT8)(((pixel >> 5) & 0x3F) * 255 / 63);
				rgbBuf[y * width + x].Blue = (UINT8)((pixel & 0x1F) * 255 / 31);
			}
		}
	}

	System::Call(SYS_MUNMAP, (USIZE)mapped, mapSize);

	return Result<void, Error>::Ok();
}

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID) || defined(PLATFORM_FREEBSD)

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
	UINT32 Offset;	 ///< Bit position of the least significant bit
	UINT32 Length;	 ///< Number of bits in this component
	UINT32 MsbRight; ///< MSB is rightmost (!=0) or leftmost (==0)
};

/// @brief Variable screen information (resolution, pixel format, virtual size)
struct FbVarScreeninfo
{
	UINT32 Xres;		 ///< Visible horizontal resolution in pixels
	UINT32 Yres;		 ///< Visible vertical resolution in pixels
	UINT32 XresVirtual;	 ///< Virtual horizontal resolution
	UINT32 YresVirtual;	 ///< Virtual vertical resolution
	UINT32 Xoffset;		 ///< Horizontal offset into virtual resolution
	UINT32 Yoffset;		 ///< Vertical offset into virtual resolution
	UINT32 BitsPerPixel; ///< Bits per pixel (16, 24, or 32)
	UINT32 Grayscale;	 ///< Non-zero for grayscale displays
	FbBitfield Red;		 ///< Red component bitfield
	FbBitfield Green;	 ///< Green component bitfield
	FbBitfield Blue;	 ///< Blue component bitfield
	FbBitfield Transp;	 ///< Transparency component bitfield
	UINT32 Nonstd;		 ///< Non-standard pixel format flag
	UINT32 Activate;	 ///< Activation flag
	UINT32 HeightMm;	 ///< Height of picture in mm
	UINT32 WidthMm;		 ///< Width of picture in mm
	UINT32 AccelFlags;	 ///< Obsolete acceleration flags
	UINT32 Pixclock;	 ///< Pixel clock in picoseconds
	UINT32 LeftMargin;	 ///< Time from sync to picture (horizontal)
	UINT32 RightMargin;	 ///< Time from picture to sync (horizontal)
	UINT32 UpperMargin;	 ///< Time from sync to picture (vertical)
	UINT32 LowerMargin;	 ///< Time from picture to sync (vertical)
	UINT32 HsyncLen;	 ///< Horizontal sync length
	UINT32 VsyncLen;	 ///< Vertical sync length
	UINT32 Sync;		 ///< Sync type flags
	UINT32 Vmode;		 ///< Video mode flags
	UINT32 Rotate;		 ///< Rotation angle (0, 90, 180, 270)
	UINT32 Colorspace;	 ///< Colorspace for FOURCC-based modes
	UINT32 Reserved[4];	 ///< Reserved for future use
};

/// @brief Fixed screen information (memory layout, line length)
struct FbFixScreeninfo
{
	CHAR Id[16];		 ///< Identification string (e.g. "VESA VGA")
	USIZE SmemStart;	 ///< Start of frame buffer memory (physical address)
	UINT32 SmemLen;		 ///< Length of frame buffer memory in bytes
	UINT32 Type;		 ///< Frame buffer type
	UINT32 TypeAux;		 ///< Interleave for interleaved planes
	UINT32 Visual;		 ///< Visual type (truecolor, pseudocolor, etc.)
	UINT16 Xpanstep;	 ///< Zero if no hardware panning
	UINT16 Ypanstep;	 ///< Zero if no hardware panning
	UINT16 Ywrapstep;	 ///< Zero if no hardware ywrap
	UINT32 LineLength;	 ///< Length of a line in bytes
	USIZE MmioStart;	 ///< Start of memory-mapped I/O (physical address)
	UINT32 MmioLen;		 ///< Length of memory-mapped I/O
	UINT32 Accel;		 ///< Acceleration capabilities
	UINT16 Capabilities; ///< Feature flags
	UINT16 Reserved[2];	 ///< Reserved for future use
};

// =============================================================================
// DRM ioctl constants and structures (/dev/dri/card*)
// =============================================================================

/// @brief DRM mode information (matches kernel struct drm_mode_modeinfo, 68 bytes)
/// @see https://www.kernel.org/doc/html/latest/gpu/drm-uapi.html
struct DrmModeModeinfo
{
	UINT32 Clock;
	UINT16 Hdisplay;
	UINT16 HsyncStart;
	UINT16 HsyncEnd;
	UINT16 Htotal;
	UINT16 Hskew;
	UINT16 Vdisplay;
	UINT16 VsyncStart;
	UINT16 VsyncEnd;
	UINT16 Vtotal;
	UINT16 Vscan;
	UINT32 Vrefresh;
	UINT32 Flags;
	UINT32 Type;
	CHAR Name[32];
};

/// @brief DRM mode card resources (drm_mode_card_res, 64 bytes)
struct DrmModeCardRes
{
	UINT64 FbIdPtr;
	UINT64 CrtcIdPtr;
	UINT64 ConnectorIdPtr;
	UINT64 EncoderIdPtr;
	UINT32 CountFbs;
	UINT32 CountCrtcs;
	UINT32 CountConnectors;
	UINT32 CountEncoders;
	UINT32 MinWidth;
	UINT32 MaxWidth;
	UINT32 MinHeight;
	UINT32 MaxHeight;
};

/// @brief DRM connector information (drm_mode_get_connector, 80 bytes)
struct DrmModeGetConnector
{
	UINT64 EncodersPtr;
	UINT64 ModesPtr;
	UINT64 PropsPtr;
	UINT64 PropValuesPtr;
	UINT32 CountModes;
	UINT32 CountProps;
	UINT32 CountEncoders;
	UINT32 EncoderId;
	UINT32 ConnectorId;
	UINT32 ConnectorType;
	UINT32 ConnectorTypeId;
	UINT32 Connection;
	UINT32 MmWidth;
	UINT32 MmHeight;
	UINT32 Subpixel;
	UINT32 Pad;
};

/// @brief DRM encoder information (drm_mode_get_encoder, 20 bytes)
struct DrmModeGetEncoder
{
	UINT32 EncoderId;
	UINT32 EncoderType;
	UINT32 CrtcId;
	UINT32 PossibleCrtcs;
	UINT32 PossibleClones;
};

/// @brief DRM CRTC information (drm_mode_crtc, 104 bytes)
struct DrmModeCrtc
{
	UINT64 SetConnectorsPtr;
	UINT32 CountConnectors;
	UINT32 CrtcId;
	UINT32 FbId;
	UINT32 X;
	UINT32 Y;
	UINT32 GammaSize;
	UINT32 ModeValid;
	DrmModeModeinfo Mode;
};

/// @brief DRM framebuffer command (drm_mode_fb_cmd, 28 bytes)
struct DrmModeFbCmd
{
	UINT32 FbId;
	UINT32 Width;
	UINT32 Height;
	UINT32 Pitch;
	UINT32 Bpp;
	UINT32 Depth;
	UINT32 Handle;
};

/// @brief DRM map dumb buffer request (drm_mode_map_dumb, 16 bytes)
struct DrmModeMapDumb
{
	UINT32 Handle;
	UINT32 Pad;
	UINT64 Offset;
};

/// @brief DRM GEM close request (drm_gem_close, 8 bytes)
struct DrmGemClose
{
	UINT32 Handle;
	UINT32 Pad;
};

/// @brief DRM connector is attached and active
constexpr UINT32 DRM_MODE_CONNECTED = 1;

/// DRM ioctl numbers: _IOWR('d', nr, struct) = (3<<30)|(sizeof<<16)|(0x64<<8)|nr
/// Note: _IOWR encoding is identical on standard and MIPS (3<<30 == 6<<29 == 0xC0000000)
constexpr USIZE DRM_IOCTL_MODE_GETRESOURCES = 0xC04064A0;
constexpr USIZE DRM_IOCTL_MODE_GETCRTC      = 0xC06864A1;
constexpr USIZE DRM_IOCTL_MODE_GETENCODER   = 0xC01464A6;
constexpr USIZE DRM_IOCTL_MODE_GETCONNECTOR = 0xC05064A7;
constexpr USIZE DRM_IOCTL_MODE_GETFB        = 0xC01C64AD;
constexpr USIZE DRM_IOCTL_MODE_MAP_DUMB     = 0xC01064B3;

/// DRM GEM close: _IOW('d', 0x09, drm_gem_close)
/// MIPS _IOW direction = 4 at bit 29; standard _IOW direction = 1 at bit 30
#if defined(ARCHITECTURE_MIPS64)
constexpr USIZE DRM_IOCTL_GEM_CLOSE         = 0x80086409;
#else
constexpr USIZE DRM_IOCTL_GEM_CLOSE         = 0x40086409;
#endif

// =============================================================================
// Internal helpers — shared
// =============================================================================

/// @brief Open a framebuffer device by index
/// @details Tries /dev/fb<N> first (Linux/FreeBSD). On Android, falls back to
/// /dev/graphics/fb<N> which is the standard Android framebuffer path.
/// @param index Framebuffer device number (0-7)
/// @return File descriptor on success, negative errno on failure
static SSIZE OpenFramebuffer(UINT32 index)
{
	auto devFb = "/dev/fb";
	CHAR path[24];
	Memory::Copy(path, (const CHAR *)devFb, 8);
	path[7] = '0' + (CHAR)index;
	path[8] = '\0';

#if ((defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32))) || \
	(defined(PLATFORM_FREEBSD) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64)))
	SSIZE fd = System::Call(SYS_OPENAT, (USIZE)AT_FDCWD, (USIZE)path, (USIZE)O_RDONLY);
#else
	SSIZE fd = System::Call(SYS_OPEN, (USIZE)path, (USIZE)O_RDONLY);
#endif

#if defined(PLATFORM_ANDROID)
	// Android framebuffer lives at /dev/graphics/fb<N> instead of /dev/fb<N>
	if (fd < 0)
	{
		auto devGfxFb = "/dev/graphics/fb";
		Memory::Copy(path, (const CHAR *)devGfxFb, 17);
		path[16] = '0' + (CHAR)index;
		path[17] = '\0';

#if defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32)
		fd = System::Call(SYS_OPENAT, (USIZE)AT_FDCWD, (USIZE)path, (USIZE)O_RDONLY);
#else
		fd = System::Call(SYS_OPEN, (USIZE)path, (USIZE)O_RDONLY);
#endif
	}
#endif

	return fd;
}

/// @brief Open a DRM card device by index (/dev/dri/card0../dev/dri/card7)
/// @param index DRM card number (0-7)
/// @return File descriptor on success, negative errno on failure
static SSIZE OpenDrmCard(UINT32 index)
{
	auto devDri = "/dev/dri/card";
	CHAR path[16];
	Memory::Copy(path, (const CHAR *)devDri, 14);
	path[13] = '0' + (CHAR)index;
	path[14] = '\0';

#if ((defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32))) || \
	(defined(PLATFORM_FREEBSD) && (defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64)))
	return System::Call(SYS_OPENAT, (USIZE)AT_FDCWD, (USIZE)path, (USIZE)O_RDWR);
#else
	return System::Call(SYS_OPEN, (USIZE)path, (USIZE)O_RDWR);
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

#if (defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)) && (defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A) || defined(ARCHITECTURE_RISCV32))
	// 32-bit Linux/Android uses mmap2 with page-shifted offset
	SSIZE result = System::Call(SYS_MMAP2, (USIZE)0, size,
								(USIZE)prot, (USIZE)flags, (USIZE)fd, (USIZE)0);
#elif defined(PLATFORM_FREEBSD) && defined(ARCHITECTURE_I386)
	// FreeBSD i386: mmap takes 64-bit off_t split across two 32-bit stack
	// slots. System::Call only pushes one slot, so use inline asm to push
	// all 7 argument slots + dummy return address = 32 bytes.
	SSIZE result;
	register USIZE r1 __asm__("ebx") = 0;			 // addr
	register USIZE r2 __asm__("ecx") = size;		 // len
	register USIZE r3 __asm__("edx") = (USIZE)prot;	 // prot
	register USIZE r4 __asm__("esi") = (USIZE)flags; // flags
	register USIZE r5 __asm__("edi") = (USIZE)fd;	 // fd
	__asm__ volatile(
		"pushl $0\n"	// off_t pos high 32 bits = 0
		"pushl $0\n"	// off_t pos low 32 bits = 0
		"pushl %%edi\n" // fd
		"pushl %%esi\n" // flags
		"pushl %%edx\n" // prot
		"pushl %%ecx\n" // len
		"pushl %%ebx\n" // addr
		"pushl $0\n"	// dummy return address
		"int $0x80\n"
		"jnc 1f\n"
		"negl %%eax\n"
		"1:\n"
		"addl $32, %%esp\n"
		: "=a"(result)
		: "a"((USIZE)SYS_MMAP), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5)
		: "memory", "cc");
#else
	SSIZE result = System::Call(SYS_MMAP, (USIZE)0, size,
								(USIZE)prot, (USIZE)flags, (USIZE)fd, (USIZE)0);
#endif

	if (result < 0 && result >= -4095)
		return nullptr;

	return (PVOID)result;
}

/// @brief Map DRM dumb buffer memory for reading
/// @param size Number of bytes to map
/// @param fd DRM device file descriptor
/// @param offset Offset from DRM_IOCTL_MODE_MAP_DUMB
/// @return Mapped address, or nullptr on failure
static PVOID DrmMmapBuffer(USIZE size, SSIZE fd, UINT64 offset)
{
	INT32 prot = PROT_READ;
	INT32 flags = MAP_SHARED;

#if (defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)) && (defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A) || defined(ARCHITECTURE_RISCV32))
	// 32-bit Linux/Android uses mmap2 with page-shifted offset
	SSIZE result = System::Call(SYS_MMAP2, (USIZE)0, size,
		(USIZE)prot, (USIZE)flags, (USIZE)fd, (USIZE)(UINT32)(offset >> 12));
#elif defined(PLATFORM_FREEBSD) && defined(ARCHITECTURE_I386)
	// FreeBSD i386: mmap takes 64-bit off_t split across two 32-bit stack
	// slots. Use inline asm to push all 7 argument slots + dummy return address.
	SSIZE result;
	UINT32 offLo = (UINT32)offset;
	UINT32 offHi = (UINT32)(offset >> 32);
	register USIZE r1 __asm__("ebx") = 0;              // addr
	register USIZE r2 __asm__("ecx") = size;            // len
	register USIZE r3 __asm__("edx") = (USIZE)prot;    // prot
	register USIZE r4 __asm__("esi") = (USIZE)flags;   // flags
	register USIZE r5 __asm__("edi") = (USIZE)fd;      // fd
	__asm__ volatile(
		"movl %[offLo], %%eax\n"  // load offLo before any push
		"pushl %[offHi]\n"        // off_t high (first push, ESP unchanged when read)
		"pushl %%eax\n"           // off_t low (from register)
		"pushl %%edi\n"           // fd
		"pushl %%esi\n"           // flags
		"pushl %%edx\n"           // prot
		"pushl %%ecx\n"           // len
		"pushl %%ebx\n"           // addr
		"pushl $0\n"              // dummy return address
		"movl %[sysno], %%eax\n"  // reload syscall number
		"int $0x80\n"
		"jnc 1f\n"
		"negl %%eax\n"
		"1:\n"
		"addl $32, %%esp\n"
		: "=&a"(result)
		: "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5),
		  [offLo] "g"(offLo), [offHi] "g"(offHi), [sysno] "i"((int)SYS_MMAP)
		: "memory", "cc"
	);
#else
	SSIZE result = System::Call(SYS_MMAP, (USIZE)0, size,
		(USIZE)prot, (USIZE)flags, (USIZE)fd, (USIZE)offset);
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
// Internal helpers — DRM enumeration and capture
// =============================================================================

/// @brief Enumerate displays via DRM (/dev/dri/card*)
/// @details Walks card devices, connectors, encoders, and CRTCs to find
/// active displays. DRM devices are encoded in ScreenDevice as:
/// Left = -(cardIndex + 1), Top = crtcId.
/// @param tempDevices Output array for discovered devices
/// @param deviceCount [in/out] Current device count, incremented per device
/// @param maxDevices Maximum capacity of tempDevices
static VOID DrmGetDevices(ScreenDevice *tempDevices, UINT32 &deviceCount, UINT32 maxDevices)
{
	constexpr UINT32 maxCards = 8;
	constexpr UINT32 maxConnectors = 16;

	for (UINT32 cardIdx = 0; cardIdx < maxCards && deviceCount < maxDevices; cardIdx++)
	{
		SSIZE fd = OpenDrmCard(cardIdx);
		if (fd < 0)
			continue;

		// First call: get connector count
		DrmModeCardRes res;
		Memory::Zero(&res, sizeof(res));

		if (Ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0 ||
			res.CountConnectors == 0)
		{
			System::Call(SYS_CLOSE, (USIZE)fd);
			continue;
		}

		// Second call: retrieve connector IDs
		UINT32 connCount = res.CountConnectors;
		if (connCount > maxConnectors)
			connCount = maxConnectors;

		UINT32 connectorIds[maxConnectors];
		Memory::Zero(connectorIds, sizeof(connectorIds));

		DrmModeCardRes res2;
		Memory::Zero(&res2, sizeof(res2));
		res2.ConnectorIdPtr = (UINT64)(USIZE)connectorIds;
		res2.CountConnectors = connCount;

		if (Ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res2) < 0)
		{
			System::Call(SYS_CLOSE, (USIZE)fd);
			continue;
		}

		// Walk connectors to find active displays
		for (UINT32 c = 0; c < connCount && deviceCount < maxDevices; c++)
		{
			DrmModeGetConnector conn;
			Memory::Zero(&conn, sizeof(conn));
			conn.ConnectorId = connectorIds[c];

			if (Ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0)
				continue;

			if (conn.Connection != DRM_MODE_CONNECTED || conn.EncoderId == 0)
				continue;

			// Get encoder to find the CRTC
			DrmModeGetEncoder enc;
			Memory::Zero(&enc, sizeof(enc));
			enc.EncoderId = conn.EncoderId;

			if (Ioctl(fd, DRM_IOCTL_MODE_GETENCODER, &enc) < 0 || enc.CrtcId == 0)
				continue;

			// Get CRTC to read the active mode
			DrmModeCrtc crtc;
			Memory::Zero(&crtc, sizeof(crtc));
			crtc.CrtcId = enc.CrtcId;

			if (Ioctl(fd, DRM_IOCTL_MODE_GETCRTC, &crtc) < 0)
				continue;

			if (!crtc.ModeValid || crtc.Mode.Hdisplay == 0 || crtc.Mode.Vdisplay == 0)
				continue;

			tempDevices[deviceCount].Left = -(INT32)(cardIdx + 1);
			tempDevices[deviceCount].Top = (INT32)crtc.CrtcId;
			tempDevices[deviceCount].Width = (UINT32)crtc.Mode.Hdisplay;
			tempDevices[deviceCount].Height = (UINT32)crtc.Mode.Vdisplay;
			tempDevices[deviceCount].Primary = (deviceCount == 0);
			deviceCount++;
		}

		System::Call(SYS_CLOSE, (USIZE)fd);
	}
}

/// @brief Capture screen via DRM dumb buffer mapping
/// @details Opens the DRM card, queries the CRTC framebuffer, maps it via
/// DRM_IOCTL_MODE_MAP_DUMB, and converts pixels to RGB.
/// @param device Display device with Left = -(cardIndex+1), Top = crtcId
/// @param buffer Output RGB pixel buffer
/// @return Ok on success, Err on failure
static Result<void, Error> DrmCapture(const ScreenDevice &device, Span<RGB> buffer)
{
	UINT32 cardIdx = (UINT32)(-(device.Left + 1));
	UINT32 crtcId = (UINT32)device.Top;

	SSIZE fd = OpenDrmCard(cardIdx);
	if (fd < 0)
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));

	// Get CRTC to find the active framebuffer
	DrmModeCrtc crtc;
	Memory::Zero(&crtc, sizeof(crtc));
	crtc.CrtcId = crtcId;

	if (Ioctl(fd, DRM_IOCTL_MODE_GETCRTC, &crtc) < 0 || crtc.FbId == 0)
	{
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	// Get framebuffer info (returns GEM handle)
	DrmModeFbCmd fb;
	Memory::Zero(&fb, sizeof(fb));
	fb.FbId = crtc.FbId;

	if (Ioctl(fd, DRM_IOCTL_MODE_GETFB, &fb) < 0 || fb.Handle == 0)
	{
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	// Validate dimensions
	if (fb.Width != device.Width || fb.Height != device.Height)
	{
		DrmGemClose gc;
		gc.Handle = fb.Handle;
		gc.Pad = 0;
		Ioctl(fd, DRM_IOCTL_GEM_CLOSE, &gc);
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	// Map the dumb buffer
	DrmModeMapDumb mapReq;
	Memory::Zero(&mapReq, sizeof(mapReq));
	mapReq.Handle = fb.Handle;

	if (Ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mapReq) < 0)
	{
		DrmGemClose gc;
		gc.Handle = fb.Handle;
		gc.Pad = 0;
		Ioctl(fd, DRM_IOCTL_GEM_CLOSE, &gc);
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	USIZE mapSize = (USIZE)fb.Pitch * (USIZE)fb.Height;
	PVOID mapped = DrmMmapBuffer(mapSize, fd, mapReq.Offset);

	if (mapped == nullptr)
	{
		DrmGemClose gc;
		gc.Handle = fb.Handle;
		gc.Pad = 0;
		Ioctl(fd, DRM_IOCTL_GEM_CLOSE, &gc);
		System::Call(SYS_CLOSE, (USIZE)fd);
		return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
	}

	// Convert framebuffer pixels to RGB
	UINT8 *fbBase = (UINT8 *)mapped;
	PRGB rgbBuf = buffer.Data();
	UINT32 width = fb.Width;
	UINT32 height = fb.Height;
	UINT32 bytesPerPixel = fb.Bpp / 8;

	for (UINT32 y = 0; y < height; y++)
	{
		UINT8 *row = fbBase + (USIZE)y * fb.Pitch;

		for (UINT32 x = 0; x < width; x++)
		{
			UINT8 *src = row + (USIZE)x * bytesPerPixel;

			if (bytesPerPixel == 4)
			{
				// 32bpp XRGB8888: bytes are B, G, R, X (little-endian)
				rgbBuf[y * width + x].Red = src[2];
				rgbBuf[y * width + x].Green = src[1];
				rgbBuf[y * width + x].Blue = src[0];
			}
			else if (bytesPerPixel == 3)
			{
				// 24bpp BGR888
				rgbBuf[y * width + x].Red = src[2];
				rgbBuf[y * width + x].Green = src[1];
				rgbBuf[y * width + x].Blue = src[0];
			}
			else if (bytesPerPixel == 2)
			{
				// 16bpp RGB565
				UINT16 pixel = (UINT16)src[0] | ((UINT16)src[1] << 8);
				rgbBuf[y * width + x].Red = (UINT8)(((pixel >> 11) & 0x1F) * 255 / 31);
				rgbBuf[y * width + x].Green = (UINT8)(((pixel >> 5) & 0x3F) * 255 / 63);
				rgbBuf[y * width + x].Blue = (UINT8)((pixel & 0x1F) * 255 / 31);
			}
		}
	}

	// Cleanup: unmap, close GEM handle, close fd
	System::Call(SYS_MUNMAP, (USIZE)mapped, mapSize);

	DrmGemClose gc;
	gc.Handle = fb.Handle;
	gc.Pad = 0;
	Ioctl(fd, DRM_IOCTL_GEM_CLOSE, &gc);
	System::Call(SYS_CLOSE, (USIZE)fd);

	return Result<void, Error>::Ok();
}

// =============================================================================
// Screen::GetDevices (DRM first, framebuffer fallback)
// =============================================================================

Result<ScreenDeviceList, Error> Screen::GetDevices()
{
	constexpr UINT32 maxDevices = 8;
	ScreenDevice tempDevices[maxDevices];
	UINT32 deviceCount = 0;

	// Try DRM first (/dev/dri/card*)
	DrmGetDevices(tempDevices, deviceCount, maxDevices);

	// Fall back to framebuffer (/dev/fb*)
	if (deviceCount == 0)
	{
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
// Internal helper — framebuffer capture by device index
// =============================================================================

/// @brief Capture screen via Linux framebuffer (/dev/fb<N>)
/// @param fbIndex Framebuffer device number (0-7)
/// @param device Display device descriptor (for resolution validation)
/// @param buffer Output RGB pixel buffer
/// @return Ok on success, Err on failure
static Result<void, Error> FbCapture(UINT32 fbIndex, const ScreenDevice &device, Span<RGB> buffer)
{
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

/// @brief Try framebuffer capture across all /dev/fb devices matching resolution
/// @param device Display device descriptor (for resolution matching)
/// @param buffer Output RGB pixel buffer
/// @return Ok on success, Err if no matching framebuffer found
static Result<void, Error> FbCaptureFallback(const ScreenDevice &device, Span<RGB> buffer)
{
	for (UINT32 i = 0; i < 8; i++)
	{
		auto result = FbCapture(i, device, buffer);
		if (result.IsOk())
			return result;
	}
	return Result<void, Error>::Err(Error(Error::Screen_CaptureFailed));
}

// =============================================================================
// Screen::Capture (DRM or framebuffer dispatch)
// =============================================================================

Result<void, Error> Screen::Capture(const ScreenDevice &device, Span<RGB> buffer)
{
	// DRM device: Left < 0 encodes -(cardIndex + 1)
	if (device.Left < 0)
	{
		auto result = DrmCapture(device, buffer);
		if (result.IsOk())
			return result;

		// DRM capture failed (e.g., no DRM master on Linux 5.1+) — try framebuffer
		return FbCaptureFallback(device, buffer);
	}

	// Framebuffer device: Left encodes /dev/fb index
	return FbCapture((UINT32)device.Left, device, buffer);
}

#endif // platform selection
