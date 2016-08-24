// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/access_black_list_manager_impl.h"

#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <base/values.h>

#include "src/commands/schema_constants.h"
#include "src/data_encoding.h"

namespace weave {

namespace {
const char kConfigFileName[] = "black_list";

const char kUser[] = "user";
const char kApp[] = "app";
const char kExpiration[] = "expiration";
}

AccessBlackListManagerImpl::AccessBlackListManagerImpl(
    provider::ConfigStore* store,
    size_t capacity,
    base::Clock* clock)
    : capacity_{capacity}, clock_{clock}, store_{store} {
  Load();
}

void AccessBlackListManagerImpl::Load() {
  if (!store_)
    return;
  if (auto list = base::ListValue::From(
          base::JSONReader::Read(store_->LoadSettings(kConfigFileName)))) {
    for (const auto& e : *list) {
      const base::DictionaryValue* entry{nullptr};
      std::string user;
      std::string app;
      decltype(entries_)::key_type key;
      int expiration;
      if (e->GetAsDictionary(&entry) && entry->GetString(kUser, &user) &&
          Base64Decode(user, &key.first) && entry->GetString(kApp, &app) &&
          Base64Decode(app, &key.second) &&
          entry->GetInteger(kExpiration, &expiration)) {
        base::Time expiration_time = base::Time::FromTimeT(expiration);
        if (expiration_time > clock_->Now())
          entries_[key] = expiration_time;
      }
    }
    if (entries_.size() < list->GetSize()) {
      // Save some storage space by saving without expired entries.
      Save({});
    }
  }
}

void AccessBlackListManagerImpl::Save(const DoneCallback& callback) {
  if (!store_) {
    if (!callback.is_null())
      callback.Run(nullptr);
    return;
  }

  base::ListValue list;
  for (const auto& e : entries_) {
    scoped_ptr<base::DictionaryValue> entry{new base::DictionaryValue};
    entry->SetString(kUser, Base64Encode(e.first.first));
    entry->SetString(kApp, Base64Encode(e.first.second));
    entry->SetInteger(kExpiration, e.second.ToTimeT());
    list.Append(std::move(entry));
  }

  std::string json;
  base::JSONWriter::Write(list, &json);
  store_->SaveSettings(kConfigFileName, json, callback);
}

void AccessBlackListManagerImpl::RemoveExpired() {
  for (auto i = begin(entries_); i != end(entries_);) {
    if (i->second <= clock_->Now())
      i = entries_.erase(i);
    else
      ++i;
  }
}

void AccessBlackListManagerImpl::Block(const std::vector<uint8_t>& user_id,
                                       const std::vector<uint8_t>& app_id,
                                       const base::Time& expiration,
                                       const DoneCallback& callback) {
  // Iterating is OK as Save below is more expensive.
  RemoveExpired();
  if (expiration <= clock_->Now()) {
    if (!callback.is_null()) {
      ErrorPtr error;
      Error::AddTo(&error, FROM_HERE, "aleady_expired",
                   "Entry already expired");
      callback.Run(std::move(error));
    }
    return;
  }
  if (entries_.size() >= capacity_) {
    if (!callback.is_null()) {
      ErrorPtr error;
      Error::AddTo(&error, FROM_HERE, "blacklist_is_full",
                   "Unable to store more entries");
      callback.Run(std::move(error));
    }
    return;
  }
  auto& value = entries_[std::make_pair(user_id, app_id)];
  value = std::max(value, expiration);
  Save(callback);
}

void AccessBlackListManagerImpl::Unblock(const std::vector<uint8_t>& user_id,
                                         const std::vector<uint8_t>& app_id,
                                         const DoneCallback& callback) {
  if (!entries_.erase(std::make_pair(user_id, app_id))) {
    if (!callback.is_null()) {
      ErrorPtr error;
      Error::AddTo(&error, FROM_HERE, "entry_not_found", "Unknown entry");
      callback.Run(std::move(error));
    }
    return;
  }
  // Iterating is OK as Save below is more expensive.
  RemoveExpired();
  Save(callback);
}

bool AccessBlackListManagerImpl::IsBlocked(
    const std::vector<uint8_t>& user_id,
    const std::vector<uint8_t>& app_id) const {
  for (const auto& user : {{}, user_id}) {
    for (const auto& app : {{}, app_id}) {
      auto both = entries_.find(std::make_pair(user, app));
      if (both != end(entries_) && both->second > clock_->Now())
        return true;
    }
  }
  return false;
}

std::vector<AccessBlackListManager::Entry>
AccessBlackListManagerImpl::GetEntries() const {
  std::vector<Entry> result;
  for (const auto& e : entries_)
    result.push_back({e.first.first, e.first.second, e.second});
  return result;
}

size_t AccessBlackListManagerImpl::GetSize() const {
  return entries_.size();
}

size_t AccessBlackListManagerImpl::GetCapacity() const {
  return capacity_;
}

}  // namespace weave
