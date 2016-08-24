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

#define LOG_TAG "hwc-drm-composition"

#include "drmcomposition.h"
#include "drmcrtc.h"
#include "drmplane.h"
#include "drmresources.h"
#include "platform.h"

#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <sw_sync.h>
#include <sync/sync.h>

namespace android {

DrmComposition::DrmComposition(DrmResources *drm, Importer *importer,
                               Planner *planner)
    : drm_(drm), importer_(importer), planner_(planner) {
  char use_overlay_planes_prop[PROPERTY_VALUE_MAX];
  property_get("hwc.drm.use_overlay_planes", use_overlay_planes_prop, "1");
  bool use_overlay_planes = atoi(use_overlay_planes_prop);

  for (auto &plane : drm->planes()) {
    if (plane->type() == DRM_PLANE_TYPE_PRIMARY)
      primary_planes_.push_back(plane.get());
    else if (use_overlay_planes && plane->type() == DRM_PLANE_TYPE_OVERLAY)
      overlay_planes_.push_back(plane.get());
  }
}

int DrmComposition::Init(uint64_t frame_no) {
  for (auto &conn : drm_->connectors()) {
    int display = conn->display();
    composition_map_[display].reset(new DrmDisplayComposition());
    if (!composition_map_[display]) {
      ALOGE("Failed to allocate new display composition\n");
      return -ENOMEM;
    }

    // If the display hasn't been modeset yet, this will be NULL
    DrmCrtc *crtc = drm_->GetCrtcForDisplay(display);

    int ret = composition_map_[display]->Init(drm_, crtc, importer_, planner_,
                                              frame_no);
    if (ret) {
      ALOGE("Failed to init display composition for %d", display);
      return ret;
    }
  }
  return 0;
}

int DrmComposition::SetLayers(size_t num_displays,
                              DrmCompositionDisplayLayersMap *maps) {
  int ret = 0;
  for (size_t display_index = 0; display_index < num_displays;
       display_index++) {
    DrmCompositionDisplayLayersMap &map = maps[display_index];
    int display = map.display;

    if (!drm_->GetConnectorForDisplay(display)) {
      ALOGE("Invalid display given to SetLayers %d", display);
      continue;
    }

    ret = composition_map_[display]->SetLayers(
        map.layers.data(), map.layers.size(), map.geometry_changed);
    if (ret)
      return ret;
  }

  return 0;
}

int DrmComposition::SetDpmsMode(int display, uint32_t dpms_mode) {
  return composition_map_[display]->SetDpmsMode(dpms_mode);
}

int DrmComposition::SetDisplayMode(int display, const DrmMode &display_mode) {
  return composition_map_[display]->SetDisplayMode(display_mode);
}

std::unique_ptr<DrmDisplayComposition> DrmComposition::TakeDisplayComposition(
    int display) {
  return std::move(composition_map_[display]);
}

int DrmComposition::Plan(std::map<int, DrmDisplayCompositor> &compositor_map) {
  int ret = 0;
  for (auto &conn : drm_->connectors()) {
    int display = conn->display();
    DrmDisplayComposition *comp = GetDisplayComposition(display);
    ret = comp->Plan(compositor_map[display].squash_state(), &primary_planes_,
                     &overlay_planes_);
    if (ret) {
      ALOGE("Failed to plan composition for dislay %d", display);
      return ret;
    }
  }

  return 0;
}

int DrmComposition::DisableUnusedPlanes() {
  for (auto &conn : drm_->connectors()) {
    int display = conn->display();
    DrmDisplayComposition *comp = GetDisplayComposition(display);

    /*
     * Leave empty compositions alone
     * TODO: re-visit this and potentially disable leftover planes after the
     *       active compositions have gobbled up all they can
     */
    if (comp->type() == DRM_COMPOSITION_TYPE_EMPTY ||
        comp->type() == DRM_COMPOSITION_TYPE_MODESET)
      continue;

    DrmCrtc *crtc = drm_->GetCrtcForDisplay(display);
    if (!crtc) {
      ALOGE("Failed to find crtc for display %d", display);
      continue;
    }

    for (std::vector<DrmPlane *>::iterator iter = primary_planes_.begin();
         iter != primary_planes_.end(); ++iter) {
      if ((*iter)->GetCrtcSupported(*crtc)) {
        comp->AddPlaneDisable(*iter);
        primary_planes_.erase(iter);
        break;
      }
    }
    for (std::vector<DrmPlane *>::iterator iter = overlay_planes_.begin();
         iter != overlay_planes_.end();) {
      if ((*iter)->GetCrtcSupported(*crtc)) {
        comp->AddPlaneDisable(*iter);
        iter = overlay_planes_.erase(iter);
      } else {
        iter++;
      }
    }
  }
  return 0;
}

DrmDisplayComposition *DrmComposition::GetDisplayComposition(int display) {
  return composition_map_[display].get();
}
}
