// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_COMMANDS_COMMAND_INSTANCE_H_
#define LIBWEAVE_SRC_COMMANDS_COMMAND_INSTANCE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>
#include <base/observer_list.h>
#include <weave/command.h>
#include <weave/error.h>

namespace base {
class Value;
}  // namespace base

namespace weave {

class CommandDictionary;
class CommandObserver;
class CommandQueue;

class CommandInstance final : public Command {
 public:
  class Observer {
   public:
    virtual void OnCommandDestroyed() = 0;
    virtual void OnErrorChanged() = 0;
    virtual void OnProgressChanged() = 0;
    virtual void OnResultsChanged() = 0;
    virtual void OnStateChanged() = 0;

   protected:
    virtual ~Observer() {}
  };

  // Construct a command instance given the full command |name| which must
  // be in format "<package_name>.<command_name>" and a list of parameters and
  // their values specified in |parameters|.
  CommandInstance(const std::string& name,
                  Command::Origin origin,
                  const base::DictionaryValue& parameters);
  ~CommandInstance() override;

  // Command overrides.
  const std::string& GetID() const override;
  const std::string& GetName() const override;
  const std::string& GetComponent() const override;
  Command::State GetState() const override;
  Command::Origin GetOrigin() const override;
  const base::DictionaryValue& GetParameters() const override;
  const base::DictionaryValue& GetProgress() const override;
  const base::DictionaryValue& GetResults() const override;
  const Error* GetError() const override;
  bool SetProgress(const base::DictionaryValue& progress,
                   ErrorPtr* error) override;
  bool Complete(const base::DictionaryValue& results, ErrorPtr* error) override;
  bool Pause(ErrorPtr* error) override;
  bool SetError(const Error* command_error, ErrorPtr* error) override;
  bool Abort(const Error* command_error, ErrorPtr* error) override;
  bool Cancel(ErrorPtr* error) override;

  // Parses a command instance JSON definition and constructs a CommandInstance
  // object.
  // On error, returns null unique_ptr and fills in error details in |error|.
  // |command_id| is the ID of the command returned, as parsed from the |value|.
  // The command ID extracted (if present in the JSON object) even if other
  // parsing/validation error occurs and command instance is not constructed.
  // This is used to report parse failures back to the server.
  static std::unique_ptr<CommandInstance> FromJson(const base::Value* value,
                                                   Command::Origin origin,
                                                   std::string* command_id,
                                                   ErrorPtr* error);

  std::unique_ptr<base::DictionaryValue> ToJson() const;

  // Sets the command ID (normally done by CommandQueue when the command
  // instance is added to it).
  void SetID(const std::string& id) { id_ = id; }
  void SetComponent(const std::string& component) { component_ = component; }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Sets the pointer to queue this command is part of.
  void AttachToQueue(CommandQueue* queue) { queue_ = queue; }
  void DetachFromQueue() { queue_ = nullptr; }

 private:
  // Helper function to update the command status.
  // Used by Abort(), Cancel(), Done() methods.
  bool SetStatus(Command::State status, ErrorPtr* error);
  // Helper method that removes this command from the command queue.
  // Note that since the command queue owns the lifetime of the command instance
  // object, removing a command from the queue will also destroy it.
  void RemoveFromQueue();

  // Unique command ID within a command queue.
  std::string id_;
  // Full command name as "<trait_name>.<command_name>".
  std::string name_;
  // Full path to the component this command is intended for.
  std::string component_;
  // The origin of the command, either "local" or "cloud".
  Command::Origin origin_ = Command::Origin::kLocal;
  // Command parameters and their values.
  base::DictionaryValue parameters_;
  // Current command execution progress.
  base::DictionaryValue progress_;
  // Command results.
  base::DictionaryValue results_;
  // Current command state.
  Command::State state_ = Command::State::kQueued;
  // Error encountered during execution of the command.
  ErrorPtr error_;
  // Command observers.
  base::ObserverList<Observer> observers_;
  // Pointer to the command queue this command instance is added to.
  // The queue owns the command instance, so it outlives this object.
  CommandQueue* queue_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(CommandInstance);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_COMMANDS_COMMAND_INSTANCE_H_
