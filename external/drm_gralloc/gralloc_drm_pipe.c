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

#define LOG_TAG "GRALLOC-PIPE"

#include <cutils/log.h>
#include <errno.h>

#include <pipe/p_screen.h>
#include <pipe/p_context.h>
#include <state_tracker/drm_driver.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>

#include "gralloc_drm.h"
#include "gralloc_drm_priv.h"

struct pipe_manager {
	struct gralloc_drm_drv_t base;

	int fd;
	char driver[16];
	pthread_mutex_t mutex;
	struct pipe_screen *screen;
	struct pipe_context *context;
};

struct pipe_buffer {
	struct gralloc_drm_bo_t base;

	struct pipe_resource *resource;
	struct winsys_handle winsys;

	struct pipe_transfer *transfer;
};

static enum pipe_format get_pipe_format(int format)
{
	enum pipe_format fmt;

	switch (format) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
		fmt = PIPE_FORMAT_R8G8B8A8_UNORM;
		break;
	case HAL_PIXEL_FORMAT_RGBX_8888:
		fmt = PIPE_FORMAT_R8G8B8X8_UNORM;
		break;
	case HAL_PIXEL_FORMAT_RGB_888:
		fmt = PIPE_FORMAT_R8G8B8_UNORM;
		break;
	case HAL_PIXEL_FORMAT_RGB_565:
		fmt = PIPE_FORMAT_B5G6R5_UNORM;
		break;
	case HAL_PIXEL_FORMAT_BGRA_8888:
		fmt = PIPE_FORMAT_B8G8R8A8_UNORM;
		break;
	case HAL_PIXEL_FORMAT_YV12:
	case HAL_PIXEL_FORMAT_DRM_NV12:
	case HAL_PIXEL_FORMAT_YCbCr_422_SP:
	case HAL_PIXEL_FORMAT_YCrCb_420_SP:
	default:
		fmt = PIPE_FORMAT_NONE;
		break;
	}

	return fmt;
}

static unsigned get_pipe_bind(int usage)
{
	unsigned bind = PIPE_BIND_SHARED;

	if (usage & GRALLOC_USAGE_SW_READ_MASK)
		bind |= PIPE_BIND_TRANSFER_READ;
	if (usage & GRALLOC_USAGE_SW_WRITE_MASK)
		bind |= PIPE_BIND_TRANSFER_WRITE;

	if (usage & GRALLOC_USAGE_HW_TEXTURE)
		bind |= PIPE_BIND_SAMPLER_VIEW;
	if (usage & GRALLOC_USAGE_HW_RENDER)
		bind |= PIPE_BIND_RENDER_TARGET;
	if (usage & GRALLOC_USAGE_HW_FB) {
		bind |= PIPE_BIND_RENDER_TARGET;
		bind |= PIPE_BIND_SCANOUT;
	}

	return bind;
}

static struct pipe_buffer *get_pipe_buffer_locked(struct pipe_manager *pm,
		const struct gralloc_drm_handle_t *handle)
{
	struct pipe_buffer *buf;
	struct pipe_resource templ;

	memset(&templ, 0, sizeof(templ));
	templ.format = get_pipe_format(handle->format);
	templ.bind = get_pipe_bind(handle->usage);
	templ.target = PIPE_TEXTURE_2D;

	if (templ.format == PIPE_FORMAT_NONE ||
	    !pm->screen->is_format_supported(pm->screen, templ.format,
				templ.target, 0, templ.bind)) {
		ALOGE("unsupported format 0x%x", handle->format);
		return NULL;
	}

	buf = CALLOC(1, sizeof(*buf));
	if (!buf) {
		ALOGE("failed to allocate pipe buffer");
		return NULL;
	}

	templ.width0 = handle->width;
	templ.height0 = handle->height;
	templ.depth0 = 1;
	templ.array_size = 1;

	if (handle->name) {
		buf->winsys.type = DRM_API_HANDLE_TYPE_SHARED;
		buf->winsys.handle = handle->name;
		buf->winsys.stride = handle->stride;

		buf->resource = pm->screen->resource_from_handle(pm->screen,
				&templ, &buf->winsys);
		if (!buf->resource)
			goto fail;
	}
	else {
		buf->resource =
			pm->screen->resource_create(pm->screen, &templ);
		if (!buf->resource)
			goto fail;

		buf->winsys.type = DRM_API_HANDLE_TYPE_SHARED;
		if (!pm->screen->resource_get_handle(pm->screen,
					buf->resource, &buf->winsys))
			goto fail;
	}

	/* need the gem handle for fb */
	if (handle->usage & GRALLOC_USAGE_HW_FB) {
		struct winsys_handle tmp;

		memset(&tmp, 0, sizeof(tmp));
		tmp.type = DRM_API_HANDLE_TYPE_KMS;
		if (!pm->screen->resource_get_handle(pm->screen,
					buf->resource, &tmp))
			goto fail;

		buf->base.fb_handle = tmp.handle;
	}

	return buf;

fail:
	ALOGE("failed to allocate pipe buffer");
	if (buf->resource)
		pipe_resource_reference(&buf->resource, NULL);
	FREE(buf);

	return NULL;
}

