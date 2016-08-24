// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DBUS_MOCK_EXPORTED_OBJECT_MANAGER_H_
#define LIBBRILLO_BRILLO_DBUS_MOCK_EXPORTED_OBJECT_MANAGER_H_

#include <string>

#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/dbus/exported_object_manager.h>
#include <dbus/object_path.h>
#include <gmock/gmock.h>

namespace brillo {

namespace dbus_utils {

class MockExportedObjectManager : public ExportedObjectManager {
 public:
  using CompletionAction =
      brillo::dbus_utils::AsyncEventSequencer::CompletionAction;

  using ExportedObjectManager::ExportedObjectManager;
  ~MockExportedObjectManager() override = default;

  MOCK_METHOD1(RegisterAsync,
               void(const CompletionAction& completion_callback));
  MOCK_METHOD3(ClaimInterface,
               void(const dbus::ObjectPath& path,
                    const std::string& interface_name,
                    const ExportedPropertySet::PropertyWriter& writer));
  MOCK_METHOD2(ReleaseInterface,
               void(const dbus::ObjectPath& path,
                    const std::string& interface_name));
};

}  // namespace dbus_utils

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_MOCK_EXPORTED_OBJECT_MANAGER_H_
