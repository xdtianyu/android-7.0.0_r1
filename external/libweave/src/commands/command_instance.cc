// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_instance.h"

#include <base/values.h>
#include <weave/enum_to_string.h>
#include <weave/error.h>
#include <weave/export.h>

#include "src/commands/command_queue.h"
#include "src/commands/schema_constants.h"
#include "src/json_error_codes.h"
#include "src/utils.h"

namespace weave {

namespace {

const EnumToStringMap<Command::State>::Map kMapStatus[] = {
    {Command::State::kQueued, "queued"},
    {Command::State::kInProgress, "inProgress"},
    {Command::State::kPaused, "paused"},
    {Command::State::kError, "error"},
    {Command::State::kDone, "done"},
    {Command::State::kCancelled, "cancelled"},
    {Command::State::kAborted, "aborted"},
    {Command::State::kExpired, "expired"},
};

const EnumToStringMap<Command::Origin>::Map kMapOrigin[] = {
    {Command::Origin::kLocal, "local"},
    {Command::Origin::kCloud, "cloud"},
};

bool ReportInvalidStateTransition(ErrorPtr* error,
                                  Command::State from,
                                  Command::State to) {
  return Error::AddToPrintf(error, FROM_HERE, errors::commands::kInvalidState,
                            "State switch impossible: '%s' -> '%s'",
                            EnumToString(from).c_str(),
                            EnumToString(to).c_str());
}

}  // namespace

template <>
LIBWEAVE_EXPORT EnumToStringMap<Command::State>::EnumToStringMap()
    : EnumToStringMap(kMapStatus) {}

template <>
LIBWEAVE_EXPORT EnumToStringMap<Command::Origin>::EnumToStringMap()
    : EnumToStringMap(kMapOrigin) {}

CommandInstance::CommandInstance(const std::string& name,
                                 Command::Origin origin,
                                 const base::DictionaryValue& parameters)
    : name_{name}, origin_{origin} {
  parameters_.MergeDictionary(&parameters);
}

CommandInstance::~CommandInstance() {
  FOR_EACH_OBSERVER(Observer, observers_, OnCommandDestroyed());
}

const std::string& CommandInstance::GetID() const {
  return id_;
}

const std::string& CommandInstance::GetName() const {
  return name_;
}

const std::string& CommandInstance::GetComponent() const {
  return component_;
}

Command::State CommandInstance::GetState() const {
  return state_;
}

Command::Origin CommandInstance::GetOrigin() const {
  return origin_;
}

const base::DictionaryValue& CommandInstance::GetParameters() const {
  return parameters_;
}

const base::DictionaryValue& CommandInstance::GetProgress() const {
  return progress_;
}

const base::DictionaryValue& CommandInstance::GetResults() const {
  return results_;
}

const Error* CommandInstance::GetError() const {
  return error_.get();
}

bool CommandInstance::SetProgress(const base::DictionaryValue& progress,
                                  ErrorPtr* error) {
  // Change status even if progress unchanged, e.g. 0% -> 0%.
  if (!SetStatus(State::kInProgress, error))
    return false;

  if (!progress_.Equals(&progress)) {
    progress_.Clear();
    progress_.MergeDictionary(&progress);
    FOR_EACH_OBSERVER(Observer, observers_, OnProgressChanged());
  }

  return true;
}

bool CommandInstance::Complete(const base::DictionaryValue& results,
                               ErrorPtr* error) {
  if (!results_.Equals(&results)) {
    results_.Clear();
    results_.MergeDictionary(&results);
    FOR_EACH_OBSERVER(Observer, observers_, OnResultsChanged());
  }
  // Change status even if result is unchanged.
  bool result = SetStatus(State::kDone, error);
  RemoveFromQueue();
  // The command will be destroyed after that, so do not access any members.
  return result;
}

bool CommandInstance::SetError(const Error* command_error, ErrorPtr* error) {
  error_ = command_error ? command_error->Clone() : nullptr;
  FOR_EACH_OBSERVER(Observer, observers_, OnErrorChanged());
  return SetStatus(State::kError, error);
}

namespace {

// Helper method to retrieve command parameters from the command definition
// object passed in as |json|.
// On success, returns |true| and the validated parameters and values through
// |parameters|. Otherwise returns |false| and additional error information in
// |error|.
std::unique_ptr<base::DictionaryValue> GetCommandParameters(
    const base::DictionaryValue* json,
    ErrorPtr* error) {
  // Get the command parameters from 'parameters' property.
  std::unique_ptr<base::DictionaryValue> params;
  const base::Value* params_value = nullptr;
  if (json->Get(commands::attributes::kCommand_Parameters, &params_value)) {
    // Make sure the "parameters" property is actually an object.
    const base::DictionaryValue* params_dict = nullptr;
    if (!params_value->GetAsDictionary(&params_dict)) {
      return Error::AddToPrintf(error, FROM_HERE, errors::json::kObjectExpected,
                                "Property '%s' must be a JSON object",
                                commands::attributes::kCommand_Parameters);
    }
    params.reset(params_dict->DeepCopy());
  } else {
    // "parameters" are not specified. Assume empty param list.
    params.reset(new base::DictionaryValue);
  }
  return params;
}

}  // anonymous namespace

std::unique_ptr<CommandInstance> CommandInstance::FromJson(
    const base::Value* value,
    Command::Origin origin,
    std::string* command_id,
    ErrorPtr* error) {
  std::unique_ptr<CommandInstance> instance;
  std::string command_id_buffer;  // used if |command_id| was nullptr.
  if (!command_id)
    command_id = &command_id_buffer;

  // Get the command JSON object from the value.
  const base::DictionaryValue* json = nullptr;
  if (!value->GetAsDictionary(&json)) {
    command_id->clear();
    return Error::AddTo(error, FROM_HERE, errors::json::kObjectExpected,
                        "Command instance is not a JSON object");
  }

  // Get the command ID from 'id' property.
  if (!json->GetString(commands::attributes::kCommand_Id, command_id))
    command_id->clear();

  // Get the command name from 'name' property.
  std::string command_name;
  if (!json->GetString(commands::attributes::kCommand_Name, &command_name)) {
    return Error::AddTo(error, FROM_HERE, errors::commands::kPropertyMissing,
                        "Command name is missing");
  }

  auto parameters = GetCommandParameters(json, error);
  if (!parameters) {
    return Error::AddToPrintf(
        error, FROM_HERE, errors::commands::kCommandFailed,
        "Failed to validate command '%s'", command_name.c_str());
  }

  instance.reset(new CommandInstance{command_name, origin, *parameters});

  if (!command_id->empty())
    instance->SetID(*command_id);

  // Get the component name this command is for.
  std::string component;
  if (json->GetString(commands::attributes::kCommand_Component, &component))
    instance->SetComponent(component);

  return instance;
}

std::unique_ptr<base::DictionaryValue> CommandInstance::ToJson() const {
  std::unique_ptr<base::DictionaryValue> json{new base::DictionaryValue};

  json->SetString(commands::attributes::kCommand_Id, id_);
  json->SetString(commands::attributes::kCommand_Name, name_);
  json->Set(commands::attributes::kCommand_Parameters, parameters_.DeepCopy());
  json->Set(commands::attributes::kCommand_Progress, progress_.DeepCopy());
  json->Set(commands::attributes::kCommand_Results, results_.DeepCopy());
  json->SetString(commands::attributes::kCommand_State, EnumToString(state_));
  if (error_) {
    json->Set(commands::attributes::kCommand_Error,
              ErrorInfoToJson(*error_).release());
  }

  return json;
}

void CommandInstance::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void CommandInstance::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool CommandInstance::Pause(ErrorPtr* error) {
  return SetStatus(State::kPaused, error);
}

bool CommandInstance::Abort(const Error* command_error, ErrorPtr* error) {
  error_ = command_error ? command_error->Clone() : nullptr;
  FOR_EACH_OBSERVER(Observer, observers_, OnErrorChanged());
  bool result = SetStatus(State::kAborted, error);
  RemoveFromQueue();
  // The command will be destroyed after that, so do not access any members.
  return result;
}

bool CommandInstance::Cancel(ErrorPtr* error) {
  bool result = SetStatus(State::kCancelled, error);
  RemoveFromQueue();
  // The command will be destroyed after that, so do not access any members.
  return result;
}

bool CommandInstance::SetStatus(Command::State status, ErrorPtr* error) {
  if (status == state_)
    return true;
  if (status == State::kQueued)
    return ReportInvalidStateTransition(error, state_, status);
  switch (state_) {
    case State::kDone:
    case State::kCancelled:
    case State::kAborted:
    case State::kExpired:
      return ReportInvalidStateTransition(error, state_, status);
    case State::kQueued:
    case State::kInProgress:
    case State::kPaused:
    case State::kError:
      break;
  }
  state_ = status;
  FOR_EACH_OBSERVER(Observer, observers_, OnStateChanged());
  return true;
}

void CommandInstance::RemoveFromQueue() {
  if (queue_)
    queue_->RemoveLater(GetID());
}

}  // namespace weave
