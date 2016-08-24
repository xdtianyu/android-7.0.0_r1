// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/privet_handler.h"

#include <set>
#include <string>
#include <utility>

#include <base/bind.h>
#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <base/strings/string_util.h>
#include <base/values.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/test/unittest_utils.h>

#include "src/privet/constants.h"
#include "src/privet/mock_delegates.h"
#include "src/test/mock_clock.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgPointee;
using testing::SaveArg;
using testing::WithArgs;

namespace weave {
namespace privet {

namespace {

void LoadTestJson(const std::string& test_json,
                  base::DictionaryValue* dictionary) {
  std::string json = test_json;
  base::ReplaceChars(json, "'", "\"", &json);
  int error = 0;
  std::string message;
  std::unique_ptr<base::Value> value(
      base::JSONReader::ReadAndReturnError(json, base::JSON_PARSE_RFC, &error,
                                           &message)
          .release());
  EXPECT_TRUE(value.get()) << "\nError: " << message << "\n" << json;
  base::DictionaryValue* dictionary_ptr = nullptr;
  if (value->GetAsDictionary(&dictionary_ptr))
    dictionary->MergeDictionary(dictionary_ptr);
}

struct CodeWithReason {
  CodeWithReason(int code_in, const std::string& reason_in)
      : code(code_in), reason(reason_in) {}
  int code;
  std::string reason;
};

std::ostream& operator<<(std::ostream& stream, const CodeWithReason& error) {
  return stream << "{" << error.code << ", " << error.reason << "}";
}

bool IsEqualError(const CodeWithReason& expected,
                  const base::DictionaryValue& dictionary) {
  std::string reason;
  int code = 0;
  return dictionary.GetInteger("error.http_status", &code) &&
         code == expected.code && dictionary.GetString("error.code", &reason) &&
         reason == expected.reason;
}

// Some error sections in response JSON objects contained debugging information
// which is of no interest for this test. So, remove the debug info from the
// JSON before running validation logic on it.
std::unique_ptr<base::DictionaryValue> StripDebugErrorDetails(
    const std::string& path_to_error_object,
    const base::DictionaryValue& value) {
  std::unique_ptr<base::DictionaryValue> result{value.DeepCopy()};
  base::DictionaryValue* error_dict = nullptr;
  EXPECT_TRUE(result->GetDictionary(path_to_error_object, &error_dict));
  scoped_ptr<base::Value> dummy;
  error_dict->RemovePath("error.debugInfo", &dummy);
  error_dict->RemovePath("error.message", &dummy);
  return result;
}

}  // namespace

class PrivetHandlerTest : public testing::Test {
 public:
  PrivetHandlerTest() {}

 protected:
  void SetUp() override {
    EXPECT_CALL(clock_, Now())
        .WillRepeatedly(Return(base::Time::FromTimeT(1410000001)));

    auth_header_ = "Privet anonymous";
    handler_.reset(
        new PrivetHandler(&cloud_, &device_, &security_, &wifi_, &clock_));
  }

  const base::DictionaryValue& HandleRequest(
      const std::string& api,
      const base::DictionaryValue* input) {
    output_.Clear();
    handler_->HandleRequest(api, auth_header_, input,
                            base::Bind(&PrivetHandlerTest::HandlerCallback,
                                       base::Unretained(this)));
    return output_;
  }

  const base::DictionaryValue& HandleRequest(const std::string& api,
                                             const std::string& json_input) {
    base::DictionaryValue dictionary;
    LoadTestJson(json_input, &dictionary);
    return HandleRequest(api, &dictionary);
  }

  void HandleUnknownRequest(const std::string& api) {
    output_.Clear();
    base::DictionaryValue dictionary;
    handler_->HandleRequest(api, auth_header_, &dictionary,
                            base::Bind(&PrivetHandlerTest::HandlerNoFound));
  }

  const base::DictionaryValue& GetResponse() const { return output_; }
  int GetResponseCount() const { return response_count_; }

  void SetNoWifiAndGcd() {
    handler_.reset(
        new PrivetHandler(&cloud_, &device_, &security_, nullptr, &clock_));
    EXPECT_CALL(cloud_, GetCloudId()).WillRepeatedly(Return(""));
    EXPECT_CALL(cloud_, GetConnectionState())
        .WillRepeatedly(ReturnRef(gcd_disabled_state_));
    auto set_error = [](const std::string&, const std::string&,
                        ErrorPtr* error) {
      Error::AddTo(error, FROM_HERE, "setupUnavailable", "");
    };
    EXPECT_CALL(cloud_, Setup(_, _, _))
        .WillRepeatedly(DoAll(Invoke(set_error), Return(false)));
  }

  test::MockClock clock_;
  testing::StrictMock<MockCloudDelegate> cloud_;
  testing::StrictMock<MockDeviceDelegate> device_;
  testing::StrictMock<MockSecurityDelegate> security_;
  testing::StrictMock<MockWifiDelegate> wifi_;
  std::string auth_header_;

 private:
  void HandlerCallback(int status, const base::DictionaryValue& output) {
    output_.Clear();
    ++response_count_;
    output_.MergeDictionary(&output);
    if (!output_.HasKey("error")) {
      EXPECT_EQ(200, status);
      return;
    }
    EXPECT_NE(200, status);
    output_.SetInteger("error.http_status", status);
  }

