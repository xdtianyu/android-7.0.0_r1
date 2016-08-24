/*
 * Copyright (C) 2011 Chia-I Wu <olvaffe@gmail.com>
 * Copyright (C) 2011 LunarG Inc.
 *
 * Based on xf86-video-nouveau, which has
 *
 * Copyright © 2007 Red Hat, Inc.
 * Copyright © 2008 Maarten Maathuis
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

#define LOG_TAG "GRALLOC-NOUVEAU"

#include <cutils/log.h>
#include <stdlib.h>
#include <errno.h>
#include <drm.h>
#include <nouveau_drmif.h>
#include <nouveau_channel.h>
#include <nouveau_bo.h>

#include "gralloc_drm.h"
#include "gralloc_drm_priv.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define NVC0_TILE_HEIGHT(m) (8 << ((m) >> 4))

enum {
	NvDmaFB = 0xd8000001,
	NvDmaTT = 0xd8000002,
};

struct nouveau_info {
	struct gralloc_drm_drv_t base;

	int fd;
	struct nouveau_device *dev;
	struct nouveau_channel *chan;
	int arch;
	int tiled_scanout;
};

struct nouveau_buffer {
	struct gralloc_drm_bo_t base;

	struct nouveau_bo *bo;
};

static struct nouveau_bo *alloc_bo(struct nouveau_info *info,
		int width, int height, int cpp, int usage, int *pitch)
{
	struct nouveau_bo *bo = NULL;
	int flags, tile_mode, tile_flags;
	int tiled, scanout;
	unsigned int align;

	flags = NOUVEAU_BO_MAP | NOUVEAU_BO_VRAM;
	tile_mode = 0;
	tile_flags = 0;

	scanout = !!(usage & GRALLOC_USAGE_HW_FB);

	tiled = !(usage & (GRALLOC_USAGE_SW_READ_OFTEN |
			   GRALLOC_USAGE_SW_WRITE_OFTEN));
	if (!info->chan)
		tiled = 0;
	else if (scanout && info->tiled_scanout)
		tiled = 1;

	/* calculate pitch align */
	align = 64;
	if (info->arch >= 0x50) {
		if (scanout && !info->tiled_scanout)
			align = 256;
		else
			tiled = 1;
	}

	*pitch = ALIGN(width * cpp, align);

	if (tiled) {
		if (info->arch >= 0xc0) {
			if (height > 64)
				tile_mode = 0x40;
			else if (height > 32)
				tile_mode = 0x30;
			else if (height > 16)
				tile_mode = 0x20;
			else if (height > 8)
				tile_mode = 0x10;
			else
				tile_mode = 0x00;

			tile_flags = 0xfe00;

			align = NVC0_TILE_HEIGHT(tile_mode);
			height = ALIGN(height, align);
		}
		else if (info->arch >= 0x50) {
			if (height > 32)
				tile_mode = 4;
			else if (height > 16)
				tile_mode = 3;
			else if (height > 8)
				tile_mode = 2;
			else if (height > 4)
				tile_mode = 1;
			else
				tile_mode = 0;

			tile_flags = (scanout && cpp != 2) ? 0x7a00 : 0x7000;

			align = 1 << (tile_mode + 2);
			height = ALIGN(height, align);
		}
		else {
			align = *pitch / 4;

			/* round down to the previous power of two */
			align >>= 1;
			align |= align >> 1;
			align |= align >> 2;
			align |= align >> 4;
			align |= align >> 8;
			align |= align >> 16;
			align++;

			align = MAX((info->dev->chipset >= 0x40) ? 1024 : 256,
					align);

			/* adjust pitch */
			*pitch = ALIGN(*pitch, align);

			tile_mode = *pitch;
		}
	}

	if (cpp == 4)
		tile_flags |= NOUVEAU_BO_TILE_32BPP;
	else if (cpp == 2)
		tile_flags |= NOUVEAU_BO_TILE_16BPP;

	if (scanout)
		tile_flags |= NOUVEAU_BO_TILE_SCANOUT;

	if (nouveau_bo_new_tile(info->dev, flags, 0, *pitch * height,
				tile_mode, tile_flags, &bo)) {
		ALOGE("failed to allocate bo (flags 0x%x, size %d, tile_mode 0x%x, tile_flags 0x%x)",
				flags, *pitch * height, tile_mode, tile_flags);
		bo = NULL;
	}

	return bo;
}

