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

#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#define LOG_TAG "hwc-drm-display-compositor"

#include "drmdisplaycompositor.h"

#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <sstream>
#include <vector>

#include <cutils/log.h>
#include <drm/drm_mode.h>
#include <sync/sync.h>
#include <utils/Trace.h>

#include "autolock.h"
#include "drmcrtc.h"
#include "drmplane.h"
#include "drmresources.h"
#include "glworker.h"

#define DRM_DISPLAY_COMPOSITOR_MAX_QUEUE_DEPTH 2

namespace android {

void SquashState::Init(DrmHwcLayer *layers, size_t num_layers) {
  generation_number_++;
  valid_history_ = 0;
  regions_.clear();
  last_handles_.clear();

  std::vector<DrmHwcRect<int>> in_rects;
  for (size_t i = 0; i < num_layers; i++) {
    DrmHwcLayer *layer = &layers[i];
    in_rects.emplace_back(layer->display_frame);
    last_handles_.push_back(layer->sf_handle);
  }

  std::vector<separate_rects::RectSet<uint64_t, int>> out_regions;
  separate_rects::separate_rects_64(in_rects, &out_regions);

  for (const separate_rects::RectSet<uint64_t, int> &out_region : out_regions) {
    regions_.emplace_back();
    Region &region = regions_.back();
    region.rect = out_region.rect;
    region.layer_refs = out_region.id_set.getBits();
  }
}

void SquashState::GenerateHistory(DrmHwcLayer *layers, size_t num_layers,
                                  std::vector<bool> &changed_regions) const {
  changed_regions.resize(regions_.size());
  if (num_layers != last_handles_.size()) {
    ALOGE("SquashState::GenerateHistory expected %zu layers but got %zu layers",
          last_handles_.size(), num_layers);
    return;
  }
  std::bitset<kMaxLayers> changed_layers;
  for (size_t i = 0; i < last_handles_.size(); i++) {
    DrmHwcLayer *layer = &layers[i];
    // Protected layers can't be squashed so we treat them as constantly
    // changing.
    if (layer->protected_usage() || last_handles_[i] != layer->sf_handle)
      changed_layers.set(i);
  }

  for (size_t i = 0; i < regions_.size(); i++) {
    changed_regions[i] = (regions_[i].layer_refs & changed_layers).any();
  }
}

void SquashState::StableRegionsWithMarginalHistory(
    const std::vector<bool> &changed_regions,
    std::vector<bool> &stable_regions) const {
  stable_regions.resize(regions_.size());
  for (size_t i = 0; i < regions_.size(); i++) {
    stable_regions[i] = !changed_regions[i] && is_stable(i);
  }
}

void SquashState::RecordHistory(DrmHwcLayer *layers, size_t num_layers,
                                const std::vector<bool> &changed_regions) {
  if (num_layers != last_handles_.size()) {
    ALOGE("SquashState::RecordHistory expected %zu layers but got %zu layers",
          last_handles_.size(), num_layers);
    return;
  }
  if (changed_regions.size() != regions_.size()) {
    ALOGE("SquashState::RecordHistory expected %zu regions but got %zu regions",
          regions_.size(), changed_regions.size());
    return;
  }

  for (size_t i = 0; i < last_handles_.size(); i++) {
    DrmHwcLayer *layer = &layers[i];
    last_handles_[i] = layer->sf_handle;
  }

  for (size_t i = 0; i < regions_.size(); i++) {
    regions_[i].change_history <<= 1;
    regions_[i].change_history.set(/* LSB */ 0, changed_regions[i]);
  }

  valid_history_++;
}

bool SquashState::RecordAndCompareSquashed(
    const std::vector<bool> &squashed_regions) {
  if (squashed_regions.size() != regions_.size()) {
    ALOGE(
        "SquashState::RecordAndCompareSquashed expected %zu regions but got "
        "%zu regions",
        regions_.size(), squashed_regions.size());
    return false;
  }
  bool changed = false;
  for (size_t i = 0; i < regions_.size(); i++) {
    if (regions_[i].squashed != squashed_regions[i]) {
      regions_[i].squashed = squashed_regions[i];
      changed = true;
    }
  }
  return changed;
}

void SquashState::Dump(std::ostringstream *out) const {
  *out << "----SquashState generation=" << generation_number_
       << " history=" << valid_history_ << "\n"
       << "    Regions: count=" << regions_.size() << "\n";
  for (size_t i = 0; i < regions_.size(); i++) {
    const Region &region = regions_[i];
    *out << "      [" << i << "]"
         << " history=" << region.change_history << " rect";
    region.rect.Dump(out);
    *out << " layers=(";
    bool first = true;
    for (size_t layer_index = 0; layer_index < kMaxLayers; layer_index++) {
      if ((region.layer_refs &
           std::bitset<kMaxLayers>((size_t)1 << layer_index))
              .any()) {
        if (!first)
          *out << " ";
        first = false;
        *out << layer_index;
      }
    }
    *out << ")";
    if (region.squashed)
      *out << " squashed";
    *out << "\n";
  }
}

static bool UsesSquash(const std::vector<DrmCompositionPlane> &comp_planes) {
  return std::any_of(comp_planes.begin(), comp_planes.end(),
                     [](const DrmCompositionPlane &plane) {
    return plane.type() == DrmCompositionPlane::Type::kSquash;
  });
}

DrmDisplayCompositor::FrameWorker::FrameWorker(DrmDisplayCompositor *compositor)
    : Worker("frame-worker", HAL_PRIORITY_URGENT_DISPLAY),
      compositor_(compositor) {
}

DrmDisplayCompositor::FrameWorker::~FrameWorker() {
}

int DrmDisplayCompositor::FrameWorker::Init() {
  return InitWorker();
}

void DrmDisplayCompositor::FrameWorker::QueueFrame(
    std::unique_ptr<DrmDisplayComposition> composition, int status) {
  Lock();
  FrameState frame;
  frame.composition = std::move(composition);
  frame.status = status;
  frame_queue_.push(std::move(frame));
  SignalLocked();
  Unlock();
}

void DrmDisplayCompositor::FrameWorker::Routine() {
  int ret = Lock();
  if (ret) {
    ALOGE("Failed to lock worker, %d", ret);
    return;
  }

  int wait_ret = 0;
  if (frame_queue_.empty()) {
    wait_ret = WaitForSignalOrExitLocked();
  }

  FrameState frame;
  if (!frame_queue_.empty()) {
    frame = std::move(frame_queue_.front());
    frame_queue_.pop();
  }

  ret = Unlock();
  if (ret) {
    ALOGE("Failed to unlock worker, %d", ret);
    return;
  }

  if (wait_ret == -EINTR) {
    return;
  } else if (wait_ret) {
    ALOGE("Failed to wait for signal, %d", wait_ret);
    return;
  }

  compositor_->ApplyFrame(std::move(frame.composition), frame.status);
}

DrmDisplayCompositor::DrmDisplayCompositor()
    : drm_(NULL),
      display_(-1),
      worker_(this),
      frame_worker_(this),
      initialized_(false),
      active_(false),
      use_hw_overlays_(true),
      framebuffer_index_(0),
      squash_framebuffer_index_(0),
      dump_frames_composited_(0),
      dump_last_timestamp_ns_(0) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts))
    return;
  dump_last_timestamp_ns_ = ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
}

