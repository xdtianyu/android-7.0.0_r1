#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#include "bo.h"
#include "dev.h"

#define MAKE_YUV_601_Y(r, g, b) \
	((( 66 * (r) + 129 * (g) +  25 * (b) + 128) >> 8) + 16)
#define MAKE_YUV_601_U(r, g, b) \
	(((-38 * (r) -  74 * (g) + 112 * (b) + 128) >> 8) + 128)
#define MAKE_YUV_601_V(r, g, b) \
	(((112 * (r) -  94 * (g) -  18 * (b) + 128) >> 8) + 128)

static void draw_rect_yuv(struct sp_bo *bo, uint32_t x, uint32_t y, uint32_t width,
		uint32_t height, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t i, j, xmax = x + width, ymax = y + height;

	if (xmax > bo->width)
		xmax = bo->width;
	if (ymax > bo->height)
		ymax = bo->height;

	for (i = y; i < ymax; i++) {
		uint8_t *luma = bo->map_addr + i * bo->pitch;

		for (j = x; j < xmax; j++)
			luma[j] = MAKE_YUV_601_Y(r, g, b);
	}

	for (i = y; i < ymax / 2; i++) {
		uint8_t *chroma = bo->map_addr + (i + height) * bo->pitch;

		for (j = x; j < xmax / 2; j++) {
			chroma[j*2] = MAKE_YUV_601_U(r, g, b);
			chroma[j*2 + 1] = MAKE_YUV_601_V(r, g, b);
		}
	}
}

void fill_bo(struct sp_bo *bo, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	if (bo->format == DRM_FORMAT_NV12)
		draw_rect_yuv(bo, 0, 0, bo->width, bo->height, a, r, g, b);
	else
		draw_rect(bo, 0, 0, bo->width, bo->height, a, r, g, b);
}

void draw_rect(struct sp_bo *bo, uint32_t x, uint32_t y, uint32_t width,
		uint32_t height, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t i, j, xmax = x + width, ymax = y + height;

	if (xmax > bo->width)
		xmax = bo->width;
	if (ymax > bo->height)
		ymax = bo->height;

	for (i = y; i < ymax; i++) {
		uint8_t *row = bo->map_addr + i * bo->pitch;

		for (j = x; j < xmax; j++) {
			uint8_t *pixel = row + j * 4;

			if (bo->format == DRM_FORMAT_ARGB8888 ||
			    bo->format == DRM_FORMAT_XRGB8888)
			{
				pixel[0] = b;
				pixel[1] = g;
				pixel[2] = r;
				pixel[3] = a;
			} else if (bo->format == DRM_FORMAT_RGBA8888) {
				pixel[0] = r;
				pixel[1] = g;
				pixel[2] = b;
				pixel[3] = a;
			}
		}
	}
}

static int add_fb_sp_bo(struct sp_bo *bo, uint32_t format)
{
	int ret;
	uint32_t handles[4], pitches[4], offsets[4];

	handles[0] = bo->handle;
	pitches[0] = bo->pitch;
	offsets[0] = 0;
	if (bo->format == DRM_FORMAT_NV12) {
		handles[1] = bo->handle;
		pitches[1] = pitches[0];
		offsets[1] = pitches[0] * bo->height;
	}

	ret = drmModeAddFB2(bo->dev->fd, bo->width, bo->height,
			format, handles, pitches, offsets,
			&bo->fb_id, bo->flags);
	if (ret) {
		printf("failed to create fb ret=%d\n", ret);
		return ret;
	}
	return 0;
}

static int map_sp_bo(struct sp_bo *bo)
{
	int ret;
	struct drm_mode_map_dumb md;

	if (bo->map_addr)
		return 0;

	md.handle = bo->handle;
	ret = drmIoctl(bo->dev->fd, DRM_IOCTL_MODE_MAP_DUMB, &md);
	if (ret) {
		printf("failed to map sp_bo ret=%d\n", ret);
		return ret;
	}

	bo->map_addr = mmap(NULL, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
				bo->dev->fd, md.offset);
	if (bo->map_addr == MAP_FAILED) {
		printf("failed to map bo ret=%d\n", -errno);
		return -errno;
	}
	return 0;
}

static int format_to_bpp(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_NV12:
		return 8;
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_RGBA8888:
	default:
		return 32;
	}
}

struct sp_bo *create_sp_bo(struct sp_dev *dev, uint32_t width, uint32_t height,
		uint32_t depth, uint32_t format, uint32_t flags)
{
	int ret;
	struct drm_mode_create_dumb cd;
	struct sp_bo *bo;

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return NULL;

	if (format == DRM_FORMAT_NV12)
		cd.height = height * 3 / 2;
	else
		cd.height = height;

	cd.width = width;
	cd.bpp = format_to_bpp(format);
	cd.flags = flags;

	ret = drmIoctl(dev->fd, DRM_IOCTL_MODE_CREATE_DUMB, &cd);
	if (ret) {
		printf("failed to create sp_bo %d\n", ret);
		goto err;
	}

	bo->dev = dev;
	bo->width = width;
	bo->height = height;
	bo->depth = depth;
	bo->bpp = format_to_bpp(format);
	bo->format = format;
	bo->flags = flags;

	bo->handle = cd.handle;
	bo->pitch = cd.pitch;
	bo->size = cd.size;

	ret = add_fb_sp_bo(bo, format);
	if (ret) {
		printf("failed to add fb ret=%d\n", ret);
		goto err;
	}

	ret = map_sp_bo(bo);
	if (ret) {
		printf("failed to map bo ret=%d\n", ret);
		goto err;
	}

	return bo;

err:
	free_sp_bo(bo);
	return NULL;
}

void free_sp_bo(struct sp_bo *bo)
{
	int ret;
	struct drm_mode_destroy_dumb dd;

	if (!bo)
		return;

	if (bo->map_addr)
		munmap(bo->map_addr, bo->size);

	if (bo->fb_id) {
		ret = drmModeRmFB(bo->dev->fd, bo->fb_id);
		if (ret)
			printf("Failed to rmfb ret=%d!\n", ret);
	}

	if (bo->handle) {
		dd.handle = bo->handle;
		ret = drmIoctl(bo->dev->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dd);
		if (ret)
			printf("Failed to destroy buffer ret=%d\n", ret);
	}

	free(bo);
}