static struct gralloc_drm_bo_t *pipe_alloc(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_handle_t *handle)
{
	struct pipe_manager *pm = (struct pipe_manager *) drv;
	struct pipe_buffer *buf;

	pthread_mutex_lock(&pm->mutex);
	buf = get_pipe_buffer_locked(pm, handle);
	pthread_mutex_unlock(&pm->mutex);

	if (buf) {
		handle->name = (int) buf->winsys.handle;
		handle->stride = (int) buf->winsys.stride;

		buf->base.handle = handle;
	}

	return &buf->base;
}

static void pipe_free(struct gralloc_drm_drv_t *drv, struct gralloc_drm_bo_t *bo)
{
	struct pipe_manager *pm = (struct pipe_manager *) drv;
	struct pipe_buffer *buf = (struct pipe_buffer *) bo;

	pthread_mutex_lock(&pm->mutex);

	if (buf->transfer)
		pipe_transfer_unmap(pm->context, buf->transfer);
	pipe_resource_reference(&buf->resource, NULL);

	pthread_mutex_unlock(&pm->mutex);

	FREE(buf);
}

static int pipe_map(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo, int x, int y, int w, int h,
		int enable_write, void **addr)
{
	struct pipe_manager *pm = (struct pipe_manager *) drv;
	struct pipe_buffer *buf = (struct pipe_buffer *) bo;
	int err = 0;

	pthread_mutex_lock(&pm->mutex);

	/* need a context to get transfer */
	if (!pm->context) {
		pm->context = pm->screen->context_create(pm->screen, NULL);
		if (!pm->context) {
			ALOGE("failed to create pipe context");
			err = -ENOMEM;
		}
	}

	if (!err) {
		enum pipe_transfer_usage usage;

		usage = PIPE_TRANSFER_READ;
		if (enable_write)
			usage |= PIPE_TRANSFER_WRITE;

		assert(!buf->transfer);

		/*
		 * ignore x, y, w and h so that returned addr points at the
		 * start of the buffer
		 */
		*addr = pipe_transfer_map(pm->context, buf->resource,
					  0, 0, usage, 0, 0,
					  buf->resource->width0, buf->resource->height0,
					  &buf->transfer);
		if (*addr == NULL)
			err = -ENOMEM;
	}

	pthread_mutex_unlock(&pm->mutex);

	return err;
}

static void pipe_unmap(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo)
{
	struct pipe_manager *pm = (struct pipe_manager *) drv;
	struct pipe_buffer *buf = (struct pipe_buffer *) bo;

	pthread_mutex_lock(&pm->mutex);

	assert(buf && buf->transfer);

	pipe_transfer_unmap(pm->context, buf->transfer);
	buf->transfer = NULL;

	pm->context->flush(pm->context, NULL, 0);

	pthread_mutex_unlock(&pm->mutex);
}

static void pipe_destroy(struct gralloc_drm_drv_t *drv)
{
	struct pipe_manager *pm = (struct pipe_manager *) drv;

	if (pm->context)
		pm->context->destroy(pm->context);
	pm->screen->destroy(pm->screen);
	FREE(pm);
}

/* for nouveau */
#include "nouveau/drm/nouveau_drm_public.h"
/* for r300 */
#include "radeon/drm/radeon_drm_public.h"
#include "r300/r300_public.h"
/* for r600 */
#include "radeon/drm/radeon_winsys.h"
#include "r600/r600_public.h"
/* for vmwgfx */
#include "svga/drm/svga_drm_public.h"
#include "svga/svga_winsys.h"
#include "svga/svga_public.h"
/* for debug */
#include "target-helpers/inline_debug_helper.h"

