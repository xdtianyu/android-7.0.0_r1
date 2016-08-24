// Copyright 2015 The Android Open Source Project
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

#ifndef LIBWEAVED_COMMAND_H_
#define LIBWEAVED_COMMAND_H_

#include <string>

#include <base/macros.h>
#include <binder/Status.h>
#include <brillo/errors/error.h>
#include <brillo/value_conversion.h>
#include <libweaved/export.h>
#include <utils/StrongPointer.h>

namespace android {
namespace weave {
class IWeaveCommand;
}  // namespace weave
}  // namespace android

namespace weaved {

class ServiceImpl;

class LIBWEAVED_EXPORT Command final {
 public:
  enum class State {
    kQueued,
    kInProgress,
    kPaused,
    kError,
    kDone,
    kCancelled,
    kAborted,
    kExpired,
  };

  enum class Origin { kLocal, kCloud };

  ~Command();

  // Returns the full command ID.
  std::string GetID() const;

  // Returns the full name of the command.
  std::string GetName() const;

  // Returns the name of the component this command was sent to.
  std::string GetComponent() const;

  // Returns the command state.
  Command::State GetState() const;

  // Returns the origin of the command.
  Command::Origin GetOrigin() const;

  // Returns the command parameters.
  const base::DictionaryValue& GetParameters() const;

  // Helper function to get a command parameter of particular type T from the
  // command parameter list. Returns default value for type T (e.g. 0 for int or
  // or "" for std::string) if the parameter with the given name is not found or
  // is of incorrect type.
  template <typename T>
  T GetParameter(const std::string& name) const {
    const base::DictionaryValue& parameters = GetParameters();
    T param_value{};
    const base::Value* value = nullptr;
    if (parameters.Get(name, &value))
      brillo::FromValue(*value, &param_value);
    return param_value;
  }

  // Updates the command progress. The |progress| should match the schema.
  // Returns false if |progress| value is incorrect.
  bool SetProgress(const base::DictionaryValue& progress,
                   brillo::ErrorPtr* error);

  // Sets command into terminal "done" state.
  // Updates the command results. The |results| should match the schema.
  // Returns false if |results| value is incorrect.
  bool Complete(const base::DictionaryValue& results,
                brillo::ErrorPtr* error);

  // Aborts command execution.
  // Sets command into terminal "aborted" state.
  bool Abort(const std::string& error_code,
             const std::string& error_message,
             brillo::ErrorPtr* error);

  // Aborts command execution.
  // Sets command into terminal "aborted" state and uses the error information
  // from the |command_error| object. The error codes extracted from
  // |command_error| are automatically prepended with an underscore ("_").
  bool AbortWithCustomError(const brillo::Error* command_error,
                            brillo::ErrorPtr* error);
  // AbortWithCustomError overload for specifying the error information as
  // binder::Status.
  bool AbortWithCustomError(android::binder::Status status,
                            brillo::ErrorPtr* error);

  // Cancels command execution.
  // Sets command into terminal "canceled" state.
  bool Cancel(brillo::ErrorPtr* error);

  // Sets command into paused state.
  // This is not terminal state. Command can be resumed with |SetProgress| call.
  bool Pause(brillo::ErrorPtr* error);

  // Sets command into error state and assign error.
  // This is not terminal state. Command can be resumed with |SetProgress| call.
  bool SetError(const std::string& error_code,
                const std::string& error_message,
                brillo::ErrorPtr* error);

  // Sets command into error state and assign error.
  // This is not terminal state. Command can be resumed with |SetProgress| call.
  // Uses the error information from the |command_error| object.
  // The error codes extracted from |command_error| are automatically prepended
  // with an underscore ("_").
  bool SetCustomError(const brillo::Error* command_error,
                      brillo::ErrorPtr* error);
  // SetError overload for specifying the error information as binder::Status.
  bool SetCustomError(android::binder::Status status,
                      brillo::ErrorPtr* error);

 protected:
  explicit Command(const android::sp<android::weave::IWeaveCommand>& proxy);

 private:
  friend class ServiceImpl;
  android::sp<android::weave::IWeaveCommand> binder_proxy_;
  mutable std::unique_ptr<base::DictionaryValue> parameter_cache_;

  DISALLOW_COPY_AND_ASSIGN(Command);
};

}  // namespace weave

#endif  // LIBWEAVED_COMMAND_H_
