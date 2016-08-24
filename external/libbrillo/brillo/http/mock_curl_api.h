// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_HTTP_MOCK_CURL_API_H_
#define LIBBRILLO_BRILLO_HTTP_MOCK_CURL_API_H_

#include <string>

#include <brillo/http/curl_api.h>
#include <gmock/gmock.h>

namespace brillo {
namespace http {

// This is a mock for CURL interfaces which allows to mock out the CURL's
// low-level C APIs in tests by intercepting the virtual function calls on
// the abstract CurlInterface.
class MockCurlInterface : public CurlInterface {
 public:
  MockCurlInterface() = default;

  MOCK_METHOD0(EasyInit, CURL*());
  MOCK_METHOD1(EasyCleanup, void(CURL*));
  MOCK_METHOD3(EasySetOptInt, CURLcode(CURL*, CURLoption, int));
  MOCK_METHOD3(EasySetOptStr, CURLcode(CURL*, CURLoption, const std::string&));
  MOCK_METHOD3(EasySetOptPtr, CURLcode(CURL*, CURLoption, void*));
  MOCK_METHOD3(EasySetOptCallback, CURLcode(CURL*, CURLoption, intptr_t));
  MOCK_METHOD3(EasySetOptOffT, CURLcode(CURL*, CURLoption, curl_off_t));
  MOCK_METHOD1(EasyPerform, CURLcode(CURL*));
  MOCK_CONST_METHOD3(EasyGetInfoInt, CURLcode(CURL*, CURLINFO, int*));
  MOCK_CONST_METHOD3(EasyGetInfoDbl, CURLcode(CURL*, CURLINFO, double*));
  MOCK_CONST_METHOD3(EasyGetInfoStr, CURLcode(CURL*, CURLINFO, std::string*));
  MOCK_CONST_METHOD3(EasyGetInfoPtr, CURLcode(CURL*, CURLINFO, void**));
  MOCK_CONST_METHOD1(EasyStrError, std::string(CURLcode));
  MOCK_METHOD0(MultiInit, CURLM*());
  MOCK_METHOD1(MultiCleanup, CURLMcode(CURLM*));
  MOCK_METHOD2(MultiInfoRead, CURLMsg*(CURLM*, int*));
  MOCK_METHOD2(MultiAddHandle, CURLMcode(CURLM*, CURL*));
  MOCK_METHOD2(MultiRemoveHandle, CURLMcode(CURLM*, CURL*));
  MOCK_METHOD3(MultiSetSocketCallback,
               CURLMcode(CURLM*, curl_socket_callback, void*));
  MOCK_METHOD3(MultiSetTimerCallback,
               CURLMcode(CURLM*, curl_multi_timer_callback, void*));
  MOCK_METHOD3(MultiAssign, CURLMcode(CURLM*, curl_socket_t, void*));
  MOCK_METHOD4(MultiSocketAction, CURLMcode(CURLM*, curl_socket_t, int, int*));
  MOCK_CONST_METHOD1(MultiStrError, std::string(CURLMcode));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockCurlInterface);
};

}  // namespace http
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_HTTP_MOCK_CURL_API_H_
