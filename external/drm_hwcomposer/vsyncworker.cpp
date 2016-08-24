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

#define LOG_TAG "hwc-vsync-worker"

#include "drmresources.h"
#include "vsyncworker.h"
#include "worker.h"

#include <map>
#include <stdlib.h>
#include <time.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <cutils/log.h>
#include <hardware/hardware.h>

namespace android {

VSyncWorker::VSyncWorker()
    : Worker("vsync", HAL_PRIORITY_URGENT_DISPLAY),
      drm_(NULL),
      procs_(NULL),
      display_(-1),
      last_timestamp_(-1) {
}

VSyncWorker::~VSyncWorker() {
}

int VSyncWorker::Init(DrmResources *drm, int display) {
  drm_ = drm;
  display_ = display;

  return InitWorker();
}

int VSyncWorker::SetProcs(hwc_procs_t const *procs) {
  int ret = Lock();
  if (ret) {
    ALOGE("Failed to lock vsync worker lock %d\n", ret);
    return ret;
  }

  procs_ = procs;

  ret = Unlock();
  if (ret) {
    ALOGE("Failed to unlock vsync worker lock %d\n", ret);
    return ret;
  }
  return 0;
}

int VSyncWorker::VSyncControl(bool enabled) {
  int ret = Lock();
  if (ret) {
    ALOGE("Failed to lock vsync worker lock %d\n", ret);
    return ret;
  }

  enabled_ = enabled;
  last_timestamp_ = -1;
  int signal_ret = SignalLocked();

  ret = Unlock();
  if (ret) {
    ALOGE("Failed to unlock vsync worker lock %d\n", ret);
    return ret;
  }

  return signal_ret;
}

/*
 * Returns the timestamp of the next vsync in phase with last_timestamp_.
 * For example:
 *  last_timestamp_ = 137
 *  frame_ns = 50
 *  current = 683
 *
 *  ret = (50 * ((683 - 137)/50 + 1)) + 137
 *  ret = 687
 *
 *  Thus, we must sleep until timestamp 687 to maintain phase with the last
 *  timestamp.
 */
int64_t VSyncWorker::GetPhasedVSync(int64_t frame_ns, int64_t current) {
  if (last_timestamp_ < 0)
    return current + frame_ns;

  return frame_ns * ((current - last_timestamp_) / frame_ns + 1) +
         last_timestamp_;
}

static const int64_t kOneSecondNs = 1 * 1000 * 1000 * 1000;

int VSyncWorker::SyntheticWaitVBlank(int64_t *timestamp) {
  struct timespec vsync;
  int ret = clock_gettime(CLOCK_MONOTONIC, &vsync);

  float refresh = 60.0f;  // Default to 60Hz refresh rate
  DrmConnector *conn = drm_->GetConnectorForDisplay(display_);
  if (conn && conn->active_mode().v_refresh() != 0.0f)
    refresh = conn->active_mode().v_refresh();
  else
    ALOGW("Vsync worker active with conn=%p refresh=%f\n", conn,
          conn ? conn->active_mode().v_refresh() : 0.0f);

  int64_t phased_timestamp = GetPhasedVSync(
      kOneSecondNs / refresh, vsync.tv_sec * kOneSecondNs + vsync.tv_nsec);
  vsync.tv_sec = phased_timestamp / kOneSecondNs;
  vsync.tv_nsec = phased_timestamp - (vsync.tv_sec * kOneSecondNs);
  do {
    ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &vsync, NULL);
  } while (ret == -1 && errno == EINTR);
  if (ret)
    return ret;

  *timestamp = (int64_t)vsync.tv_sec * kOneSecondNs + (int64_t)vsync.tv_nsec;
  return 0;
}

void VSyncWorker::Routine() {
  int ret = Lock();
  if (ret) {
    ALOGE("Failed to lock worker %d", ret);
    return;
  }

  if (!enabled_) {
    ret = WaitForSignalOrExitLocked();
    if (ret == -EINTR) {
      return;
    }
  }

  bool enabled = enabled_;
  int display = display_;
  hwc_procs_t const *procs = procs_;

  ret = Unlock();
  if (ret) {
    ALOGE("Failed to unlock worker %d", ret);
  }

  if (!enabled)
    return;

  DrmCrtc *crtc = drm_->GetCrtcForDisplay(display);
  if (!crtc) {
    ALOGE("Failed to get crtc for display");
    return;
  }
  uint32_t high_crtc = (crtc->pipe() << DRM_VBLANK_HIGH_CRTC_SHIFT);

  drmVBlank vblank;
  memset(&vblank, 0, sizeof(vblank));
  vblank.request.type = (drmVBlankSeqType)(
      DRM_VBLANK_RELATIVE | (high_crtc & DRM_VBLANK_HIGH_CRTC_MASK));
  vblank.request.sequence = 1;

  int64_t timestamp;
  ret = drmWaitVBlank(drm_->fd(), &vblank);
  if (ret == -EINTR) {
    return;
  } else if (ret) {
    ret = SyntheticWaitVBlank(&timestamp);
    if (ret)
      return;
  } else {
    timestamp = (int64_t)vblank.reply.tval_sec * kOneSecondNs +
                (int64_t)vblank.reply.tval_usec * 1000;
  }

  /*
   * There's a race here where a change in procs_ will not take effect until
   * the next subsequent requested vsync. This is unavoidable since we can't
   * call the vsync hook while holding the thread lock.
   *
   * We could shorten the race window by caching procs_ right before calling
   * the hook. However, in practice, procs_ is only updated once, so it's not
   * worth the overhead.
   */
  if (procs && procs->vsync)
    procs->vsync(procs, display, timestamp);
  last_timestamp_ = timestamp;
}
}