  static void HandlerNoFound(int status, const base::DictionaryValue&) {
    EXPECT_EQ(404, status);
  }

  std::unique_ptr<PrivetHandler> handler_;
  base::DictionaryValue output_;
  int response_count_{0};
  ConnectionState gcd_disabled_state_{ConnectionState::kDisabled};
};

TEST_F(PrivetHandlerTest, UnknownApi) {
  HandleUnknownRequest("/privet/foo");
}

TEST_F(PrivetHandlerTest, InvalidFormat) {
  auth_header_ = "";
  EXPECT_PRED2(IsEqualError, CodeWithReason(400, "invalidFormat"),
               HandleRequest("/privet/info", nullptr));
}

TEST_F(PrivetHandlerTest, MissingAuth) {
  auth_header_ = "";
  EXPECT_PRED2(IsEqualError, CodeWithReason(401, "missingAuthorization"),
               HandleRequest("/privet/info", "{}"));
}

TEST_F(PrivetHandlerTest, InvalidAuth) {
  auth_header_ = "foo";
  EXPECT_PRED2(IsEqualError, CodeWithReason(401, "invalidAuthorization"),
               HandleRequest("/privet/info", "{}"));
}

TEST_F(PrivetHandlerTest, ExpiredAuth) {
  auth_header_ = "Privet 123";
  EXPECT_CALL(security_, ParseAccessToken(_, _, _))
      .WillRepeatedly(WithArgs<2>(Invoke([](ErrorPtr* error) {
        return Error::AddTo(error, FROM_HERE, "authorizationExpired", "");
      })));
  EXPECT_PRED2(IsEqualError, CodeWithReason(403, "authorizationExpired"),
               HandleRequest("/privet/info", "{}"));
}

TEST_F(PrivetHandlerTest, InvalidAuthScope) {
  EXPECT_PRED2(IsEqualError, CodeWithReason(403, "invalidAuthorizationScope"),
               HandleRequest("/privet/v3/setup/start", "{}"));
}

TEST_F(PrivetHandlerTest, InfoMinimal) {
  SetNoWifiAndGcd();
  EXPECT_CALL(security_, GetPairingTypes())
      .WillRepeatedly(Return(std::set<PairingType>{}));
  EXPECT_CALL(security_, GetCryptoTypes())
      .WillRepeatedly(Return(std::set<CryptoType>{}));
  EXPECT_CALL(security_, GetAuthTypes())
      .WillRepeatedly(Return(std::set<AuthType>{}));

  const char kExpected[] = R"({
    'version': '3.0',
    'id': 'TestId',
    'name': 'TestDevice',
    'services': [ "developmentBoard" ],
    'modelManifestId': "ABMID",
    'basicModelManifest': {
      'uiDeviceKind': 'developmentBoard',
      'oemName': 'Chromium',
      'modelName': 'Brillo'
    },
    'endpoints': {
      'httpPort': 0,
      'httpUpdatesPort': 0,
      'httpsPort': 0,
      'httpsUpdatesPort': 0
    },
    'authentication': {
      'anonymousMaxScope': 'user',
      'mode': [
      ],
      'pairing': [
      ],
      'crypto': [
      ]
    },
    'gcd': {
      'id': '',
      'status': 'disabled'
    },
    'time': 1410000001000.0,
    'sessionId': 'SessionId'
  })";
  EXPECT_JSON_EQ(kExpected, HandleRequest("/privet/info", "{}"));
}

TEST_F(PrivetHandlerTest, Info) {
  EXPECT_CALL(cloud_, GetDescription())
      .WillRepeatedly(Return("TestDescription"));
  EXPECT_CALL(cloud_, GetLocation()).WillRepeatedly(Return("TestLocation"));
  EXPECT_CALL(device_, GetHttpEnpoint())
      .WillRepeatedly(Return(std::make_pair(80, 10080)));
  EXPECT_CALL(device_, GetHttpsEnpoint())
      .WillRepeatedly(Return(std::make_pair(443, 10443)));
  EXPECT_CALL(wifi_, GetHostedSsid())
      .WillRepeatedly(Return("Test_device.BBABCLAprv"));

  const char kExpected[] = R"({
    'version': '3.0',
    'id': 'TestId',
    'name': 'TestDevice',
    'description': 'TestDescription',
    'location': 'TestLocation',
    'services': [ "developmentBoard" ],
    'modelManifestId': "ABMID",
    'basicModelManifest': {
      'uiDeviceKind': 'developmentBoard',
      'oemName': 'Chromium',
      'modelName': 'Brillo'
    },
    'endpoints': {
      'httpPort': 80,
      'httpUpdatesPort': 10080,
      'httpsPort': 443,
      'httpsUpdatesPort': 10443
    },
    'authentication': {
      'anonymousMaxScope': 'none',
      'mode': [
        'anonymous',
        'pairing',
        'local'
      ],
      'pairing': [
        'pinCode',
        'embeddedCode'
      ],
      'crypto': [
        'p224_spake2'
      ]
    },
    'wifi': {
      'capabilities': [
        '2.4GHz'
      ],
      'ssid': 'TestSsid',
      'hostedSsid': 'Test_device.BBABCLAprv',
      'status': 'offline'
    },
    'gcd': {
      'id': 'TestCloudId',
      'status': 'online'
    },
    'time': 1410000001000.0,
    'sessionId': 'SessionId'
  })";
  EXPECT_JSON_EQ(kExpected, HandleRequest("/privet/info", "{}"));
}

