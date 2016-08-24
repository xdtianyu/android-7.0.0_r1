//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef SHILL_MOCK_PROCESS_MANAGER_H_
#define SHILL_MOCK_PROCESS_MANAGER_H_

#include "shill/process_manager.h"

#include <map>
#include <string>
#include <vector>

#include <base/macros.h>
#include <gmock/gmock.h>

namespace shill {

class MockProcessManager : public ProcessManager {
 public:
  MockProcessManager();
  ~MockProcessManager() override;

  MOCK_METHOD1(Init, void(EventDispatcher* dispatcher));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD6(StartProcess,
               pid_t(const tracked_objects::Location& spawn_source,
                     const base::FilePath& program,
                     const std::vector<std::string>& arguments,
                     const std::map<std::string, std::string>& env,
                     bool terminate_with_parent,
                     const base::Callback<void(int)>& exit_callback));
  MOCK_METHOD7(StartProcessInMinijail,
               pid_t(const tracked_objects::Location& spawn_source,
                     const base::FilePath& program,
                     const std::vector<std::string>& arguments,
                     const std::string& user,
                     const std::string& group,
                     uint64_t capmask,
                     const base::Callback<void(int)>& exit_callback));
  MOCK_METHOD10(StartProcessInMinijailWithPipes,
                pid_t(const tracked_objects::Location& spawn_source,
                      const base::FilePath& program,
                      const std::vector<std::string>& arguments,
                      const std::string& user,
                      const std::string& group,
                      uint64_t capmask,
                      const base::Callback<void(int)>& exit_callback,
                      int* stdin_fd,
                      int* stdout_fd,
                      int* stderr_fd));
  MOCK_METHOD1(StopProcess, bool(pid_t pid));
  MOCK_METHOD1(StopProcessAndBlock, bool(pid_t pid));
  MOCK_METHOD2(UpdateExitCallback,
               bool(pid_t pid, const base::Callback<void(int)>& new_callback));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockProcessManager);
};

}  // namespace shill

#endif  // SHILL_MOCK_PROCESS_MANAGER_H_
