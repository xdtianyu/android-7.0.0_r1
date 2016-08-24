/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ANDROID_DRM_EVENT_LISTENER_H_
#define ANDROID_DRM_EVENT_LISTENER_H_

#include "autofd.h"
#include "worker.h"

namespace android {

class DrmResources;

class DrmEventHandler {
 public:
  DrmEventHandler() {
  }
  virtual ~DrmEventHandler() {
  }

  virtual void HandleEvent(uint64_t timestamp_us) = 0;
};

class DrmEventListener : public Worker {
 public:
  DrmEventListener(DrmResources *drm);
  virtual ~DrmEventListener() {
  }

  int Init();

  void RegisterHotplugHandler(DrmEventHandler *handler);

  static void FlipHandler(int fd, unsigned int sequence, unsigned int tv_sec,
                          unsigned int tv_usec, void *user_data);

 protected:
  virtual void Routine();

 private:
  void UEventHandler();

  fd_set fds_;
  UniqueFd uevent_fd_;
  int max_fd_ = -1;

  DrmResources *drm_;
  DrmEventHandler *hotplug_handler_ = NULL;
};
}

#endif