TEST_F(PrivetHandlerTest, PairingStartInvalidParams) {
  EXPECT_PRED2(IsEqualError, CodeWithReason(400, "invalidParams"),
               HandleRequest("/privet/v3/pairing/start",
                             "{'pairing':'embeddedCode','crypto':'crypto'}"));

  EXPECT_PRED2(IsEqualError, CodeWithReason(400, "invalidParams"),
               HandleRequest("/privet/v3/pairing/start",
                             "{'pairing':'code','crypto':'p224_spake2'}"));
}

TEST_F(PrivetHandlerTest, PairingStart) {
  EXPECT_JSON_EQ(
      "{'deviceCommitment': 'testCommitment', 'sessionId': 'testSession'}",
      HandleRequest("/privet/v3/pairing/start",
                    "{'pairing': 'embeddedCode', 'crypto': 'p224_spake2'}"));
}

TEST_F(PrivetHandlerTest, PairingConfirm) {
  EXPECT_JSON_EQ(
      "{'certFingerprint':'testFingerprint','certSignature':'testSignature'}",
      HandleRequest(
          "/privet/v3/pairing/confirm",
          "{'sessionId':'testSession','clientCommitment':'testCommitment'}"));
}

TEST_F(PrivetHandlerTest, PairingCancel) {
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/pairing/cancel",
                                     "{'sessionId': 'testSession'}"));
}

TEST_F(PrivetHandlerTest, AuthErrorNoType) {
  EXPECT_PRED2(IsEqualError, CodeWithReason(400, "invalidAuthMode"),
               HandleRequest("/privet/v3/auth", "{}"));
}

TEST_F(PrivetHandlerTest, AuthErrorInvalidType) {
  EXPECT_PRED2(IsEqualError, CodeWithReason(400, "invalidAuthMode"),
               HandleRequest("/privet/v3/auth", "{'mode':'unknown'}"));
}

TEST_F(PrivetHandlerTest, AuthErrorNoScope) {
  EXPECT_PRED2(IsEqualError, CodeWithReason(400, "invalidRequestedScope"),
               HandleRequest("/privet/v3/auth", "{'mode':'anonymous'}"));
}

TEST_F(PrivetHandlerTest, AuthErrorInvalidScope) {
  EXPECT_PRED2(
      IsEqualError, CodeWithReason(400, "invalidRequestedScope"),
      HandleRequest("/privet/v3/auth",
                    "{'mode':'anonymous','requestedScope':'unknown'}"));
}

TEST_F(PrivetHandlerTest, AuthErrorAccessDenied) {
  EXPECT_PRED2(IsEqualError, CodeWithReason(403, "accessDenied"),
               HandleRequest("/privet/v3/auth",
                             "{'mode':'anonymous','requestedScope':'owner'}"));
}

TEST_F(PrivetHandlerTest, AuthErrorInvalidAuthCode) {
  auto set_error = [](ErrorPtr* error) {
    return Error::AddTo(error, FROM_HERE, "invalidAuthCode", "");
  };
  EXPECT_CALL(security_, CreateAccessToken(_, "testToken", _, _, _, _, _))
      .WillRepeatedly(WithArgs<6>(Invoke(set_error)));
  const char kInput[] = R"({
    'mode': 'pairing',
    'requestedScope': 'user',
    'authCode': 'testToken'
  })";
  EXPECT_PRED2(IsEqualError, CodeWithReason(403, "invalidAuthCode"),
               HandleRequest("/privet/v3/auth", kInput));
}

TEST_F(PrivetHandlerTest, AuthAnonymous) {
  const char kExpected[] = R"({
    'accessToken': 'GuestAccessToken',
    'expiresIn': 15,
    'scope': 'viewer',
    'tokenType': 'Privet'
  })";
  EXPECT_JSON_EQ(kExpected,
                 HandleRequest("/privet/v3/auth",
                               "{'mode':'anonymous','requestedScope':'auto'}"));
}

TEST_F(PrivetHandlerTest, AuthPairing) {
  EXPECT_CALL(security_, CreateAccessToken(_, _, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<3>("OwnerAccessToken"),
                            SetArgPointee<4>(AuthScope::kOwner),
                            SetArgPointee<5>(base::TimeDelta::FromSeconds(15)),
                            Return(true)));
  const char kInput[] = R"({
    'mode': 'pairing',
    'requestedScope': 'owner',
    'authCode': 'testToken'
  })";
  const char kExpected[] = R"({
    'accessToken': 'OwnerAccessToken',
    'expiresIn': 15,
    'scope': 'owner',
    'tokenType': 'Privet'
  })";
  EXPECT_JSON_EQ(kExpected, HandleRequest("/privet/v3/auth", kInput));
}

