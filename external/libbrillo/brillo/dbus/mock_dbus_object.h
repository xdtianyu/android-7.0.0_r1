// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DBUS_MOCK_DBUS_OBJECT_H_
#define LIBBRILLO_BRILLO_DBUS_MOCK_DBUS_OBJECT_H_

#include <string>

#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/dbus/dbus_object.h>
#include <gmock/gmock.h>

namespace brillo {
namespace dbus_utils {

class MockDBusObject : public DBusObject {
 public:
  MockDBusObject(ExportedObjectManager* object_manager,
                 const scoped_refptr<dbus::Bus>& bus,
                 const dbus::ObjectPath& object_path)
      : DBusObject(object_manager, bus, object_path) {}
  ~MockDBusObject() override = default;

  MOCK_METHOD1(RegisterAsync,
               void(const AsyncEventSequencer::CompletionAction&));
};  // class MockDBusObject

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_MOCK_DBUS_OBJECT_H_