DrmDisplayCompositor::~DrmDisplayCompositor() {
  if (!initialized_)
    return;

  worker_.Exit();
  frame_worker_.Exit();

  int ret = pthread_mutex_lock(&lock_);
  if (ret)
    ALOGE("Failed to acquire compositor lock %d", ret);

  if (mode_.blob_id)
    drm_->DestroyPropertyBlob(mode_.blob_id);
  if (mode_.old_blob_id)
    drm_->DestroyPropertyBlob(mode_.old_blob_id);

  while (!composite_queue_.empty()) {
    composite_queue_.front().reset();
    composite_queue_.pop();
  }
  active_composition_.reset();

  ret = pthread_mutex_unlock(&lock_);
  if (ret)
    ALOGE("Failed to acquire compositor lock %d", ret);

  pthread_mutex_destroy(&lock_);
}

int DrmDisplayCompositor::Init(DrmResources *drm, int display) {
  drm_ = drm;
  display_ = display;

  int ret = pthread_mutex_init(&lock_, NULL);
  if (ret) {
    ALOGE("Failed to initialize drm compositor lock %d\n", ret);
    return ret;
  }
  ret = worker_.Init();
  if (ret) {
    pthread_mutex_destroy(&lock_);
    ALOGE("Failed to initialize compositor worker %d\n", ret);
    return ret;
  }
  ret = frame_worker_.Init();
  if (ret) {
    pthread_mutex_destroy(&lock_);
    ALOGE("Failed to initialize frame worker %d\n", ret);
    return ret;
  }

  initialized_ = true;
  return 0;
}

std::unique_ptr<DrmDisplayComposition> DrmDisplayCompositor::CreateComposition()
    const {
  return std::unique_ptr<DrmDisplayComposition>(new DrmDisplayComposition());
}

int DrmDisplayCompositor::QueueComposition(
    std::unique_ptr<DrmDisplayComposition> composition) {
  switch (composition->type()) {
    case DRM_COMPOSITION_TYPE_FRAME:
      if (!active_)
        return -ENODEV;
      break;
    case DRM_COMPOSITION_TYPE_DPMS:
      /*
       * Update the state as soon as we get it so we can start/stop queuing
       * frames asap.
       */
      active_ = (composition->dpms_mode() == DRM_MODE_DPMS_ON);
      break;
    case DRM_COMPOSITION_TYPE_MODESET:
      break;
    case DRM_COMPOSITION_TYPE_EMPTY:
      return 0;
    default:
      ALOGE("Unknown composition type %d/%d", composition->type(), display_);
      return -ENOENT;
  }

  int ret = pthread_mutex_lock(&lock_);
  if (ret) {
    ALOGE("Failed to acquire compositor lock %d", ret);
    return ret;
  }

  // Block the queue if it gets too large. Otherwise, SurfaceFlinger will start
  // to eat our buffer handles when we get about 1 second behind.
  while (composite_queue_.size() >= DRM_DISPLAY_COMPOSITOR_MAX_QUEUE_DEPTH) {
    pthread_mutex_unlock(&lock_);
    sched_yield();
    pthread_mutex_lock(&lock_);
  }

  composite_queue_.push(std::move(composition));

  ret = pthread_mutex_unlock(&lock_);
  if (ret) {
    ALOGE("Failed to release compositor lock %d", ret);
    return ret;
  }

  worker_.Signal();
  return 0;
}

