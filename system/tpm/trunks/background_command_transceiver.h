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

#ifndef TRUNKS_BACKGROUND_COMMAND_TRANSCEIVER_H_
#define TRUNKS_BACKGROUND_COMMAND_TRANSCEIVER_H_

#include "trunks/command_transceiver.h"

#include <string>

#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <base/sequenced_task_runner.h>

#include "trunks/trunks_export.h"

namespace trunks {

// Sends commands to another CommandTransceiver on a background thread. Response
// callbacks are called on the original calling thread.
// Example:
//   base::Thread background_thread("my thread");
//   ...
//   BackgroundCommandTransceiver background_transceiver(
//       next_transceiver,
//       background_thread.message_loop_proxy());
//   ...
//   background_transceiver.SendCommand(my_command, MyCallback);
class TRUNKS_EXPORT BackgroundCommandTransceiver: public CommandTransceiver  {
 public:
  // All commands will be forwarded to |next_transceiver| on |task_runner|,
  // regardless of whether the synchronous or asynchronous method is used. This
  // class will hold a reference count to |task_runner|. If |task_runner| is
  // nullptr, all commands will be forwarded on the current thread. This class
  // does not take ownership of |next_transceiver|; it must remain valid for
  // the lifetime of the object.
  explicit BackgroundCommandTransceiver(
      CommandTransceiver* next_transceiver,
      const scoped_refptr<base::SequencedTaskRunner>& task_runner);
  ~BackgroundCommandTransceiver() override;

  // CommandTranceiver methods.
  void SendCommand(const std::string& command,
                   const ResponseCallback& callback) override;
  std::string SendCommandAndWait(const std::string& command) override;

 private:
  // Sends a |command| to the |next_transceiver_| and invokes a |callback| with
  // the command response.
  void SendCommandTask(const std::string& command,
                       const ResponseCallback& callback);

  base::WeakPtr<BackgroundCommandTransceiver> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  CommandTransceiver* next_transceiver_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // Declared last so weak pointers are invalidated first on destruction.
  base::WeakPtrFactory<BackgroundCommandTransceiver> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundCommandTransceiver);
};

}  // namespace trunks

#endif  // TRUNKS_BACKGROUND_COMMAND_TRANSCEIVER_H_
