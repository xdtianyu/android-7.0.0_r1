// Copyright 2016 The Android Open Source Project
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

#ifndef LIBWEAVED_SERVICE_H_
#define LIBWEAVED_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/compiler_specific.h>
#include <base/macros.h>
#include <brillo/errors/error.h>
#include <libweaved/command.h>
#include <libweaved/export.h>

namespace brillo {
class MessageLoop;
}  // namespace brillo

namespace weaved {

// A weaved service is an abstract interface representing an instance of weave
// services for a particular client daemon. Apart from providing an API to
// weaved process, it manages resources specific for an instance of the client.
// For example, when a client exits, it removes any resources (e.g. components)
// that were added by this client from the weaved's component tree.
class LIBWEAVED_EXPORT Service {
 public:
  // Callback type for AddCommandHandler.
  using CommandHandlerCallback =
      base::Callback<void(std::unique_ptr<Command> command)>;

  // Callback type for AddPairingInfoListener.
  struct PairingInfo {
    std::string session_id;
    std::string pairing_mode;
    std::string pairing_code;
  };
  using PairingInfoCallback =
      base::Callback<void(const PairingInfo* pairing_info)>;

  Service() = default;
  virtual ~Service() = default;

  // Adds a new component instance to device.
  // |component| is a component name being added.
  // |traits| is a list of trait names this component supports.
  virtual bool AddComponent(const std::string& component,
                            const std::vector<std::string>& traits,
                            brillo::ErrorPtr* error) = 0;

  // Sets handler for new commands added to the queue for a given |component|.
  // |command_name| is the name of the command to handle (e.g. "reboot").
  // |trait_name| is the name of a trait the command belongs to (e.g. "base").
  // Each command can have no more than one handler.
  virtual void AddCommandHandler(const std::string& component,
                                 const std::string& trait_name,
                                 const std::string& command_name,
                                 const CommandHandlerCallback& callback) = 0;

  // Sets a number of state properties for a given |component|.
  // |dict| is a dictionary containing property-name/property-value pairs.
  virtual bool SetStateProperties(const std::string& component,
                                  const base::DictionaryValue& dict,
                                  brillo::ErrorPtr* error) = 0;

  // Sets value of the single property.
  // |component| is a path to the component to set the property value to.
  // |trait_name| is the name of the trait this property is part of.
  // |property_name| is the property name.
  virtual bool SetStateProperty(const std::string& component,
                                const std::string& trait_name,
                                const std::string& property_name,
                                const base::Value& value,
                                brillo::ErrorPtr* error) = 0;

  // Specifies a callback to be invoked when the device enters/exist pairing
  // mode. The |pairing_info| parameter is set to a pointer to pairing
  // information on starting the pairing session and is nullptr when the pairing
  // session ends.
  virtual void SetPairingInfoListener(const PairingInfoCallback& callback) = 0;

  // Service creation functionality.
  // Subscription is a base class for an object responsible for life-time
  // management for the service. The service instance is kept alive for as long
  // as the service connection is alive. See comments for Service::Connect for
  // more details.
  class Subscription {
   public:
    virtual ~Subscription() = default;

   protected:
    Subscription() = default;

   private:
    DISALLOW_COPY_AND_ASSIGN(Subscription);
  };

  using ConnectionCallback =
      base::Callback<void(const std::weak_ptr<Service>& service)>;

  // Creates an instance of weaved service asynchronously. This not only creates
  // the service class instance but also establishes an RPC connection to
  // weaved daemon. Upon connection having been established, a |callback| is
  // invoked and an instance of Service is passed to it as an argument.
  // The service instance provided to the callback is a weak pointer to the
  // actual service which may be destroyed at any time if RPC connection to
  // weaved is lost. If this happens, a connection is re-established and the
  // |callback| is called again with a new instance of the service.
  // Therefore, if locking the |service| produces nullptr, this means that the
  // service got disconnected, so no further action can be taken. Since the
  // |callback| will be invoked with the new service instance when connection
  // is re-established, it's a good idea to update the device state on each
  // invocation of the callback (along with registering command handlers, etc).
  // IMPORTANT: Keep the returned subscription object around for as long as the
  // service is needed. As soon as the subscription is destroyed, the connection
  // to weaved is terminated and the service instance is discarded.
  static std::unique_ptr<Subscription> Connect(
      brillo::MessageLoop* message_loop,
      const ConnectionCallback& callback) WARN_UNUSED_RESULT;

 private:
  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace weaved

#endif  // LIBWEAVED_SERVICE_H_