std::tuple<uint32_t, uint32_t, int>
DrmDisplayCompositor::GetActiveModeResolution() {
  DrmConnector *connector = drm_->GetConnectorForDisplay(display_);
  if (connector == NULL) {
    ALOGE("Failed to determine display mode: no connector for display %d",
          display_);
    return std::make_tuple(0, 0, -ENODEV);
  }

  const DrmMode &mode = connector->active_mode();
  return std::make_tuple(mode.h_display(), mode.v_display(), 0);
}

int DrmDisplayCompositor::PrepareFramebuffer(
    DrmFramebuffer &fb, DrmDisplayComposition *display_comp) {
  int ret = fb.WaitReleased(-1);
  if (ret) {
    ALOGE("Failed to wait for framebuffer release %d", ret);
    return ret;
  }
  uint32_t width, height;
  std::tie(width, height, ret) = GetActiveModeResolution();
  if (ret) {
    ALOGE(
        "Failed to allocate framebuffer because the display resolution could "
        "not be determined %d",
        ret);
    return ret;
  }

  fb.set_release_fence_fd(-1);
  if (!fb.Allocate(width, height)) {
    ALOGE("Failed to allocate framebuffer with size %dx%d", width, height);
    return -ENOMEM;
  }

  display_comp->layers().emplace_back();
  DrmHwcLayer &pre_comp_layer = display_comp->layers().back();
  pre_comp_layer.sf_handle = fb.buffer()->handle;
  pre_comp_layer.blending = DrmHwcBlending::kPreMult;
  pre_comp_layer.source_crop = DrmHwcRect<float>(0, 0, width, height);
  pre_comp_layer.display_frame = DrmHwcRect<int>(0, 0, width, height);
  ret = pre_comp_layer.buffer.ImportBuffer(fb.buffer()->handle,
                                           display_comp->importer());
  if (ret) {
    ALOGE("Failed to import framebuffer for display %d", ret);
    return ret;
  }

  return ret;
}

int DrmDisplayCompositor::ApplySquash(DrmDisplayComposition *display_comp) {
  int ret = 0;

  DrmFramebuffer &fb = squash_framebuffers_[squash_framebuffer_index_];
  ret = PrepareFramebuffer(fb, display_comp);
  if (ret) {
    ALOGE("Failed to prepare framebuffer for squash %d", ret);
    return ret;
  }

  std::vector<DrmCompositionRegion> &regions = display_comp->squash_regions();
  ret = pre_compositor_->Composite(display_comp->layers().data(),
                                   regions.data(), regions.size(), fb.buffer());
  pre_compositor_->Finish();

  if (ret) {
    ALOGE("Failed to squash layers");
    return ret;
  }

  ret = display_comp->CreateNextTimelineFence();
  if (ret <= 0) {
    ALOGE("Failed to create squash framebuffer release fence %d", ret);
    return ret;
  }

  fb.set_release_fence_fd(ret);
  display_comp->SignalSquashDone();

  return 0;
}

int DrmDisplayCompositor::ApplyPreComposite(
    DrmDisplayComposition *display_comp) {
  int ret = 0;

  DrmFramebuffer &fb = framebuffers_[framebuffer_index_];
  ret = PrepareFramebuffer(fb, display_comp);
  if (ret) {
    ALOGE("Failed to prepare framebuffer for pre-composite %d", ret);
    return ret;
  }

  std::vector<DrmCompositionRegion> &regions = display_comp->pre_comp_regions();
  ret = pre_compositor_->Composite(display_comp->layers().data(),
                                   regions.data(), regions.size(), fb.buffer());
  pre_compositor_->Finish();

  if (ret) {
    ALOGE("Failed to pre-composite layers");
    return ret;
  }

  ret = display_comp->CreateNextTimelineFence();
  if (ret <= 0) {
    ALOGE("Failed to create pre-composite framebuffer release fence %d", ret);
    return ret;
  }

  fb.set_release_fence_fd(ret);
  display_comp->SignalPreCompDone();

  return 0;
}

