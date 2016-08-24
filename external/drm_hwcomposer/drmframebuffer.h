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

#ifndef ANDROID_DRM_FRAMEBUFFER_
#define ANDROID_DRM_FRAMEBUFFER_

#include <stdint.h>

#include <sync/sync.h>

#include <ui/GraphicBuffer.h>

namespace android {

struct DrmFramebuffer {
  DrmFramebuffer() : release_fence_fd_(-1) {
  }

  ~DrmFramebuffer() {
    if (release_fence_fd() >= 0)
      close(release_fence_fd());
  }

  bool is_valid() {
    return buffer_ != NULL;
  }

  sp<GraphicBuffer> buffer() {
    return buffer_;
  }

  int release_fence_fd() {
    return release_fence_fd_;
  }

  void set_release_fence_fd(int fd) {
    if (release_fence_fd_ >= 0)
      close(release_fence_fd_);
    release_fence_fd_ = fd;
  }

  bool Allocate(uint32_t w, uint32_t h) {
    if (is_valid()) {
      if (buffer_->getWidth() == w && buffer_->getHeight() == h)
        return true;

      if (release_fence_fd_ >= 0) {
        if (sync_wait(release_fence_fd_, kReleaseWaitTimeoutMs) != 0) {
          ALOGE("Wait for release fence failed\n");
          return false;
        }
      }
      Clear();
    }
    buffer_ = new GraphicBuffer(w, h, PIXEL_FORMAT_RGBA_8888,
                                GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_RENDER |
                                    GRALLOC_USAGE_HW_COMPOSER);
    release_fence_fd_ = -1;
    return is_valid();
  }

  void Clear() {
    if (!is_valid())
      return;

    if (release_fence_fd_ >= 0) {
      close(release_fence_fd_);
      release_fence_fd_ = -1;
    }

    buffer_.clear();
  }

  int WaitReleased(int timeout_milliseconds) {
    if (!is_valid())
      return 0;
    if (release_fence_fd_ < 0)
      return 0;

    int ret = sync_wait(release_fence_fd_, timeout_milliseconds);
    return ret;
  }

  // Somewhat arbitrarily chosen, but wanted to stay below 3000ms, which is the
  // system timeout
  static const int kReleaseWaitTimeoutMs = 1500;

 private:
  sp<GraphicBuffer> buffer_;
  int release_fence_fd_;
};
}

#endif  // ANDROID_DRM_FRAMEBUFFER_
