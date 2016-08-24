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

#define LOG_TAG "hwc-drm-display-composition"

#include "drmdisplaycomposition.h"
#include "drmcrtc.h"
#include "drmplane.h"
#include "drmresources.h"
#include "platform.h"

#include <stdlib.h>

#include <algorithm>
#include <unordered_set>

#include <cutils/log.h>
#include <sw_sync.h>
#include <sync/sync.h>
#include <xf86drmMode.h>

namespace android {

DrmDisplayComposition::~DrmDisplayComposition() {
  if (timeline_fd_ >= 0) {
    SignalCompositionDone();
    close(timeline_fd_);
  }
}

int DrmDisplayComposition::Init(DrmResources *drm, DrmCrtc *crtc,
                                Importer *importer, Planner *planner,
                                uint64_t frame_no) {
  drm_ = drm;
  crtc_ = crtc;  // Can be NULL if we haven't modeset yet
  importer_ = importer;
  planner_ = planner;
  frame_no_ = frame_no;

  int ret = sw_sync_timeline_create();
  if (ret < 0) {
    ALOGE("Failed to create sw sync timeline %d", ret);
    return ret;
  }
  timeline_fd_ = ret;
  return 0;
}

bool DrmDisplayComposition::validate_composition_type(DrmCompositionType des) {
  return type_ == DRM_COMPOSITION_TYPE_EMPTY || type_ == des;
}

int DrmDisplayComposition::CreateNextTimelineFence() {
  ++timeline_;
  return sw_sync_fence_create(timeline_fd_, "hwc drm display composition fence",
                              timeline_);
}

int DrmDisplayComposition::IncreaseTimelineToPoint(int point) {
  int timeline_increase = point - timeline_current_;
  if (timeline_increase <= 0)
    return 0;

  int ret = sw_sync_timeline_inc(timeline_fd_, timeline_increase);
  if (ret)
    ALOGE("Failed to increment sync timeline %d", ret);
  else
    timeline_current_ = point;

  return ret;
}

int DrmDisplayComposition::SetLayers(DrmHwcLayer *layers, size_t num_layers,
                                     bool geometry_changed) {
  if (!validate_composition_type(DRM_COMPOSITION_TYPE_FRAME))
    return -EINVAL;

  geometry_changed_ = geometry_changed;

  for (size_t layer_index = 0; layer_index < num_layers; layer_index++) {
    layers_.emplace_back(std::move(layers[layer_index]));
  }

  type_ = DRM_COMPOSITION_TYPE_FRAME;
  return 0;
}

int DrmDisplayComposition::SetDpmsMode(uint32_t dpms_mode) {
  if (!validate_composition_type(DRM_COMPOSITION_TYPE_DPMS))
    return -EINVAL;
  dpms_mode_ = dpms_mode;
  type_ = DRM_COMPOSITION_TYPE_DPMS;
  return 0;
}

int DrmDisplayComposition::SetDisplayMode(const DrmMode &display_mode) {
  if (!validate_composition_type(DRM_COMPOSITION_TYPE_MODESET))
    return -EINVAL;
  display_mode_ = display_mode;
  dpms_mode_ = DRM_MODE_DPMS_ON;
  type_ = DRM_COMPOSITION_TYPE_MODESET;
  return 0;
}

int DrmDisplayComposition::AddPlaneDisable(DrmPlane *plane) {
  composition_planes_.emplace_back(DrmCompositionPlane::Type::kDisable, plane,
                                   crtc_);
  return 0;
}

static std::vector<size_t> SetBitsToVector(
    uint64_t in, const std::vector<size_t> &index_map) {
  std::vector<size_t> out;
  size_t msb = sizeof(in) * 8 - 1;
  uint64_t mask = (uint64_t)1 << msb;
  for (size_t i = msb; mask != (uint64_t)0; i--, mask >>= 1)
    if (in & mask)
      out.push_back(index_map[i]);
  return out;
}

int DrmDisplayComposition::AddPlaneComposition(DrmCompositionPlane plane) {
  composition_planes_.emplace_back(std::move(plane));
  return 0;
}

void DrmDisplayComposition::SeparateLayers(DrmHwcRect<int> *exclude_rects,
                                           size_t num_exclude_rects) {
  DrmCompositionPlane *comp = NULL;
  std::vector<size_t> dedicated_layers;

  // Go through the composition and find the precomp layer as well as any
  // layers that have a dedicated plane located below the precomp layer.
  for (auto &i : composition_planes_) {
    if (i.type() == DrmCompositionPlane::Type::kLayer) {
      dedicated_layers.insert(dedicated_layers.end(), i.source_layers().begin(),
                              i.source_layers().end());
    } else if (i.type() == DrmCompositionPlane::Type::kPrecomp) {
      comp = &i;
      break;
    }
  }
  if (!comp)
    return;

  const std::vector<size_t> &comp_layers = comp->source_layers();
  if (comp_layers.size() > 64) {
    ALOGE("Failed to separate layers because there are more than 64");
    return;
  }

  // Index at which the actual layers begin
  size_t layer_offset = num_exclude_rects + dedicated_layers.size();
  if (comp_layers.size() + layer_offset > 64) {
    ALOGW(
        "Exclusion rectangles are being truncated to make the rectangle count "
        "fit into 64");
    num_exclude_rects = 64 - comp_layers.size() - dedicated_layers.size();
  }

  // We inject all the exclude rects into the rects list. Any resulting rect
  // that includes ANY of the first num_exclude_rects is rejected. After the
  // exclude rects, we add the lower layers. The rects that intersect with
  // these layers will be inspected and only those which are to be composited
  // above the layer will be included in the composition regions.
  std::vector<DrmHwcRect<int>> layer_rects(comp_layers.size() + layer_offset);
  std::copy(exclude_rects, exclude_rects + num_exclude_rects,
            layer_rects.begin());
  std::transform(
      dedicated_layers.begin(), dedicated_layers.end(),
      layer_rects.begin() + num_exclude_rects,
      [=](size_t layer_index) { return layers_[layer_index].display_frame; });
  std::transform(comp_layers.begin(), comp_layers.end(),
                 layer_rects.begin() + layer_offset, [=](size_t layer_index) {
    return layers_[layer_index].display_frame;
  });

  std::vector<separate_rects::RectSet<uint64_t, int>> separate_regions;
  separate_rects::separate_rects_64(layer_rects, &separate_regions);
  uint64_t exclude_mask = ((uint64_t)1 << num_exclude_rects) - 1;
  uint64_t dedicated_mask = (((uint64_t)1 << dedicated_layers.size()) - 1)
                            << num_exclude_rects;

  for (separate_rects::RectSet<uint64_t, int> &region : separate_regions) {
    if (region.id_set.getBits() & exclude_mask)
      continue;

    // If a rect intersects one of the dedicated layers, we need to remove the
    // layers from the composition region which appear *below* the dedicated
    // layer. This effectively punches a hole through the composition layer such
    // that the dedicated layer can be placed below the composition and not
    // be occluded.
    uint64_t dedicated_intersect = region.id_set.getBits() & dedicated_mask;
    for (size_t i = 0; dedicated_intersect && i < dedicated_layers.size();
         ++i) {
      // Only exclude layers if they intersect this particular dedicated layer
      if (!(dedicated_intersect & (1 << (i + num_exclude_rects))))
        continue;

      for (size_t j = 0; j < comp_layers.size(); ++j) {
        if (comp_layers[j] < dedicated_layers[i])
          region.id_set.subtract(j + layer_offset);
      }
    }
    if (!(region.id_set.getBits() >> layer_offset))
      continue;

    pre_comp_regions_.emplace_back(DrmCompositionRegion{
        region.rect,
        SetBitsToVector(region.id_set.getBits() >> layer_offset, comp_layers)});
  }
}

int DrmDisplayComposition::CreateAndAssignReleaseFences() {
  std::unordered_set<DrmHwcLayer *> squash_layers;
  std::unordered_set<DrmHwcLayer *> pre_comp_layers;
  std::unordered_set<DrmHwcLayer *> comp_layers;

  for (const DrmCompositionRegion &region : squash_regions_) {
    for (size_t source_layer_index : region.source_layers) {
      DrmHwcLayer *source_layer = &layers_[source_layer_index];
      squash_layers.emplace(source_layer);
    }
  }

  for (const DrmCompositionRegion &region : pre_comp_regions_) {
    for (size_t source_layer_index : region.source_layers) {
      DrmHwcLayer *source_layer = &layers_[source_layer_index];
      pre_comp_layers.emplace(source_layer);
      squash_layers.erase(source_layer);
    }
  }

  for (const DrmCompositionPlane &plane : composition_planes_) {
    if (plane.type() == DrmCompositionPlane::Type::kLayer) {
      for (auto i : plane.source_layers()) {
        DrmHwcLayer *source_layer = &layers_[i];
        comp_layers.emplace(source_layer);
        pre_comp_layers.erase(source_layer);
      }
    }
  }

  for (DrmHwcLayer *layer : squash_layers) {
    if (!layer->release_fence)
      continue;
    int ret = layer->release_fence.Set(CreateNextTimelineFence());
    if (ret < 0)
      return ret;
  }
  timeline_squash_done_ = timeline_;

  for (DrmHwcLayer *layer : pre_comp_layers) {
    if (!layer->release_fence)
      continue;
    int ret = layer->release_fence.Set(CreateNextTimelineFence());
    if (ret < 0)
      return ret;
  }
  timeline_pre_comp_done_ = timeline_;

  for (DrmHwcLayer *layer : comp_layers) {
    if (!layer->release_fence)
      continue;
    int ret = layer->release_fence.Set(CreateNextTimelineFence());
    if (ret < 0)
      return ret;
  }

  return 0;
}

int DrmDisplayComposition::Plan(SquashState *squash,
                                std::vector<DrmPlane *> *primary_planes,
                                std::vector<DrmPlane *> *overlay_planes) {
  if (type_ != DRM_COMPOSITION_TYPE_FRAME)
    return 0;

  // Used to track which layers should be sent to the planner. We exclude layers
  // that are entirely squashed so the planner can provision a precomposition
  // layer as appropriate (ex: if 5 layers are squashed and 1 is not, we don't
  // want to plan a precomposition layer that will be comprised of the already
  // squashed layers).
  std::map<size_t, DrmHwcLayer *> to_composite;

  bool use_squash_framebuffer = false;
  // Used to determine which layers were entirely squashed
  std::vector<int> layer_squash_area(layers_.size(), 0);
  // Used to avoid rerendering regions that were squashed
  std::vector<DrmHwcRect<int>> exclude_rects;
  if (squash != NULL) {
    if (geometry_changed_) {
      squash->Init(layers_.data(), layers_.size());
    } else {
      std::vector<bool> changed_regions;
      squash->GenerateHistory(layers_.data(), layers_.size(), changed_regions);

      std::vector<bool> stable_regions;
      squash->StableRegionsWithMarginalHistory(changed_regions, stable_regions);

      // Only if SOME region is stable
      use_squash_framebuffer =
          std::find(stable_regions.begin(), stable_regions.end(), true) !=
          stable_regions.end();

      squash->RecordHistory(layers_.data(), layers_.size(), changed_regions);

      // Changes in which regions are squashed triggers a rerender via
      // squash_regions.
      bool render_squash = squash->RecordAndCompareSquashed(stable_regions);

      for (size_t region_index = 0; region_index < stable_regions.size();
           region_index++) {
        const SquashState::Region &region = squash->regions()[region_index];
        if (!stable_regions[region_index])
          continue;

        exclude_rects.emplace_back(region.rect);

        if (render_squash) {
          squash_regions_.emplace_back();
          squash_regions_.back().frame = region.rect;
        }

        int frame_area = region.rect.area();
        // Source layers are sorted front to back i.e. top layer has lowest
        // index.
        for (size_t layer_index = layers_.size();
             layer_index-- > 0;  // Yes, I double checked this
             /* See condition */) {
          if (!region.layer_refs[layer_index])
            continue;
          layer_squash_area[layer_index] += frame_area;
          if (render_squash)
            squash_regions_.back().source_layers.push_back(layer_index);
        }
      }
    }

    for (size_t i = 0; i < layers_.size(); ++i) {
      if (layer_squash_area[i] < layers_[i].display_frame.area())
        to_composite.emplace(std::make_pair(i, &layers_[i]));
    }
  } else {
    for (size_t i = 0; i < layers_.size(); ++i)
      to_composite.emplace(std::make_pair(i, &layers_[i]));
  }

  int ret;
  std::vector<DrmCompositionPlane> plan;
  std::tie(ret, composition_planes_) =
      planner_->ProvisionPlanes(to_composite, use_squash_framebuffer, crtc_,
                                primary_planes, overlay_planes);
  if (ret) {
    ALOGE("Planner failed provisioning planes ret=%d", ret);
    return ret;
  }

  // Remove the planes we used from the pool before returning. This ensures they
  // won't be reused by another display in the composition.
  for (auto &i : composition_planes_) {
    if (!i.plane())
      continue;

    std::vector<DrmPlane *> *container;
    if (i.plane()->type() == DRM_PLANE_TYPE_PRIMARY)
      container = primary_planes;
    else
      container = overlay_planes;
    for (auto j = container->begin(); j != container->end(); ++j) {
      if (*j == i.plane()) {
        container->erase(j);
        break;
      }
    }
  }

  return FinalizeComposition(exclude_rects.data(), exclude_rects.size());
}

int DrmDisplayComposition::FinalizeComposition() {
  return FinalizeComposition(NULL, 0);
}

int DrmDisplayComposition::FinalizeComposition(DrmHwcRect<int> *exclude_rects,
                                               size_t num_exclude_rects) {
  SeparateLayers(exclude_rects, num_exclude_rects);
  return CreateAndAssignReleaseFences();
}

static const char *DrmCompositionTypeToString(DrmCompositionType type) {
  switch (type) {
    case DRM_COMPOSITION_TYPE_EMPTY:
      return "EMPTY";
    case DRM_COMPOSITION_TYPE_FRAME:
      return "FRAME";
    case DRM_COMPOSITION_TYPE_DPMS:
      return "DPMS";
    case DRM_COMPOSITION_TYPE_MODESET:
      return "MODESET";
    default:
      return "<invalid>";
  }
}

static const char *DPMSModeToString(int dpms_mode) {
  switch (dpms_mode) {
    case DRM_MODE_DPMS_ON:
      return "ON";
    case DRM_MODE_DPMS_OFF:
      return "OFF";
    default:
      return "<invalid>";
  }
}

static void DumpBuffer(const DrmHwcBuffer &buffer, std::ostringstream *out) {
  if (!buffer) {
    *out << "buffer=<invalid>";
    return;
  }

  *out << "buffer[w/h/format]=";
  *out << buffer->width << "/" << buffer->height << "/" << buffer->format;
}

static void DumpTransform(uint32_t transform, std::ostringstream *out) {
  *out << "[";

  if (transform == 0)
    *out << "IDENTITY";

  bool separator = false;
  if (transform & DrmHwcTransform::kFlipH) {
    *out << "FLIPH";
    separator = true;
  }
  if (transform & DrmHwcTransform::kFlipV) {
    if (separator)
      *out << "|";
    *out << "FLIPV";
    separator = true;
  }
  if (transform & DrmHwcTransform::kRotate90) {
    if (separator)
      *out << "|";
    *out << "ROTATE90";
    separator = true;
  }
  if (transform & DrmHwcTransform::kRotate180) {
    if (separator)
      *out << "|";
    *out << "ROTATE180";
    separator = true;
  }
  if (transform & DrmHwcTransform::kRotate270) {
    if (separator)
      *out << "|";
    *out << "ROTATE270";
    separator = true;
  }

  uint32_t valid_bits = DrmHwcTransform::kFlipH | DrmHwcTransform::kFlipH |
                        DrmHwcTransform::kRotate90 |
                        DrmHwcTransform::kRotate180 |
                        DrmHwcTransform::kRotate270;
  if (transform & ~valid_bits) {
    if (separator)
      *out << "|";
    *out << "INVALID";
  }
  *out << "]";
}

static const char *BlendingToString(DrmHwcBlending blending) {
  switch (blending) {
    case DrmHwcBlending::kNone:
      return "NONE";
    case DrmHwcBlending::kPreMult:
      return "PREMULT";
    case DrmHwcBlending::kCoverage:
      return "COVERAGE";
    default:
      return "<invalid>";
  }
}

static void DumpRegion(const DrmCompositionRegion &region,
                       std::ostringstream *out) {
  *out << "frame";
  region.frame.Dump(out);
  *out << " source_layers=(";

  const std::vector<size_t> &source_layers = region.source_layers;
  for (size_t i = 0; i < source_layers.size(); i++) {
    *out << source_layers[i];
    if (i < source_layers.size() - 1) {
      *out << " ";
    }
  }

  *out << ")";
}

void DrmDisplayComposition::Dump(std::ostringstream *out) const {
  *out << "----DrmDisplayComposition"
       << " crtc=" << (crtc_ ? crtc_->id() : -1)
       << " type=" << DrmCompositionTypeToString(type_);

  switch (type_) {
    case DRM_COMPOSITION_TYPE_DPMS:
      *out << " dpms_mode=" << DPMSModeToString(dpms_mode_);
      break;
    case DRM_COMPOSITION_TYPE_MODESET:
      *out << " display_mode=" << display_mode_.h_display() << "x"
           << display_mode_.v_display();
      break;
    default:
      break;
  }

  *out << " timeline[current/squash/pre-comp/done]=" << timeline_current_ << "/"
       << timeline_squash_done_ << "/" << timeline_pre_comp_done_ << "/"
       << timeline_ << "\n";

  *out << "    Layers: count=" << layers_.size() << "\n";
  for (size_t i = 0; i < layers_.size(); i++) {
    const DrmHwcLayer &layer = layers_[i];
    *out << "      [" << i << "] ";

    DumpBuffer(layer.buffer, out);

    if (layer.protected_usage())
      *out << " protected";

    *out << " transform=";
    DumpTransform(layer.transform, out);
    *out << " blending[a=" << (int)layer.alpha
         << "]=" << BlendingToString(layer.blending) << " source_crop";
    layer.source_crop.Dump(out);
    *out << " display_frame";
    layer.display_frame.Dump(out);

    *out << "\n";
  }

  *out << "    Planes: count=" << composition_planes_.size() << "\n";
  for (size_t i = 0; i < composition_planes_.size(); i++) {
    const DrmCompositionPlane &comp_plane = composition_planes_[i];
    *out << "      [" << i << "]"
         << " plane=" << (comp_plane.plane() ? comp_plane.plane()->id() : -1)
         << " type=";
    switch (comp_plane.type()) {
      case DrmCompositionPlane::Type::kDisable:
        *out << "DISABLE";
        break;
      case DrmCompositionPlane::Type::kLayer:
        *out << "LAYER";
        break;
      case DrmCompositionPlane::Type::kPrecomp:
        *out << "PRECOMP";
        break;
      case DrmCompositionPlane::Type::kSquash:
        *out << "SQUASH";
        break;
      default:
        *out << "<invalid>";
        break;
    }

    *out << " source_layer=";
    for (auto i : comp_plane.source_layers()) {
      *out << i << " ";
    }
    *out << "\n";
  }

  *out << "    Squash Regions: count=" << squash_regions_.size() << "\n";
  for (size_t i = 0; i < squash_regions_.size(); i++) {
    *out << "      [" << i << "] ";
    DumpRegion(squash_regions_[i], out);
    *out << "\n";
  }

  *out << "    Pre-Comp Regions: count=" << pre_comp_regions_.size() << "\n";
  for (size_t i = 0; i < pre_comp_regions_.size(); i++) {
    *out << "      [" << i << "] ";
    DumpRegion(pre_comp_regions_[i], out);
    *out << "\n";
  }
}
}
