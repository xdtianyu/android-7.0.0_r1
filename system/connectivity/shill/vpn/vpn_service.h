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

#ifndef SHILL_VPN_VPN_SERVICE_H_
#define SHILL_VPN_VPN_SERVICE_H_

#include <memory>
#include <string>

#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/connection.h"
#include "shill/service.h"

namespace shill {

class KeyValueStore;
class VPNDriver;

class VPNService : public Service {
 public:
  VPNService(ControlInterface* control,
             EventDispatcher* dispatcher,
             Metrics* metrics,
             Manager* manager,
             VPNDriver* driver);  // Takes ownership of |driver|.
  ~VPNService() override;

  // Inherited from Service.
  void Connect(Error* error, const char* reason) override;
  void Disconnect(Error* error, const char* reason) override;
  std::string GetStorageIdentifier() const override;
  bool Load(StoreInterface* storage) override;
  bool Save(StoreInterface* storage) override;
  bool Unload() override;
  void EnableAndRetainAutoConnect() override;
  void SetConnection(const ConnectionRefPtr& connection) override;
  bool SetNameProperty(const std::string& name, Error* error) override;

  virtual void InitDriverPropertyStore();

  VPNDriver* driver() const { return driver_.get(); }

  static std::string CreateStorageIdentifier(const KeyValueStore& args,
                                             Error* error);
  void set_storage_id(const std::string& id) { storage_id_ = id; }

 protected:
  // Inherited from Service.
  bool IsAutoConnectable(const char** reason) const override;
  std::string GetTethering(Error* error) const override;

 private:
  friend class VPNServiceTest;
  FRIEND_TEST(VPNServiceTest, GetDeviceRpcId);
  FRIEND_TEST(VPNServiceTest, SetConnection);
  FRIEND_TEST(VPNServiceTest, GetPhysicalTechnologyPropertyFailsIfNoCarrier);
  FRIEND_TEST(VPNServiceTest, GetPhysicalTechnologyPropertyOverWifi);
  FRIEND_TEST(VPNServiceTest, GetTethering);

  static const char kAutoConnNeverConnected[];
  static const char kAutoConnVPNAlreadyActive[];

  std::string GetDeviceRpcId(Error* error) const override;

  // Returns the Type name of the lowest connection (presumably the "physical"
  // connection) that this service depends on.
  std::string GetPhysicalTechnologyProperty(Error* error);

  std::string storage_id_;
  std::unique_ptr<VPNDriver> driver_;
  std::unique_ptr<Connection::Binder> connection_binder_;

  // Provided only for compatibility.  crbug.com/211858
  std::string vpn_domain_;

  DISALLOW_COPY_AND_ASSIGN(VPNService);
};

}  // namespace shill

#endif  // SHILL_VPN_VPN_SERVICE_H_
