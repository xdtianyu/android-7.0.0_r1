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

#include <pthread.h>

namespace android {

class AutoLock {
 public:
  AutoLock(pthread_mutex_t *mutex, const char *const name)
      : mutex_(mutex), name_(name) {
  }
  ~AutoLock() {
    if (locked_)
      Unlock();
  }

  AutoLock(const AutoLock &rhs) = delete;
  AutoLock &operator=(const AutoLock &rhs) = delete;

  int Lock();
  int Unlock();

 private:
  pthread_mutex_t *const mutex_;
  bool locked_ = false;
  const char *const name_;
};
}
