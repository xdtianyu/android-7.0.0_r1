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

#ifndef ANDROID_DRM_DISPLAY_COMPOSITOR_H_
#define ANDROID_DRM_DISPLAY_COMPOSITOR_H_

#include "drmhwcomposer.h"
#include "drmcomposition.h"
#include "drmcompositorworker.h"
#include "drmframebuffer.h"
#include "separate_rects.h"

#include <pthread.h>
#include <memory>
#include <queue>
#include <sstream>
#include <tuple>

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>

// One for the front, one for the back, and one for cases where we need to
// squash a frame that the hw can't display with hw overlays.
#define DRM_DISPLAY_BUFFERS 3

namespace android {

class GLWorkerCompositor;

class SquashState {
 public:
  static const unsigned kHistoryLength = 6;  // TODO: make this number not magic
  static const unsigned kMaxLayers = 64;

  struct Region {
    DrmHwcRect<int> rect;
    std::bitset<kMaxLayers> layer_refs;
    std::bitset<kHistoryLength> change_history;
    bool squashed = false;
  };

  bool is_stable(int region_index) const {
    return valid_history_ >= kHistoryLength &&
           regions_[region_index].change_history.none();
  }

  const std::vector<Region> &regions() const {
    return regions_;
  }

  void Init(DrmHwcLayer *layers, size_t num_layers);
  void GenerateHistory(DrmHwcLayer *layers, size_t num_layers,
                       std::vector<bool> &changed_regions) const;
  void StableRegionsWithMarginalHistory(
      const std::vector<bool> &changed_regions,
      std::vector<bool> &stable_regions) const;
  void RecordHistory(DrmHwcLayer *layers, size_t num_layers,
                     const std::vector<bool> &changed_regions);
  bool RecordAndCompareSquashed(const std::vector<bool> &squashed_regions);

  void Dump(std::ostringstream *out) const;

 private:
  size_t generation_number_ = 0;
  unsigned valid_history_ = 0;
  std::vector<buffer_handle_t> last_handles_;

  std::vector<Region> regions_;
};

class DrmDisplayCompositor {
 public:
  DrmDisplayCompositor();
  ~DrmDisplayCompositor();

  int Init(DrmResources *drm, int display);

  std::unique_ptr<DrmDisplayComposition> CreateComposition() const;
  int QueueComposition(std::unique_ptr<DrmDisplayComposition> composition);
  int Composite();
  int SquashAll();
  void Dump(std::ostringstream *out) const;

  std::tuple<uint32_t, uint32_t, int> GetActiveModeResolution();

  bool HaveQueuedComposites() const;

  SquashState *squash_state() {
    return &squash_state_;
  }

 private:
  struct FrameState {
    std::unique_ptr<DrmDisplayComposition> composition;
    int status = 0;
  };

  class FrameWorker : public Worker {
   public:
    FrameWorker(DrmDisplayCompositor *compositor);
    ~FrameWorker() override;

    int Init();
    void QueueFrame(std::unique_ptr<DrmDisplayComposition> composition,
                    int status);

   protected:
    void Routine() override;

   private:
    DrmDisplayCompositor *compositor_;
    std::queue<FrameState> frame_queue_;
  };

  struct ModeState {
    bool needs_modeset = false;
    DrmMode mode;
    uint32_t blob_id = 0;
    uint32_t old_blob_id = 0;
  };

  DrmDisplayCompositor(const DrmDisplayCompositor &) = delete;

  // We'll wait for acquire fences to fire for kAcquireWaitTimeoutMs,
  // kAcquireWaitTries times, logging a warning in between.
  static const int kAcquireWaitTries = 5;
  static const int kAcquireWaitTimeoutMs = 100;

  int PrepareFramebuffer(DrmFramebuffer &fb,
                         DrmDisplayComposition *display_comp);
  int ApplySquash(DrmDisplayComposition *display_comp);
  int ApplyPreComposite(DrmDisplayComposition *display_comp);
  int PrepareFrame(DrmDisplayComposition *display_comp);
  int CommitFrame(DrmDisplayComposition *display_comp, bool test_only);
  int SquashFrame(DrmDisplayComposition *src, DrmDisplayComposition *dst);
  int ApplyDpms(DrmDisplayComposition *display_comp);
  int DisablePlanes(DrmDisplayComposition *display_comp);

  void ClearDisplay();
  void ApplyFrame(std::unique_ptr<DrmDisplayComposition> composition,
                  int status);

  std::tuple<int, uint32_t> CreateModeBlob(const DrmMode &mode);

  DrmResources *drm_;
  int display_;

  DrmCompositorWorker worker_;
  FrameWorker frame_worker_;

  std::queue<std::unique_ptr<DrmDisplayComposition>> composite_queue_;
  std::unique_ptr<DrmDisplayComposition> active_composition_;

  bool initialized_;
  bool active_;
  bool use_hw_overlays_;

  ModeState mode_;

  int framebuffer_index_;
  DrmFramebuffer framebuffers_[DRM_DISPLAY_BUFFERS];
  std::unique_ptr<GLWorkerCompositor> pre_compositor_;

  SquashState squash_state_;
  int squash_framebuffer_index_;
  DrmFramebuffer squash_framebuffers_[2];

  // mutable since we need to acquire in HaveQueuedComposites
  mutable pthread_mutex_t lock_;

  // State tracking progress since our last Dump(). These are mutable since
  // we need to reset them on every Dump() call.
  mutable uint64_t dump_frames_composited_;
  mutable uint64_t dump_last_timestamp_ns_;
};
}

#endif  // ANDROID_DRM_DISPLAY_COMPOSITOR_H_