TEST_F(PrivetHandlerTest, AuthLocalAuto) {
  EXPECT_CALL(security_, CreateAccessToken(_, _, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<3>("UserAccessToken"),
                            SetArgPointee<4>(AuthScope::kUser),
                            SetArgPointee<5>(base::TimeDelta::FromSeconds(15)),
                            Return(true)));
  const char kInput[] = R"({
    'mode': 'local',
    'requestedScope': 'auto',
    'authCode': 'localAuthToken'
  })";
  const char kExpected[] = R"({
    'accessToken': 'UserAccessToken',
    'expiresIn': 15,
    'scope': 'user',
    'tokenType': 'Privet'
  })";
  EXPECT_JSON_EQ(kExpected, HandleRequest("/privet/v3/auth", kInput));
}

TEST_F(PrivetHandlerTest, AuthLocal) {
  EXPECT_CALL(security_, CreateAccessToken(_, _, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<3>("ManagerAccessToken"),
                            SetArgPointee<4>(AuthScope::kManager),
                            SetArgPointee<5>(base::TimeDelta::FromSeconds(15)),
                            Return(true)));
  const char kInput[] = R"({
    'mode': 'local',
    'requestedScope': 'manager',
    'authCode': 'localAuthToken'
  })";
  const char kExpected[] = R"({
    'accessToken': 'ManagerAccessToken',
    'expiresIn': 15,
    'scope': 'manager',
    'tokenType': 'Privet'
  })";
  EXPECT_JSON_EQ(kExpected, HandleRequest("/privet/v3/auth", kInput));
}

TEST_F(PrivetHandlerTest, AuthLocalHighScope) {
  EXPECT_CALL(security_, CreateAccessToken(_, _, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<3>("UserAccessToken"),
                            SetArgPointee<4>(AuthScope::kUser),
                            SetArgPointee<5>(base::TimeDelta::FromSeconds(1)),
                            Return(true)));
  const char kInput[] = R"({
    'mode': 'local',
    'requestedScope': 'manager',
    'authCode': 'localAuthToken'
  })";
  EXPECT_PRED2(IsEqualError, CodeWithReason(403, "accessDenied"),
               HandleRequest("/privet/v3/auth", kInput));
}

class PrivetHandlerTestWithAuth : public PrivetHandlerTest {
 public:
  void SetUp() override {
    PrivetHandlerTest::SetUp();
    auth_header_ = "Privet 123";
    EXPECT_CALL(security_, ParseAccessToken(_, _, _))
        .WillRepeatedly(DoAll(
            SetArgPointee<1>(UserInfo{AuthScope::kOwner, TestUserId{"1"}}),
            Return(true)));
  }
};

class PrivetHandlerSetupTest : public PrivetHandlerTestWithAuth {};

TEST_F(PrivetHandlerSetupTest, StatusEmpty) {
  SetNoWifiAndGcd();
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/setup/status", "{}"));
}

TEST_F(PrivetHandlerSetupTest, StatusWifi) {
  wifi_.setup_state_ = SetupState{SetupState::kSuccess};

  const char kExpected[] = R"({
    'wifi': {
        'ssid': 'TestSsid',
        'status': 'success'
     }
  })";
  EXPECT_JSON_EQ(kExpected, HandleRequest("/privet/v3/setup/status", "{}"));
}

TEST_F(PrivetHandlerSetupTest, StatusWifiError) {
  ErrorPtr error;
  Error::AddTo(&error, FROM_HERE, "invalidPassphrase", "");
  wifi_.setup_state_ = SetupState{std::move(error)};

  const char kExpected[] = R"({
    'wifi': {
        'status': 'error',
        'error': {
          'code': 'invalidPassphrase'
        }
     }
  })";
  EXPECT_JSON_EQ(kExpected,
                 *StripDebugErrorDetails(
                     "wifi", HandleRequest("/privet/v3/setup/status", "{}")));
}

TEST_F(PrivetHandlerSetupTest, StatusGcd) {
  cloud_.setup_state_ = SetupState{SetupState::kSuccess};

  const char kExpected[] = R"({
    'gcd': {
        'id': 'TestCloudId',
        'status': 'success'
     }
  })";
  EXPECT_JSON_EQ(kExpected, HandleRequest("/privet/v3/setup/status", "{}"));
}

TEST_F(PrivetHandlerSetupTest, StatusGcdError) {
  ErrorPtr error;
  Error::AddTo(&error, FROM_HERE, "invalidTicket", "");
  cloud_.setup_state_ = SetupState{std::move(error)};

  const char kExpected[] = R"({
    'gcd': {
        'status': 'error',
        'error': {
          'code': 'invalidTicket'
        }
     }
  })";
  EXPECT_JSON_EQ(kExpected,
                 *StripDebugErrorDetails(
                     "gcd", HandleRequest("/privet/v3/setup/status", "{}")));
}

TEST_F(PrivetHandlerSetupTest, SetupNameDescriptionLocation) {
  EXPECT_CALL(cloud_,
              UpdateDeviceInfo("testName", "testDescription", "testLocation"))
      .Times(1);
  const char kInput[] = R"({
    'name': 'testName',
    'description': 'testDescription',
    'location': 'testLocation'
  })";
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/setup/start", kInput));
}

