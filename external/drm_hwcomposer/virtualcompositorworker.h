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

#ifndef ANDROID_VIRTUAL_COMPOSITOR_WORKER_H_
#define ANDROID_VIRTUAL_COMPOSITOR_WORKER_H_

#include "drmhwcomposer.h"
#include "worker.h"

#include <queue>

namespace android {

class VirtualCompositorWorker : public Worker {
 public:
  VirtualCompositorWorker();
  ~VirtualCompositorWorker() override;

  int Init();
  void QueueComposite(hwc_display_contents_1_t *dc);

 protected:
  void Routine() override;

 private:
  struct VirtualComposition {
    UniqueFd outbuf_acquire_fence;
    std::vector<UniqueFd> layer_acquire_fences;
    int release_timeline;
  };

  int CreateNextTimelineFence();
  int FinishComposition(int timeline);
  void Compose(std::unique_ptr<VirtualComposition> composition);

  std::queue<std::unique_ptr<VirtualComposition>> composite_queue_;
  int timeline_fd_;
  int timeline_;
  int timeline_current_;
};
}

#endif
