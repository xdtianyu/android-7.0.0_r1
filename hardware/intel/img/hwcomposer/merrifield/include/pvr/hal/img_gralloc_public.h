/* Copyright (c) Imagination Technologies Ltd.
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HAL_PUBLIC_H
#define HAL_PUBLIC_H

/* Authors of third party hardware composer (HWC) modules will need to include
 * this header to access functionality in the gralloc HAL.
 */

#include <hardware/gralloc.h>

#define ALIGN(x,a)	(((x) + (a) - 1L) & ~((a) - 1L))
#define HW_ALIGN	32

/* Use bits [0-3] of "vendor format" bits as real format. Customers should
 * use *only* the unassigned bits below for custom pixel formats, YUV or RGB.
 *
 * If there are no bits set in this part of the field, or other bits are set
 * in the format outside of the "vendor format" mask, the non-extension format
 * is used instead. Reserve 0 for this purpose.
 */

#define HAL_PIXEL_FORMAT_VENDOR_EXT(fmt) (0x100 | (fmt & 0xF))

/*      Reserved ** DO NOT USE **    HAL_PIXEL_FORMAT_VENDOR_EXT(0) */
#define HAL_PIXEL_FORMAT_BGRX_8888   HAL_PIXEL_FORMAT_VENDOR_EXT(1)
#define HAL_PIXEL_FORMAT_sBGR_A_8888 HAL_PIXEL_FORMAT_VENDOR_EXT(2)
#define HAL_PIXEL_FORMAT_sBGR_X_8888 HAL_PIXEL_FORMAT_VENDOR_EXT(3)
/*      HAL_PIXEL_FORMAT_RGB_565     HAL_PIXEL_FORMAT_VENDOR_EXT(4) */
/*      HAL_PIXEL_FORMAT_BGRA_8888   HAL_PIXEL_FORMAT_VENDOR_EXT(5) */
#define HAL_PIXEL_FORMAT_NV12        HAL_PIXEL_FORMAT_VENDOR_EXT(6)
/*      Free for customer use        HAL_PIXEL_FORMAT_VENDOR_EXT(7) */
/*      Free for customer use        HAL_PIXEL_FORMAT_VENDOR_EXT(8) */
/*      Free for customer use        HAL_PIXEL_FORMAT_VENDOR_EXT(9) */
/*      Free for customer use        HAL_PIXEL_FORMAT_VENDOR_EXT(10) */
/*      Free for customer use        HAL_PIXEL_FORMAT_VENDOR_EXT(11) */
/*      Free for customer use        HAL_PIXEL_FORMAT_VENDOR_EXT(12) */
/*      Free for customer use        HAL_PIXEL_FORMAT_VENDOR_EXT(13) */
/*      Free for customer use        HAL_PIXEL_FORMAT_VENDOR_EXT(14) */
/*      Free for customer use        HAL_PIXEL_FORMAT_VENDOR_EXT(15) */

/* One of the below compression modes is OR'ed into bits [4-6] of the 8 bit
 * "vendor format" field. If no bits are set in this "compression mask", the
 * normal memory format for the pixel format is used. Otherwise the pixel
 * data will be compressed in memory with the Rogue framebuffer compressor.
 */

#define HAL_FB_COMPRESSION_NONE                0
#define HAL_FB_COMPRESSION_DIRECT_8x8          1
#define HAL_FB_COMPRESSION_DIRECT_16x4         2
#define HAL_FB_COMPRESSION_DIRECT_32x2         3
#define HAL_FB_COMPRESSION_INDIRECT_8x8        4
#define HAL_FB_COMPRESSION_INDIRECT_16x4       5
#define HAL_FB_COMPRESSION_INDIRECT_4TILE_8x8  6
#define HAL_FB_COMPRESSION_INDIRECT_4TILE_16x4 7

/* The memory layout is OR'ed into bit 7 (top bit) of the 8 bit "vendor
 * format" field. Only STRIDED and TWIDDLED are supported; there is no space
 * for PAGETILED.
 */
#define HAL_FB_MEMLAYOUT_STRIDED               0
#define HAL_FB_MEMLAYOUT_TWIDDLED              1

/* This can be tuned down as appropriate for the SOC.
 *
 * IMG formats are usually a single sub-alloc.
 * Some OEM video formats are two sub-allocs (Y, UV planes).
 * Future OEM video formats might be three sub-allocs (Y, U, V planes).
 */
