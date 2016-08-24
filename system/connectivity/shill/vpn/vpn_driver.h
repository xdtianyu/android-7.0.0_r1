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

#ifndef SHILL_VPN_VPN_DRIVER_H_
#define SHILL_VPN_VPN_DRIVER_H_

#include <string>
#include <vector>

#include <base/cancelable_callback.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/accessor_interface.h"
#include "shill/key_value_store.h"
#include "shill/refptr_types.h"

namespace shill {

class Error;
class EventDispatcher;
class Manager;
class PropertyStore;
class StoreInterface;

class VPNDriver {
 public:
  virtual ~VPNDriver();

  virtual bool ClaimInterface(const std::string& link_name,
                              int interface_index) = 0;
  virtual void Connect(const VPNServiceRefPtr& service, Error* error) = 0;
  virtual void Disconnect() = 0;
  virtual std::string GetProviderType() const = 0;

  // Invoked by VPNService when the underlying connection disconnects.
  virtual void OnConnectionDisconnected() = 0;

  virtual void InitPropertyStore(PropertyStore* store);

  virtual bool Load(StoreInterface* storage, const std::string& storage_id);
  virtual bool Save(StoreInterface* storage,
                    const std::string& storage_id,
                    bool save_credentials);
  virtual void UnloadCredentials();

  std::string GetHost() const;

  KeyValueStore* args() { return &args_; }
  const KeyValueStore* const_args() const { return &args_; }

 protected:
  struct Property {
    enum Flags {
      kEphemeral = 1 << 0,   // Never load or save.
      kCredential = 1 << 1,  // Save if saving credentials (crypted).
      kWriteOnly = 1 << 2,   // Never read over RPC.
      kArray = 1 << 3,       // Property is an array of strings.
    };

    const char* property;
    int flags;
  };

  static const int kDefaultConnectTimeoutSeconds;

  VPNDriver(EventDispatcher* dispatcher,
            Manager* manager,
            const Property* properties,
            size_t property_count);

  EventDispatcher* dispatcher() const { return dispatcher_; }
  Manager* manager() const { return manager_; }

  virtual KeyValueStore GetProvider(Error* error);

  // Initializes a callback that will invoke OnConnectTimeout after
  // |timeout_seconds|. The timeout will not be restarted if it's already
  // scheduled.
  void StartConnectTimeout(int timeout_seconds);
  // Cancels the connect timeout callback, if any, previously scheduled through
  // StartConnectTimeout.
  void StopConnectTimeout();
  // Returns true if a connect timeout is scheduled, false otherwise.
  bool IsConnectTimeoutStarted() const;

  // Called if a connect timeout scheduled through StartConnectTimeout
  // fires. Cancels the timeout callback.
  virtual void OnConnectTimeout();

  int connect_timeout_seconds() const { return connect_timeout_seconds_; }

 private:
  friend class VPNDriverTest;

  void ClearMappedStringProperty(const size_t& index, Error* error);
  void ClearMappedStringsProperty(const size_t& index, Error* error);
  std::string GetMappedStringProperty(const size_t& index, Error* error);
  std::vector<std::string> GetMappedStringsProperty(
      const size_t& index, Error* error);
  bool SetMappedStringProperty(
      const size_t& index, const std::string& value, Error* error);
  bool SetMappedStringsProperty(
      const size_t& index, const std::vector<std::string>& value, Error* error);

  base::WeakPtrFactory<VPNDriver> weak_ptr_factory_;
  EventDispatcher* dispatcher_;
  Manager* manager_;
  const Property* const properties_;
  const size_t property_count_;
  KeyValueStore args_;

  base::CancelableClosure connect_timeout_callback_;
  int connect_timeout_seconds_;

  DISALLOW_COPY_AND_ASSIGN(VPNDriver);
};

}  // namespace shill

#endif  // SHILL_VPN_VPN_DRIVER_H_
