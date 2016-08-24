// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_SETTINGS_H_
#define LIBWEAVE_INCLUDE_WEAVE_SETTINGS_H_

#include <set>
#include <string>

#include <base/time/time.h>

namespace weave {

// Scopes in order of increasing privileges.
enum class AuthScope {
  kNone,
  kViewer,
  kUser,
  kManager,
  kOwner,
};

// Type client-device pairing.
enum class PairingType {
  kPinCode,
  kEmbeddedCode,
};

struct Settings {
  // Model specific information. Must be set by ConfigStore::LoadDefaults.
  std::string firmware_version;
  std::string oem_name;
  std::string model_name;
  std::string model_id;

  // Basic device information. Must be set from ConfigStore::LoadDefaults.
  std::string name;
  std::string description;
  std::string location;

  // OAuth 2.0 related options. Must be set from ConfigStore::LoadDefaults.
  std::string api_key;
  std::string client_id;
  std::string client_secret;

  // Options mirrored into "base" state.
  // Maximum role for local anonymous user.
  AuthScope local_anonymous_access_role{AuthScope::kViewer};
  // If true, allows local discovery using DNS-SD.
  bool local_discovery_enabled{true};
  // If true, allows local pairing using Privet API.
  bool local_pairing_enabled{true};

  // Set of pairing modes supported by device.
  std::set<PairingType> pairing_modes;

  // Embedded code. Will be used only if pairing_modes contains kEmbeddedCode.
  std::string embedded_code;

  // Optional cloud information. Can be used for testing or debugging.
  std::string oauth_url;
  std::string service_url;
  std::string xmpp_endpoint;

  // Cloud ID of the registered device. Empty if device is not registered.
  std::string cloud_id;

  // Local device id.
  std::string device_id;

  // Internal options to tweak some library functionality. External code should
  // avoid using them.
  bool wifi_auto_setup_enabled{true};
  std::string test_privet_ssid;
};

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_SETTINGS_H_
