// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_NOTIFICATION_NOTIFICATION_DELEGATE_H_
#define LIBWEAVE_SRC_NOTIFICATION_NOTIFICATION_DELEGATE_H_

#include <memory>
#include <string>

#include <base/values.h>

namespace weave {

class NotificationDelegate {
 public:
  virtual void OnConnected(const std::string& channel_name) = 0;
  virtual void OnDisconnected() = 0;
  virtual void OnPermanentFailure() = 0;
  // Called when a new command is sent via the notification channel.
  virtual void OnCommandCreated(const base::DictionaryValue& command,
                                const std::string& channel_name) = 0;
  // Called when DEVICE_DELETED notification is received.
  virtual void OnDeviceDeleted(const std::string& cloud_id) = 0;

 protected:
  virtual ~NotificationDelegate() {}
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_NOTIFICATION_NOTIFICATION_DELEGATE_H_