TEST_F(PrivetHandlerSetupTest, InvalidParams) {
  const char kInputWifi[] = R"({
    'wifi': {
      'ssid': ''
    }
  })";
  EXPECT_PRED2(IsEqualError, CodeWithReason(400, "invalidParams"),
               HandleRequest("/privet/v3/setup/start", kInputWifi));

  const char kInputRegistration[] = R"({
    'gcd': {
      'ticketId': ''
    }
  })";
  EXPECT_PRED2(IsEqualError, CodeWithReason(400, "invalidParams"),
               HandleRequest("/privet/v3/setup/start", kInputRegistration));
}

TEST_F(PrivetHandlerSetupTest, WifiSetupUnavailable) {
  SetNoWifiAndGcd();
  EXPECT_PRED2(IsEqualError, CodeWithReason(400, "setupUnavailable"),
               HandleRequest("/privet/v3/setup/start", "{'wifi': {}}"));
}

TEST_F(PrivetHandlerSetupTest, WifiSetup) {
  const char kInput[] = R"({
    'wifi': {
      'ssid': 'testSsid',
      'passphrase': 'testPass'
    }
  })";
  auto set_error = [](const std::string&, const std::string&, ErrorPtr* error) {
    return Error::AddTo(error, FROM_HERE, "deviceBusy", "");
  };
  EXPECT_CALL(wifi_, ConfigureCredentials(_, _, _)).WillOnce(Invoke(set_error));
  EXPECT_PRED2(IsEqualError, CodeWithReason(503, "deviceBusy"),
               HandleRequest("/privet/v3/setup/start", kInput));

  const char kExpected[] = R"({
    'wifi': {
      'status': 'inProgress'
    }
  })";
  wifi_.setup_state_ = SetupState{SetupState::kInProgress};
  EXPECT_CALL(wifi_, ConfigureCredentials("testSsid", "testPass", _))
      .WillOnce(Return(true));
  EXPECT_JSON_EQ(kExpected, HandleRequest("/privet/v3/setup/start", kInput));
}

TEST_F(PrivetHandlerSetupTest, GcdSetupUnavailable) {
  SetNoWifiAndGcd();
  const char kInput[] = R"({
    'gcd': {
      'ticketId': 'testTicket',
      'user': 'testUser'
    }
  })";

  EXPECT_PRED2(IsEqualError, CodeWithReason(400, "setupUnavailable"),
               HandleRequest("/privet/v3/setup/start", kInput));
}

TEST_F(PrivetHandlerSetupTest, GcdSetup) {
  const char kInput[] = R"({
    'gcd': {
      'ticketId': 'testTicket',
      'user': 'testUser'
    }
  })";

  auto set_error = [](const std::string&, const std::string&, ErrorPtr* error) {
    return Error::AddTo(error, FROM_HERE, "deviceBusy", "");
  };
  EXPECT_CALL(cloud_, Setup(_, _, _)).WillOnce(Invoke(set_error));
  EXPECT_PRED2(IsEqualError, CodeWithReason(503, "deviceBusy"),
               HandleRequest("/privet/v3/setup/start", kInput));

  const char kExpected[] = R"({
    'gcd': {
      'status': 'inProgress'
    }
  })";
  cloud_.setup_state_ = SetupState{SetupState::kInProgress};
  EXPECT_CALL(cloud_, Setup("testTicket", "testUser", _))
      .WillOnce(Return(true));
  EXPECT_JSON_EQ(kExpected, HandleRequest("/privet/v3/setup/start", kInput));
}

TEST_F(PrivetHandlerSetupTest, GcdSetupAsMaster) {
  EXPECT_CALL(security_, ParseAccessToken(_, _, _))
      .WillRepeatedly(DoAll(
          SetArgPointee<1>(UserInfo{AuthScope::kManager, TestUserId{"1"}}),
          Return(true)));
  const char kInput[] = R"({
    'gcd': {
      'ticketId': 'testTicket',
      'user': 'testUser'
    }
  })";

  EXPECT_PRED2(IsEqualError, CodeWithReason(403, "invalidAuthorizationScope"),
               HandleRequest("/privet/v3/setup/start", kInput));
}

TEST_F(PrivetHandlerTestWithAuth, ClaimAccessControl) {
  EXPECT_JSON_EQ("{'clientToken': 'RootClientAuthToken'}",
                 HandleRequest("/privet/v3/accessControl/claim", "{}"));
}

TEST_F(PrivetHandlerTestWithAuth, ConfirmAccessControl) {
  EXPECT_JSON_EQ("{}",
                 HandleRequest("/privet/v3/accessControl/confirm",
                               "{'clientToken': 'DerivedClientAuthToken'}"));
}

TEST_F(PrivetHandlerTestWithAuth, State) {
  EXPECT_JSON_EQ("{'state': {'test': {}}, 'fingerprint': '1'}",
                 HandleRequest("/privet/v3/state", "{}"));

  cloud_.NotifyOnStateChanged();

  EXPECT_JSON_EQ("{'state': {'test': {}}, 'fingerprint': '2'}",
                 HandleRequest("/privet/v3/state", "{}"));
}

