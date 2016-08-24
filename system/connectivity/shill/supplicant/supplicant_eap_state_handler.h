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

#ifndef SHILL_SUPPLICANT_SUPPLICANT_EAP_STATE_HANDLER_H_
#define SHILL_SUPPLICANT_SUPPLICANT_EAP_STATE_HANDLER_H_

#include <string>

#include "shill/service.h"

namespace shill {

// This object tracks the state of wpa_supplicant's EAP association.
// It parses events from wpa_supplicant and can notify callers when
// wpa_supplicant succeeds or fails authentication.  In the latter
// case it can explain the failure in detail based on the course of
// events leading up to it.
class SupplicantEAPStateHandler {
 public:
  SupplicantEAPStateHandler();
  virtual ~SupplicantEAPStateHandler();

  // Receive the |status| and |parameter| from an EAP event and returns
  // true if this state transition indicates that the EAP authentication
  // process has succeeded.  If instead the EAP authentication has failed,
  // |failure| will be set to reflect the type of failure that occurred,
  // false will be returned.  If this EAP event has no direct outcome,
  // this function returns false without changing |failure|.
  virtual bool ParseStatus(const std::string& status,
                           const std::string& parameter,
                           Service::ConnectFailure* failure);

  // Resets the internal state of the handler.
  virtual void Reset();

  virtual bool is_eap_in_progress() { return is_eap_in_progress_; }

 private:
  friend class SupplicantEAPStateHandlerTest;

  // The stored TLS error type which may lead to an EAP failure.
  std::string tls_error_;

  // Whether or not an EAP authentication is in progress.  Note
  // specifically that an EAP failure in wpa_supplicant does not
  // automatically cause the EAP process to stop, while success does.
  bool is_eap_in_progress_;
};

}  // namespace shill

#endif  // SHILL_SUPPLICANT_SUPPLICANT_EAP_STATE_HANDLER_H_
