// Copyright 2012 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "polo/pairing/pairingcontext.h"

namespace polo {
namespace pairing {

PairingContext::PairingContext(X509 *local_certificate,
                               X509 *peer_certificate,
                               bool server)
    : local_certificate_(local_certificate),
      peer_certificate_(peer_certificate),
      server_(server) {
}

void PairingContext::set_local_certificate(X509* local_certificate) {
  local_certificate_ = local_certificate;
}

void PairingContext::set_peer_certificate(X509* peer_certificate) {
  peer_certificate_ = peer_certificate;
}

X509* PairingContext::client_certificate() const {
  return server_ ? peer_certificate_ : local_certificate_;
}

X509* PairingContext::server_certificate() const {
  return server_ ? local_certificate_ : peer_certificate_;
}

bool PairingContext::is_server() const {
  return server_;
}

bool PairingContext::is_client() const {
  return !server_;
}

}  // namespace pairing
}  // namespace polo
