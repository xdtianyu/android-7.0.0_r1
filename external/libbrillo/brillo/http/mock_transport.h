// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_HTTP_MOCK_TRANSPORT_H_
#define LIBBRILLO_BRILLO_HTTP_MOCK_TRANSPORT_H_

#include <memory>
#include <string>

#include <base/macros.h>
#include <brillo/http/http_transport.h>
#include <gmock/gmock.h>

namespace brillo {
namespace http {

class MockTransport : public Transport {
 public:
  MockTransport() = default;

  MOCK_METHOD6(CreateConnection,
               std::shared_ptr<Connection>(const std::string&,
                                           const std::string&,
                                           const HeaderList&,
                                           const std::string&,
                                           const std::string&,
                                           brillo::ErrorPtr*));
  MOCK_METHOD2(RunCallbackAsync,
               void(const tracked_objects::Location&, const base::Closure&));
  MOCK_METHOD3(StartAsyncTransfer, RequestID(Connection*,
                                             const SuccessCallback&,
                                             const ErrorCallback&));
  MOCK_METHOD1(CancelRequest, bool(RequestID));
  MOCK_METHOD1(SetDefaultTimeout, void(base::TimeDelta));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTransport);
};

}  // namespace http
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_HTTP_MOCK_TRANSPORT_H_
