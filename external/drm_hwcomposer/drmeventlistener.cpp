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

#define LOG_TAG "hwc-drm-event-listener"

#include "drmeventlistener.h"
#include "drmresources.h"

#include <linux/netlink.h>
#include <sys/socket.h>

#include <cutils/log.h>
#include <xf86drm.h>

namespace android {

DrmEventListener::DrmEventListener(DrmResources *drm)
    : Worker("drm-event-listener", HAL_PRIORITY_URGENT_DISPLAY),
      drm_(drm) {
}

int DrmEventListener::Init() {
  uevent_fd_.Set(socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT));
  if (uevent_fd_.get() < 0) {
    ALOGE("Failed to open uevent socket %d", uevent_fd_.get());
    return uevent_fd_.get();
  }

  struct sockaddr_nl addr;
  memset(&addr, 0, sizeof(addr));
  addr.nl_family = AF_NETLINK;
  addr.nl_pid = getpid();
  addr.nl_groups = 0xFFFFFFFF;

  int ret = bind(uevent_fd_.get(), (struct sockaddr *)&addr, sizeof(addr));
  if (ret) {
    ALOGE("Failed to bind uevent socket %d", -errno);
    return -errno;
  }

  FD_ZERO(&fds_);
  FD_SET(drm_->fd(), &fds_);
  FD_SET(uevent_fd_.get(), &fds_);
  max_fd_ = std::max(drm_->fd(), uevent_fd_.get());

  return InitWorker();
}

void DrmEventListener::RegisterHotplugHandler(DrmEventHandler *handler) {
  assert(!hotplug_handler_);
  hotplug_handler_ = handler;
}

void DrmEventListener::FlipHandler(int /* fd */, unsigned int /* sequence */,
                                   unsigned int tv_sec, unsigned int tv_usec,
                                   void *user_data) {
  DrmEventHandler *handler = (DrmEventHandler *)user_data;
  if (!handler)
    return;

  handler->HandleEvent((uint64_t)tv_sec * 1000 * 1000 + tv_usec);
  delete handler;
}

void DrmEventListener::UEventHandler() {
  char buffer[1024];
  int ret;

  struct timespec ts;
  uint64_t timestamp = 0;
  ret = clock_gettime(CLOCK_MONOTONIC, &ts);
  if (!ret)
    timestamp = ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
  else
    ALOGE("Failed to get monotonic clock on hotplug %d", ret);

  while (true) {
    ret = read(uevent_fd_.get(), &buffer, sizeof(buffer));
    if (ret == 0) {
      return;
    } else if (ret < 0) {
      ALOGE("Got error reading uevent %d", ret);
      return;
    }

    if (!hotplug_handler_)
      continue;

    bool drm_event = false, hotplug_event = false;
    for (int i = 0; i < ret;) {
      char *event = buffer + i;
      if (strcmp(event, "DEVTYPE=drm_minor"))
        drm_event = true;
      else if (strcmp(event, "HOTPLUG=1"))
        hotplug_event = true;

      i += strlen(event) + 1;
    }

    if (drm_event && hotplug_event)
      hotplug_handler_->HandleEvent(timestamp);
  }
}

void DrmEventListener::Routine() {
  int ret;
  do {
    ret = select(max_fd_ + 1, &fds_, NULL, NULL, NULL);
  } while (ret == -1 && errno == EINTR);

  if (FD_ISSET(drm_->fd(), &fds_)) {
    drmEventContext event_context = {
        .version = DRM_EVENT_CONTEXT_VERSION,
        .vblank_handler = NULL,
        .page_flip_handler = DrmEventListener::FlipHandler};
    drmHandleEvent(drm_->fd(), &event_context);
  }

  if (FD_ISSET(uevent_fd_.get(), &fds_))
    UEventHandler();
}
}
