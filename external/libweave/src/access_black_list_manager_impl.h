// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_ACCESS_BLACK_LIST_IMPL_H_
#define LIBWEAVE_SRC_ACCESS_BLACK_LIST_IMPL_H_

#include <map>
#include <utility>

#include <base/time/default_clock.h>
#include <base/time/time.h>
#include <weave/error.h>
#include <weave/provider/config_store.h>

#include "src/access_black_list_manager.h"

namespace weave {

class AccessBlackListManagerImpl : public AccessBlackListManager {
 public:
  explicit AccessBlackListManagerImpl(provider::ConfigStore* store,
                                      size_t capacity = 1024,
                                      base::Clock* clock = nullptr);

  // AccessBlackListManager implementation.
  void Block(const std::vector<uint8_t>& user_id,
             const std::vector<uint8_t>& app_id,
             const base::Time& expiration,
             const DoneCallback& callback) override;
  void Unblock(const std::vector<uint8_t>& user_id,
               const std::vector<uint8_t>& app_id,
               const DoneCallback& callback) override;
  bool IsBlocked(const std::vector<uint8_t>& user_id,
                 const std::vector<uint8_t>& app_id) const override;
  std::vector<Entry> GetEntries() const override;
  size_t GetSize() const override;
  size_t GetCapacity() const override;

 private:
  void Load();
  void Save(const DoneCallback& callback);
  void RemoveExpired();

  const size_t capacity_{0};
  base::DefaultClock default_clock_;
  base::Clock* clock_{&default_clock_};

  provider::ConfigStore* store_{nullptr};
  std::map<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>, base::Time>
      entries_;

  DISALLOW_COPY_AND_ASSIGN(AccessBlackListManagerImpl);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_ACCESS_BLACK_LIST_IMPL_H_
