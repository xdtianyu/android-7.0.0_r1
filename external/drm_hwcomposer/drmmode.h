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

#ifndef ANDROID_DRM_MODE_H_
#define ANDROID_DRM_MODE_H_

#include <stdint.h>
#include <string>
#include <xf86drmMode.h>

namespace android {

class DrmMode {
 public:
  DrmMode() = default;
  DrmMode(drmModeModeInfoPtr m);

  bool operator==(const drmModeModeInfo &m) const;
  void ToDrmModeModeInfo(drm_mode_modeinfo *m) const;

  uint32_t id() const;
  void set_id(uint32_t id);

  uint32_t clock() const;

  uint32_t h_display() const;
  uint32_t h_sync_start() const;
  uint32_t h_sync_end() const;
  uint32_t h_total() const;
  uint32_t h_skew() const;

  uint32_t v_display() const;
  uint32_t v_sync_start() const;
  uint32_t v_sync_end() const;
  uint32_t v_total() const;
  uint32_t v_scan() const;
  float v_refresh() const;

  uint32_t flags() const;
  uint32_t type() const;

  std::string name() const;

 private:
  uint32_t id_ = 0;

  uint32_t clock_ = 0;

  uint32_t h_display_ = 0;
  uint32_t h_sync_start_ = 0;
  uint32_t h_sync_end_ = 0;
  uint32_t h_total_ = 0;
  uint32_t h_skew_ = 0;

  uint32_t v_display_ = 0;
  uint32_t v_sync_start_ = 0;
  uint32_t v_sync_end_ = 0;
  uint32_t v_total_ = 0;
  uint32_t v_scan_ = 0;
  uint32_t v_refresh_ = 0;

  uint32_t flags_ = 0;
  uint32_t type_ = 0;

  std::string name_;
};
}

#endif  // ANDROID_DRM_MODE_H_