int DrmDisplayCompositor::DisablePlanes(DrmDisplayComposition *display_comp) {
  drmModeAtomicReqPtr pset = drmModeAtomicAlloc();
  if (!pset) {
    ALOGE("Failed to allocate property set");
    return -ENOMEM;
  }

  int ret;
  std::vector<DrmCompositionPlane> &comp_planes =
      display_comp->composition_planes();
  for (DrmCompositionPlane &comp_plane : comp_planes) {
    DrmPlane *plane = comp_plane.plane();
    ret = drmModeAtomicAddProperty(pset, plane->id(),
                                   plane->crtc_property().id(), 0) < 0 ||
          drmModeAtomicAddProperty(pset, plane->id(), plane->fb_property().id(),
                                   0) < 0;
    if (ret) {
      ALOGE("Failed to add plane %d disable to pset", plane->id());
      drmModeAtomicFree(pset);
      return ret;
    }
  }

  ret = drmModeAtomicCommit(drm_->fd(), pset, 0, drm_);
  if (ret) {
    ALOGE("Failed to commit pset ret=%d\n", ret);
    drmModeAtomicFree(pset);
    return ret;
  }

  drmModeAtomicFree(pset);
  return 0;
}

int DrmDisplayCompositor::PrepareFrame(DrmDisplayComposition *display_comp) {
  int ret = 0;

  std::vector<DrmHwcLayer> &layers = display_comp->layers();
  std::vector<DrmCompositionPlane> &comp_planes =
      display_comp->composition_planes();
  std::vector<DrmCompositionRegion> &squash_regions =
      display_comp->squash_regions();
  std::vector<DrmCompositionRegion> &pre_comp_regions =
      display_comp->pre_comp_regions();

  int squash_layer_index = -1;
  if (squash_regions.size() > 0) {
    squash_framebuffer_index_ = (squash_framebuffer_index_ + 1) % 2;
    ret = ApplySquash(display_comp);
    if (ret)
      return ret;

    squash_layer_index = layers.size() - 1;
  } else {
    if (UsesSquash(comp_planes)) {
      DrmFramebuffer &fb = squash_framebuffers_[squash_framebuffer_index_];
      layers.emplace_back();
      squash_layer_index = layers.size() - 1;
      DrmHwcLayer &squash_layer = layers.back();
      ret = squash_layer.buffer.ImportBuffer(fb.buffer()->handle,
                                             display_comp->importer());
      if (ret) {
        ALOGE("Failed to import old squashed framebuffer %d", ret);
        return ret;
      }
      squash_layer.sf_handle = fb.buffer()->handle;
      squash_layer.blending = DrmHwcBlending::kPreMult;
      squash_layer.source_crop = DrmHwcRect<float>(
          0, 0, squash_layer.buffer->width, squash_layer.buffer->height);
      squash_layer.display_frame = DrmHwcRect<int>(
          0, 0, squash_layer.buffer->width, squash_layer.buffer->height);
      ret = display_comp->CreateNextTimelineFence();

      if (ret <= 0) {
        ALOGE("Failed to create squash framebuffer release fence %d", ret);
        return ret;
      }

      fb.set_release_fence_fd(ret);
      ret = 0;
    }
  }

  bool do_pre_comp = pre_comp_regions.size() > 0;
  int pre_comp_layer_index = -1;
  if (do_pre_comp) {
    ret = ApplyPreComposite(display_comp);
    if (ret)
      return ret;

    pre_comp_layer_index = layers.size() - 1;
    framebuffer_index_ = (framebuffer_index_ + 1) % DRM_DISPLAY_BUFFERS;
  }

  for (DrmCompositionPlane &comp_plane : comp_planes) {
    std::vector<size_t> &source_layers = comp_plane.source_layers();
    switch (comp_plane.type()) {
      case DrmCompositionPlane::Type::kSquash:
        if (source_layers.size())
          ALOGE("Squash source_layers is expected to be empty (%zu/%d)",
                source_layers[0], squash_layer_index);
        source_layers.push_back(squash_layer_index);
        break;
      case DrmCompositionPlane::Type::kPrecomp:
        if (!do_pre_comp) {
          ALOGE(
              "Can not use pre composite framebuffer with no pre composite "
              "regions");
          return -EINVAL;
        }
        // Replace source_layers with the output of the precomposite
        source_layers.clear();
        source_layers.push_back(pre_comp_layer_index);
        break;
      default:
        break;
    }
  }

  return ret;
}

