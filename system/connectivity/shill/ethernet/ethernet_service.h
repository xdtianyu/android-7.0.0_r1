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

#ifndef SHILL_ETHERNET_ETHERNET_SERVICE_H_
#define SHILL_ETHERNET_ETHERNET_SERVICE_H_

#include <string>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/event_dispatcher.h"
#include "shill/service.h"

namespace shill {

class ControlInterface;
class Ethernet;
class EventDispatcher;
class Manager;
class Metrics;

class EthernetService : public Service {
 public:
  EthernetService(ControlInterface* control_interface,
                  EventDispatcher* dispatcher,
                  Metrics* metrics,
                  Manager* manager,
                  base::WeakPtr<Ethernet> ethernet);
  ~EthernetService() override;

  // Inherited from Service.
  void Connect(Error* error, const char* reason) override;
  void Disconnect(Error* error, const char* reason) override;

  // ethernet_<MAC>
  std::string GetStorageIdentifier() const override;
  bool IsAutoConnectByDefault() const override;
  bool SetAutoConnectFull(const bool& connect, Error* error) override;

  void Remove(Error* error) override;
  bool IsVisible() const override;
  bool IsAutoConnectable(const char** reason) const override;

  // Called by the Ethernet device when link state has caused the service
  // visibility to change.
  virtual void OnVisibilityChanged();

 protected:
  // This constructor performs none of the initialization that the normal
  // constructor does and sets the reported technology to |technology|.  It is
  // intended for use by subclasses which want to override specific aspects of
  // EthernetService behavior, while still retaining their own technology
  // identifier.
  EthernetService(ControlInterface* control_interface,
                  EventDispatcher* dispatcher,
                  Metrics* metrics,
                  Manager* manager,
                  Technology::Identifier technology,
                  base::WeakPtr<Ethernet> ethernet);

  Ethernet* ethernet() const { return ethernet_.get(); }
  std::string GetTethering(Error* error) const override;

 private:
  FRIEND_TEST(EthernetServiceTest, GetTethering);

  static const char kAutoConnNoCarrier[];
  static const char kServiceType[];

  std::string GetDeviceRpcId(Error* error) const override;

  base::WeakPtr<Ethernet> ethernet_;
  DISALLOW_COPY_AND_ASSIGN(EthernetService);
};

}  // namespace shill

#endif  // SHILL_ETHERNET_ETHERNET_SERVICE_H_
