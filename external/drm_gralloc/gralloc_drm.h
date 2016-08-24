/*
 * Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
 * Copyright (C) 2010-2011 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _GRALLOC_DRM_H_
#define _GRALLOC_DRM_H_

#include <hardware/gralloc.h>
#include <system/graphics.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALIGN(val, align) (((val) + (align) - 1) & ~((align) - 1))

struct gralloc_drm_t;
struct gralloc_drm_bo_t;

enum {
	GRALLOC_MODULE_PERFORM_GET_DRM_FD                = 0x80000002,
};

struct gralloc_drm_t *gralloc_drm_create(void);
void gralloc_drm_destroy(struct gralloc_drm_t *drm);

int gralloc_drm_get_fd(struct gralloc_drm_t *drm);

static inline int gralloc_drm_get_bpp(int format)
{
	int bpp;

	switch (format) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
	case HAL_PIXEL_FORMAT_RGBX_8888:
	case HAL_PIXEL_FORMAT_BGRA_8888:
		bpp = 4;
		break;
	case HAL_PIXEL_FORMAT_RGB_888:
		bpp = 3;
		break;
	case HAL_PIXEL_FORMAT_RGB_565:
	case HAL_PIXEL_FORMAT_YCbCr_422_I:
		bpp = 2;
		break;
	/* planar; only Y is considered */
	case HAL_PIXEL_FORMAT_YV12:
	case HAL_PIXEL_FORMAT_YCbCr_422_SP:
	case HAL_PIXEL_FORMAT_YCrCb_420_SP:
	case HAL_PIXEL_FORMAT_YCbCr_420_888:
		bpp = 1;
		break;
	default:
		bpp = 0;
		break;
	}

	return bpp;
}

static inline void gralloc_drm_align_geometry(int format, int *width, int *height)
{
	int align_w = 1, align_h = 1, extra_height_div = 0;

	switch (format) {
	case HAL_PIXEL_FORMAT_YV12:
		align_w = 32;
		align_h = 2;
		extra_height_div = 2;
		break;
	case HAL_PIXEL_FORMAT_YCbCr_422_SP:
		align_w = 2;
		extra_height_div = 1;
		break;
	case HAL_PIXEL_FORMAT_YCrCb_420_SP:
	case HAL_PIXEL_FORMAT_YCbCr_420_888:
		align_w = 2;
		align_h = 2;
		extra_height_div = 2;
		break;
	case HAL_PIXEL_FORMAT_YCbCr_422_I:
		align_w = 2;
		break;
	}

	*width = ALIGN(*width, align_w);
	*height = ALIGN(*height, align_h);

	if (extra_height_div)
		*height += *height / extra_height_div;
}

int gralloc_drm_handle_register(buffer_handle_t handle, struct gralloc_drm_t *drm);
int gralloc_drm_handle_unregister(buffer_handle_t handle);

struct gralloc_drm_bo_t *gralloc_drm_bo_create(struct gralloc_drm_t *drm, int width, int height, int format, int usage);
void gralloc_drm_bo_decref(struct gralloc_drm_bo_t *bo);

struct gralloc_drm_bo_t *gralloc_drm_bo_from_handle(buffer_handle_t handle);
buffer_handle_t gralloc_drm_bo_get_handle(struct gralloc_drm_bo_t *bo, int *stride);
int gralloc_drm_get_gem_handle(buffer_handle_t handle);
void gralloc_drm_resolve_format(buffer_handle_t _handle, uint32_t *pitches, uint32_t *offsets, uint32_t *handles);
unsigned int planes_for_format(struct gralloc_drm_t *drm, int hal_format);

int gralloc_drm_bo_lock(struct gralloc_drm_bo_t *bo, int x, int y, int w, int h, int enable_write, void **addr);
void gralloc_drm_bo_unlock(struct gralloc_drm_bo_t *bo);

#ifdef __cplusplus
}
#endif
#endif /* _GRALLOC_DRM_H_ */
