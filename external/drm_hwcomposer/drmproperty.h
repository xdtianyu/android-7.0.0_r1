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

#ifndef ANDROID_DRM_PROPERTY_H_
#define ANDROID_DRM_PROPERTY_H_

#include <stdint.h>
#include <string>
#include <xf86drmMode.h>
#include <vector>

namespace android {

enum DrmPropertyType {
  DRM_PROPERTY_TYPE_INT,
  DRM_PROPERTY_TYPE_ENUM,
  DRM_PROPERTY_TYPE_OBJECT,
  DRM_PROPERTY_TYPE_BLOB,
  DRM_PROPERTY_TYPE_INVALID,
};

class DrmProperty {
 public:
  DrmProperty() = default;
  DrmProperty(drmModePropertyPtr p, uint64_t value);
  DrmProperty(const DrmProperty &) = delete;
  DrmProperty &operator=(const DrmProperty &) = delete;

  void Init(drmModePropertyPtr p, uint64_t value);

  uint32_t id() const;
  std::string name() const;

  int value(uint64_t *value) const;

 private:
  class DrmPropertyEnum {
   public:
    DrmPropertyEnum(drm_mode_property_enum *e);
    ~DrmPropertyEnum();

    uint64_t value_;
    std::string name_;
  };

  uint32_t id_ = 0;

  DrmPropertyType type_ = DRM_PROPERTY_TYPE_INVALID;
  uint32_t flags_ = 0;
  std::string name_;
  uint64_t value_ = 0;

  std::vector<uint64_t> values_;
  std::vector<DrmPropertyEnum> enums_;
  std::vector<uint32_t> blob_ids_;
};
}

#endif  // ANDROID_DRM_PROPERTY_H_
