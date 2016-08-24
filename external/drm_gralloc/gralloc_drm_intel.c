/*
 * Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
 * Copyright (C) 2010-2011 LunarG Inc.
 *
 * drm_gem_intel_copy is based on xorg-driver-intel, which has
 *
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright (c) 2005 Jesse Barnes <jbarnes@virtuousgeek.org>
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

#define LOG_TAG "GRALLOC-I915"

#include <cutils/log.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <drm.h>
#include <intel_bufmgr.h>
#include <i915_drm.h>

#include "gralloc_drm.h"
#include "gralloc_drm_priv.h"

#define MI_NOOP                     (0)
#define MI_BATCH_BUFFER_END         (0x0a << 23)
#define MI_FLUSH                    (0x04 << 23)
#define MI_FLUSH_DW                 (0x26 << 23)
#define MI_WRITE_DIRTY_STATE        (1 << 4) 
#define MI_INVALIDATE_MAP_CACHE     (1 << 0)
#define XY_SRC_COPY_BLT_CMD         ((2 << 29) | (0x53 << 22) | 6)
#define XY_SRC_COPY_BLT_WRITE_ALPHA (1 << 21)
#define XY_SRC_COPY_BLT_WRITE_RGB   (1 << 20)
#define XY_SRC_COPY_BLT_SRC_TILED   (1 << 15)
#define XY_SRC_COPY_BLT_DST_TILED   (1 << 11)

struct intel_info {
	struct gralloc_drm_drv_t base;

	int fd;
	drm_intel_bufmgr *bufmgr;
	int gen;

	drm_intel_bo *batch_ibo;
	uint32_t *batch, *cur;
	int capacity, size;
	int exec_blt;
};

struct intel_buffer {
	struct gralloc_drm_bo_t base;
	drm_intel_bo *ibo;
	uint32_t tiling;
};

static int
batch_next(struct intel_info *info)
{
	info->cur = info->batch;

	if (info->batch_ibo)
		drm_intel_bo_unreference(info->batch_ibo);

	info->batch_ibo = drm_intel_bo_alloc(info->bufmgr,
			"gralloc-batchbuffer", info->size, 4096);

	return (info->batch_ibo) ? 0 : -ENOMEM;
}

static int
batch_count(struct intel_info *info)
{
	return info->cur - info->batch;
}

static void
batch_dword(struct intel_info *info, uint32_t dword)
{
	*info->cur++ = dword;
}

static int
batch_reloc(struct intel_info *info, struct gralloc_drm_bo_t *bo,
		uint32_t read_domains, uint32_t write_domain)
{
	struct intel_buffer *target = (struct intel_buffer *) bo;
	uint32_t offset = (info->cur - info->batch) * sizeof(info->batch[0]);
	int ret;

	ret = drm_intel_bo_emit_reloc(info->batch_ibo, offset,
			target->ibo, 0, read_domains, write_domain);
	if (!ret)
		batch_dword(info, target->ibo->offset);

	return ret;
}

static int
batch_flush(struct intel_info *info)
{
	int size, ret;

	batch_dword(info, MI_BATCH_BUFFER_END);
	size = batch_count(info);
	if (size & 1) {
		batch_dword(info, MI_NOOP);
		size = batch_count(info);
	}

	size *= sizeof(info->batch[0]);
	ret = drm_intel_bo_subdata(info->batch_ibo, 0, size, info->batch);
	if (ret) {
		ALOGE("failed to subdata batch");
		goto fail;
	}
	ret = drm_intel_bo_mrb_exec(info->batch_ibo, size,
		NULL, 0, 0, info->exec_blt);
	if (ret) {
		ALOGE("failed to exec batch");
		goto fail;
	}

	return batch_next(info);

fail:
	info->cur = info->batch;

	return ret;
}

static int
batch_reserve(struct intel_info *info, int count)
{
	int ret = 0;

	if (batch_count(info) + count > info->capacity)
		ret = batch_flush(info);

	return ret;
}

static void
batch_destroy(struct intel_info *info)
{
	if (info->batch_ibo) {
		drm_intel_bo_unreference(info->batch_ibo);
		info->batch_ibo = NULL;
	}

	if (info->batch) {
		free(info->batch);
		info->batch = NULL;
	}
}

static int
batch_init(struct intel_info *info)
{
	int ret;

	info->capacity = 512;
	info->size = (info->capacity + 16) * sizeof(info->batch[0]);

	info->batch = malloc(info->size);
	if (!info->batch)
		return -ENOMEM;

	ret = batch_next(info);
	if (ret) {
		free(info->batch);
		info->batch = NULL;
	}

	return ret;
}

static void intel_resolve_format(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo,
		uint32_t *pitches, uint32_t *offsets, uint32_t *handles)
{
	/*
	 * TODO - should take account hw specific padding, alignment
	 * for camera, video decoder etc.
	 */

	struct intel_buffer *ib = (struct intel_buffer *) bo;

	memset(pitches, 0, 4 * sizeof(uint32_t));
	memset(offsets, 0, 4 * sizeof(uint32_t));
	memset(handles, 0, 4 * sizeof(uint32_t));

	pitches[0] = ib->base.handle->stride;
	handles[0] = ib->base.fb_handle;

	switch(ib->base.handle->format) {
		case HAL_PIXEL_FORMAT_YV12:

			// U and V stride are half of Y plane
			pitches[2] = pitches[0]/2;
			pitches[1] = pitches[0]/2;

			// like I420 but U and V are in reverse order
			offsets[2] = offsets[0] +
				pitches[0] * ib->base.handle->height;
			offsets[1] = offsets[2] +
				pitches[2] * ib->base.handle->height/2;

			handles[1] = handles[2] = handles[0];
			break;

		case HAL_PIXEL_FORMAT_DRM_NV12:

			// U and V are interleaved in 2nd plane
			pitches[1] = pitches[0];
			offsets[1] = offsets[0] +
				pitches[0] * ib->base.handle->height;

			handles[1] = handles[0];
			break;
	}
}

