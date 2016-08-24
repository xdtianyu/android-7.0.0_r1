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

#include "shill/hook_table.h"

#include <list>
#include <string>

#include <base/bind.h>
#include <base/callback.h>
#include <base/cancelable_callback.h>

#include "shill/error.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"

using base::Bind;
using base::Closure;
using base::Unretained;
using std::list;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kManager;
static string ObjectID(const HookTable* h) { return "(hook_table)"; }
}

HookTable::HookTable(EventDispatcher* event_dispatcher)
    : event_dispatcher_(event_dispatcher) {}

void HookTable::Add(const string& name, const Closure& start_callback) {
  SLOG(this, 2) << __func__ << ": " << name;
  Remove(name);
  hook_table_.emplace(name, HookAction(start_callback));
}

HookTable::~HookTable() {
  timeout_callback_.Cancel();
}

void HookTable::Remove(const std::string& name) {
  SLOG(this, 2) << __func__ << ": " << name;
  hook_table_.erase(name);
}

void HookTable::ActionComplete(const std::string& name) {
  SLOG(this, 2) << __func__ << ": " << name;
  HookTableMap::iterator it = hook_table_.find(name);
  if (it != hook_table_.end()) {
    HookAction* action = &it->second;
    if (action->started && !action->completed) {
      action->completed = true;
    }
  }
  if (AllActionsComplete() && !done_callback_.is_null()) {
    timeout_callback_.Cancel();
    done_callback_.Run(Error(Error::kSuccess));
    done_callback_.Reset();
  }
}

void HookTable::Run(int timeout_ms, const ResultCallback& done) {
  SLOG(this, 2) << __func__;
  if (hook_table_.empty()) {
    done.Run(Error(Error::kSuccess));
    return;
  }
  done_callback_ = done;
  timeout_callback_.Reset(Bind(&HookTable::ActionsTimedOut, Unretained(this)));
  event_dispatcher_->PostDelayedTask(timeout_callback_.callback(), timeout_ms);

  // Mark all actions as having started before we execute any actions.
  // Otherwise, if the first action completes inline, its call to
  // ActionComplete() will cause the |done| callback to be invoked before the
  // rest of the actions get started.
  //
  // An action that completes inline could call HookTable::Remove(), which
  // modifies |hook_table_|. It is thus not safe to iterate through
  // |hook_table_| to execute the actions. Instead, we keep a list of start
  // callback of each action and iterate through that to invoke the callback.
  list<Closure> action_start_callbacks;
  for (auto& hook_entry : hook_table_) {
    HookAction* action = &hook_entry.second;
    action_start_callbacks.push_back(action->start_callback);
    action->started = true;
    action->completed = false;
  }
  // Now start the actions.
  for (auto& callback : action_start_callbacks) {
    callback.Run();
  }
}

bool HookTable::AllActionsComplete() const {
  SLOG(this, 2) << __func__;
  for (const auto& hook_entry : hook_table_) {
    const HookAction& action = hook_entry.second;
    if (action.started && !action.completed) {
        return false;
    }
  }
  return true;
}

void HookTable::ActionsTimedOut() {
  done_callback_.Run(Error(Error::kOperationTimeout));
  done_callback_.Reset();
}

}  // namespace shill
