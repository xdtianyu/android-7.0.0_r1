/*
 * Copyright (C) 2015 Intel Corporation
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

#include <cutils/log.h>
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>
#include <hardware/sensors.h>
#include "AcquisitionThread.hpp"
#include "Utils.hpp"

void* AcquisitionThread::acquisitionRoutine(void *param) {
  AcquisitionThread *acquisitionThread = reinterpret_cast<AcquisitionThread *>(param);
  Sensor *sensor = nullptr;
  int64_t timestamp = 0;
  struct timespec target_time;
  int rc = 0;
  sensors_event_t data;

  if (acquisitionThread == nullptr) {
    ALOGE("%s: The acquisition thread was not initialized", __func__);
    return nullptr;
  }

  sensor = acquisitionThread->sensor;
  if (sensor == nullptr) {
    ALOGE("%s: The acquisition thread doesn't have an associated sensor", __func__);
    return nullptr;
  }

  /* initialize sensor data structure */
  memset(&data, 0, sizeof(sensors_event_t));
  data.version = sizeof(sensors_event_t);
  data.sensor = sensor->getHandle();
  data.type = sensor->getType();

  pthread_mutex_lock(&acquisitionThread->pthreadMutex);

  /* get current timestamp */
  timestamp = get_timestamp_monotonic();

  /* loop until the thread is canceled */
  while(acquisitionThread->getWritePipeFd() != -1) {
    /* get one event from the sensor */
    if (sensor->pollEvents(&data, 1) != 1) {
      ALOGE("%s: Sensor %d: Cannot read data", __func__, data.sensor);
      goto exit;
    }

    /* send the data over the pipe to the main thread */
    rc = write(acquisitionThread->getWritePipeFd(), &data, sizeof(sensors_event_t));
    if (rc != sizeof(sensors_event_t)) {
      ALOGE("%s: Sensor %d: Cannot write data to pipe", __func__, data.sensor);
      goto exit;
    }

    if (acquisitionThread->getWritePipeFd() == -1) {
      ALOGE("%s: Sensor %d: The write pipe file descriptor is invalid", __func__, data.sensor);
      goto exit;
    }

    timestamp += sensor->getDelay();
    set_timestamp(&target_time, timestamp);
    pthread_cond_timedwait(&acquisitionThread->pthreadCond, &acquisitionThread->pthreadMutex, &target_time);
  }

exit:
  pthread_mutex_unlock(&acquisitionThread->pthreadMutex);
  return nullptr;
}

AcquisitionThread::AcquisitionThread(int pollFd, Sensor *sensor)
    : pollFd(pollFd), sensor(sensor), initialized(false) {
  pipeFds[0] = pipeFds[1] = -1;
}

bool AcquisitionThread::init() {
  struct epoll_event ev;
  int rc;

  memset(&ev, 0, sizeof(ev));

  if (initialized) {
    ALOGE("%s: acquisition thread already initialized", __func__);
    return false;
  }

  /* create condition variable & mutex for quick thread release */
  rc = pthread_condattr_init(&pthreadCondAttr);
  if (rc != 0) {
    ALOGE("%s: Cannot initialize the pthread condattr", __func__);
    return false;
  }
  rc = pthread_condattr_setclock(&pthreadCondAttr, CLOCK_MONOTONIC);
  if (rc != 0) {
    ALOGE("%s: Cannot set the clock of condattr to monotonic", __func__);
    return false;
  }
  rc = pthread_cond_init(&pthreadCond, &pthreadCondAttr);
  if (rc != 0) {
    ALOGE("%s: Cannot intialize the pthread structure", __func__);
    return false;
  }
  rc = pthread_mutex_init(&pthreadMutex, nullptr);
  if (rc != 0) {
    ALOGE("%s: Cannot initialize the mutex associated with the pthread cond", __func__);
    goto mutex_err;
  }

  /* create pipe to signal events to the main thread */
  rc = pipe2(pipeFds, O_NONBLOCK);
  if (rc != 0) {
    ALOGE("%s: Cannot initialize pipe", __func__);
    goto pipe_err;
  }

  ev.events = EPOLLIN;
  ev.data.u32 = sensor->getHandle();

  /* add read pipe fd to pollFd */
  rc = epoll_ctl(pollFd, EPOLL_CTL_ADD, pipeFds[0] , &ev);
  if (rc != 0) {
    ALOGE("%s: Cannot add the read file descriptor to poll set", __func__);
    goto epoll_err;
  }

  /* launch the thread */
  rc = pthread_create(&pthread, nullptr, acquisitionRoutine, (void *)this);
  if (rc != 0) {
    ALOGE("%s: Cannot create acquisition pthread", __func__);
    goto thread_create_err;
  }

  initialized = true;
  return true;

thread_create_err:
  epoll_ctl(pollFd, EPOLL_CTL_DEL, pipeFds[0], nullptr);
epoll_err:
  close(pipeFds[0]);
  close(pipeFds[1]);
  pipeFds[0] = pipeFds[1] = -1;
pipe_err:
  pthread_mutex_destroy(&pthreadMutex);
mutex_err:
  pthread_cond_destroy(&pthreadCond);
  return false;
}

bool AcquisitionThread::generateFlushCompleteEvent() {
  sensors_event_t data;
  int rc;

  if (!initialized) {
    return false;
  }

  /* batching is not supported; return one META_DATA_FLUSH_COMPLETE event */
  memset(&data, 0, sizeof(sensors_event_t));
  data.version = META_DATA_VERSION;
  data.type = SENSOR_TYPE_META_DATA;
  data.meta_data.sensor = sensor->getHandle();
  data.meta_data.what = META_DATA_FLUSH_COMPLETE;

  /*
   * Send the event via the associated pipe. It doesn't need to be in a loop
   * as O_NONBLOCK is enabled and the number of bytes is <= PIPE_BUF.
   * If there is room to write n bytes to the pipe, then write succeeds
   * immediately, writing all n bytes; otherwise write fails.
   */
  rc = write(getWritePipeFd(), &data, sizeof(sensors_event_t));
  if (rc != sizeof(sensors_event_t)) {
    ALOGE("%s: not all data has been sent over the pipe", __func__);
    return false;
  }

  return true;
}

int AcquisitionThread::wakeup() {
  if (initialized) {
    return pthread_cond_signal(&pthreadCond);
  }

  return -EINVAL;
}

AcquisitionThread::~AcquisitionThread() {
  int readPipeEnd, writePipeEnd;

  if (initialized) {
    readPipeEnd = pipeFds[0];
    writePipeEnd = pipeFds[1];
    epoll_ctl(pollFd, EPOLL_CTL_DEL, readPipeEnd, nullptr);

    /* take the mutex to correctly signal the thread */
    pthread_mutex_lock(&pthreadMutex);
    pipeFds[0] = pipeFds[1] = -1;
    close(readPipeEnd);
    close(writePipeEnd);

    /* wakeup and wait for the thread */
    pthread_cond_signal(&pthreadCond);
    pthread_mutex_unlock(&pthreadMutex);
    pthread_join(pthread, nullptr);

    pthread_cond_destroy(&pthreadCond);
    pthread_mutex_destroy(&pthreadMutex);
  }
}
