/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "hwc-platform-drm-generic"

#include "drmresources.h"
#include "platform.h"
#include "platformdrmgeneric.h"

#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <cutils/log.h>
#include <gralloc_drm.h>
#include <gralloc_drm_priv.h>
#include <gralloc_drm_handle.h>
#include <hardware/gralloc.h>

namespace android {

#ifdef USE_DRM_GENERIC_IMPORTER
// static
Importer *Importer::CreateInstance(DrmResources *drm) {
  DrmGenericImporter *importer = new DrmGenericImporter(drm);
  if (!importer)
    return NULL;

  int ret = importer->Init();
  if (ret) {
    ALOGE("Failed to initialize the nv importer %d", ret);
    delete importer;
    return NULL;
  }
  return importer;
}
#endif

DrmGenericImporter::DrmGenericImporter(DrmResources *drm) : drm_(drm) {
}

DrmGenericImporter::~DrmGenericImporter() {
}

int DrmGenericImporter::Init() {
  int ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
                          (const hw_module_t **)&gralloc_);
  if (ret) {
    ALOGE("Failed to open gralloc module");
    return ret;
  }
  return 0;
}

uint32_t DrmGenericImporter::ConvertHalFormatToDrm(uint32_t hal_format) {
  switch (hal_format) {
    case HAL_PIXEL_FORMAT_RGB_888:
      return DRM_FORMAT_BGR888;
    case HAL_PIXEL_FORMAT_BGRA_8888:
      return DRM_FORMAT_ARGB8888;
    case HAL_PIXEL_FORMAT_RGBX_8888:
      return DRM_FORMAT_XBGR8888;
    case HAL_PIXEL_FORMAT_RGBA_8888:
      return DRM_FORMAT_ABGR8888;
    case HAL_PIXEL_FORMAT_RGB_565:
      return DRM_FORMAT_BGR565;
    case HAL_PIXEL_FORMAT_YV12:
      return DRM_FORMAT_YVU420;
    default:
      ALOGE("Cannot convert hal format to drm format %u", hal_format);
      return -EINVAL;
  }
}

int DrmGenericImporter::ImportBuffer(buffer_handle_t handle, hwc_drm_bo_t *bo) {
  gralloc_drm_handle_t *gr_handle = gralloc_drm_handle(handle);
  if (!gr_handle)
    return -EINVAL;

  struct gralloc_drm_bo_t *gralloc_bo = gr_handle->data;
  if (!gralloc_bo) {
    ALOGE("Could not get drm bo from handle");
    return -EINVAL;
  }

  uint32_t gem_handle;
  int ret = drmPrimeFDToHandle(drm_->fd(), gr_handle->prime_fd, &gem_handle);
  if (ret) {
    ALOGE("failed to import prime fd %d ret=%d", gr_handle->prime_fd, ret);
    return ret;
  }

  memset(bo, 0, sizeof(hwc_drm_bo_t));
  bo->width = gr_handle->width;
  bo->height = gr_handle->height;
  bo->format = ConvertHalFormatToDrm(gr_handle->format);
  bo->pitches[0] = gr_handle->stride;
  bo->gem_handles[0] = gem_handle;
  bo->offsets[0] = 0;

  ret = drmModeAddFB2(drm_->fd(), bo->width, bo->height, bo->format,
                      bo->gem_handles, bo->pitches, bo->offsets, &bo->fb_id, 0);
  if (ret) {
    ALOGE("could not create drm fb %d", ret);
    return ret;
  }

  return ret;
}

int DrmGenericImporter::ReleaseBuffer(hwc_drm_bo_t *bo) {
  if (bo->fb_id)
    if (drmModeRmFB(drm_->fd(), bo->fb_id))
      ALOGE("Failed to rm fb");

  struct drm_gem_close gem_close;
  memset(&gem_close, 0, sizeof(gem_close));
  int num_gem_handles = sizeof(bo->gem_handles) / sizeof(bo->gem_handles[0]);
  for (int i = 0; i < num_gem_handles; i++) {
    if (!bo->gem_handles[i])
      continue;

    gem_close.handle = bo->gem_handles[i];
    int ret = drmIoctl(drm_->fd(), DRM_IOCTL_GEM_CLOSE, &gem_close);
    if (ret)
      ALOGE("Failed to close gem handle %d %d", i, ret);
    else
      bo->gem_handles[i] = 0;
  }
  return 0;
}

#ifdef USE_DRM_GENERIC_IMPORTER
std::unique_ptr<Planner> Planner::CreateInstance(DrmResources *) {
  std::unique_ptr<Planner> planner(new Planner);
  planner->AddStage<PlanStageGreedy>();
  return planner;
}
#endif
}
