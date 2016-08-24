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

#ifndef POLO_PAIRING_PAIRINGCONTEXT_H_
#define POLO_PAIRING_PAIRINGCONTEXT_H_

#include <openssl/x509v3.h>
#include <openssl/ssl.h>

namespace polo {
namespace pairing {

// Context for a Polo pairing session.
class PairingContext {
 public:
  // Creates a new Polo pairing context. This class does not take ownership of
  // the certificates but they must remain valid for as long as this context
  // exists.
  // @param local_certificate the local SSL certificate
  // @param peer_certificate the peer SSL certificate
  // @param server whether this client is acting as the pairing server
  PairingContext(X509* local_certificate,
                 X509* peer_certificate,
                 bool server);

  // Sets the local certificate. No ownership is taken of the given pointer.
  void set_local_certificate(X509 *local_certificate);

  // Sets the peer certificate. No ownership is taken of the given pointer.
  void set_peer_certificate(X509 *peer_certificate);

  // Gets the client certificate.
  X509* client_certificate() const;

  // Gets the server certificate.
  X509* server_certificate() const;

  // Determines whether this client is the pairing server.
  bool is_server() const;

  // Determines whether this client is the pairing client.
  bool is_client() const;

 private:
  X509 *local_certificate_;
  X509 *peer_certificate_;
  bool server_;
};

}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_PAIRINGCONTEXT_H_
