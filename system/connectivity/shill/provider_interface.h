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

#ifndef SHILL_PROVIDER_INTERFACE_H_
#define SHILL_PROVIDER_INTERFACE_H_

#include <string>

#include "shill/refptr_types.h"

namespace shill {

class Error;
class KeyValueStore;

// This is an interface for objects that creates and manages service objects.
class ProviderInterface {
 public:
  virtual ~ProviderInterface() {}

  // Creates services from the entries within |profile|.
  virtual void CreateServicesFromProfile(const ProfileRefPtr& profile) = 0;

  // Finds a Service with similar properties to |args|.  The criteria
  // used are specific to the provider subclass.  Returns a reference
  // to a matching service if one exists.  Otherwise it returns a NULL
  // reference and populates |error|.
  virtual ServiceRefPtr FindSimilarService(
      const KeyValueStore& args, Error* error) const = 0;

  // Retrieves (see FindSimilarService) or creates a service with the
  // unique attributes in |args|.  The remaining attributes will be
  // populated (by Manager) via a later call to Service::Configure().
  // Returns a NULL reference and populates |error| on failure.
  virtual ServiceRefPtr GetService(const KeyValueStore& args, Error* error) = 0;

  // Creates a temporary service with the identifying properties populated
  // from |args|.  Callers outside of the Provider must never register
  // this service with the Manager or connect it since it was never added
  // to the provider's service list.
  virtual ServiceRefPtr CreateTemporaryService(
      const KeyValueStore& args, Error* error) = 0;

  // Create a temporary service for an entry |entry_name| within |profile|.
  // Callers outside of the Provider must never register this service with the
  // Manager or connect it since it was never added to the provider's service
  // list.
  virtual ServiceRefPtr CreateTemporaryServiceFromProfile(
      const ProfileRefPtr& profile,
      const std::string& entry_name,
      Error* error) = 0;

  // Starts the provider.
  virtual void Start() = 0;

  // Stops the provider (will de-register all services).
  virtual void Stop() = 0;
};

}  // namespace shill

#endif  // SHILL_PROVIDER_INTERFACE_H_
