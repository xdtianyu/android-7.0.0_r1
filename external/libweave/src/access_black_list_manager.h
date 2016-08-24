// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_ACCESS_BLACK_LIST_H_
#define LIBWEAVE_SRC_ACCESS_BLACK_LIST_H_

#include <vector>

#include <base/time/time.h>

namespace weave {

class AccessBlackListManager {
 public:
  struct Entry {
    // user_id is empty, app_id is empty: block everything.
    // user_id is not empty, app_id is empty: block if user_id matches.
    // user_id is empty, app_id is not empty: block if app_id matches.
    // user_id is not empty, app_id is not empty: block if both match.
    std::vector<uint8_t> user_id;
    std::vector<uint8_t> app_id;

    // Time after which to discard the rule.
    base::Time expiration;
  };
  virtual ~AccessBlackListManager() = default;

  virtual void Block(const std::vector<uint8_t>& user_id,
                     const std::vector<uint8_t>& app_id,
                     const base::Time& expiration,
                     const DoneCallback& callback) = 0;
  virtual void Unblock(const std::vector<uint8_t>& user_id,
                       const std::vector<uint8_t>& app_id,
                       const DoneCallback& callback) = 0;
  virtual bool IsBlocked(const std::vector<uint8_t>& user_id,
                         const std::vector<uint8_t>& app_id) const = 0;
  virtual std::vector<Entry> GetEntries() const = 0;
  virtual size_t GetSize() const = 0;
  virtual size_t GetCapacity() const = 0;
};

inline bool operator==(const AccessBlackListManager::Entry& l,
                       const AccessBlackListManager::Entry& r) {
  return l.user_id == r.user_id && l.app_id == r.app_id &&
         l.expiration == r.expiration;
}

inline bool operator!=(const AccessBlackListManager::Entry& l,
                       const AccessBlackListManager::Entry& r) {
  return !(l == r);
}

}  // namespace weave

#endif  // LIBWEAVE_SRC_ACCESS_BLACK_LIST_H_
