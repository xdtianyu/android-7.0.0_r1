// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/async_event_sequencer.h>

namespace brillo {

namespace dbus_utils {

AsyncEventSequencer::AsyncEventSequencer() {
}
AsyncEventSequencer::~AsyncEventSequencer() {
}

AsyncEventSequencer::Handler AsyncEventSequencer::GetHandler(
    const std::string& descriptive_message,
    bool failure_is_fatal) {
  CHECK(!started_) << "Cannot create handlers after OnAllTasksCompletedCall()";
  int unique_registration_id = ++registration_counter_;
  outstanding_registrations_.insert(unique_registration_id);
  return base::Bind(&AsyncEventSequencer::HandleFinish,
                    this,
                    unique_registration_id,
                    descriptive_message,
                    failure_is_fatal);
}

AsyncEventSequencer::ExportHandler AsyncEventSequencer::GetExportHandler(
    const std::string& interface_name,
    const std::string& method_name,
    const std::string& descriptive_message,
    bool failure_is_fatal) {
  auto finish_handler = GetHandler(descriptive_message, failure_is_fatal);
  return base::Bind(&AsyncEventSequencer::HandleDBusMethodExported,
                    this,
                    finish_handler,
                    interface_name,
                    method_name);
}

void AsyncEventSequencer::OnAllTasksCompletedCall(
    std::vector<CompletionAction> actions) {
  CHECK(!started_) << "OnAllTasksCompletedCall called twice!";
  started_ = true;
  completion_actions_.assign(actions.begin(), actions.end());
  // All of our callbacks might have been called already.
  PossiblyRunCompletionActions();
}

namespace {
void IgnoreSuccess(const AsyncEventSequencer::CompletionTask& task,
                   bool /*success*/) {
  task.Run();
}
void DoNothing(bool /* success */) {
}
}  // namespace

AsyncEventSequencer::CompletionAction AsyncEventSequencer::WrapCompletionTask(
    const CompletionTask& task) {
  return base::Bind(&IgnoreSuccess, task);
}

AsyncEventSequencer::CompletionAction
AsyncEventSequencer::GetDefaultCompletionAction() {
  return base::Bind(&DoNothing);
}

void AsyncEventSequencer::HandleFinish(int registration_number,
                                       const std::string& error_message,
                                       bool failure_is_fatal,
                                       bool success) {
  RetireRegistration(registration_number);
  CheckForFailure(failure_is_fatal, success, error_message);
  PossiblyRunCompletionActions();
}

void AsyncEventSequencer::HandleDBusMethodExported(
    const AsyncEventSequencer::Handler& finish_handler,
    const std::string& expected_interface_name,
    const std::string& expected_method_name,
    const std::string& actual_interface_name,
    const std::string& actual_method_name,
    bool success) {
  CHECK_EQ(expected_method_name, actual_method_name)
      << "Exported DBus method '" << actual_method_name << "' "
      << "but expected '" << expected_method_name << "'";
  CHECK_EQ(expected_interface_name, actual_interface_name)
      << "Exported method DBus interface '" << actual_interface_name << "' "
      << "but expected '" << expected_interface_name << "'";
  finish_handler.Run(success);
}

void AsyncEventSequencer::RetireRegistration(int registration_number) {
  const size_t handlers_retired =
      outstanding_registrations_.erase(registration_number);
  CHECK_EQ(1U, handlers_retired) << "Tried to retire invalid handler "
                                 << registration_number << ")";
}

void AsyncEventSequencer::CheckForFailure(bool failure_is_fatal,
                                          bool success,
                                          const std::string& error_message) {
  if (failure_is_fatal) {
    CHECK(success) << error_message;
  }
  if (!success) {
    LOG(ERROR) << error_message;
    had_failures_ = true;
  }
}

void AsyncEventSequencer::PossiblyRunCompletionActions() {
  if (!started_ || !outstanding_registrations_.empty()) {
    // Don't run completion actions if we have any outstanding
    // Handlers outstanding or if any more handlers might
    // be scheduled in the future.
    return;
  }
  for (const auto& completion_action : completion_actions_) {
    // Should this be put on the message loop or run directly?
    completion_action.Run(!had_failures_);
  }
  // Discard our references to those actions.
  completion_actions_.clear();
}

}  // namespace dbus_utils

}  // namespace brillo