static struct gralloc_drm_bo_t *
nouveau_alloc(struct gralloc_drm_drv_t *drv, struct gralloc_drm_handle_t *handle)
{
	struct nouveau_info *info = (struct nouveau_info *) drv;
	struct nouveau_buffer *nb;
	int cpp;

	cpp = gralloc_drm_get_bpp(handle->format);
	if (!cpp) {
		ALOGE("unrecognized format 0x%x", handle->format);
		return NULL;
	}

	nb = calloc(1, sizeof(*nb));
	if (!nb)
		return NULL;

	if (handle->name) {
		if (nouveau_bo_handle_ref(info->dev, handle->name, &nb->bo)) {
			ALOGE("failed to create nouveau bo from name %u",
					handle->name);
			free(nb);
			return NULL;
		}
	}
	else {
		int width, height, pitch;

		width = handle->width;
		height = handle->height;
		gralloc_drm_align_geometry(handle->format, &width, &height);

		nb->bo = alloc_bo(info, width, height,
				cpp, handle->usage, &pitch);
		if (!nb->bo) {
			ALOGE("failed to allocate nouveau bo %dx%dx%d",
					handle->width, handle->height, cpp);
			free(nb);
			return NULL;
		}

		if (nouveau_bo_handle_get(nb->bo,
					(uint32_t *) &handle->name)) {
			ALOGE("failed to flink nouveau bo");
			nouveau_bo_ref(NULL, &nb->bo);
			free(nb);
			return NULL;
		}

		handle->stride = pitch;
	}

	if (handle->usage & GRALLOC_USAGE_HW_FB)
		nb->base.fb_handle = nb->bo->handle;

	nb->base.handle = handle;

	return &nb->base;
}

static void nouveau_free(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo)
{
	struct nouveau_buffer *nb = (struct nouveau_buffer *) bo;
	nouveau_bo_ref(NULL, &nb->bo);
	free(nb);
}

static int nouveau_map(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo, int x, int y, int w, int h,
		int enable_write, void **addr)
{
	struct nouveau_buffer *nb = (struct nouveau_buffer *) bo;
	uint32_t flags;
	int err;

	flags = NOUVEAU_BO_RD;
	if (enable_write)
		flags |= NOUVEAU_BO_WR;

	/* TODO if tiled, allocate a linear copy of bo in GART and map it */
	err = nouveau_bo_map(nb->bo, flags);
	if (!err)
		*addr = nb->bo->map;

	return err;
}

static void nouveau_unmap(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo)
{
	struct nouveau_buffer *nb = (struct nouveau_buffer *) bo;
	/* TODO if tiled, unmap the linear bo and copy back */
	nouveau_bo_unmap(nb->bo);
}

static void nouveau_destroy(struct gralloc_drm_drv_t *drv)
{
	struct nouveau_info *info = (struct nouveau_info *) drv;

	if (info->chan)
		nouveau_channel_free(&info->chan);
	nouveau_device_close(&info->dev);
	free(info);
}

static int nouveau_init(struct nouveau_info *info)
{
	int err = 0;

	switch (info->dev->chipset & 0xf0) {
	case 0x00:
		info->arch = 0x04;
		break;
	case 0x10:
		info->arch = 0x10;
		break;
	case 0x20:
		info->arch = 0x20;
		break;
	case 0x30:
		info->arch = 0x30;
		break;
	case 0x40:
	case 0x60:
		info->arch = 0x40;
		break;
	case 0x50:
	case 0x80:
	case 0x90:
	case 0xa0:
		info->arch = 0x50;
		break;
	case 0xc0:
		info->arch = 0xc0;
		break;
	default:
		ALOGE("unknown nouveau chipset 0x%x", info->dev->chipset);
		err = -EINVAL;
		break;
	}

	info->tiled_scanout = (info->chan != NULL);

	return err;
}

struct gralloc_drm_drv_t *gralloc_drm_drv_create_for_nouveau(int fd)
{
	struct nouveau_info *info;
	int err;

	info = calloc(1, sizeof(*info));
	if (!info)
		return NULL;

	info->fd = fd;
	err = nouveau_device_open_existing(&info->dev, 0, info->fd, 0);
	if (err) {
		ALOGE("failed to create nouveau device");
		free(info);
		return NULL;
	}

	err = nouveau_channel_alloc(info->dev, NvDmaFB, NvDmaTT,
			24 * 1024, &info->chan);
	if (err) {
		/* make it non-fatal temporarily as it may require firmwares */
		ALOGW("failed to create nouveau channel");
		info->chan = NULL;
	}

	err = nouveau_init(info);
	if (err) {
		if (info->chan)
			nouveau_channel_free(&info->chan);
		nouveau_device_close(&info->dev);
		free(info);
		return NULL;
	}

	info->base.destroy = nouveau_destroy;
	info->base.alloc = nouveau_alloc;
	info->base.free = nouveau_free;
	info->base.map = nouveau_map;
	info->base.unmap = nouveau_unmap;

	return &info->base;
}
