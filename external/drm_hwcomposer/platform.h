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

#ifndef ANDROID_DRM_PLATFORM_H_
#define ANDROID_DRM_PLATFORM_H_

#include "drmdisplaycomposition.h"
#include "drmhwcomposer.h"

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>

#include <map>
#include <vector>

namespace android {

class DrmResources;

class Importer {
 public:
  virtual ~Importer() {
  }

  // Creates a platform-specific importer instance
  static Importer *CreateInstance(DrmResources *drm);

  // Imports the buffer referred to by handle into bo.
  //
  // Note: This can be called from a different thread than ReleaseBuffer. The
  //       implementation is responsible for ensuring thread safety.
  virtual int ImportBuffer(buffer_handle_t handle, hwc_drm_bo_t *bo) = 0;

  // Releases the buffer object (ie: does the inverse of ImportBuffer)
  //
  // Note: This can be called from a different thread than ImportBuffer. The
  //       implementation is responsible for ensuring thread safety.
  virtual int ReleaseBuffer(hwc_drm_bo_t *bo) = 0;
};

class Planner {
 public:
  class PlanStage {
   public:
    virtual ~PlanStage() {
    }

    virtual int ProvisionPlanes(std::vector<DrmCompositionPlane> *composition,
                                std::map<size_t, DrmHwcLayer *> &layers,
                                DrmCrtc *crtc,
                                std::vector<DrmPlane *> *planes) = 0;

   protected:
    // Removes and returns the next available plane from planes
    static DrmPlane *PopPlane(std::vector<DrmPlane *> *planes) {
      if (planes->empty())
        return NULL;
      DrmPlane *plane = planes->front();
      planes->erase(planes->begin());
      return plane;
    }

    // Finds and returns the squash layer from the composition
    static DrmCompositionPlane *GetPrecomp(
        std::vector<DrmCompositionPlane> *composition) {
      auto l = GetPrecompIter(composition);
      if (l == composition->end())
        return NULL;
      return &(*l);
    }

    // Inserts the given layer:plane in the composition right before the precomp
    // layer
    static int Emplace(std::vector<DrmCompositionPlane> *composition,
                       std::vector<DrmPlane *> *planes,
                       DrmCompositionPlane::Type type, DrmCrtc *crtc,
                       size_t source_layer) {
      DrmPlane *plane = PopPlane(planes);
      if (!plane)
        return -ENOENT;

      auto precomp = GetPrecompIter(composition);
      composition->emplace(precomp, type, plane, crtc, source_layer);
      return 0;
    }

   private:
    static std::vector<DrmCompositionPlane>::iterator GetPrecompIter(
        std::vector<DrmCompositionPlane> *composition) {
      return std::find_if(composition->begin(), composition->end(),
                          [](const DrmCompositionPlane &p) {
        return p.type() == DrmCompositionPlane::Type::kPrecomp;
      });
    }
  };

  // Creates a planner instance with platform-specific planning stages
  static std::unique_ptr<Planner> CreateInstance(DrmResources *drm);

  // Takes a stack of layers and provisions hardware planes for them. If the
  // entire stack can't fit in hardware, the Planner may place the remaining
  // layers in a PRECOMP plane. Layers in the PRECOMP plane will be composited
  // using GL. PRECOMP planes should be placed above any 1:1 layer:plane
  // compositions. If use_squash_fb is true, the Planner should try to reserve a
  // plane at the highest z-order with type SQUASH.
  //
  // @layers: a map of index:layer of layers to composite
  // @use_squash_fb: reserve a squash framebuffer
  // @primary_planes: a vector of primary planes available for this frame
  // @overlay_planes: a vector of overlay planes available for this frame
  //
  // Returns: A tuple with the status of the operation (0 for success) and
  //          a vector of the resulting plan (ie: layer->plane mapping).
  std::tuple<int, std::vector<DrmCompositionPlane>> ProvisionPlanes(
      std::map<size_t, DrmHwcLayer *> &layers, bool use_squash_fb,
      DrmCrtc *crtc, std::vector<DrmPlane *> *primary_planes,
      std::vector<DrmPlane *> *overlay_planes);

  template <typename T, typename... A>
  void AddStage(A &&... args) {
    stages_.emplace_back(
        std::unique_ptr<PlanStage>(new T(std::forward(args)...)));
  }

 private:
  std::vector<DrmPlane *> GetUsablePlanes(
      DrmCrtc *crtc, std::vector<DrmPlane *> *primary_planes,
      std::vector<DrmPlane *> *overlay_planes);

  std::vector<std::unique_ptr<PlanStage>> stages_;
};

// This plan stage extracts all protected layers and places them on dedicated
// planes.
class PlanStageProtected : public Planner::PlanStage {
 public:
  int ProvisionPlanes(std::vector<DrmCompositionPlane> *composition,
                      std::map<size_t, DrmHwcLayer *> &layers, DrmCrtc *crtc,
                      std::vector<DrmPlane *> *planes);
};

// This plan stage places as many layers on dedicated planes as possible (first
// come first serve), and then sticks the rest in a precomposition plane (if
// needed).
class PlanStageGreedy : public Planner::PlanStage {
 public:
  int ProvisionPlanes(std::vector<DrmCompositionPlane> *composition,
                      std::map<size_t, DrmHwcLayer *> &layers, DrmCrtc *crtc,
                      std::vector<DrmPlane *> *planes);
};
}
#endif