int DrmDisplayCompositor::CommitFrame(DrmDisplayComposition *display_comp,
                                      bool test_only) {
  ATRACE_CALL();

  int ret = 0;

  std::vector<DrmHwcLayer> &layers = display_comp->layers();
  std::vector<DrmCompositionPlane> &comp_planes =
      display_comp->composition_planes();
  std::vector<DrmCompositionRegion> &pre_comp_regions =
      display_comp->pre_comp_regions();

  DrmConnector *connector = drm_->GetConnectorForDisplay(display_);
  if (!connector) {
    ALOGE("Could not locate connector for display %d", display_);
    return -ENODEV;
  }
  DrmCrtc *crtc = drm_->GetCrtcForDisplay(display_);
  if (!crtc) {
    ALOGE("Could not locate crtc for display %d", display_);
    return -ENODEV;
  }

  drmModeAtomicReqPtr pset = drmModeAtomicAlloc();
  if (!pset) {
    ALOGE("Failed to allocate property set");
    return -ENOMEM;
  }

  if (mode_.needs_modeset) {
    ret = drmModeAtomicAddProperty(pset, crtc->id(), crtc->mode_property().id(),
                                   mode_.blob_id) < 0 ||
          drmModeAtomicAddProperty(pset, connector->id(),
                                   connector->crtc_id_property().id(),
                                   crtc->id()) < 0;
    if (ret) {
      ALOGE("Failed to add blob %d to pset", mode_.blob_id);
      drmModeAtomicFree(pset);
      return ret;
    }
  }

  for (DrmCompositionPlane &comp_plane : comp_planes) {
    DrmPlane *plane = comp_plane.plane();
    DrmCrtc *crtc = comp_plane.crtc();
    std::vector<size_t> &source_layers = comp_plane.source_layers();

    int fb_id = -1;
    DrmHwcRect<int> display_frame;
    DrmHwcRect<float> source_crop;
    uint64_t rotation = 0;
    uint64_t alpha = 0xFF;

    if (comp_plane.type() != DrmCompositionPlane::Type::kDisable) {
      if (source_layers.size() > 1) {
        ALOGE("Can't handle more than one source layer sz=%zu type=%d",
              source_layers.size(), comp_plane.type());
        continue;
      }

      if (source_layers.empty() || source_layers.front() >= layers.size()) {
        ALOGE("Source layer index %zu out of bounds %zu type=%d",
              source_layers.front(), layers.size(), comp_plane.type());
        break;
      }
      DrmHwcLayer &layer = layers[source_layers.front()];
      if (!test_only && layer.acquire_fence.get() >= 0) {
        int acquire_fence = layer.acquire_fence.get();
        int total_fence_timeout = 0;
        for (int i = 0; i < kAcquireWaitTries; ++i) {
          int fence_timeout = kAcquireWaitTimeoutMs * (1 << i);
          total_fence_timeout += fence_timeout;
          ret = sync_wait(acquire_fence, fence_timeout);
          if (ret)
            ALOGW("Acquire fence %d wait %d failed (%d). Total time %d",
                  acquire_fence, i, ret, total_fence_timeout);
        }
        if (ret) {
          ALOGE("Failed to wait for acquire %d/%d", acquire_fence, ret);
          break;
        }
        layer.acquire_fence.Close();
      }
      if (!layer.buffer) {
        ALOGE("Expected a valid framebuffer for pset");
        break;
      }
      fb_id = layer.buffer->fb_id;
      display_frame = layer.display_frame;
      source_crop = layer.source_crop;
      if (layer.blending == DrmHwcBlending::kPreMult)
        alpha = layer.alpha;

      rotation = 0;
      if (layer.transform & DrmHwcTransform::kFlipH)
        rotation |= 1 << DRM_REFLECT_X;
      if (layer.transform & DrmHwcTransform::kFlipV)
        rotation |= 1 << DRM_REFLECT_Y;
      if (layer.transform & DrmHwcTransform::kRotate90)
        rotation |= 1 << DRM_ROTATE_90;
      else if (layer.transform & DrmHwcTransform::kRotate180)
        rotation |= 1 << DRM_ROTATE_180;
      else if (layer.transform & DrmHwcTransform::kRotate270)
        rotation |= 1 << DRM_ROTATE_270;
    }
    // Disable the plane if there's no framebuffer
    if (fb_id < 0) {
      ret = drmModeAtomicAddProperty(pset, plane->id(),
                                     plane->crtc_property().id(), 0) < 0 ||
            drmModeAtomicAddProperty(pset, plane->id(),
                                     plane->fb_property().id(), 0) < 0;
      if (ret) {
        ALOGE("Failed to add plane %d disable to pset", plane->id());
        break;
      }
      continue;
    }

    // TODO: Once we have atomic test, this should fall back to GL
    if (rotation && plane->rotation_property().id() == 0) {
      ALOGE("Rotation is not supported on plane %d", plane->id());
      ret = -EINVAL;
      break;
    }

    // TODO: Once we have atomic test, this should fall back to GL
    if (alpha != 0xFF && plane->alpha_property().id() == 0) {
      ALOGE("Alpha is not supported on plane %d", plane->id());
      ret = -EINVAL;
      break;
    }

    ret = drmModeAtomicAddProperty(pset, plane->id(),
                                   plane->crtc_property().id(), crtc->id()) < 0;
    ret |= drmModeAtomicAddProperty(pset, plane->id(),
                                    plane->fb_property().id(), fb_id) < 0;
    ret |= drmModeAtomicAddProperty(pset, plane->id(),
                                    plane->crtc_x_property().id(),
                                    display_frame.left) < 0;
    ret |= drmModeAtomicAddProperty(pset, plane->id(),
                                    plane->crtc_y_property().id(),
                                    display_frame.top) < 0;
    ret |= drmModeAtomicAddProperty(
               pset, plane->id(), plane->crtc_w_property().id(),
               display_frame.right - display_frame.left) < 0;
    ret |= drmModeAtomicAddProperty(
               pset, plane->id(), plane->crtc_h_property().id(),
               display_frame.bottom - display_frame.top) < 0;
    ret |= drmModeAtomicAddProperty(pset, plane->id(),
                                    plane->src_x_property().id(),
                                    (int)(source_crop.left) << 16) < 0;
    ret |= drmModeAtomicAddProperty(pset, plane->id(),
                                    plane->src_y_property().id(),
                                    (int)(source_crop.top) << 16) < 0;
    ret |= drmModeAtomicAddProperty(
               pset, plane->id(), plane->src_w_property().id(),
               (int)(source_crop.right - source_crop.left) << 16) < 0;
    ret |= drmModeAtomicAddProperty(
               pset, plane->id(), plane->src_h_property().id(),
               (int)(source_crop.bottom - source_crop.top) << 16) < 0;
    if (ret) {
      ALOGE("Failed to add plane %d to set", plane->id());
      break;
    }

    if (plane->rotation_property().id()) {
      ret = drmModeAtomicAddProperty(pset, plane->id(),
                                     plane->rotation_property().id(),
                                     rotation) < 0;
      if (ret) {
        ALOGE("Failed to add rotation property %d to plane %d",
              plane->rotation_property().id(), plane->id());
        break;
      }
    }

    if (plane->alpha_property().id()) {
      ret = drmModeAtomicAddProperty(pset, plane->id(),
                                     plane->alpha_property().id(),
                                     alpha) < 0;
      if (ret) {
        ALOGE("Failed to add alpha property %d to plane %d",
              plane->alpha_property().id(), plane->id());
        break;
      }
    }
  }

out:
  if (!ret) {
    uint32_t flags = DRM_MODE_ATOMIC_ALLOW_MODESET;
    if (test_only)
      flags |= DRM_MODE_ATOMIC_TEST_ONLY;

    ret = drmModeAtomicCommit(drm_->fd(), pset, flags, drm_);
    if (ret) {
      if (test_only)
        ALOGI("Commit test pset failed ret=%d\n", ret);
      else
        ALOGE("Failed to commit pset ret=%d\n", ret);
      drmModeAtomicFree(pset);
      return ret;
    }
  }
  if (pset)
    drmModeAtomicFree(pset);

  if (!test_only && mode_.needs_modeset) {
    ret = drm_->DestroyPropertyBlob(mode_.old_blob_id);
    if (ret) {
      ALOGE("Failed to destroy old mode property blob %" PRIu32 "/%d",
            mode_.old_blob_id, ret);
      return ret;
    }

    /* TODO: Add dpms to the pset when the kernel supports it */
    ret = ApplyDpms(display_comp);
    if (ret) {
      ALOGE("Failed to apply DPMS after modeset %d\n", ret);
      return ret;
    }

    connector->set_active_mode(mode_.mode);
    mode_.old_blob_id = mode_.blob_id;
    mode_.blob_id = 0;
    mode_.needs_modeset = false;
  }

  return ret;
}

