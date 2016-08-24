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

#include "shill/supplicant/supplicant_eap_state_handler.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/mock_log.h"
#include "shill/supplicant/wpa_supplicant.h"

using std::string;
using testing::_;
using testing::EndsWith;
using testing::Mock;

namespace shill {

class SupplicantEAPStateHandlerTest : public testing::Test {
 public:
  SupplicantEAPStateHandlerTest() : failure_(Service::kFailureUnknown) {}
  virtual ~SupplicantEAPStateHandlerTest() {}

 protected:
  void StartEAP() {
    EXPECT_CALL(log_, Log(logging::LOG_INFO, _,
                          EndsWith("Authentication starting.")));
    EXPECT_FALSE(handler_.ParseStatus(WPASupplicant::kEAPStatusStarted, "",
                                      &failure_));
    Mock::VerifyAndClearExpectations(&log_);
  }

  const string& GetTLSError() { return handler_.tls_error_; }

  SupplicantEAPStateHandler handler_;
  Service::ConnectFailure failure_;
  ScopedMockLog log_;
};

TEST_F(SupplicantEAPStateHandlerTest, Construct) {
  EXPECT_FALSE(handler_.is_eap_in_progress());
  EXPECT_EQ("", GetTLSError());
}

TEST_F(SupplicantEAPStateHandlerTest, AuthenticationStarting) {
  StartEAP();
  EXPECT_TRUE(handler_.is_eap_in_progress());
  EXPECT_EQ("", GetTLSError());
  EXPECT_EQ(Service::kFailureUnknown, failure_);
}

TEST_F(SupplicantEAPStateHandlerTest, AcceptedMethod) {
  StartEAP();
  const string kEAPMethod("EAP-ROCHAMBEAU");
  EXPECT_CALL(log_, Log(logging::LOG_INFO, _,
                        EndsWith("accepted method " + kEAPMethod)));
  EXPECT_FALSE(handler_.ParseStatus(
      WPASupplicant::kEAPStatusAcceptProposedMethod, kEAPMethod, &failure_));
  EXPECT_TRUE(handler_.is_eap_in_progress());
  EXPECT_EQ("", GetTLSError());
  EXPECT_EQ(Service::kFailureUnknown, failure_);
}

TEST_F(SupplicantEAPStateHandlerTest, SuccessfulCompletion) {
  StartEAP();
  EXPECT_CALL(log_, Log(_, _,
                        EndsWith("Completed authentication successfully.")));
  EXPECT_TRUE(handler_.ParseStatus(WPASupplicant::kEAPStatusCompletion,
                                   WPASupplicant::kEAPParameterSuccess,
                                   &failure_));
  EXPECT_FALSE(handler_.is_eap_in_progress());
  EXPECT_EQ("", GetTLSError());
  EXPECT_EQ(Service::kFailureUnknown, failure_);
}

TEST_F(SupplicantEAPStateHandlerTest, EAPFailureGeneric) {
  StartEAP();
  // An EAP failure without a previous TLS indication yields a generic failure.
  EXPECT_FALSE(handler_.ParseStatus(WPASupplicant::kEAPStatusCompletion,
                                    WPASupplicant::kEAPParameterFailure,
                                    &failure_));

  // Since it hasn't completed successfully, we must assume even in failure
  // that wpa_supplicant is continuing the EAP authentication process.
  EXPECT_TRUE(handler_.is_eap_in_progress());
  EXPECT_EQ("", GetTLSError());
  EXPECT_EQ(Service::kFailureEAPAuthentication, failure_);
}

TEST_F(SupplicantEAPStateHandlerTest, EAPFailureLocalTLSIndication) {
  StartEAP();
  // A TLS indication should be stored but a failure should not be returned.
  EXPECT_FALSE(handler_.ParseStatus(WPASupplicant::kEAPStatusLocalTLSAlert, "",
                                    &failure_));
  EXPECT_TRUE(handler_.is_eap_in_progress());
  EXPECT_EQ(WPASupplicant::kEAPStatusLocalTLSAlert, GetTLSError());
  EXPECT_EQ(Service::kFailureUnknown, failure_);

  // An EAP failure with a previous TLS indication yields a specific failure.
  EXPECT_FALSE(handler_.ParseStatus(WPASupplicant::kEAPStatusCompletion,
                                    WPASupplicant::kEAPParameterFailure,
                                    &failure_));
  EXPECT_TRUE(handler_.is_eap_in_progress());
  EXPECT_EQ(Service::kFailureEAPLocalTLS, failure_);
}

TEST_F(SupplicantEAPStateHandlerTest, EAPFailureRemoteTLSIndication) {
  StartEAP();
  // A TLS indication should be stored but a failure should not be returned.
  EXPECT_FALSE(handler_.ParseStatus(WPASupplicant::kEAPStatusRemoteTLSAlert, "",
                                    &failure_));
  EXPECT_TRUE(handler_.is_eap_in_progress());
  EXPECT_EQ(WPASupplicant::kEAPStatusRemoteTLSAlert, GetTLSError());
  EXPECT_EQ(Service::kFailureUnknown, failure_);

  // An EAP failure with a previous TLS indication yields a specific failure.
  EXPECT_FALSE(handler_.ParseStatus(WPASupplicant::kEAPStatusCompletion,
                                    WPASupplicant::kEAPParameterFailure,
                                    &failure_));
  EXPECT_TRUE(handler_.is_eap_in_progress());
  EXPECT_EQ(Service::kFailureEAPRemoteTLS, failure_);
}


TEST_F(SupplicantEAPStateHandlerTest, BadRemoteCertificateVerification) {
  StartEAP();
  const string kStrangeParameter("ennui");
  EXPECT_CALL(log_, Log(logging::LOG_ERROR, _, EndsWith(
      string("Unexpected ") +
      WPASupplicant::kEAPStatusRemoteCertificateVerification +
      " parameter: " + kStrangeParameter)));
  EXPECT_FALSE(handler_.ParseStatus(
      WPASupplicant::kEAPStatusRemoteCertificateVerification, kStrangeParameter,
      &failure_));
  // Although we reported an error, this shouldn't mean failure.
  EXPECT_TRUE(handler_.is_eap_in_progress());
  EXPECT_EQ("", GetTLSError());
  EXPECT_EQ(Service::kFailureUnknown, failure_);
}

TEST_F(SupplicantEAPStateHandlerTest, ParameterNeeded) {
  StartEAP();
  const string kAuthenticationParameter("nudge nudge say no more");
  EXPECT_CALL(log_, Log(logging::LOG_ERROR, _, EndsWith(
      string("aborted due to missing authentication parameter: ") +
      kAuthenticationParameter)));
  EXPECT_FALSE(handler_.ParseStatus(
      WPASupplicant::kEAPStatusParameterNeeded, kAuthenticationParameter,
      &failure_));
  EXPECT_TRUE(handler_.is_eap_in_progress());
  EXPECT_EQ("", GetTLSError());
  EXPECT_EQ(Service::kFailureEAPAuthentication, failure_);
}

TEST_F(SupplicantEAPStateHandlerTest, ParameterNeededPin) {
  StartEAP();
  EXPECT_FALSE(handler_.ParseStatus(
      WPASupplicant::kEAPStatusParameterNeeded,
      WPASupplicant::kEAPRequestedParameterPIN,
      &failure_));
  EXPECT_TRUE(handler_.is_eap_in_progress());
  EXPECT_EQ("", GetTLSError());
  EXPECT_EQ(Service::kFailurePinMissing, failure_);
}

}  // namespace shill
