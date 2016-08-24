//
// Copyright (C) 2011 The Android Open Source Project
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

#include "update_engine/chrome_browser_proxy_resolver.h"

#include <deque>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <base/bind.h>
#include <brillo/make_unique_ptr.h>
#include <brillo/message_loops/fake_message_loop.h>

#include "libcros/dbus-proxies.h"
#include "libcros/dbus-proxy-mocks.h"
#include "update_engine/dbus_test_utils.h"

using ::testing::Return;
using ::testing::StrEq;
using ::testing::_;
using brillo::MessageLoop;
using org::chromium::LibCrosServiceInterfaceProxyMock;
using org::chromium::UpdateEngineLibcrosProxyResolvedInterfaceProxyMock;
using std::deque;
using std::string;
using std::vector;

namespace chromeos_update_engine {

class ChromeBrowserProxyResolverTest : public ::testing::Test {
 protected:
  ChromeBrowserProxyResolverTest()
      : service_interface_mock_(new LibCrosServiceInterfaceProxyMock()),
        ue_proxy_resolved_interface_mock_(
            new UpdateEngineLibcrosProxyResolvedInterfaceProxyMock()),
        libcros_proxy_(
            brillo::make_unique_ptr(service_interface_mock_),
            brillo::make_unique_ptr(ue_proxy_resolved_interface_mock_)) {}

  void SetUp() override {
    loop_.SetAsCurrent();
    // The ProxyResolved signal should be subscribed to.
    MOCK_SIGNAL_HANDLER_EXPECT_SIGNAL_HANDLER(
        ue_proxy_resolved_signal_,
        *ue_proxy_resolved_interface_mock_,
        ProxyResolved);

    EXPECT_TRUE(resolver_.Init());
    // Run the loop once to dispatch the successfully registered signal handler.
    EXPECT_TRUE(loop_.RunOnce(false));
  }

  void TearDown() override {
    EXPECT_FALSE(loop_.PendingTasks());
  }

  // Send the signal to the callback passed during registration of the
  // ProxyResolved.
  void SendReplySignal(const string& source_url,
                       const string& proxy_info,
                       const string& error_message);

  void RunTest(bool chrome_replies, bool chrome_alive);

 private:
  brillo::FakeMessageLoop loop_{nullptr};

  // Local pointers to the mocks. The instances are owned by the
  // |libcros_proxy_|.
  LibCrosServiceInterfaceProxyMock* service_interface_mock_;
  UpdateEngineLibcrosProxyResolvedInterfaceProxyMock*
      ue_proxy_resolved_interface_mock_;

  // The registered signal handler for the signal
  // UpdateEngineLibcrosProxyResolvedInterface.ProxyResolved.
  chromeos_update_engine::dbus_test_utils::MockSignalHandler<
      void(const string&, const string&, const string&)>
      ue_proxy_resolved_signal_;

  LibCrosProxy libcros_proxy_;
  ChromeBrowserProxyResolver resolver_{&libcros_proxy_};
};


void ChromeBrowserProxyResolverTest::SendReplySignal(
    const string& source_url,
    const string& proxy_info,
    const string& error_message) {
  ASSERT_TRUE(ue_proxy_resolved_signal_.IsHandlerRegistered());
  ue_proxy_resolved_signal_.signal_callback().Run(
      source_url, proxy_info, error_message);
}

namespace {
void CheckResponseResolved(const deque<string>& proxies,
                           void* /* pirv_data */) {
  EXPECT_EQ(2U, proxies.size());
  EXPECT_EQ("socks5://192.168.52.83:5555", proxies[0]);
  EXPECT_EQ(kNoProxy, proxies[1]);
  MessageLoop::current()->BreakLoop();
}

void CheckResponseNoReply(const deque<string>& proxies, void* /* pirv_data */) {
  EXPECT_EQ(1U, proxies.size());
  EXPECT_EQ(kNoProxy, proxies[0]);
  MessageLoop::current()->BreakLoop();
}
}  // namespace

// chrome_replies should be set to whether or not we fake a reply from
// chrome. If there's no reply, the resolver should time out.
// If chrome_alive is false, assume that sending to chrome fails.
void ChromeBrowserProxyResolverTest::RunTest(bool chrome_replies,
                                             bool chrome_alive) {
  char kUrl[] = "http://example.com/blah";
  char kProxyConfig[] = "SOCKS5 192.168.52.83:5555;DIRECT";

  EXPECT_CALL(*service_interface_mock_,
              ResolveNetworkProxy(StrEq(kUrl),
                                  StrEq(kLibCrosProxyResolveSignalInterface),
                                  StrEq(kLibCrosProxyResolveName),
                                  _,
                                  _))
      .WillOnce(Return(chrome_alive));

  ProxiesResolvedFn get_proxies_response = &CheckResponseNoReply;
  if (chrome_replies) {
    get_proxies_response = &CheckResponseResolved;
    MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&ChromeBrowserProxyResolverTest::SendReplySignal,
                   base::Unretained(this),
                   kUrl,
                   kProxyConfig,
                   ""),
        base::TimeDelta::FromSeconds(1));
  }

  EXPECT_TRUE(resolver_.GetProxiesForUrl(kUrl, get_proxies_response, nullptr));
  MessageLoop::current()->Run();
}


TEST_F(ChromeBrowserProxyResolverTest, ParseTest) {
  // Test ideas from
  // http://src.chromium.org/svn/trunk/src/net/proxy/proxy_list_unittest.cc
  vector<string> inputs = {
      "PROXY foopy:10",
      " DIRECT",  // leading space.
      "PROXY foopy1 ; proxy foopy2;\t DIRECT",
      "proxy foopy1 ; SOCKS foopy2",
      "DIRECT ; proxy foopy1 ; DIRECT ; SOCKS5 foopy2;DIRECT ",
      "DIRECT ; proxy foopy1:80; DIRECT ; DIRECT",
      "PROXY-foopy:10",
      "PROXY",
      "PROXY foopy1 ; JUNK ; JUNK ; SOCKS5 foopy2 ; ;",
      "HTTP foopy1; SOCKS5 foopy2"};
  vector<deque<string>> outputs = {
      {"http://foopy:10", kNoProxy},
      {kNoProxy},
      {"http://foopy1", "http://foopy2", kNoProxy},
      {"http://foopy1", "socks4://foopy2", kNoProxy},
      {kNoProxy, "http://foopy1", kNoProxy, "socks5://foopy2", kNoProxy},
      {kNoProxy, "http://foopy1:80", kNoProxy, kNoProxy},
      {kNoProxy},
      {kNoProxy},
      {"http://foopy1", "socks5://foopy2", kNoProxy},
      {"socks5://foopy2", kNoProxy}};
  ASSERT_EQ(inputs.size(), outputs.size());

  for (size_t i = 0; i < inputs.size(); i++) {
    deque<string> results =
        ChromeBrowserProxyResolver::ParseProxyString(inputs[i]);
    deque<string>& expected = outputs[i];
    EXPECT_EQ(results.size(), expected.size()) << "i = " << i;
    if (expected.size() != results.size())
      continue;
    for (size_t j = 0; j < expected.size(); j++) {
      EXPECT_EQ(expected[j], results[j]) << "i = " << i;
    }
  }
}

TEST_F(ChromeBrowserProxyResolverTest, SuccessTest) {
  RunTest(true, true);
}

TEST_F(ChromeBrowserProxyResolverTest, NoReplyTest) {
  RunTest(false, true);
}

TEST_F(ChromeBrowserProxyResolverTest, NoChromeTest) {
  RunTest(false, false);
}

}  // namespace chromeos_update_engine
