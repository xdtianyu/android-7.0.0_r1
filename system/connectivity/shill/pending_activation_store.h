//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_PENDING_ACTIVATION_STORE_H_
#define SHILL_PENDING_ACTIVATION_STORE_H_

#include <memory>
#include <string>

#include <base/files/file_path.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

namespace shill {

class StoreInterface;

// PendingActivationStore stores the network activation status for a
// particular SIM. Once an online payment for the activation of a 3GPP
// network is successful, the associated SIM is regarded as pending
// activation and stored in the persistent profile. Once shill knows that
// the activation associated with a particular SIM is successful, it is removed
// from the profile and the cellular service is marked as activated.
class PendingActivationStore {
 public:
  enum State {
    // This state indicates that information for a particular SIM was never
    // stored in this database.
    kStateUnknown,
    // This state indicates that an online payment has been made but the modem
    // has not yet been able to register with the network.
    kStatePending,
    // This state indicates that the modem has registered with the network but
    // the network has not yet confirmed that the service has been activated.
    // Currently, shill knows that activation has gone through, when a non-zero
    // MDN has been received OTA.
    kStateActivated,
    // This state is used in CDMA activation to indicate that OTA activation
    // failed and was scheduled for a retry.
    kStateFailureRetry,
    kStateMax,
  };

  enum IdentifierType {
    kIdentifierICCID,
    kIdentifierMEID,
  };

  // Constructor performs no initialization.
  PendingActivationStore();
  virtual ~PendingActivationStore();

  // Tries to open the underlying store interface from the given file path.
  // Returns false if it fails to open the file.
  //
  // If called more than once on the same instance, the file that was already
  // open will allways be flushed and closed, however it is not guaranteed that
  // the file will always be successfully reopened (technically it should, but
  // it is not guaranteed).
  virtual bool InitStorage(const base::FilePath& storage_path);

  // Returns the activation state for a SIM with the given identifier. A return
  // value of kStateUnknown indicates that the given identifier was not found.
  virtual State GetActivationState(IdentifierType type,
                                   const std::string& identifier) const;

  // Sets the activation state for the given identifier. If an entry for this
  // identifier was not found, a new entry will be created. Returns true on
  // success.
  virtual bool SetActivationState(IdentifierType type,
                                  const std::string& identifier,
                                  State state);

  // Removes the entry for the given identifier from the database. Returns true
  // if the operation was successful. If the identifier did not exist in the
  // database, still returns true.
  virtual bool RemoveEntry(IdentifierType type, const std::string& identifier);

 private:
  friend class PendingActivationStoreTest;
  friend class CellularCapabilityUniversalTest;
  FRIEND_TEST(PendingActivationStoreTest, FileInteractions);
  FRIEND_TEST(PendingActivationStoreTest, GetActivationState);
  FRIEND_TEST(PendingActivationStoreTest, RemoveEntry);
  FRIEND_TEST(PendingActivationStoreTest, SetActivationState);

  static const char kIccidGroupId[];
  static const char kMeidGroupId[];
  static const char kStorageFileName[];

  static std::string IdentifierTypeToGroupId(IdentifierType type);

  std::unique_ptr<StoreInterface> storage_;

  DISALLOW_COPY_AND_ASSIGN(PendingActivationStore);
};

}  // namespace shill

#endif  // SHILL_PENDING_ACTIVATION_STORE_H_