#define MAX_SUB_ALLOCS (3)

typedef struct
{
	native_handle_t base;

	/* These fields can be sent cross process. They are also valid
	 * to duplicate within the same process.
	 *
	 * A table is stored within psPrivateData on gralloc_module_t (this
	 * is obviously per-process) which maps stamps to a mapped
	 * PVRSRV_MEMDESC in that process. Each map entry has a lock
	 * count associated with it, satisfying the requirements of the
	 * Android API. This also prevents us from leaking maps/allocations.
	 *
	 * This table has entries inserted either by alloc()
	 * (alloc_device_t) or map() (gralloc_module_t). Entries are removed
	 * by free() (alloc_device_t) and unmap() (gralloc_module_t).
	 */

#define IMG_NATIVE_HANDLE_NUMFDS (MAX_SUB_ALLOCS)
	/* The `fd' field is used to "export" a meminfo to another process.
	 * Therefore, it is allocated by alloc_device_t, and consumed by
	 * gralloc_module_t.
	 */
	int fd[IMG_NATIVE_HANDLE_NUMFDS];

	/* This define should represent the number of packed 'int's required to
	 * represent the fields following it. If you add a data type that is
	 * 64-bit, for example using 'unsigned long long', you should write that
	 * as "sizeof(unsigned long long) / sizeof(int)". Please keep the order
	 * of the additions the same as the defined field order.
	 */
#define IMG_NATIVE_HANDLE_NUMINTS \
	(sizeof(unsigned long long) / sizeof(int) + \
	 6 + MAX_SUB_ALLOCS + MAX_SUB_ALLOCS + \
	 sizeof(unsigned long long) / sizeof(int) * MAX_SUB_ALLOCS + \
	 1)
	/* A KERNEL unique identifier for any exported kernel meminfo. Each
	 * exported kernel meminfo will have a unique stamp, but note that in
	 * userspace, several meminfos across multiple processes could have
	 * the same stamp. As the native_handle can be dup(2)'d, there could be
	 * multiple handles with the same stamp but different file descriptors.
	 */
	unsigned long long ui64Stamp;

	/* This is used for buffer usage validation */
	int usage;

	/* In order to do efficient cache flushes we need the buffer dimensions,
	 * format and bits per pixel. There are ANativeWindow queries for the
	 * width, height and format, but the graphics HAL might have remapped the
	 * request to different values at allocation time. These are the 'true'
	 * values of the buffer allocation.
	 */
	int iWidth;
	int iHeight;
	int iFormat;
	unsigned int uiBpp;

	/* Planes are not the same as the `fd' suballocs. A multi-planar YUV
	 * allocation has different planes (interleaved = 1, semi-planar = 2,
	 * fully-planar = 3) but might be spread across 1, 2 or 3 independent
	 * memory allocations (or not).
	 */
	int iPlanes;

	/* For multi-planar allocations, there will be multiple hstrides */
	int aiStride[MAX_SUB_ALLOCS];

	/* For multi-planar allocations, there will be multiple vstrides */
	int aiVStride[MAX_SUB_ALLOCS];

	/* These byte offsets are reconciled with the number of sub-allocs used
	 * for a multi-planar allocation. If there is a 1:1 mapping between the
	 * number of planes and the number of sub-allocs, these will all be zero.
	 *
	 * Otherwise, normally the zeroth entry will be zero, and the latter
	 * entries will be non-zero.
	 */
	unsigned long long aulPlaneOffset[MAX_SUB_ALLOCS];

	/* This records the number of MAX_SUB_ALLOCS fds actually used by the
	 * buffer allocation. File descriptors up to fd[iNumSubAllocs - 1] are
	 * guaranteed to be valid. (This does not have any bearing on the aiStride,
	 * aiVStride or aulPlaneOffset fields, as `iPlanes' of those arrays should
	 * be initialized, not `iNumSubAllocs'.)
	 */
	int iNumSubAllocs;
}
__attribute__((aligned(sizeof(int)),packed)) IMG_native_handle_t;

typedef struct
{
	int l, t, w, h;
}
IMG_write_lock_rect_t;