int DrmDisplayCompositor::ApplyDpms(DrmDisplayComposition *display_comp) {
  DrmConnector *conn = drm_->GetConnectorForDisplay(display_);
  if (!conn) {
    ALOGE("Failed to get DrmConnector for display %d", display_);
    return -ENODEV;
  }

  const DrmProperty &prop = conn->dpms_property();
  int ret = drmModeConnectorSetProperty(drm_->fd(), conn->id(), prop.id(),
                                        display_comp->dpms_mode());
  if (ret) {
    ALOGE("Failed to set DPMS property for connector %d", conn->id());
    return ret;
  }
  return 0;
}

std::tuple<int, uint32_t> DrmDisplayCompositor::CreateModeBlob(
    const DrmMode &mode) {
  struct drm_mode_modeinfo drm_mode;
  memset(&drm_mode, 0, sizeof(drm_mode));
  mode.ToDrmModeModeInfo(&drm_mode);

  uint32_t id = 0;
  int ret = drm_->CreatePropertyBlob(&drm_mode,
                                     sizeof(struct drm_mode_modeinfo), &id);
  if (ret) {
    ALOGE("Failed to create mode property blob %d", ret);
    return std::make_tuple(ret, 0);
  }
  ALOGE("Create blob_id %" PRIu32 "\n", id);
  return std::make_tuple(ret, id);
}

void DrmDisplayCompositor::ClearDisplay() {
  AutoLock lock(&lock_, "compositor");
  int ret = lock.Lock();
  if (ret)
    return;

  if (!active_composition_)
    return;

  if (DisablePlanes(active_composition_.get()))
    return;

  active_composition_->SignalCompositionDone();

  active_composition_.reset(NULL);
}

