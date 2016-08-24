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

#ifndef ANDROID_DRM_HWCOMPOSER_H_
#define ANDROID_DRM_HWCOMPOSER_H_

#include <stdbool.h>
#include <stdint.h>

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include "autofd.h"
#include "separate_rects.h"
#include "drmhwcgralloc.h"

struct hwc_import_context;

int hwc_import_init(struct hwc_import_context **ctx);
int hwc_import_destroy(struct hwc_import_context *ctx);

int hwc_import_bo_create(int fd, struct hwc_import_context *ctx,
                         buffer_handle_t buf, struct hwc_drm_bo *bo);
bool hwc_import_bo_release(int fd, struct hwc_import_context *ctx,
                           struct hwc_drm_bo *bo);

namespace android {

class Importer;

class DrmHwcBuffer {
 public:
  DrmHwcBuffer() = default;
  DrmHwcBuffer(const hwc_drm_bo &bo, Importer *importer)
      : bo_(bo), importer_(importer) {
  }
  DrmHwcBuffer(DrmHwcBuffer &&rhs) : bo_(rhs.bo_), importer_(rhs.importer_) {
    rhs.importer_ = NULL;
  }

  ~DrmHwcBuffer() {
    Clear();
  }

  DrmHwcBuffer &operator=(DrmHwcBuffer &&rhs) {
    Clear();
    importer_ = rhs.importer_;
    rhs.importer_ = NULL;
    bo_ = rhs.bo_;
    return *this;
  }

  operator bool() const {
    return importer_ != NULL;
  }

  const hwc_drm_bo *operator->() const;

  void Clear();

  int ImportBuffer(buffer_handle_t handle, Importer *importer);

 private:
  hwc_drm_bo bo_;
  Importer *importer_ = NULL;
};

class DrmHwcNativeHandle {
 public:
  DrmHwcNativeHandle() = default;

  DrmHwcNativeHandle(const gralloc_module_t *gralloc, native_handle_t *handle)
      : gralloc_(gralloc), handle_(handle) {
  }

  DrmHwcNativeHandle(DrmHwcNativeHandle &&rhs) {
    gralloc_ = rhs.gralloc_;
    rhs.gralloc_ = NULL;
    handle_ = rhs.handle_;
    rhs.handle_ = NULL;
  }

  ~DrmHwcNativeHandle();

  DrmHwcNativeHandle &operator=(DrmHwcNativeHandle &&rhs) {
    Clear();
    gralloc_ = rhs.gralloc_;
    rhs.gralloc_ = NULL;
    handle_ = rhs.handle_;
    rhs.handle_ = NULL;
    return *this;
  }

  int CopyBufferHandle(buffer_handle_t handle, const gralloc_module_t *gralloc);

  void Clear();

  buffer_handle_t get() const {
    return handle_;
  }

 private:
  const gralloc_module_t *gralloc_ = NULL;
  native_handle_t *handle_ = NULL;
};

template <typename T>
using DrmHwcRect = separate_rects::Rect<T>;

enum DrmHwcTransform {
  kIdentity = 0,
  kFlipH = 1 << 0,
  kFlipV = 1 << 1,
  kRotate90 = 1 << 2,
  kRotate180 = 1 << 3,
  kRotate270 = 1 << 4,
};

enum class DrmHwcBlending : int32_t {
  kNone = HWC_BLENDING_NONE,
  kPreMult = HWC_BLENDING_PREMULT,
  kCoverage = HWC_BLENDING_COVERAGE,
};

struct DrmHwcLayer {
  buffer_handle_t sf_handle = NULL;
  int gralloc_buffer_usage = 0;
  DrmHwcBuffer buffer;
  DrmHwcNativeHandle handle;
  uint32_t transform;
  DrmHwcBlending blending = DrmHwcBlending::kNone;
  uint8_t alpha = 0xff;
  DrmHwcRect<float> source_crop;
  DrmHwcRect<int> display_frame;
  std::vector<DrmHwcRect<int>> source_damage;

  UniqueFd acquire_fence;
  OutputFd release_fence;

  int InitFromHwcLayer(hwc_layer_1_t *sf_layer, Importer *importer,
                       const gralloc_module_t *gralloc);

  buffer_handle_t get_usable_handle() const {
    return handle.get() != NULL ? handle.get() : sf_handle;
  }

  bool protected_usage() const {
    return (gralloc_buffer_usage & GRALLOC_USAGE_PROTECTED) ==
           GRALLOC_USAGE_PROTECTED;
  }
};

struct DrmHwcDisplayContents {
  OutputFd retire_fence;
  std::vector<DrmHwcLayer> layers;
};
}

#endif