TEST_F(PrivetHandlerTestWithAuth, CommandsDefs) {
  EXPECT_JSON_EQ("{'commands': {'test':{}}, 'fingerprint': '1'}",
                 HandleRequest("/privet/v3/commandDefs", "{}"));

  cloud_.NotifyOnTraitDefsChanged();

  EXPECT_JSON_EQ("{'commands': {'test':{}}, 'fingerprint': '2'}",
                 HandleRequest("/privet/v3/commandDefs", "{}"));
}

TEST_F(PrivetHandlerTestWithAuth, Traits) {
  EXPECT_JSON_EQ("{'traits': {'test': {}}, 'fingerprint': '1'}",
                 HandleRequest("/privet/v3/traits", "{}"));

  cloud_.NotifyOnTraitDefsChanged();

  EXPECT_JSON_EQ("{'traits': {'test': {}}, 'fingerprint': '2'}",
                 HandleRequest("/privet/v3/traits", "{}"));
}

TEST_F(PrivetHandlerTestWithAuth, Components) {
  EXPECT_JSON_EQ("{'components': {'test': {}}, 'fingerprint': '1'}",
                 HandleRequest("/privet/v3/components", "{}"));

  cloud_.NotifyOnComponentTreeChanged();

  EXPECT_JSON_EQ("{'components': {'test': {}}, 'fingerprint': '2'}",
                 HandleRequest("/privet/v3/components", "{}"));

  // State change will also change the components fingerprint.
  cloud_.NotifyOnStateChanged();

  EXPECT_JSON_EQ("{'components': {'test': {}}, 'fingerprint': '3'}",
                 HandleRequest("/privet/v3/components", "{}"));
}

TEST_F(PrivetHandlerTestWithAuth, ComponentsWithFiltersAndPaths) {
  const char kComponents[] = R"({
    "comp1": {
      "traits": ["a", "b"],
      "state": {
        "a" : {
          "prop": 1
        }
      },
      "components": {
        "comp2": {
          "traits": ["c"],
          "components": {
            "comp4": {
              "traits": ["d"]
            }
          }
        },
        "comp3": {
          "traits": ["e"]
        }
      }
    }
  })";
  base::DictionaryValue components;
  LoadTestJson(kComponents, &components);
  EXPECT_CALL(cloud_, FindComponent(_, _)).WillRepeatedly(Return(nullptr));
  EXPECT_CALL(cloud_, GetComponents()).WillRepeatedly(ReturnRef(components));
  const char kExpected1[] = R"({
    "components": {
      "comp1": {
        "state": {
          "a" : {
            "prop": 1
          }
        }
      }
    },
    "fingerprint": "1"
  })";
  EXPECT_JSON_EQ(kExpected1, HandleRequest("/privet/v3/components",
                                           "{'filter':['state']}"));

  const char kExpected2[] = R"({
    "components": {
      "comp1": {
        "traits": ["a", "b"]
      }
    },
    "fingerprint": "1"
  })";
  EXPECT_JSON_EQ(kExpected2, HandleRequest("/privet/v3/components",
                                           "{'filter':['traits']}"));

  const char kExpected3[] = R"({
    "components": {
      "comp1": {
        "components": {
          "comp2": {
            "components": {
              "comp4": {}
            }
          },
          "comp3": {}
        }
      }
    },
    "fingerprint": "1"
  })";
  EXPECT_JSON_EQ(kExpected3, HandleRequest("/privet/v3/components",
                                           "{'filter':['components']}"));

  const char kExpected4[] = R"({
    "components": {
      "comp1": {
        "traits": ["a", "b"],
        "state": {
          "a" : {
            "prop": 1
          }
        },
        "components": {
          "comp2": {
            "traits": ["c"],
            "components": {
              "comp4": {
                "traits": ["d"]
              }
            }
          },
          "comp3": {
            "traits": ["e"]
          }
        }
      }
    },
    "fingerprint": "1"
  })";
  EXPECT_JSON_EQ(kExpected4,
                 HandleRequest("/privet/v3/components",
                               "{'filter':['traits', 'components', 'state']}"));

  const base::DictionaryValue* comp2 = nullptr;
  ASSERT_TRUE(components.GetDictionary("comp1.components.comp2", &comp2));
  EXPECT_CALL(cloud_, FindComponent("comp1.comp2", _)).WillOnce(Return(comp2));

  const char kExpected5[] = R"({
    "components": {
      "comp2": {
        "traits": ["c"],
        "components": {
          "comp4": {
            "traits": ["d"]
          }
        }
      }
    },
    "fingerprint": "1"
  })";
  EXPECT_JSON_EQ(
      kExpected5,
      HandleRequest(
          "/privet/v3/components",
          "{'path':'comp1.comp2', 'filter':['traits', 'components']}"));

  auto error_handler = [](ErrorPtr* error) -> const base::DictionaryValue* {
    return Error::AddTo(error, FROM_HERE, "componentNotFound", "");
  };
  EXPECT_CALL(cloud_, FindComponent("comp7", _))
      .WillOnce(WithArgs<1>(Invoke(error_handler)));

  EXPECT_PRED2(
      IsEqualError, CodeWithReason(500, "componentNotFound"),
      HandleRequest("/privet/v3/components",
                    "{'path':'comp7', 'filter':['traits', 'components']}"));
}

