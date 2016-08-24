// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_DEVICE_DELEGATE_H_
#define LIBWEAVE_SRC_PRIVET_DEVICE_DELEGATE_H_

#include <memory>
#include <utility>

#include <base/callback.h>
#include <base/location.h>
#include <base/time/time.h>

namespace weave {

namespace provider {
class TaskRunner;
}

namespace privet {

// Interface to provide access to general information about device.
class DeviceDelegate {
 public:
  DeviceDelegate();
  virtual ~DeviceDelegate();

  // Returns HTTP ports for Privet. The first one is the primary port,
  // the second is the port for a pooling updates requests. The second value
  // could be 0. In this case the first port would be used for regular and for
  // updates requests.
  virtual std::pair<uint16_t, uint16_t> GetHttpEnpoint() const = 0;

  // The same |GetHttpEnpoint| but for HTTPS.
  virtual std::pair<uint16_t, uint16_t> GetHttpsEnpoint() const = 0;

  // Returns the max request timeout of http server. Returns TimeDelta::Max() if
  // no timeout is set.
  virtual base::TimeDelta GetHttpRequestTimeout() const = 0;

  // Schedules a background task on the embedded TaskRunner.
  virtual void PostDelayedTask(const tracked_objects::Location& from_here,
                               const base::Closure& task,
                               base::TimeDelta delay) = 0;

  // Create default instance.
  static std::unique_ptr<DeviceDelegate> CreateDefault(
      provider::TaskRunner* task_runner,
      uint16_t http_port,
      uint16_t https_port,
      base::TimeDelta http_request_timeout);
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_DEVICE_DELEGATE_H_
