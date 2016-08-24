// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_NOTIFICATION_NOTIFICATION_CHANNEL_H_
#define LIBWEAVE_SRC_NOTIFICATION_NOTIFICATION_CHANNEL_H_

#include <string>

namespace base {
class DictionaryValue;
}  // namespace base

namespace weave {

class NotificationDelegate;

class NotificationChannel {
 public:
  virtual ~NotificationChannel() {}

  virtual std::string GetName() const = 0;
  virtual bool IsConnected() const = 0;
  virtual void AddChannelParameters(base::DictionaryValue* channel_json) = 0;

  virtual void Start(NotificationDelegate* delegate) = 0;
  virtual void Stop() = 0;
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_NOTIFICATION_NOTIFICATION_CHANNEL_H_
