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

#ifndef SHILL_MOCK_CRYPTO_UTIL_PROXY_H_
#define SHILL_MOCK_CRYPTO_UTIL_PROXY_H_

#include "shill/crypto_util_proxy.h"

#include <string>
#include <vector>

#include <base/macros.h>
#include <gmock/gmock.h>

namespace shill {

class Error;
class EventDispatcher;

class MockCryptoUtilProxy
    : public CryptoUtilProxy,
      public base::SupportsWeakPtr<MockCryptoUtilProxy> {
 public:
  explicit MockCryptoUtilProxy(EventDispatcher* dispatcher);
  ~MockCryptoUtilProxy() override;

  MOCK_METHOD9(VerifyDestination,
               bool(const std::string& certificate,
                    const std::string& public_key,
                    const std::string& nonce,
                    const std::string& signed_data,
                    const std::string& destination_udn,
                    const std::vector<uint8_t>& ssid,
                    const std::string& bssid,
                    const ResultBoolCallback& result_callback,
                    Error* error));
  MOCK_METHOD4(EncryptData, bool(const std::string& public_key,
                                 const std::string& data,
                                 const ResultStringCallback& result_callback,
                                 Error* error));

  bool RealVerifyDestination(const std::string& certificate,
                             const std::string& public_key,
                             const std::string& nonce,
                             const std::string& signed_data,
                             const std::string& destination_udn,
                             const std::vector<uint8_t>& ssid,
                             const std::string& bssid,
                             const ResultBoolCallback& result_callback,
                             Error* error);

  bool RealEncryptData(const std::string& public_key,
                       const std::string& data,
                       const ResultStringCallback& result_callback,
                       Error* error);

  // Mock methods with useful callback signatures.  You can bind these to check
  // that appropriate async callbacks are firing at expected times.
  MOCK_METHOD2(TestResultBoolCallback, void(const Error& error, bool));
  MOCK_METHOD2(TestResultStringCallback, void(const Error& error,
                                              const std::string&));
  MOCK_METHOD2(TestResultHandlerCallback, void(const std::string& result,
                                               const Error& error));
  MOCK_METHOD3(StartShimForCommand, bool(const std::string& command,
                                         const std::string& input,
                                         const StringCallback& result_handler));

  // Methods injected to permit us to call the real method implementations.
  bool RealStartShimForCommand(const std::string& command,
                               const std::string& input,
                               const StringCallback& result_handler);

 private:
  DISALLOW_COPY_AND_ASSIGN(MockCryptoUtilProxy);
};

}  // namespace shill

#endif  // SHILL_MOCK_CRYPTO_UTIL_PROXY_H_
