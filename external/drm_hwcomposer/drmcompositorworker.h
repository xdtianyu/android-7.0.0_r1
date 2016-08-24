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

#ifndef ANDROID_DRM_COMPOSITOR_WORKER_H_
#define ANDROID_DRM_COMPOSITOR_WORKER_H_

#include "worker.h"

namespace android {

class DrmDisplayCompositor;

class DrmCompositorWorker : public Worker {
 public:
  DrmCompositorWorker(DrmDisplayCompositor *compositor);
  ~DrmCompositorWorker() override;

  int Init();

 protected:
  void Routine() override;

  DrmDisplayCompositor *compositor_;
  bool did_squash_all_ = false;
};
}

#endif
