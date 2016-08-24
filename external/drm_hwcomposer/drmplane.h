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

#ifndef ANDROID_DRM_PLANE_H_
#define ANDROID_DRM_PLANE_H_

#include "drmcrtc.h"
#include "drmproperty.h"

#include <stdint.h>
#include <xf86drmMode.h>
#include <vector>

namespace android {

class DrmResources;

class DrmPlane {
 public:
  DrmPlane(DrmResources *drm, drmModePlanePtr p);
  DrmPlane(const DrmPlane &) = delete;
  DrmPlane &operator=(const DrmPlane &) = delete;

  int Init();

  uint32_t id() const;

  bool GetCrtcSupported(const DrmCrtc &crtc) const;

  uint32_t type() const;

  const DrmProperty &crtc_property() const;
  const DrmProperty &fb_property() const;
  const DrmProperty &crtc_x_property() const;
  const DrmProperty &crtc_y_property() const;
  const DrmProperty &crtc_w_property() const;
  const DrmProperty &crtc_h_property() const;
  const DrmProperty &src_x_property() const;
  const DrmProperty &src_y_property() const;
  const DrmProperty &src_w_property() const;
  const DrmProperty &src_h_property() const;
  const DrmProperty &rotation_property() const;
  const DrmProperty &alpha_property() const;

 private:
  DrmResources *drm_;
  uint32_t id_;

  uint32_t possible_crtc_mask_;

  uint32_t type_;

  DrmProperty crtc_property_;
  DrmProperty fb_property_;
  DrmProperty crtc_x_property_;
  DrmProperty crtc_y_property_;
  DrmProperty crtc_w_property_;
  DrmProperty crtc_h_property_;
  DrmProperty src_x_property_;
  DrmProperty src_y_property_;
  DrmProperty src_w_property_;
  DrmProperty src_h_property_;
  DrmProperty rotation_property_;
  DrmProperty alpha_property_;
};
}

#endif  // ANDROID_DRM_PLANE_H_
