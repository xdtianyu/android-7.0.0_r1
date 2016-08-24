// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/notification/notification_parser.h"

#include <base/logging.h>

namespace weave {

namespace {

// Processes COMMAND_CREATED notifications.
bool ParseCommandCreated(const base::DictionaryValue& notification,
                         NotificationDelegate* delegate,
                         const std::string& channel_name) {
  const base::DictionaryValue* command = nullptr;
  if (!notification.GetDictionary("command", &command)) {
    LOG(ERROR) << "COMMAND_CREATED notification is missing 'command' property";
    return false;
  }

  delegate->OnCommandCreated(*command, channel_name);
  return true;
}

// Processes DEVICE_DELETED notifications.
bool ParseDeviceDeleted(const base::DictionaryValue& notification,
                        NotificationDelegate* delegate) {
  std::string cloud_id;
  if (!notification.GetString("deviceId", &cloud_id)) {
    LOG(ERROR) << "DEVICE_DELETED notification is missing 'deviceId' property";
    return false;
  }

  delegate->OnDeviceDeleted(cloud_id);
  return true;
}

}  // anonymous namespace

bool ParseNotificationJson(const base::DictionaryValue& notification,
                           NotificationDelegate* delegate,
                           const std::string& channel_name) {
  CHECK(delegate);

  std::string kind;
  if (!notification.GetString("kind", &kind) || kind != "weave#notification") {
    LOG(WARNING) << "Push notification should have 'kind' property set to "
                    "weave#notification";
    return false;
  }

  std::string type;
  if (!notification.GetString("type", &type)) {
    LOG(WARNING) << "Push notification should have 'type' property";
    return false;
  }

  if (type == "COMMAND_CREATED")
    return ParseCommandCreated(notification, delegate, channel_name);

  if (type == "DEVICE_DELETED")
    return ParseDeviceDeleted(notification, delegate);

  // Here we ignore other types of notifications for now.
  LOG(INFO) << "Ignoring push notification of type " << type;
  return true;
}

}  // namespace weave
