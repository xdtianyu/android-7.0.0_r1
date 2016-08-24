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

#ifndef _GRALLOC_DRM_PRIV_H_
#define _GRALLOC_DRM_PRIV_H_

#include <pthread.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "gralloc_drm_handle.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gralloc_drm_t {
	/* initialized by gralloc_drm_create */
	int fd;
	struct gralloc_drm_drv_t *drv;
};

struct drm_module_t {
	gralloc_module_t base;

	pthread_mutex_t mutex;
	struct gralloc_drm_t *drm;
};

struct gralloc_drm_drv_t {
	/* destroy the driver */
	void (*destroy)(struct gralloc_drm_drv_t *drv);

	/* allocate or import a bo */
	struct gralloc_drm_bo_t *(*alloc)(struct gralloc_drm_drv_t *drv,
			                  struct gralloc_drm_handle_t *handle);

	/* free a bo */
	void (*free)(struct gralloc_drm_drv_t *drv,
		     struct gralloc_drm_bo_t *bo);

	/* map a bo for CPU access */
	int (*map)(struct gralloc_drm_drv_t *drv,
		   struct gralloc_drm_bo_t *bo,
		   int x, int y, int w, int h, int enable_write, void **addr);

	/* unmap a bo */
	void (*unmap)(struct gralloc_drm_drv_t *drv,
		      struct gralloc_drm_bo_t *bo);

	/* query component offsets, strides and handles for a format */
	void (*resolve_format)(struct gralloc_drm_drv_t *drv,
		     struct gralloc_drm_bo_t *bo,
		     uint32_t *pitches, uint32_t *offsets, uint32_t *handles);
};

struct gralloc_drm_bo_t {
	struct gralloc_drm_t *drm;
	struct gralloc_drm_handle_t *handle;

	int imported;  /* the handle is from a remote proces when true */
	int fb_handle; /* the GEM handle of the bo */
	int fb_id;     /* the fb id */

	int lock_count;
	int locked_for;

	unsigned int refcount;
};

struct gralloc_drm_drv_t *gralloc_drm_drv_create_for_pipe(int fd, const char *name);
struct gralloc_drm_drv_t *gralloc_drm_drv_create_for_intel(int fd);
struct gralloc_drm_drv_t *gralloc_drm_drv_create_for_radeon(int fd);
struct gralloc_drm_drv_t *gralloc_drm_drv_create_for_rockchip(int fd);
struct gralloc_drm_drv_t *gralloc_drm_drv_create_for_nouveau(int fd);

#ifdef __cplusplus
}
#endif
#endif /* _GRALLOC_DRM_PRIV_H_ */
