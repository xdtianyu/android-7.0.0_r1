// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_STATES_STATE_CHANGE_QUEUE_H_
#define LIBWEAVE_SRC_STATES_STATE_CHANGE_QUEUE_H_

#include <map>
#include <memory>
#include <vector>

#include <base/macros.h>
#include <base/time/time.h>
#include <base/values.h>

namespace weave {

// A simple notification record event to track device state changes.
// The |timestamp| records the time of the state change.
// |changed_properties| contains a property set with the new property values
// which were updated at the time the event was recorded.
struct StateChange {
  StateChange(base::Time time,
              std::unique_ptr<base::DictionaryValue> properties)
      : timestamp{time}, changed_properties{std::move(properties)} {}
  base::Time timestamp;
  std::unique_ptr<base::DictionaryValue> changed_properties;
};

// An object to record and retrieve device state change notification events.
class StateChangeQueue {
 public:
  explicit StateChangeQueue(size_t max_queue_size);

  bool NotifyPropertiesUpdated(base::Time timestamp,
                               const base::DictionaryValue& changed_properties);
  std::vector<StateChange> GetAndClearRecordedStateChanges();

 private:
  // Maximum queue size. If it is full, the oldest state update records are
  // merged together until the queue size is within the size limit.
  const size_t max_queue_size_;

  // Accumulated list of device state change notifications.
  std::map<base::Time, std::unique_ptr<base::DictionaryValue>> state_changes_;

  DISALLOW_COPY_AND_ASSIGN(StateChangeQueue);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_STATES_STATE_CHANGE_QUEUE_H_
