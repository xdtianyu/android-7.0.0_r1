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

#ifndef SHILL_HOOK_TABLE_H_
#define SHILL_HOOK_TABLE_H_

// HookTable provides a facility for starting a set of generic actions and
// reporting for their completion.  For example, on shutdown, each service gets
// disconnected.  A disconnect action may be instantaneous or it may require
// some time to complete.  Users of this facility use the Add() function to
// provide a closure for starting an action. Users report the completion of an
// action.  When an event occurs, the Run() function is called, which starts
// each action and sets a timer.  Upon completion or timeout, Run() calls a
// user-supplied callback to notify the caller of the state of actions.
//
// Usage example.  Add an action to a hook table like this:
//
//   HookTable hook_table_(&event_dispatcher);
//   Closure start_callback = Bind(&MyService::Disconnect, &my_service);
//   hook_table_.Add("MyService", start_callback);
//
// The code that catches an event runs the actions of the hook table like this:
//
//   ResultCallback done_callback = Bind(Manager::OnDisconnect, &manager);
//   hook_table_.Run(kTimeout, done_callback);
//
// When |my_service| has completed its disconnect process,
// Manager::OnDisconnect() gets called with Error::kSuccess.  If |my_service|
// does not finish its disconnect processing before kTimeout, then it gets
// called with kOperationTimeout.

#include <map>
#include <string>

#include <base/cancelable_callback.h>
#include <base/macros.h>
#include <gtest/gtest_prod.h>

#include "shill/callbacks.h"

namespace shill {
class EventDispatcher;
class Error;

class HookTable {
 public:
  explicit HookTable(EventDispatcher* event_dispatcher);
  ~HookTable();

  // Adds a closure to the hook table.  |name| should be unique; otherwise, a
  // previous closure by the same name will be replaced.  |start| will be called
  // when Run() is called.
  void Add(const std::string& name, const base::Closure& start);

  // Users call this function to report the completion of an action |name|.
  void ActionComplete(const std::string& name);

  // Removes the action associated with |name| from the hook table.  If |name|
  // does not exist, the hook table is unchanged.
  void Remove(const std::string& name);

  // Runs the actions that have been added to the HookTable via Add().  It
  // starts a timer for completion in |timeout_ms|.  If all actions complete
  // successfully within the timeout period, |done| is called with a value of
  // Error::kSuccess.  Otherwise, it is called with Error::kOperationTimeout.
  void Run(int timeout_ms, const ResultCallback& done);

  bool IsEmpty() const { return hook_table_.empty(); }

 private:
  friend class HookTableTest;

  // For each action, there is a |start| callback which is stored in this
  // structure.
  struct HookAction {
    explicit HookAction(const base::Closure& start_callback)
        : start_callback(start_callback),
          started(false),
          completed(false) {}
    const base::Closure start_callback;
    bool started;
    bool completed;
  };

  // Each action is stored in this table.  The key is |name| passed to Add().
  typedef std::map<std::string, HookAction> HookTableMap;

  // Returns true if all started actions have completed; false otherwise.  If no
  // actions have started, returns true.
  bool AllActionsComplete() const;

  // This function runs if all the actions do not complete before the timeout
  // period.  It invokes the user-supplied callback to Run() with an error value
  // kOperationTimeout.
  void ActionsTimedOut();

  // Each action is stored in this table.
  HookTableMap hook_table_;

  // This is the user-supplied callback to Run().
  ResultCallback done_callback_;

  // This callback is created in Run() and is queued to the event dispatcher to
  // run after a timeout period.  If all the actions complete before the
  // timeout, then this callback is canceled.
  base::CancelableClosure timeout_callback_;

  // Used for setting a timeout action to run in case all the actions do not
  // complete in time.
  EventDispatcher* const event_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(HookTable);
};

}  // namespace shill

#endif  // SHILL_HOOK_TABLE_H_
