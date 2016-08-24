#define LOG_TAG "GRALLOC-ROCKCHIP"

#include <cutils/log.h>
#include <stdlib.h>
#include <errno.h>
#include <drm.h>
#include <rockchip/rockchip_drmif.h>

#include "gralloc_drm.h"
#include "gralloc_drm_priv.h"

#define UNUSED(...) (void)(__VA_ARGS__)

struct rockchip_info {
	struct gralloc_drm_drv_t base;

	struct rockchip_device *rockchip;
	int fd;
};

struct rockchip_buffer {
	struct gralloc_drm_bo_t base;

	struct rockchip_bo *bo;
};

static void drm_gem_rockchip_destroy(struct gralloc_drm_drv_t *drv)
{
	struct rockchip_info *info = (struct rockchip_info *)drv;

	if (info->rockchip)
		rockchip_device_destroy(info->rockchip);
	free(info);
}

static struct gralloc_drm_bo_t *drm_gem_rockchip_alloc(
		struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_handle_t *handle)
{
	struct rockchip_info *info = (struct rockchip_info *)drv;
	struct rockchip_buffer *buf;
	struct drm_gem_close args;
	int ret, cpp, pitch, aligned_width, aligned_height;
	uint32_t size, gem_handle;

	buf = calloc(1, sizeof(*buf));
	if (!buf) {
		ALOGE("Failed to allocate buffer wrapper\n");
		return NULL;
	}

	cpp = gralloc_drm_get_bpp(handle->format);
	if (!cpp) {
		ALOGE("unrecognized format 0x%x", handle->format);
		return NULL;
	}

	aligned_width = handle->width;
	aligned_height = handle->height;
	gralloc_drm_align_geometry(handle->format,
			&aligned_width, &aligned_height);

	/* TODO: We need to sort out alignment */
	pitch = ALIGN(aligned_width * cpp, 64);
	size = aligned_height * pitch;

	if (handle->format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
		/*
		 * WAR for H264 decoder requiring additional space
		 * at the end of destination buffers.
		 */
		uint32_t w_mbs, h_mbs;

		w_mbs = ALIGN(handle->width, 16) / 16;
		h_mbs = ALIGN(handle->height, 16) / 16;
		size += 64 * w_mbs * h_mbs;
	}

	if (handle->prime_fd >= 0) {
		ret = drmPrimeFDToHandle(info->fd, handle->prime_fd,
			&gem_handle);
		if (ret) {
			char *c = NULL;
			ALOGE("failed to convert prime fd to handle %d ret=%d",
				handle->prime_fd, ret);
			*c = 0;
			goto err;
		}
		ALOGV("Got handle %d for fd %d\n", gem_handle, handle->prime_fd);

		buf->bo = rockchip_bo_from_handle(info->rockchip, gem_handle,
			0, size);
		if (!buf->bo) {
			ALOGE("failed to wrap bo handle=%d size=%d\n",
				gem_handle, size);

			memset(&args, 0, sizeof(args));
			args.handle = gem_handle;
			drmIoctl(info->fd, DRM_IOCTL_GEM_CLOSE, &args);
			return NULL;
		}
	} else {
		buf->bo = rockchip_bo_create(info->rockchip, size, 0);
		if (!buf->bo) {
			ALOGE("failed to allocate bo %dx%dx%dx%d\n",
				handle->height, pitch, cpp, size);
			goto err;
		}

		gem_handle = rockchip_bo_handle(buf->bo);
		ret = drmPrimeHandleToFD(info->fd, gem_handle, 0,
			&handle->prime_fd);
		ALOGV("Got fd %d for handle %d\n", handle->prime_fd, gem_handle);
		if (ret) {
			ALOGE("failed to get prime fd %d", ret);
			goto err_unref;
		}

		buf->base.fb_handle = gem_handle;
	}

	handle->name = 0;
	handle->stride = pitch;
	buf->base.handle = handle;

	return &buf->base;

err_unref:
	rockchip_bo_destroy(buf->bo);
err:
	free(buf);
	return NULL;
}

static void drm_gem_rockchip_free(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo)
{
	struct rockchip_buffer *buf = (struct rockchip_buffer *)bo;

	UNUSED(drv);

	if (bo->handle && bo->handle->prime_fd)
		close(bo->handle->prime_fd);

	/* TODO: Is destroy correct here? */
	rockchip_bo_destroy(buf->bo);
	free(buf);
}

static int drm_gem_rockchip_map(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo, int x, int y, int w, int h,
		int enable_write, void **addr)
{
	struct rockchip_buffer *buf = (struct rockchip_buffer *)bo;

	UNUSED(drv, x, y, w, h, enable_write);

	*addr = rockchip_bo_map(buf->bo);
	if (!*addr) {
		ALOGE("failed to map bo\n");
		return -1;
	}

	return 0;
}

static void drm_gem_rockchip_unmap(struct gralloc_drm_drv_t *drv,
		struct gralloc_drm_bo_t *bo)
{
	UNUSED(drv, bo);
}

struct gralloc_drm_drv_t *gralloc_drm_drv_create_for_rockchip(int fd)
{
	struct rockchip_info *info;
	int ret;

	info = calloc(1, sizeof(*info));
	if (!info) {
		ALOGE("Failed to allocate rockchip gralloc device\n");
		return NULL;
	}

	info->rockchip = rockchip_device_create(fd);
	if (!info->rockchip) {
		ALOGE("Failed to create new rockchip instance\n");
		free(info);
		return NULL;
	}

	info->fd = fd;
	info->base.destroy = drm_gem_rockchip_destroy;
	info->base.alloc = drm_gem_rockchip_alloc;
	info->base.free = drm_gem_rockchip_free;
	info->base.map = drm_gem_rockchip_map;
	info->base.unmap = drm_gem_rockchip_unmap;

	return &info->base;
}