static int pipe_init_screen(struct pipe_manager *pm)
{
	struct pipe_screen *screen = NULL;

#ifdef ENABLE_PIPE_NOUVEAU
	if (strcmp(pm->driver, "nouveau") == 0)
		screen = nouveau_drm_screen_create(pm->fd);
#endif
#ifdef ENABLE_PIPE_R300
	if (strcmp(pm->driver, "r300") == 0) {
		struct radeon_winsys *sws = radeon_drm_winsys_create(pm->fd);

		if (sws) {
			screen = r300_screen_create(sws);
			if (!screen)
				sws->destroy(sws);
		}
	}
#endif
#ifdef ENABLE_PIPE_R600
	if (strcmp(pm->driver, "r600") == 0) {
		struct radeon_winsys *sws = radeon_drm_winsys_create(pm->fd);

		if (sws) {
			screen = r600_screen_create(sws);
			if (!screen)
				sws->destroy(sws);
		}
	}
#endif
#ifdef ENABLE_PIPE_VMWGFX
	if (strcmp(pm->driver, "vmwgfx") == 0) {
		struct svga_winsys_screen *sws =
			svga_drm_winsys_screen_create(pm->fd);

		if (sws) {
			screen = svga_screen_create(sws);
			if (!screen)
				sws->destroy(sws);
		}
	}
#endif

	if (!screen) {
		ALOGW("failed to create screen for %s", pm->driver);
		return -EINVAL;
	}

	pm->screen = debug_screen_wrap(screen);

	return 0;
}

#include <xf86drm.h>
#include <i915_drm.h>
#include <radeon_drm.h>
static int pipe_get_pci_id(struct pipe_manager *pm,
		const char *name, int *vendor, int *device)
{
	int err = -EINVAL;

	if (strcmp(name, "i915") == 0) {
		struct drm_i915_getparam gp;

		*vendor = 0x8086;

		memset(&gp, 0, sizeof(gp));
		gp.param = I915_PARAM_CHIPSET_ID;
		gp.value = device;
		err = drmCommandWriteRead(pm->fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
	}
	else if (strcmp(name, "radeon") == 0) {
		struct drm_radeon_info info;

		*vendor = 0x1002;

		memset(&info, 0, sizeof(info));
		info.request = RADEON_INFO_DEVICE_ID;
		info.value = (long) device;
		err = drmCommandWriteRead(pm->fd, DRM_RADEON_INFO, &info, sizeof(info));
	}
	else if (strcmp(name, "nouveau") == 0) {
		*vendor = 0x10de;
		*device = 0;
		err = 0;
	}
	else if (strcmp(name, "vmwgfx") == 0) {
		*vendor = 0x15ad;
		/* assume SVGA II */
		*device = 0x0405;
		err = 0;
	}
	else {
		err = -EINVAL;
	}

	return err;
}

#define DRIVER_MAP_GALLIUM_ONLY
#include "pci_ids/pci_id_driver_map.h"
static int pipe_find_driver(struct pipe_manager *pm, const char *name)
{
	int vendor, device;
	int err;
	const char *driver;

	err = pipe_get_pci_id(pm, name, &vendor, &device);
	if (!err) {
		int idx;

		/* look up in the driver map */
		for (idx = 0; driver_map[idx].driver; idx++) {
			int i;

			if (vendor != driver_map[idx].vendor_id)
				continue;

			if (driver_map[idx].num_chips_ids == -1)
				break;

			for (i = 0; i < driver_map[idx].num_chips_ids; i++) {
				if (driver_map[idx].chip_ids[i] == device)
					break;
			}
			if (i < driver_map[idx].num_chips_ids)
				break;
		}

		driver = driver_map[idx].driver;
		err = (driver) ? 0 : -ENODEV;
	}
	else {
		if (strcmp(name, "vmwgfx") == 0) {
			driver = "vmwgfx";
			err = 0;
		}
	}

	if (!err)
		strncpy(pm->driver, driver, sizeof(pm->driver) - 1);

	return err;
}

struct gralloc_drm_drv_t *gralloc_drm_drv_create_for_pipe(int fd, const char *name)
{
	struct pipe_manager *pm;

	pm = CALLOC(1, sizeof(*pm));
	if (!pm) {
		ALOGE("failed to allocate pipe manager for %s", name);
		return NULL;
	}

	pm->fd = fd;
	pthread_mutex_init(&pm->mutex, NULL);

	if (pipe_find_driver(pm, name)) {
		FREE(pm);
		return NULL;
	}

	if (pipe_init_screen(pm)) {
		FREE(pm);
		return NULL;
	}

	pm->base.destroy = pipe_destroy;
	pm->base.alloc = pipe_alloc;
	pm->base.free = pipe_free;
	pm->base.map = pipe_map;
	pm->base.unmap = pipe_unmap;

	return &pm->base;
}
