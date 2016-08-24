// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/device_delegate.h"

#include <base/guid.h>
#include <weave/provider/task_runner.h>

#include "src/privet/constants.h"

namespace weave {
namespace privet {

namespace {

class DeviceDelegateImpl : public DeviceDelegate {
 public:
  DeviceDelegateImpl(provider::TaskRunner* task_runner,
                     uint16_t http_port,
                     uint16_t https_port,
                     base::TimeDelta http_request_timeout)
      : task_runner_{task_runner},
        http_request_timeout_{http_request_timeout},
        http_port_{http_port},
        https_port_{https_port} {}
  ~DeviceDelegateImpl() override = default;

  std::pair<uint16_t, uint16_t> GetHttpEnpoint() const override {
    return std::make_pair(http_port_, http_port_);
  }
  std::pair<uint16_t, uint16_t> GetHttpsEnpoint() const override {
    return std::make_pair(https_port_, https_port_);
  }
  base::TimeDelta GetHttpRequestTimeout() const override {
    return http_request_timeout_;
  }

  void PostDelayedTask(const tracked_objects::Location& from_here,
                       const base::Closure& task,
                       base::TimeDelta delay) override {
    task_runner_->PostDelayedTask(from_here, task, delay);
  }

 private:
  provider::TaskRunner* task_runner_;
  base::TimeDelta http_request_timeout_;
  uint16_t http_port_{0};
  uint16_t https_port_{0};
};

}  // namespace

DeviceDelegate::DeviceDelegate() {}

DeviceDelegate::~DeviceDelegate() {}

// static
std::unique_ptr<DeviceDelegate> DeviceDelegate::CreateDefault(
    provider::TaskRunner* task_runner,
    uint16_t http_port,
    uint16_t https_port,
    base::TimeDelta http_request_timeout) {
  return std::unique_ptr<DeviceDelegate>(new DeviceDelegateImpl(
      task_runner, http_port, https_port, http_request_timeout));
}

}  // namespace privet
}  // namespace weave
