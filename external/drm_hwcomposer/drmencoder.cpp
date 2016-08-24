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

#include "drmcrtc.h"
#include "drmencoder.h"
#include "drmresources.h"

#include <stdint.h>
#include <xf86drmMode.h>

namespace android {

DrmEncoder::DrmEncoder(drmModeEncoderPtr e, DrmCrtc *current_crtc,
                       const std::vector<DrmCrtc *> &possible_crtcs)
    : id_(e->encoder_id),
      crtc_(current_crtc),
      type_(e->encoder_type),
      possible_crtcs_(possible_crtcs) {
}

uint32_t DrmEncoder::id() const {
  return id_;
}

DrmCrtc *DrmEncoder::crtc() const {
  return crtc_;
}

void DrmEncoder::set_crtc(DrmCrtc *crtc) {
  crtc_ = crtc;
}
}
