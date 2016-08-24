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

#include "drmmode.h"
#include "drmresources.h"

#include <stdint.h>
#include <string>
#include <xf86drmMode.h>

namespace android {

DrmMode::DrmMode(drmModeModeInfoPtr m)
    : id_(0),
      clock_(m->clock),
      h_display_(m->hdisplay),
      h_sync_start_(m->hsync_start),
      h_sync_end_(m->hsync_end),
      h_total_(m->htotal),
      h_skew_(m->hskew),
      v_display_(m->vdisplay),
      v_sync_start_(m->vsync_start),
      v_sync_end_(m->vsync_end),
      v_total_(m->vtotal),
      v_scan_(m->vscan),
      v_refresh_(m->vrefresh),
      flags_(m->flags),
      type_(m->type),
      name_(m->name) {
}

bool DrmMode::operator==(const drmModeModeInfo &m) const {
  return clock_ == m.clock && h_display_ == m.hdisplay &&
         h_sync_start_ == m.hsync_start && h_sync_end_ == m.hsync_end &&
         h_total_ == m.htotal && h_skew_ == m.hskew &&
         v_display_ == m.vdisplay && v_sync_start_ == m.vsync_start &&
         v_sync_end_ == m.vsync_end && v_total_ == m.vtotal &&
         v_scan_ == m.vscan && flags_ == m.flags && type_ == m.type;
}

void DrmMode::ToDrmModeModeInfo(drm_mode_modeinfo *m) const {
  m->clock = clock_;
  m->hdisplay = h_display_;
  m->hsync_start = h_sync_start_;
  m->hsync_end = h_sync_end_;
  m->htotal = h_total_;
  m->hskew = h_skew_;
  m->vdisplay = v_display_;
  m->vsync_start = v_sync_start_;
  m->vsync_end = v_sync_end_;
  m->vtotal = v_total_;
  m->vscan = v_scan_;
  m->vrefresh = v_refresh_;
  m->flags = flags_;
  m->type = type_;
  strncpy(m->name, name_.c_str(), DRM_DISPLAY_MODE_LEN);
}

uint32_t DrmMode::id() const {
  return id_;
}

void DrmMode::set_id(uint32_t id) {
  id_ = id;
}

uint32_t DrmMode::clock() const {
  return clock_;
}

uint32_t DrmMode::h_display() const {
  return h_display_;
}

uint32_t DrmMode::h_sync_start() const {
  return h_sync_start_;
}

uint32_t DrmMode::h_sync_end() const {
  return h_sync_end_;
}

uint32_t DrmMode::h_total() const {
  return h_total_;
}

uint32_t DrmMode::h_skew() const {
  return h_skew_;
}

uint32_t DrmMode::v_display() const {
  return v_display_;
}

uint32_t DrmMode::v_sync_start() const {
  return v_sync_start_;
}

uint32_t DrmMode::v_sync_end() const {
  return v_sync_end_;
}

uint32_t DrmMode::v_total() const {
  return v_total_;
}

uint32_t DrmMode::v_scan() const {
  return v_scan_;
}

float DrmMode::v_refresh() const {
  return v_refresh_ ? v_refresh_ * 1.0f :
                      clock_ / (float)(v_total_ * h_total_) * 1000.0f;
}

uint32_t DrmMode::flags() const {
  return flags_;
}

uint32_t DrmMode::type() const {
  return type_;
}

std::string DrmMode::name() const {
  return name_;
}
}
