// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMMANDS_CLOUD_COMMAND_UPDATE_INTERFACE_H_
#define LIBWEAVE_SRC_COMMANDS_CLOUD_COMMAND_UPDATE_INTERFACE_H_

#include <string>

#include <base/callback_forward.h>
#include <base/values.h>

namespace weave {

// An abstract interface to allow for sending command update requests to the
// cloud server.
class CloudCommandUpdateInterface {
 public:
  virtual void UpdateCommand(const std::string& command_id,
                             const base::DictionaryValue& command_patch,
                             const DoneCallback& callback) = 0;

 protected:
  virtual ~CloudCommandUpdateInterface() {}
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMMANDS_CLOUD_COMMAND_UPDATE_INTERFACE_H_