static drm_intel_bo *alloc_ibo(struct intel_info *info,
		const struct gralloc_drm_handle_t *handle,
		uint32_t *tiling, unsigned long *stride)
{
	drm_intel_bo *ibo;
	const char *name;
	int aligned_width, aligned_height, bpp;
	unsigned long flags;

	flags = 0;
	bpp = gralloc_drm_get_bpp(handle->format);
	if (!bpp) {
		ALOGE("unrecognized format 0x%x", handle->format);
		return NULL;
	}

	aligned_width = handle->width;
	aligned_height = handle->height;
	gralloc_drm_align_geometry(handle->format,
			&aligned_width, &aligned_height);

	if (handle->usage & GRALLOC_USAGE_HW_FB) {
		unsigned long max_stride;

		max_stride = 32 * 1024;
		if (info->gen < 50)
			max_stride /= 2;
		if (info->gen < 40)
			max_stride /= 2;

		name = "gralloc-fb";
		aligned_width = ALIGN(aligned_width, 64);
		flags = BO_ALLOC_FOR_RENDER;

		*tiling = I915_TILING_X;
		*stride = aligned_width * bpp;
		if (*stride > max_stride) {
			*tiling = I915_TILING_NONE;
			max_stride = 32 * 1024;
			if (*stride > max_stride)
				return NULL;
		}

		while (1) {
			ibo = drm_intel_bo_alloc_tiled(info->bufmgr, name,
					aligned_width, aligned_height,
					bpp, tiling, stride, flags);
			if (!ibo || *stride > max_stride) {
				if (ibo) {
					drm_intel_bo_unreference(ibo);
					ibo = NULL;
				}

				if (*tiling != I915_TILING_NONE) {
					/* retry */
					*tiling = I915_TILING_NONE;
					max_stride = 32 * 1024;
					continue;
				}
			}
			if (ibo)
				drm_intel_bo_disable_reuse(ibo);
			break;
		}
	}
	else {
		if (handle->usage & (GRALLOC_USAGE_SW_READ_OFTEN |
				     GRALLOC_USAGE_SW_WRITE_OFTEN))
			*tiling = I915_TILING_NONE;
		else if ((handle->usage & GRALLOC_USAGE_HW_RENDER) ||
			 ((handle->usage & GRALLOC_USAGE_HW_TEXTURE) &&
			  handle->width >= 64))
			*tiling = I915_TILING_X;
		else
			*tiling = I915_TILING_NONE;

		if (handle->usage & GRALLOC_USAGE_HW_TEXTURE) {
			name = "gralloc-texture";
			/* see 2D texture layout of DRI drivers */
			aligned_width = ALIGN(aligned_width, 4);
			aligned_height = ALIGN(aligned_height, 2);
		}
		else {
			name = "gralloc-buffer";
		}

		if (handle->usage & GRALLOC_USAGE_HW_RENDER)
			flags = BO_ALLOC_FOR_RENDER;

		ibo = drm_intel_bo_alloc_tiled(info->bufmgr, name,
				aligned_width, aligned_height,
				bpp, tiling, stride, flags);
	}

	return ibo;
}

static struct gralloc_drm_bo_t *intel_alloc(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_handle_t *handle)
{
	struct intel_info *info = (struct intel_info *) drv;
	struct intel_buffer *ib;

	ib = calloc(1, sizeof(*ib));
	if (!ib)
		return NULL;

