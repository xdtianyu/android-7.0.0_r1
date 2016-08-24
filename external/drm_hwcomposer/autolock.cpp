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
#define LOG_TAG "hwc-drm-auto-lock"

#include "autolock.h"

#include <errno.h>
#include <pthread.h>

#include <cutils/log.h>

namespace android {

int AutoLock::Lock() {
  if (locked_) {
    ALOGE("Invalid attempt to double lock AutoLock %s", name_);
    return -EINVAL;
  }
  int ret = pthread_mutex_lock(mutex_);
  if (ret) {
    ALOGE("Failed to acquire %s lock %d", name_, ret);
    return ret;
  }
  locked_ = true;
  return 0;
}

int AutoLock::Unlock() {
  if (!locked_) {
    ALOGE("Invalid attempt to unlock unlocked AutoLock %s", name_);
    return -EINVAL;
  }
  int ret = pthread_mutex_unlock(mutex_);
  if (ret) {
    ALOGE("Failed to release %s lock %d", name_, ret);
    return ret;
  }
  locked_ = false;
  return 0;
}
}
