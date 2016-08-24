//
// Copyright (C) 2012 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef SHILL_RPC_TASK_H_
#define SHILL_RPC_TASK_H_

#include <map>
#include <memory>
#include <string>

#include <base/macros.h>

namespace shill {

// Declared in the header to avoid linking unused code into shims.
static const char kRPCTaskServiceVariable[] = "SHILL_TASK_SERVICE";
static const char kRPCTaskPathVariable[] = "SHILL_TASK_PATH";

class ControlInterface;
class RPCTaskAdaptorInterface;

// TODO(petkov): Switch from delegate interface to registered callbacks
// (crbug.com/212273).
class RPCTaskDelegate {
 public:
  virtual ~RPCTaskDelegate() {}

  virtual void GetLogin(std::string* user, std::string* password) = 0;
  virtual void Notify(const std::string& reason,
                      const std::map<std::string, std::string>& dict) = 0;
};

// RPC tasks are currently used by VPN drivers for communication with external
// VPN processes. The RPC task should be owned by a single owner -- its
// RPCTaskDelegate -- so no need to be reference counted.
class RPCTask {
 public:
  // A constructor for the RPCTask object.
  RPCTask(ControlInterface* control_interface, RPCTaskDelegate* delegate);
  virtual ~RPCTask();

  virtual void GetLogin(std::string* user, std::string* password) const;
  virtual void Notify(const std::string& reason,
                      const std::map<std::string, std::string>& dict);

  // Returns a string that is guaranteed to uniquely identify this RPCTask
  // instance.
  const std::string& UniqueName() const { return unique_name_; }

  // Generates environment variable strings for a child process to
  // communicate back to us over RPC.
  virtual std::map<std::string, std::string> GetEnvironment() const;
  std::string GetRpcIdentifier() const;
  std::string GetRpcConnectionIdentifier() const;

 private:
  RPCTaskDelegate* delegate_;
  static unsigned int serial_number_;
  std::string unique_name_;  // MUST be unique amongst RPC task instances
  std::unique_ptr<RPCTaskAdaptorInterface> adaptor_;

  DISALLOW_COPY_AND_ASSIGN(RPCTask);
};

}  // namespace shill

#endif  // SHILL_RPC_TASK_H_