void DrmDisplayCompositor::ApplyFrame(
    std::unique_ptr<DrmDisplayComposition> composition, int status) {
  int ret = status;

  if (!ret)
    ret = CommitFrame(composition.get(), false);

  if (ret) {
    ALOGE("Composite failed for display %d", display_);
    // Disable the hw used by the last active composition. This allows us to
    // signal the release fences from that composition to avoid hanging.
    ClearDisplay();
    return;
  }
  ++dump_frames_composited_;

  if (active_composition_)
    active_composition_->SignalCompositionDone();

  ret = pthread_mutex_lock(&lock_);
  if (ret)
    ALOGE("Failed to acquire lock for active_composition swap");

  active_composition_.swap(composition);

  if (!ret)
    ret = pthread_mutex_unlock(&lock_);
  if (ret)
    ALOGE("Failed to release lock for active_composition swap");
}

int DrmDisplayCompositor::Composite() {
  ATRACE_CALL();

  if (!pre_compositor_) {
    pre_compositor_.reset(new GLWorkerCompositor());
    int ret = pre_compositor_->Init();
    if (ret) {
      ALOGE("Failed to initialize OpenGL compositor %d", ret);
      return ret;
    }
  }

  int ret = pthread_mutex_lock(&lock_);
  if (ret) {
    ALOGE("Failed to acquire compositor lock %d", ret);
    return ret;
  }
  if (composite_queue_.empty()) {
    ret = pthread_mutex_unlock(&lock_);
    if (ret)
      ALOGE("Failed to release compositor lock %d", ret);
    return ret;
  }

  std::unique_ptr<DrmDisplayComposition> composition(
      std::move(composite_queue_.front()));

  composite_queue_.pop();

  ret = pthread_mutex_unlock(&lock_);
  if (ret) {
    ALOGE("Failed to release compositor lock %d", ret);
    return ret;
  }

  switch (composition->type()) {
    case DRM_COMPOSITION_TYPE_FRAME:
      ret = PrepareFrame(composition.get());
      if (ret) {
        ALOGE("Failed to prepare frame for display %d", display_);
        return ret;
      }
      if (composition->geometry_changed()) {
        // Send the composition to the kernel to ensure we can commit it. This
        // is just a test, it won't actually commit the frame. If rejected,
        // squash the frame into one layer and use the squashed composition
        ret = CommitFrame(composition.get(), true);
        if (ret)
          ALOGI("Commit test failed, squashing frame for display %d", display_);
        use_hw_overlays_ = !ret;
      }

      // If use_hw_overlays_ is false, we can't use hardware to composite the
      // frame. So squash all layers into a single composition and queue that
      // instead.
      if (!use_hw_overlays_) {
        std::unique_ptr<DrmDisplayComposition> squashed = CreateComposition();
        ret = SquashFrame(composition.get(), squashed.get());
        if (!ret) {
          composition = std::move(squashed);
        } else {
          ALOGE("Failed to squash frame for display %d", display_);
          // Disable the hw used by the last active composition. This allows us
          // to signal the release fences from that composition to avoid
          // hanging.
          ClearDisplay();
          return ret;
        }
      }
      frame_worker_.QueueFrame(std::move(composition), ret);
      break;
    case DRM_COMPOSITION_TYPE_DPMS:
      ret = ApplyDpms(composition.get());
      if (ret)
        ALOGE("Failed to apply dpms for display %d", display_);
      return ret;
    case DRM_COMPOSITION_TYPE_MODESET:
      mode_.mode = composition->display_mode();
      if (mode_.blob_id)
        drm_->DestroyPropertyBlob(mode_.blob_id);
      std::tie(ret, mode_.blob_id) = CreateModeBlob(mode_.mode);
      if (ret) {
        ALOGE("Failed to create mode blob for display %d", display_);
        return ret;
      }
      mode_.needs_modeset = true;
      return 0;
    default:
      ALOGE("Unknown composition type %d", composition->type());
      return -EINVAL;
  }

  return ret;
}

bool DrmDisplayCompositor::HaveQueuedComposites() const {
  int ret = pthread_mutex_lock(&lock_);
  if (ret) {
    ALOGE("Failed to acquire compositor lock %d", ret);
    return false;
  }

  bool empty_ret = !composite_queue_.empty();

  ret = pthread_mutex_unlock(&lock_);
  if (ret) {
    ALOGE("Failed to release compositor lock %d", ret);
    return false;
  }

  return empty_ret;
}

int DrmDisplayCompositor::SquashAll() {
  AutoLock lock(&lock_, "compositor");
  int ret = lock.Lock();
  if (ret)
    return ret;

  if (!active_composition_)
    return 0;

  std::unique_ptr<DrmDisplayComposition> comp = CreateComposition();
  ret = SquashFrame(active_composition_.get(), comp.get());

  // ApplyFrame needs the lock
  lock.Unlock();

  if (!ret)
    ApplyFrame(std::move(comp), 0);

  return ret;
}

