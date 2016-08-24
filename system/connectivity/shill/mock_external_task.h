//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_MOCK_EXTERNAL_TASK_H_
#define SHILL_MOCK_EXTERNAL_TASK_H_

#include <map>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "shill/external_task.h"

namespace shill {

class MockExternalTask : public ExternalTask {
 public:
  MockExternalTask(ControlInterface* control,
                   ProcessManager* process_manager,
                   const base::WeakPtr<RPCTaskDelegate>& task_delegate,
                   const base::Callback<void(pid_t, int)>& death_callback);
  ~MockExternalTask() override;

  MOCK_METHOD5(Start,
               bool(const base::FilePath& file,
                    const std::vector<std::string>& arguments,
                    const std::map<std::string, std::string>& environment,
                    bool terminate_with_parent,
                    Error* error));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD0(OnDelete, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockExternalTask);
};

}  // namespace shill

#endif  // SHILL_MOCK_EXTERNAL_TASK_H_
