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

#ifndef SHILL_EXTERNAL_TASK_H_
#define SHILL_EXTERNAL_TASK_H_

#include <sys/types.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/files/file_path.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/rpc_task.h"

namespace shill {

class ControlInterface;
class Error;
class EventDispatcher;
class ProcessManager;

class ExternalTask : public RPCTaskDelegate {
 public:
  ExternalTask(ControlInterface* control,
               ProcessManager* process_manager,
               const base::WeakPtr<RPCTaskDelegate>& task_delegate,
               const base::Callback<void(pid_t, int)>& death_callback);
  ~ExternalTask() override;  // But consider DestroyLater...

  // Schedule later deletion of the ExternalTask. Useful when in the
  // middle of an ExternalTask callback. Note that the caller _must_
  // release ownership of |this|. For example:
  //
  //   class Foo : public SupportsWeakPtr<Foo>, public RPCTaskDelegate {
  //    public:
  //      Foo() {
  //        task_.reset(new ExternalTask(...));
  //      }
  //
  //      void Notify(...) {
  //        task_.release()->DestroyLater(...);  // Passes ownership.
  //      }
  //
  //    private:
  //      std::unique_ptr<ExternalTask> task_;
  //   }
  void DestroyLater(EventDispatcher* dispatcher);

  // Forks off a process to run |program|, with the command-line
  // arguments |arguments|, and the environment variables specified in
  // |environment|.
  //
  // If |terminate_with_parent| is true, the child process will be
  // configured to terminate itself if this process dies. Otherwise,
  // the child process will retain its default behavior.
  //
  // On success, returns true, and leaves |error| unmodified.
  // On failure, returns false, and sets |error|.
  //
  // |environment| SHOULD NOT contain kRPCTaskServiceVariable or
  // kRPCTaskPathVariable, as that may prevent the child process
  // from communicating back to the ExternalTask.
  virtual bool Start(const base::FilePath& program,
                     const std::vector<std::string>& arguments,
                     const std::map<std::string, std::string>& environment,
                     bool terminate_with_parent,
                     Error* error);
  virtual void Stop();

 private:
  friend class ExternalTaskTest;
  FRIEND_TEST(ExternalTaskTest, Destructor);
  FRIEND_TEST(ExternalTaskTest, GetLogin);
  FRIEND_TEST(ExternalTaskTest, Notify);
  FRIEND_TEST(ExternalTaskTest, OnTaskDied);
  FRIEND_TEST(ExternalTaskTest, Start);
  FRIEND_TEST(ExternalTaskTest, Stop);
  FRIEND_TEST(ExternalTaskTest, StopNotStarted);

  // Implements RPCTaskDelegate.
  void GetLogin(std::string* user, std::string* password) override;
  void Notify(
      const std::string& event,
      const std::map<std::string, std::string>& details) override;

  // Called when the external process exits.
  void OnTaskDied(int exit_status);

  static void Destroy(ExternalTask* task);

  ControlInterface* control_;
  ProcessManager* process_manager_;

  std::unique_ptr<RPCTask> rpc_task_;
  base::WeakPtr<RPCTaskDelegate> task_delegate_;
  base::Callback<void(pid_t, int)> death_callback_;

  // The PID of the spawned process. May be 0 if no process has been
  // spawned yet or the process has died.
  pid_t pid_;

  DISALLOW_COPY_AND_ASSIGN(ExternalTask);
};

}  // namespace shill

#endif  // SHILL_EXTERNAL_TASK_H_