TEST_F(PrivetHandlerTestWithAuth, CommandsExecute) {
  const char kInput[] = "{'name': 'test'}";
  base::DictionaryValue command;
  LoadTestJson(kInput, &command);
  LoadTestJson("{'id':'5'}", &command);
  EXPECT_CALL(cloud_, AddCommand(_, _, _))
      .WillOnce(WithArgs<2>(Invoke(
          [&command](const CloudDelegate::CommandDoneCallback& callback) {
            callback.Run(command, nullptr);
          })));

  EXPECT_JSON_EQ("{'name':'test', 'id':'5'}",
                 HandleRequest("/privet/v3/commands/execute", kInput));
}

TEST_F(PrivetHandlerTestWithAuth, CommandsStatus) {
  const char kInput[] = "{'id': '5'}";
  base::DictionaryValue command;
  LoadTestJson(kInput, &command);
  LoadTestJson("{'name':'test'}", &command);
  EXPECT_CALL(cloud_, GetCommand(_, _, _))
      .WillOnce(WithArgs<2>(Invoke(
          [&command](const CloudDelegate::CommandDoneCallback& callback) {
            callback.Run(command, nullptr);
          })));

  EXPECT_JSON_EQ("{'name':'test', 'id':'5'}",
                 HandleRequest("/privet/v3/commands/status", kInput));

  ErrorPtr error;
  Error::AddTo(&error, FROM_HERE, "notFound", "");
  EXPECT_CALL(cloud_, GetCommand(_, _, _))
      .WillOnce(WithArgs<2>(
          Invoke([&error](const CloudDelegate::CommandDoneCallback& callback) {
            callback.Run({}, std::move(error));
          })));

  EXPECT_PRED2(IsEqualError, CodeWithReason(404, "notFound"),
               HandleRequest("/privet/v3/commands/status", "{'id': '15'}"));
}

TEST_F(PrivetHandlerTestWithAuth, CommandsCancel) {
  const char kExpected[] = "{'id': '5', 'name':'test', 'state':'cancelled'}";
  base::DictionaryValue command;
  LoadTestJson(kExpected, &command);
  EXPECT_CALL(cloud_, CancelCommand(_, _, _))
      .WillOnce(WithArgs<2>(Invoke(
          [&command](const CloudDelegate::CommandDoneCallback& callback) {
            callback.Run(command, nullptr);
          })));

  EXPECT_JSON_EQ(kExpected,
                 HandleRequest("/privet/v3/commands/cancel", "{'id': '8'}"));

  ErrorPtr error;
  Error::AddTo(&error, FROM_HERE, "notFound", "");
  EXPECT_CALL(cloud_, CancelCommand(_, _, _))
      .WillOnce(WithArgs<2>(
          Invoke([&error](const CloudDelegate::CommandDoneCallback& callback) {
            callback.Run({}, std::move(error));
          })));

  EXPECT_PRED2(IsEqualError, CodeWithReason(404, "notFound"),
               HandleRequest("/privet/v3/commands/cancel", "{'id': '11'}"));
}

TEST_F(PrivetHandlerTestWithAuth, CommandsList) {
  const char kExpected[] = R"({
    'commands' : [
        {'id':'5', 'state':'cancelled'},
        {'id':'15', 'state':'inProgress'}
     ]})";

  base::DictionaryValue commands;
  LoadTestJson(kExpected, &commands);

  EXPECT_CALL(cloud_, ListCommands(_, _))
      .WillOnce(WithArgs<1>(Invoke(
          [&commands](const CloudDelegate::CommandDoneCallback& callback) {
            callback.Run(commands, nullptr);
          })));

  EXPECT_JSON_EQ(kExpected, HandleRequest("/privet/v3/commands/list", "{}"));
}

class PrivetHandlerCheckForUpdatesTest : public PrivetHandlerTestWithAuth {};

