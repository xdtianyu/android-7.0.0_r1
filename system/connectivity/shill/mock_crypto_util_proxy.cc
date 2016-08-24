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

#include "shill/mock_crypto_util_proxy.h"

#include <string>
#include <vector>

#include "shill/callbacks.h"
#include "shill/error.h"
#include "shill/event_dispatcher.h"
#include "shill/testing.h"

using testing::_;
using testing::DoAll;
using testing::Return;

namespace shill {

MockCryptoUtilProxy::MockCryptoUtilProxy(
    EventDispatcher* dispatcher)
    : CryptoUtilProxy(dispatcher) {
  ON_CALL(*this, VerifyDestination(_, _, _, _, _, _, _, _, _))
      .WillByDefault(DoAll(SetOperationFailedInArgumentAndWarn<8>(),
                           Return(false)));
  ON_CALL(*this, EncryptData(_, _, _, _))
      .WillByDefault(DoAll(SetOperationFailedInArgumentAndWarn<3>(),
                           Return(false)));
}

MockCryptoUtilProxy::~MockCryptoUtilProxy() {}

bool MockCryptoUtilProxy::RealVerifyDestination(
    const std::string& certificate,
    const std::string& public_key,
    const std::string& nonce,
    const std::string& signed_data,
    const std::string& destination_udn,
    const std::vector<uint8_t>& ssid,
    const std::string& bssid,
    const ResultBoolCallback& result_callback,
    Error* error) {
  return CryptoUtilProxy::VerifyDestination(certificate, public_key,
                                            nonce, signed_data,
                                            destination_udn, ssid, bssid,
                                            result_callback, error);
}

bool MockCryptoUtilProxy::RealEncryptData(
    const std::string& public_key,
    const std::string& data,
    const ResultStringCallback& result_callback,
    Error* error) {
  return CryptoUtilProxy::EncryptData(public_key, data,
                                      result_callback, error);
}

bool MockCryptoUtilProxy::RealStartShimForCommand(
    const std::string& command,
    const std::string& input,
    const StringCallback& result_handler) {
  return CryptoUtilProxy::StartShimForCommand(command, input,
                                              result_handler);
}

}  // namespace shill