	if (handle->name) {
		uint32_t dummy;

		ib->ibo = drm_intel_bo_gem_create_from_name(info->bufmgr,
				"gralloc-r", handle->name);
		if (!ib->ibo) {
			ALOGE("failed to create ibo from name %u",
					handle->name);
			free(ib);
			return NULL;
		}

		if (drm_intel_bo_get_tiling(ib->ibo, &ib->tiling, &dummy)) {
			ALOGE("failed to get ibo tiling");
			drm_intel_bo_unreference(ib->ibo);
			free(ib);
			return NULL;
		}
	}
	else {
		unsigned long stride;

		ib->ibo = alloc_ibo(info, handle, &ib->tiling, &stride);
		if (!ib->ibo) {
			ALOGE("failed to allocate ibo %dx%d (format %d)",
					handle->width,
					handle->height,
					handle->format);
			free(ib);
			return NULL;
		}

		handle->stride = stride;

		if (drm_intel_bo_flink(ib->ibo, (uint32_t *) &handle->name)) {
			ALOGE("failed to flink ibo");
			drm_intel_bo_unreference(ib->ibo);
			free(ib);
			return NULL;
		}
	}

	ib->base.fb_handle = ib->ibo->handle;

	ib->base.handle = handle;

	return &ib->base;
}

static void intel_free(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo)
{
	struct intel_buffer *ib = (struct intel_buffer *) bo;

	drm_intel_bo_unreference(ib->ibo);
	free(ib);
}

static int intel_map(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo,
		int x, int y, int w, int h,
		int enable_write, void **addr)
{
	struct intel_buffer *ib = (struct intel_buffer *) bo;
	int err;

	if (ib->tiling != I915_TILING_NONE ||
	    (ib->base.handle->usage & GRALLOC_USAGE_HW_FB))
		err = drm_intel_gem_bo_map_gtt(ib->ibo);
	else
		err = drm_intel_bo_map(ib->ibo, enable_write);
	if (!err)
		*addr = ib->ibo->virtual;

	return err;
}

static void intel_unmap(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo)
{
	struct intel_buffer *ib = (struct intel_buffer *) bo;

	if (ib->tiling != I915_TILING_NONE ||
	    (ib->base.handle->usage & GRALLOC_USAGE_HW_FB))
		drm_intel_gem_bo_unmap_gtt(ib->ibo);
	else
		drm_intel_bo_unmap(ib->ibo);
}

#include "intel_chipset.h" /* for platform detection macros */
static void gen_init(struct intel_info *info)
{
	struct drm_i915_getparam gp;
	int pageflipping, id, has_blt;

	memset(&gp, 0, sizeof(gp));
	gp.param = I915_PARAM_CHIPSET_ID;
	gp.value = &id;
	if (drmCommandWriteRead(info->fd, DRM_I915_GETPARAM, &gp, sizeof(gp)))
		id = 0;

	memset(&gp, 0, sizeof(gp));
	gp.param = I915_PARAM_HAS_BLT;
	gp.value = &has_blt;
	if (drmCommandWriteRead(info->fd, DRM_I915_GETPARAM, &gp, sizeof(gp)))
		has_blt = 0;
	info->exec_blt = has_blt ? I915_EXEC_BLT : 0;

	/* GEN4, G4X, GEN5, GEN6, GEN7 */
	if ((IS_9XX(id) || IS_G4X(id)) && !IS_GEN3(id)) {
		if (IS_GEN7(id))
			info->gen = 70;
		else if (IS_GEN6(id))
			info->gen = 60;
		else if (IS_GEN5(id))
			info->gen = 50;
		else
			info->gen = 40;
	}
	else {
		info->gen = 30;
	}
}

static void intel_destroy(struct gralloc_drm_drv_t *drv)
{
	struct intel_info *info = (struct intel_info *) drv;

	batch_destroy(info);
	drm_intel_bufmgr_destroy(info->bufmgr);
	free(info);
}

struct gralloc_drm_drv_t *gralloc_drm_drv_create_for_intel(int fd)
{
	struct intel_info *info;

	info = calloc(1, sizeof(*info));
	if (!info) {
		ALOGE("failed to allocate driver info");
		return NULL;
	}

	info->fd = fd;
	info->bufmgr = drm_intel_bufmgr_gem_init(info->fd, 16 * 1024);
	if (!info->bufmgr) {
		ALOGE("failed to create buffer manager");
		free(info);
		return NULL;
	}

	batch_init(info);
	gen_init(info);

	info->base.destroy = intel_destroy;
	info->base.alloc = intel_alloc;
	info->base.free = intel_free;
	info->base.map = intel_map;
	info->base.unmap = intel_unmap;
	info->base.resolve_format = intel_resolve_format;

	return &info->base;
}
