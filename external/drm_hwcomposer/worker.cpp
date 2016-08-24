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

#define LOG_TAG "hwc-drm-worker"

#include "worker.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <time.h>

#include <cutils/log.h>

namespace android {

static const int64_t kBillion = 1000000000LL;

Worker::Worker(const char *name, int priority)
    : name_(name), priority_(priority), exit_(false), initialized_(false) {
}

Worker::~Worker() {
  if (!initialized_)
    return;

  pthread_kill(thread_, SIGTERM);
  pthread_cond_destroy(&cond_);
  pthread_mutex_destroy(&lock_);
}

int Worker::InitWorker() {
  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
  int ret = pthread_cond_init(&cond_, &cond_attr);
  if (ret) {
    ALOGE("Failed to int thread %s condition %d", name_.c_str(), ret);
    return ret;
  }

  ret = pthread_mutex_init(&lock_, NULL);
  if (ret) {
    ALOGE("Failed to init thread %s lock %d", name_.c_str(), ret);
    pthread_cond_destroy(&cond_);
    return ret;
  }

  ret = pthread_create(&thread_, NULL, InternalRoutine, this);
  if (ret) {
    ALOGE("Could not create thread %s %d", name_.c_str(), ret);
    pthread_mutex_destroy(&lock_);
    pthread_cond_destroy(&cond_);
    return ret;
  }
  initialized_ = true;
  return 0;
}

bool Worker::initialized() const {
  return initialized_;
}

int Worker::Lock() {
  return pthread_mutex_lock(&lock_);
}

int Worker::Unlock() {
  return pthread_mutex_unlock(&lock_);
}

int Worker::SignalLocked() {
  return SignalThreadLocked(false);
}

int Worker::ExitLocked() {
  int signal_ret = SignalThreadLocked(true);
  if (signal_ret)
    ALOGE("Failed to signal thread %s with exit %d", name_.c_str(), signal_ret);

  int join_ret = pthread_join(thread_, NULL);
  if (join_ret && join_ret != ESRCH)
    ALOGE("Failed to join thread %s in exit %d", name_.c_str(), join_ret);

  return signal_ret | join_ret;
}

int Worker::Signal() {
  int ret = Lock();
  if (ret) {
    ALOGE("Failed to acquire lock in Signal() %d\n", ret);
    return ret;
  }

  int signal_ret = SignalLocked();

  ret = Unlock();
  if (ret) {
    ALOGE("Failed to release lock in Signal() %d\n", ret);
    return ret;
  }
  return signal_ret;
}

int Worker::Exit() {
  int ret = Lock();
  if (ret) {
    ALOGE("Failed to acquire lock in Exit() %d\n", ret);
    return ret;
  }

  int exit_ret = ExitLocked();

  ret = Unlock();
  if (ret) {
    ALOGE("Failed to release lock in Exit() %d\n", ret);
    return ret;
  }
  return exit_ret;
}

int Worker::WaitForSignalOrExitLocked(int64_t max_nanoseconds) {
  if (exit_)
    return -EINTR;

  int ret = 0;
  if (max_nanoseconds < 0) {
    ret = pthread_cond_wait(&cond_, &lock_);
  } else {
    struct timespec abs_deadline;
    ret = clock_gettime(CLOCK_MONOTONIC, &abs_deadline);
    if (ret)
      return ret;
    int64_t nanos = (int64_t)abs_deadline.tv_nsec + max_nanoseconds;
    abs_deadline.tv_sec += nanos / kBillion;
    abs_deadline.tv_nsec = nanos % kBillion;
    ret = pthread_cond_timedwait(&cond_, &lock_, &abs_deadline);
    if (ret == ETIMEDOUT)
      ret = -ETIMEDOUT;
  }

  if (exit_)
    return -EINTR;

  return ret;
}

// static
void *Worker::InternalRoutine(void *arg) {
  Worker *worker = (Worker *)arg;

  setpriority(PRIO_PROCESS, 0, worker->priority_);

  while (true) {
    int ret = worker->Lock();
    if (ret) {
      ALOGE("Failed to lock %s thread %d", worker->name_.c_str(), ret);
      continue;
    }

    bool exit = worker->exit_;

    ret = worker->Unlock();
    if (ret) {
      ALOGE("Failed to unlock %s thread %d", worker->name_.c_str(), ret);
      break;
    }
    if (exit)
      break;

    worker->Routine();
  }
  return NULL;
}

int Worker::SignalThreadLocked(bool exit) {
  if (exit)
    exit_ = exit;

  int ret = pthread_cond_signal(&cond_);
  if (ret) {
    ALOGE("Failed to signal condition on %s thread %d", name_.c_str(), ret);
    return ret;
  }

  return 0;
}
}
