// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/proxy_generator.h"

#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gtest/gtest.h>

#include "chromeos-dbus-bindings/interface.h"
#include "chromeos-dbus-bindings/test_utils.h"

using std::string;
using std::vector;
using testing::Test;

namespace chromeos_dbus_bindings {

namespace {

const char kDBusTypeArryOfObjects[] = "ao";
const char kDBusTypeArryOfStrings[] = "as";
const char kDBusTypeBool[] = "b";
const char kDBusTypeByte[] = "y";
const char kDBusTypeInt32[] = "i";
const char kDBusTypeInt64[] = "x";
const char kDBusTypeString[] = "s";

const char kExpectedContent[] = R"literal_string(
#include <string>
#include <vector>

#include <base/callback_forward.h>
#include <base/logging.h>
#include <base/macros.h>
#include <brillo/any.h>
#include <brillo/errors/error.h>
#include <brillo/variant_dictionary.h>
#include <gmock/gmock.h>

#include "proxies.h"

namespace org {
namespace chromium {

// Mock object for TestInterfaceProxyInterface.
class TestInterfaceProxyMock : public TestInterfaceProxyInterface {
 public:
  TestInterfaceProxyMock() = default;

  MOCK_METHOD5(Elements,
               bool(const std::string& /*in_space_walk*/,
                    const std::vector<dbus::ObjectPath>& /*in_ramblin_man*/,
                    std::string*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD5(ElementsAsync,
               void(const std::string& /*in_space_walk*/,
                    const std::vector<dbus::ObjectPath>& /*in_ramblin_man*/,
                    const base::Callback<void(const std::string&)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(ReturnToPatagonia,
               bool(int64_t*,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(ReturnToPatagoniaAsync,
               void(const base::Callback<void(int64_t)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(NiceWeatherForDucks,
               bool(bool,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD4(NiceWeatherForDucksAsync,
               void(bool,
                    const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_METHOD2(ExperimentNumberSix,
               bool(brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(ExperimentNumberSixAsync,
               void(const base::Callback<void()>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  bool AllTheWayUpToEleven(bool /*in_arg1*/,
                           bool /*in_arg2*/,
                           bool /*in_arg3*/,
                           bool /*in_arg4*/,
                           bool /*in_arg5*/,
                           bool /*in_arg6*/,
                           bool /*in_arg7*/,
                           bool /*in_arg8*/,
                           bool* /*out_arg9*/,
                           brillo::ErrorPtr* /*error*/,
                           int /*timeout_ms*/) override {
    LOG(WARNING) << "AllTheWayUpToEleven(): gmock can't handle methods with 11 arguments. You can override this method in a subclass if you need to.";
    return false;
  }
  void AllTheWayUpToElevenAsync(bool /*in_arg1*/,
                                bool /*in_arg2*/,
                                bool /*in_arg3*/,
                                bool /*in_arg4*/,
                                bool /*in_arg5*/,
                                bool /*in_arg6*/,
                                bool /*in_arg7*/,
                                bool /*in_arg8*/,
                                const base::Callback<void(bool /*arg9*/)>& /*success_callback*/,
                                const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                                int /*timeout_ms*/) override {
    LOG(WARNING) << "AllTheWayUpToElevenAsync(): gmock can't handle methods with 11 arguments. You can override this method in a subclass if you need to.";
  }
  MOCK_METHOD2(RegisterCloserSignalHandler,
               void(const base::Closure& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_METHOD2(RegisterTheCurseOfKaZarSignalHandler,
               void(const base::Callback<void(const std::vector<std::string>&,
                                              uint8_t)>& /*signal_callback*/,
                    dbus::ObjectProxy::OnConnectedCallback /*on_connected_callback*/));
  MOCK_CONST_METHOD0(GetObjectPath, const dbus::ObjectPath&());

 private:
  DISALLOW_COPY_AND_ASSIGN(TestInterfaceProxyMock);
};
}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Mock object for TestInterface2ProxyInterface.
class TestInterface2ProxyMock : public TestInterface2ProxyInterface {
 public:
  TestInterface2ProxyMock() = default;

  MOCK_METHOD4(GetPersonInfo,
               bool(std::string* /*out_name*/,
                    int32_t* /*out_age*/,
                    brillo::ErrorPtr* /*error*/,
                    int /*timeout_ms*/));
  MOCK_METHOD3(GetPersonInfoAsync,
               void(const base::Callback<void(const std::string& /*name*/, int32_t /*age*/)>& /*success_callback*/,
                    const base::Callback<void(brillo::Error*)>& /*error_callback*/,
                    int /*timeout_ms*/));
  MOCK_CONST_METHOD0(data, const std::string&());
  MOCK_CONST_METHOD0(name, const std::string&());
  MOCK_METHOD2(set_name, void(const std::string&, const base::Callback<bool>&));
  MOCK_CONST_METHOD0(GetObjectPath, const dbus::ObjectPath&());
  MOCK_METHOD1(SetPropertyChangedCallback,
               void(const base::Callback<void(TestInterface2ProxyInterface*, const std::string&)>&));

 private:
  DISALLOW_COPY_AND_ASSIGN(TestInterface2ProxyMock);
};
}  // namespace chromium
}  // namespace org
)literal_string";

}  // namespace

class ProxyGeneratorMockTest : public Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

 protected:
  base::FilePath CreateInputFile(const string& contents) {
    base::FilePath path;
    EXPECT_TRUE(base::CreateTemporaryFileInDir(temp_dir_.path(), &path));
    int written = base::WriteFile(path, contents.c_str(), contents.size());
    EXPECT_EQ(contents.size(), static_cast<size_t>(written));
    return path;
  }

  base::ScopedTempDir temp_dir_;
};

TEST_F(ProxyGeneratorMockTest, GenerateMocks) {
  Interface interface;
  interface.name = "org.chromium.TestInterface";
  interface.path = "/org/chromium/Test";
  interface.methods.emplace_back(
      "Elements",
      vector<Interface::Argument>{
          {"space_walk", kDBusTypeString},
          {"ramblin_man", kDBusTypeArryOfObjects}},
      vector<Interface::Argument>{{"", kDBusTypeString}});
  interface.methods.emplace_back(
      "ReturnToPatagonia",
      vector<Interface::Argument>{},
      vector<Interface::Argument>{{"", kDBusTypeInt64}});
  interface.methods.emplace_back(
      "NiceWeatherForDucks",
      vector<Interface::Argument>{{"", kDBusTypeBool}},
      vector<Interface::Argument>{});
  interface.methods.emplace_back("ExperimentNumberSix");
  // gmock can't handle more than 10 args. The generated method will also
  // include the timeout and error arguments in the synchronous case, and two
  // callbacks and the timeout in the asynchronous case.
  interface.methods.emplace_back(
      "AllTheWayUpToEleven",
      vector<Interface::Argument>{
          {"arg1", kDBusTypeBool},
          {"arg2", kDBusTypeBool},
          {"arg3", kDBusTypeBool},
          {"arg4", kDBusTypeBool},
          {"arg5", kDBusTypeBool},
          {"arg6", kDBusTypeBool},
          {"arg7", kDBusTypeBool},
          {"arg8", kDBusTypeBool}},
      vector<Interface::Argument>{{"arg9", kDBusTypeBool}});
  interface.signals.emplace_back("Closer");
  interface.signals.emplace_back(
      "TheCurseOfKaZar",
      vector<Interface::Argument>{
          {"", kDBusTypeArryOfStrings},
          {"", kDBusTypeByte}});
  interface.methods.back().doc_string = "Comment line1\nline2";
  Interface interface2;
  interface2.name = "org.chromium.TestInterface2";
  interface2.methods.emplace_back(
      "GetPersonInfo",
      vector<Interface::Argument>{},
      vector<Interface::Argument>{
          {"name", kDBusTypeString},
          {"age", kDBusTypeInt32}});
  interface2.properties.emplace_back("Data", "s", "read");
  interface2.properties.emplace_back("Name", "s", "readwrite");
  vector<Interface> interfaces{interface, interface2};
  base::FilePath output_path = temp_dir_.path().Append("output.h");
  base::FilePath proxy_path = temp_dir_.path().Append("proxies.h");
  ServiceConfig config;
  config.object_manager.name = "ObjectManager";
  EXPECT_TRUE(ProxyGenerator::GenerateMocks(config, interfaces, output_path,
                                            proxy_path, false));
  string contents;
  EXPECT_TRUE(base::ReadFileToString(output_path, &contents));
  // The header guards contain the (temporary) filename, so we search for
  // the content we need within the string.
  test_utils::EXPECT_TEXT_CONTAINED(kExpectedContent, contents);
}

}  // namespace chromeos_dbus_bindings