// Returns:
//   - 0 if src is successfully squashed into dst
//   - -EALREADY if the src is already squashed
//   - Appropriate error if the squash fails
int DrmDisplayCompositor::SquashFrame(DrmDisplayComposition *src,
                                      DrmDisplayComposition *dst) {
  if (src->type() != DRM_COMPOSITION_TYPE_FRAME)
    return -ENOTSUP;

  std::vector<DrmCompositionPlane> &src_planes = src->composition_planes();
  std::vector<DrmHwcLayer> &src_layers = src->layers();

  // Make sure there is more than one layer to squash.
  size_t src_planes_with_layer = std::count_if(
      src_planes.begin(), src_planes.end(), [](DrmCompositionPlane &p) {
        return p.type() != DrmCompositionPlane::Type::kDisable;
      });
  if (src_planes_with_layer <= 1)
    return -EALREADY;

  int pre_comp_layer_index;

  int ret = dst->Init(drm_, src->crtc(), src->importer(), src->planner(),
                      src->frame_no());
  if (ret) {
    ALOGE("Failed to init squash all composition %d", ret);
    return ret;
  }

  DrmCompositionPlane squashed_comp(DrmCompositionPlane::Type::kPrecomp, NULL,
                                    src->crtc());
  std::vector<DrmHwcLayer> dst_layers;
  for (DrmCompositionPlane &comp_plane : src_planes) {
    // Composition planes without DRM planes should never happen
    if (comp_plane.plane() == NULL) {
      ALOGE("Skipping squash all because of NULL plane");
      ret = -EINVAL;
      goto move_layers_back;
    }

    if (comp_plane.type() == DrmCompositionPlane::Type::kDisable) {
      dst->AddPlaneDisable(comp_plane.plane());
      continue;
    }

    for (auto i : comp_plane.source_layers()) {
      DrmHwcLayer &layer = src_layers[i];

      // Squashing protected layers is impossible.
      if (layer.protected_usage()) {
        ret = -ENOTSUP;
        goto move_layers_back;
      }

      // The OutputFds point to freed memory after hwc_set returns. They are
      // returned to the default to prevent DrmDisplayComposition::Plan from
      // filling the OutputFds.
      layer.release_fence = OutputFd();
      dst_layers.emplace_back(std::move(layer));
      squashed_comp.source_layers().push_back(
          squashed_comp.source_layers().size());
    }

    if (comp_plane.plane()->type() == DRM_PLANE_TYPE_PRIMARY)
      squashed_comp.set_plane(comp_plane.plane());
    else
      dst->AddPlaneDisable(comp_plane.plane());
  }

  ret = dst->SetLayers(dst_layers.data(), dst_layers.size(), false);
  if (ret) {
    ALOGE("Failed to set layers for squash all composition %d", ret);
    goto move_layers_back;
  }

  ret = dst->AddPlaneComposition(std::move(squashed_comp));
  if (ret) {
    ALOGE("Failed to add squashed plane composition %d", ret);
    goto move_layers_back;
  }

  ret = dst->FinalizeComposition();
  if (ret) {
    ALOGE("Failed to plan for squash all composition %d", ret);
    goto move_layers_back;
  }

  ret = ApplyPreComposite(dst);
  if (ret) {
    ALOGE("Failed to pre-composite for squash all composition %d", ret);
    goto move_layers_back;
  }

  pre_comp_layer_index = dst->layers().size() - 1;
  framebuffer_index_ = (framebuffer_index_ + 1) % DRM_DISPLAY_BUFFERS;

  for (DrmCompositionPlane &plane : dst->composition_planes()) {
    if (plane.type() == DrmCompositionPlane::Type::kPrecomp) {
      // Replace source_layers with the output of the precomposite
      plane.source_layers().clear();
      plane.source_layers().push_back(pre_comp_layer_index);
      break;
    }
  }

  return 0;

// TODO(zachr): think of a better way to transfer ownership back to the active
// composition.
move_layers_back:
  for (size_t plane_index = 0;
       plane_index < src_planes.size() && plane_index < dst_layers.size();) {
    if (src_planes[plane_index].source_layers().empty()) {
      plane_index++;
      continue;
    }
    for (auto i : src_planes[plane_index].source_layers())
      src_layers[i] = std::move(dst_layers[plane_index++]);
  }

  return ret;
}

void DrmDisplayCompositor::Dump(std::ostringstream *out) const {
  int ret = pthread_mutex_lock(&lock_);
  if (ret)
    return;

  uint64_t num_frames = dump_frames_composited_;
  dump_frames_composited_ = 0;

  struct timespec ts;
  ret = clock_gettime(CLOCK_MONOTONIC, &ts);
  if (ret) {
    pthread_mutex_unlock(&lock_);
    return;
  }

  uint64_t cur_ts = ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
  uint64_t num_ms = (cur_ts - dump_last_timestamp_ns_) / (1000 * 1000);
  float fps = num_ms ? (num_frames * 1000.0f) / (num_ms) : 0.0f;

  *out << "--DrmDisplayCompositor[" << display_
       << "]: num_frames=" << num_frames << " num_ms=" << num_ms
       << " fps=" << fps << "\n";

  dump_last_timestamp_ns_ = cur_ts;

  if (active_composition_)
    active_composition_->Dump(out);

  squash_state_.Dump(out);

  pthread_mutex_unlock(&lock_);
}
}
