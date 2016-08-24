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

#define LOG_TAG "hwc-virtual-compositor-worker"

#include "virtualcompositorworker.h"
#include "worker.h"

#include <errno.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <sched.h>
#include <sw_sync.h>
#include <sync/sync.h>

namespace android {

static const int kMaxQueueDepth = 3;
static const int kAcquireWaitTimeoutMs = 3000;

VirtualCompositorWorker::VirtualCompositorWorker()
    : Worker("virtual-compositor", HAL_PRIORITY_URGENT_DISPLAY),
      timeline_fd_(-1),
      timeline_(0),
      timeline_current_(0) {
}

VirtualCompositorWorker::~VirtualCompositorWorker() {
  if (timeline_fd_ >= 0) {
    FinishComposition(timeline_);
    close(timeline_fd_);
    timeline_fd_ = -1;
  }
}

int VirtualCompositorWorker::Init() {
  int ret = sw_sync_timeline_create();
  if (ret < 0) {
    ALOGE("Failed to create sw sync timeline %d", ret);
    return ret;
  }
  timeline_fd_ = ret;
  return InitWorker();
}

void VirtualCompositorWorker::QueueComposite(hwc_display_contents_1_t *dc) {
  std::unique_ptr<VirtualComposition> composition(new VirtualComposition);

  composition->outbuf_acquire_fence.Set(dc->outbufAcquireFenceFd);
  dc->outbufAcquireFenceFd = -1;
  if (dc->retireFenceFd >= 0)
    close(dc->retireFenceFd);
  dc->retireFenceFd = CreateNextTimelineFence();

  for (size_t i = 0; i < dc->numHwLayers; ++i) {
    hwc_layer_1_t *layer = &dc->hwLayers[i];
    if (layer->flags & HWC_SKIP_LAYER)
      continue;
    composition->layer_acquire_fences.emplace_back(layer->acquireFenceFd);
    layer->acquireFenceFd = -1;
    if (layer->releaseFenceFd >= 0)
      close(layer->releaseFenceFd);
    layer->releaseFenceFd = CreateNextTimelineFence();
  }

  composition->release_timeline = timeline_;

  Lock();
  while (composite_queue_.size() >= kMaxQueueDepth) {
    Unlock();
    sched_yield();
    Lock();
  }

  composite_queue_.push(std::move(composition));
  SignalLocked();
  Unlock();
}

void VirtualCompositorWorker::Routine() {
  int ret = Lock();
  if (ret) {
    ALOGE("Failed to lock worker, %d", ret);
    return;
  }

  int wait_ret = 0;
  if (composite_queue_.empty()) {
    wait_ret = WaitForSignalOrExitLocked();
  }

  std::unique_ptr<VirtualComposition> composition;
  if (!composite_queue_.empty()) {
    composition = std::move(composite_queue_.front());
    composite_queue_.pop();
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

  Compose(std::move(composition));
}

int VirtualCompositorWorker::CreateNextTimelineFence() {
  ++timeline_;
  return sw_sync_fence_create(timeline_fd_, "drm_fence", timeline_);
}

int VirtualCompositorWorker::FinishComposition(int point) {
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

void VirtualCompositorWorker::Compose(
    std::unique_ptr<VirtualComposition> composition) {
  if (!composition.get())
    return;

  int ret;
  int outbuf_acquire_fence = composition->outbuf_acquire_fence.get();
  if (outbuf_acquire_fence >= 0) {
    ret = sync_wait(outbuf_acquire_fence, kAcquireWaitTimeoutMs);
    if (ret) {
      ALOGE("Failed to wait for outbuf acquire %d/%d", outbuf_acquire_fence,
            ret);
      return;
    }
    composition->outbuf_acquire_fence.Close();
  }
  for (size_t i = 0; i < composition->layer_acquire_fences.size(); ++i) {
    int layer_acquire_fence = composition->layer_acquire_fences[i].get();
    if (layer_acquire_fence >= 0) {
      ret = sync_wait(layer_acquire_fence, kAcquireWaitTimeoutMs);
      if (ret) {
        ALOGE("Failed to wait for layer acquire %d/%d", layer_acquire_fence,
              ret);
        return;
      }
      composition->layer_acquire_fences[i].Close();
    }
  }
  FinishComposition(composition->release_timeline);
}
}