#define IMG_BFF_YUV					(1 << 0)
#define IMG_BFF_UVCbCrORDERING		(1 << 1)
#define IMG_BFF_CPU_CLEAR			(1 << 2)
#define IMG_BFF_DONT_GPU_CLEAR		(1 << 3)
#define IMG_BFF_PARTIAL_ALLOC		(1 << 4)
#define IMG_BFF_NEVER_COMPRESS		(1 << 5)

/* Keep this in sync with SGX */
typedef struct IMG_buffer_format_public_t
{
	/* Buffer formats are returned as a linked list */
	struct IMG_buffer_format_public_t *psNext;

	/* HAL_PIXEL_FORMAT_... enumerant */
	int iHalPixelFormat;

	/* IMG_PIXFMT_... enumerant */
	int iIMGPixelFormat;

	/* Friendly name for format */
	const char *const szName;

	/* Bits (not bytes) per pixel */
	unsigned int uiBpp;

	/* Supported HW usage bits. If this is GRALLOC_USAGE_HW_MASK, all usages
	 * are supported. Used for HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED.
	 */
	int iSupportedUsage;

	/* Allocation description flags */
	unsigned int uiFlags;
}
IMG_buffer_format_public_t;

/* NOTE: This interface is deprecated. Use module->perform() instead. */
typedef struct IMG_gralloc_module_public_t
{
	gralloc_module_t base;

	/* Gets the head of the linked list of all registered formats */
	const IMG_buffer_format_public_t *(*GetBufferFormats)(void);

	/* Custom-blit components in lieu of overlay hardware */
	int (*Blit)(struct IMG_gralloc_module_public_t const *module,
				 buffer_handle_t src, buffer_handle_t dest,
				 int w, int h, int x, int y, int transform,
				 int iInputFenceFd, int *piOutputFenceFd);

	int (*Blit3)(struct IMG_gralloc_module_public_t const *module,
				 unsigned long long ui64SrcStamp, int iSrcWidth,
				 int iSrcHeight, int iSrcFormat, int iSrcStrideInPixels,
				 int eSrcRotation, buffer_handle_t dest, int eDestRotation,
				 int iInputFenceFd, int *piOutputFenceFd);

	/* Walk the above list and return only the specified format */
	const IMG_buffer_format_public_t *(*GetBufferFormat)(int iFormat);
}
IMG_gralloc_module_public_t;

/* Helpers for using the non-type-safe perform() extension functions. Use
 * these helpers instead of calling perform() directly in your application.
 */

#define GRALLOC_MODULE_GET_BUFFER_FORMAT_IMG     1
#define GRALLOC_MODULE_GET_BUFFER_FORMATS_IMG    2
#define GRALLOC_MODULE_BLIT_HANDLE_TO_HANDLE_IMG 3
#define GRALLOC_MODULE_BLIT_STAMP_TO_HANDLE_IMG  4

static inline int
gralloc_module_get_buffer_format_img(const gralloc_module_t *module,
									 int format,
									 const IMG_buffer_format_public_t **v)
{
	return module->perform(module, GRALLOC_MODULE_GET_BUFFER_FORMAT_IMG,
						   format, v);
}

static inline int
gralloc_module_get_buffer_formats_img(const gralloc_module_t *module,
									  const IMG_buffer_format_public_t **v)
{
	return module->perform(module, GRALLOC_MODULE_GET_BUFFER_FORMATS_IMG, v);
}

static inline int
gralloc_module_blit_handle_to_handle_img(const gralloc_module_t *module,
										 buffer_handle_t src,
										 buffer_handle_t dest,
										 int w, int h, int x, int y,
										 int transform, int input_fence,
										 int *output_fence)
{
	return module->perform(module, GRALLOC_MODULE_BLIT_HANDLE_TO_HANDLE_IMG,
						   src, dest, w, h, x, y, transform, input_fence,
						   output_fence);
}

static inline int
gralloc_module_blit_stamp_to_handle(const gralloc_module_t *module,
									unsigned long long src_stamp,
									int src_width, int src_height,
									int src_format, int src_stride_in_pixels,
									int src_rotation, buffer_handle_t dest,
									int dest_rotation, int input_fence,
									int *output_fence)
{
	return module->perform(module, GRALLOC_MODULE_BLIT_STAMP_TO_HANDLE_IMG,
						   src_stamp, src_width, src_height, src_format,
						   src_stride_in_pixels, src_rotation, dest,
						   dest_rotation, input_fence, output_fence);
}

#endif /* HAL_PUBLIC_H */
