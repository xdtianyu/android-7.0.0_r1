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

#ifndef ANDROID_PLATFORM_NV_H_
#define ANDROID_PLATFORM_NV_H_

#include "drmresources.h"
#include "platform.h"
#include "platformdrmgeneric.h"

#include <stdatomic.h>

#include <hardware/gralloc.h>

namespace android {

class NvImporter : public Importer {
 public:
  NvImporter(DrmResources *drm);
  ~NvImporter() override;

  int Init();

  int ImportBuffer(buffer_handle_t handle, hwc_drm_bo_t *bo) override;
  int ReleaseBuffer(hwc_drm_bo_t *bo) override;

 private:
  typedef struct NvBuffer {
    NvImporter *importer;
    hwc_drm_bo_t bo;
    atomic_int ref;
  } NvBuffer_t;

  static void NvGrallocRelease(void *nv_buffer);
  void ReleaseBufferImpl(hwc_drm_bo_t *bo);

  NvBuffer_t *GrallocGetNvBuffer(buffer_handle_t handle);
  int GrallocSetNvBuffer(buffer_handle_t handle, NvBuffer_t *buf);

  DrmResources *drm_;

  const gralloc_module_t *gralloc_;
};

// This stage looks for any layers that contain transformed protected content
// and puts it in the primary plane since Tegra doesn't support planar rotation
// on the overlay planes.
//
// There are two caveats to this approach: 1- Protected content isn't
// necessarily planar, but it's usually a safe bet, and 2- This doesn't catch
// non-protected planar content. If we wanted to fix this, we'd need to import
// the buffer in this stage and peek at it's format. The overhead of doing this
// doesn't seem worth it since we'll end up displaying the right thing in both
// cases anyways.
class PlanStageProtectedRotated : public Planner::PlanStage {
 public:
  int ProvisionPlanes(std::vector<DrmCompositionPlane> *composition,
                      std::map<size_t, DrmHwcLayer *> &layers, DrmCrtc *crtc,
                      std::vector<DrmPlane *> *planes);
};
}

#endif
