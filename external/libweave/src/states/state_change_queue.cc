// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/states/state_change_queue.h"

#include <base/logging.h>

namespace weave {

StateChangeQueue::StateChangeQueue(size_t max_queue_size)
    : max_queue_size_(max_queue_size) {
  CHECK_GT(max_queue_size_, 0U) << "Max queue size must not be zero";
}

bool StateChangeQueue::NotifyPropertiesUpdated(
    base::Time timestamp,
    const base::DictionaryValue& changed_properties) {
  auto& stored_changes = state_changes_[timestamp];
  // Merge the old property set.
  if (stored_changes)
    stored_changes->MergeDictionary(&changed_properties);
  else
    stored_changes.reset(changed_properties.DeepCopy());

  while (state_changes_.size() > max_queue_size_) {
    // Queue is full.
    // Merge the two oldest records into one. The merge strategy is:
    //  - Move non-existent properties from element [old] to [new].
    //  - If both [old] and [new] specify the same property,
    //    keep the value of [new].
    //  - Keep the timestamp of [new].
    auto element_old = state_changes_.begin();
    auto element_new = std::next(element_old);
    // This will skip elements that exist in both [old] and [new].
    element_old->second->MergeDictionary(element_new->second.get());
    std::swap(element_old->second, element_new->second);
    state_changes_.erase(element_old);
  }
  return true;
}

std::vector<StateChange> StateChangeQueue::GetAndClearRecordedStateChanges() {
  std::vector<StateChange> changes;
  changes.reserve(state_changes_.size());
  for (auto& pair : state_changes_) {
    changes.push_back(StateChange{pair.first, std::move(pair.second)});
  }
  state_changes_.clear();
  return changes;
}

}  // namespace weave