TEST_F(PrivetHandlerCheckForUpdatesTest, NoInput) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  cloud_.NotifyOnTraitDefsChanged();
  cloud_.NotifyOnComponentTreeChanged();
  cloud_.NotifyOnStateChanged();
  const char kInput[] = "{}";
  const char kExpected[] = R"({
   'commandsFingerprint': '2',
   'stateFingerprint': '2',
   'traitsFingerprint': '2',
   'componentsFingerprint': '3'
  })";
  EXPECT_JSON_EQ(kExpected,
                 HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(1, GetResponseCount());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, AlreadyChanged) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  cloud_.NotifyOnTraitDefsChanged();
  cloud_.NotifyOnComponentTreeChanged();
  cloud_.NotifyOnStateChanged();
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  const char kExpected[] = R"({
   'commandsFingerprint': '2',
   'stateFingerprint': '2',
   'traitsFingerprint': '2',
   'componentsFingerprint': '3'
  })";
  EXPECT_JSON_EQ(kExpected,
                 HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(1, GetResponseCount());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, LongPollCommands) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(0, GetResponseCount());
  cloud_.NotifyOnTraitDefsChanged();
  EXPECT_EQ(1, GetResponseCount());
  const char kExpected[] = R"({
   'commandsFingerprint': '2',
   'stateFingerprint': '1',
   'traitsFingerprint': '2',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ(kExpected, GetResponse());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, LongPollTraits) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(0, GetResponseCount());
  cloud_.NotifyOnTraitDefsChanged();
  EXPECT_EQ(1, GetResponseCount());
  const char kExpected[] = R"({
   'commandsFingerprint': '2',
   'stateFingerprint': '1',
   'traitsFingerprint': '2',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ(kExpected, GetResponse());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, LongPollState) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(0, GetResponseCount());
  cloud_.NotifyOnStateChanged();
  EXPECT_EQ(1, GetResponseCount());
  const char kExpected[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '2',
   'traitsFingerprint': '1',
   'componentsFingerprint': '2'
  })";
  EXPECT_JSON_EQ(kExpected, GetResponse());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, LongPollComponents) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(0, GetResponseCount());
  cloud_.NotifyOnComponentTreeChanged();
  EXPECT_EQ(1, GetResponseCount());
  const char kExpected[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '2'
  })";
  EXPECT_JSON_EQ(kExpected, GetResponse());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, LongPollIgnoreTraits) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  const char kInput[] = R"({
   'stateFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(0, GetResponseCount());
  cloud_.NotifyOnTraitDefsChanged();
  EXPECT_EQ(0, GetResponseCount());
  cloud_.NotifyOnComponentTreeChanged();
  EXPECT_EQ(1, GetResponseCount());
  const char kExpected[] = R"({
   'commandsFingerprint': '2',
   'stateFingerprint': '1',
   'traitsFingerprint': '2',
   'componentsFingerprint': '2'
  })";
  EXPECT_JSON_EQ(kExpected, GetResponse());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, LongPollIgnoreState) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'traitsFingerprint': '1'
  })";
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(0, GetResponseCount());
  cloud_.NotifyOnStateChanged();
  EXPECT_EQ(0, GetResponseCount());
  cloud_.NotifyOnComponentTreeChanged();
  EXPECT_EQ(0, GetResponseCount());
  cloud_.NotifyOnTraitDefsChanged();
  EXPECT_EQ(1, GetResponseCount());
  const char kExpected[] = R"({
   'commandsFingerprint': '2',
   'stateFingerprint': '2',
   'traitsFingerprint': '2',
   'componentsFingerprint': '3'
  })";
  EXPECT_JSON_EQ(kExpected, GetResponse());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, InstantTimeout) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1',
   'waitTimeout': 0
  })";
  const char kExpected[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ(kExpected,
                 HandleRequest("/privet/v3/checkForUpdates", kInput));
}

TEST_F(PrivetHandlerCheckForUpdatesTest, UserTimeout) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1',
   'waitTimeout': 3
  })";
  base::Closure callback;
  EXPECT_CALL(device_, PostDelayedTask(_, _, base::TimeDelta::FromSeconds(3)))
      .WillOnce(SaveArg<1>(&callback));
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(0, GetResponseCount());
  callback.Run();
  EXPECT_EQ(1, GetResponseCount());
  const char kExpected[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ(kExpected, GetResponse());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, ServerTimeout) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::FromMinutes(1)));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  base::Closure callback;
  EXPECT_CALL(device_, PostDelayedTask(_, _, base::TimeDelta::FromSeconds(50)))
      .WillOnce(SaveArg<1>(&callback));
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(0, GetResponseCount());
  callback.Run();
  EXPECT_EQ(1, GetResponseCount());
  const char kExpected[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ(kExpected, GetResponse());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, VeryShortServerTimeout) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::FromSeconds(5)));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ(kInput, HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(1, GetResponseCount());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, ServerAndUserTimeout) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::FromMinutes(1)));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1',
   'waitTimeout': 10
  })";
  base::Closure callback;
  EXPECT_CALL(device_, PostDelayedTask(_, _, base::TimeDelta::FromSeconds(10)))
      .WillOnce(SaveArg<1>(&callback));
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(0, GetResponseCount());
  callback.Run();
  EXPECT_EQ(1, GetResponseCount());
  const char kExpected[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ(kExpected, GetResponse());
}

TEST_F(PrivetHandlerCheckForUpdatesTest, ChangeBeforeTimeout) {
  EXPECT_CALL(device_, GetHttpRequestTimeout())
      .WillOnce(Return(base::TimeDelta::Max()));
  const char kInput[] = R"({
   'commandsFingerprint': '1',
   'stateFingerprint': '1',
   'traitsFingerprint': '1',
   'componentsFingerprint': '1',
   'waitTimeout': 10
  })";
  base::Closure callback;
  EXPECT_CALL(device_, PostDelayedTask(_, _, base::TimeDelta::FromSeconds(10)))
      .WillOnce(SaveArg<1>(&callback));
  EXPECT_JSON_EQ("{}", HandleRequest("/privet/v3/checkForUpdates", kInput));
  EXPECT_EQ(0, GetResponseCount());
  cloud_.NotifyOnTraitDefsChanged();
  EXPECT_EQ(1, GetResponseCount());
  const char kExpected[] = R"({
   'commandsFingerprint': '2',
   'stateFingerprint': '1',
   'traitsFingerprint': '2',
   'componentsFingerprint': '1'
  })";
  EXPECT_JSON_EQ(kExpected, GetResponse());
  callback.Run();
  EXPECT_EQ(1, GetResponseCount());
}

}  // namespace privet
}  // namespace weave
