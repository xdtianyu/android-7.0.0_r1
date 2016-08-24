//
// Copyright (C) 2011 The Android Open Source Project
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

#ifndef SHILL_STORE_INTERFACE_H_
#define SHILL_STORE_INTERFACE_H_

#include <set>
#include <string>
#include <vector>

namespace shill {

class KeyValueStore;

// An interface to a persistent store implementation.
class StoreInterface {
 public:
  virtual ~StoreInterface() {}

  // Returns true if the store exists and is non-empty.
  virtual bool IsNonEmpty() const = 0;

  // Opens the store. Returns true on success. The effects of
  // re-opening an open store are undefined. The effects of calling a
  // getter or setter on an unopened store are also undefined.
  virtual bool Open() = 0;

  // Closes the store and flushes it to persistent storage. Returns
  // true on success. Note that the store is considered closed even if
  // Close returns false. The effects of closing and already closed
  // store are undefined.
  virtual bool Close() = 0;

  // Flush current in-memory data to disk.
  virtual bool Flush() = 0;

  // Mark the underlying file store as corrupted, moving the data file
  // to a new filename.  This will prevent the file from being re-opened
  // the next time Open() is called.
  virtual bool MarkAsCorrupted() = 0;

  // Returns a set of all groups contained in the store.
  virtual std::set<std::string> GetGroups() const = 0;

  // Returns the names of all groups that contain the named |key|.
  virtual std::set<std::string> GetGroupsWithKey(
      const std::string& key) const = 0;

  // Returns the names of all groups that contain the named |properties|.
  // Only the Bool, Int and String properties are checked.
  virtual std::set<std::string> GetGroupsWithProperties(
      const KeyValueStore& properties) const = 0;

  // Returns true if the store contains |group|, false otherwise.
  virtual bool ContainsGroup(const std::string& group) const = 0;

  // Deletes |group|:|key|. Returns true on success. It is an error to
  // delete from a group that does not exist. It is, however,
  // permitted to delete a non-existent key from a group that does
  // exist.
  virtual bool DeleteKey(const std::string& group, const std::string& key) = 0;

  // Deletes |group|. Returns true on success. It is not an error to
  // delete a group that does not exist.
  virtual bool DeleteGroup(const std::string& group) = 0;

  // Sets a descriptive header on the key file.
  virtual bool SetHeader(const std::string& header) = 0;

  // Gets a string |value| associated with |group|:|key|. Returns true on
  // success and false on failure (including when |group|:|key| is not present
  // in the store).  It is not an error to pass NULL as |value| to simply
  // test for the presence of this value.
  virtual bool GetString(const std::string& group,
                         const std::string& key,
                         std::string* value) const = 0;

  // Associates |group|:|key| with a string |value|. Returns true on success,
  // false otherwise.
  virtual bool SetString(const std::string& group,
                         const std::string& key,
                         const std::string& value) = 0;

  // Gets a boolean |value| associated with |group|:|key|. Returns true on
  // success and false on failure (including when the |group|:|key| is not
  // present in the store).  It is not an error to pass NULL as |value| to
  // simply test for the presence of this value.

  virtual bool GetBool(const std::string& group,
                       const std::string& key,
                       bool* value) const = 0;

  // Associates |group|:|key| with a boolean |value|. Returns true on success,
  // false otherwise.
  virtual bool SetBool(const std::string& group,
                       const std::string& key,
                       bool value) = 0;

  // Gets a integer |value| associated with |group|:|key|. Returns true on
  // success and false on failure (including when the |group|:|key| is not
  // present in the store).  It is not an error to pass NULL as |value| to
  // simply test for the presence of this value.
  virtual bool GetInt(const std::string& group,
                      const std::string& key,
                      int* value) const = 0;

  // Associates |group|:|key| with an integer |value|. Returns true on success,
  // false otherwise.
  virtual bool SetInt(const std::string& group,
                      const std::string& key,
                      int value) = 0;

  // Gets a 64-bit unsigned integer |value| associated with |group|:|key|.
  // Returns true on success and false on failure (including when the
  // |group|:|key| is not present in the store).  It is not an error to
  // pass NULL as |value| to simply test for the presence of this value.
  virtual bool GetUint64(const std::string& group,
                         const std::string& key,
                         uint64_t* value) const = 0;

  // Associates |group|:|key| with a 64-bit unsigned integer |value|. Returns
  // true on success, false otherwise.
  virtual bool SetUint64(const std::string& group,
                         const std::string& key,
                         uint64_t value) = 0;

  // Gets a string list |value| associated with |group|:|key|. Returns true on
  // success and false on failure (including when |group|:|key| is not present
  // in the store).  It is not an error to pass NULL as |value| to simply test
  // for the presence of this value.
  virtual bool GetStringList(const std::string& group,
                             const std::string& key,
                             std::vector<std::string>* value) const = 0;

  // Associates |group|:|key| with a string list |value|. Returns true on
  // success, false otherwise.
  virtual bool SetStringList(const std::string& group,
                             const std::string& key,
                             const std::vector<std::string>& value) = 0;

  // Gets and decrypts string |value| associated with |group|:|key|. Returns
  // true on success and false on failure (including when |group|:|key| is not
  // present in the store).  It is not an error to pass NULL as |value| to
  // simply test for the presence of this value.
  virtual bool GetCryptedString(const std::string& group,
                                const std::string& key,
                                std::string* value) = 0;

  // Associates |group|:|key| with a string |value| after encrypting it. Returns
  // true on success, false otherwise.
  virtual bool SetCryptedString(const std::string& group,
                                const std::string& key,
                                const std::string& value) = 0;
};

}  // namespace shill

#endif  // SHILL_STORE_INTERFACE_H_
