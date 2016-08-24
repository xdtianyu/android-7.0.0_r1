// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/notification/xmpp_channel.h"

#include <algorithm>
#include <queue>

#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>
#include <weave/provider/test/mock_network.h>
#include <weave/test/fake_stream.h>

#include "src/bind_lambda.h"

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;
using testing::WithArgs;

namespace weave {

namespace {

constexpr char kAccountName[] = "Account@Name";
constexpr char kAccessToken[] = "AccessToken";
constexpr char kEndpoint[] = "endpoint:456";

constexpr char kStartStreamMessage[] =
    "<stream:stream to='clouddevices.gserviceaccount.com' "
    "xmlns:stream='http://etherx.jabber.org/streams' xml:lang='*' "
    "version='1.0' xmlns='jabber:client'>";
constexpr char kStartStreamResponse[] =
    "<stream:stream from=\"clouddevices.gserviceaccount.com\" "
    "id=\"0CCF520913ABA04B\" version=\"1.0\" "
    "xmlns:stream=\"http://etherx.jabber.org/streams\" "
    "xmlns=\"jabber:client\">";
constexpr char kAuthenticationMessage[] =
    "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='X-OAUTH2' "
    "auth:service='oauth2' auth:allow-non-google-login='true' "
    "auth:client-uses-full-bind-result='true' "
    "xmlns:auth='http://www.google.com/talk/protocol/auth'>"
    "AEFjY291bnRATmFtZQBBY2Nlc3NUb2tlbg==</auth>";
constexpr char kConnectedResponse[] =
    "<stream:features><mechanisms xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\">"
    "<mechanism>X-OAUTH2</mechanism>"
    "<mechanism>X-GOOGLE-TOKEN</mechanism></mechanisms></stream:features>";
constexpr char kAuthenticationSucceededResponse[] =
    "<success xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"/>";
constexpr char kAuthenticationFailedResponse[] =
    "<failure xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\"><not-authorized/>"
    "</failure>";
constexpr char kRestartStreamResponse[] =
    "<stream:features><bind xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\"/>"
    "<session xmlns=\"urn:ietf:params:xml:ns:xmpp-session\"/>"
    "</stream:features>";
constexpr char kBindResponse[] =
    "<iq id=\"1\" type=\"result\">"
    "<bind xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\">"
    "<jid>110cc78f78d7032cc7bf2c6e14c1fa7d@clouddevices.gserviceaccount.com"
    "/19853128</jid></bind></iq>";
constexpr char kSessionResponse[] = "<iq type=\"result\" id=\"2\"/>";
constexpr char kSubscribedResponse[] =
    "<iq to=\""
    "110cc78f78d7032cc7bf2c6e14c1fa7d@clouddevices.gserviceaccount.com/"
    "19853128\" from=\""
    "110cc78f78d7032cc7bf2c6e14c1fa7d@clouddevices.gserviceaccount.com\" "
    "id=\"3\" type=\"result\"/>";
constexpr char kBindMessage[] =
    "<iq id='1' type='set'><bind "
    "xmlns='urn:ietf:params:xml:ns:xmpp-bind'/></iq>";
constexpr char kSessionMessage[] =
    "<iq id='2' type='set'><session "
    "xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>";
constexpr char kSubscribeMessage[] =
    "<iq id='3' type='set' to='Account@Name'>"
    "<subscribe xmlns='google:push'><item channel='cloud_devices' from=''/>"
    "</subscribe></iq>";

}  // namespace

class FakeXmppChannel : public XmppChannel {
 public:
  explicit FakeXmppChannel(provider::TaskRunner* task_runner,
                           provider::Network* network)
      : XmppChannel{kAccountName, kAccessToken, kEndpoint, task_runner,
                    network},
        stream_{new test::FakeStream{task_runner_}},
        fake_stream_{stream_.get()} {}

  void Connect(const base::Callback<void(std::unique_ptr<Stream>,
                                         ErrorPtr error)>& callback) {
    callback.Run(std::move(stream_), nullptr);
  }

  XmppState state() const { return state_; }
  void set_state(XmppState state) { state_ = state; }

  void SchedulePing(base::TimeDelta interval,
                    base::TimeDelta timeout) override {}

  void ExpectWritePacketString(base::TimeDelta delta, const std::string& data) {
    fake_stream_->ExpectWritePacketString(delta, data);
  }

  void AddReadPacketString(base::TimeDelta delta, const std::string& data) {
    fake_stream_->AddReadPacketString(delta, data);
  }

  std::unique_ptr<test::FakeStream> stream_;
  test::FakeStream* fake_stream_{nullptr};
};

class MockNetwork : public provider::test::MockNetwork {
 public:
  MockNetwork() {
    EXPECT_CALL(*this, AddConnectionChangedCallback(_))
        .WillRepeatedly(Return());
  }
};

class XmppChannelTest : public ::testing::Test {
 protected:
  XmppChannelTest() {
    EXPECT_CALL(network_, OpenSslSocket("endpoint", 456, _))
        .WillOnce(
            WithArgs<2>(Invoke(&xmpp_client_, &FakeXmppChannel::Connect)));
  }

  void StartStream() {
    xmpp_client_.ExpectWritePacketString({}, kStartStreamMessage);
    xmpp_client_.AddReadPacketString({}, kStartStreamResponse);
    xmpp_client_.Start(nullptr);
    RunUntil(XmppChannel::XmppState::kConnected);
  }

  void StartWithState(XmppChannel::XmppState state) {
    StartStream();
    xmpp_client_.set_state(state);
  }

  void RunUntil(XmppChannel::XmppState st) {
    for (size_t n = 15; n && xmpp_client_.state() != st; --n)
      task_runner_.RunOnce();
    EXPECT_EQ(st, xmpp_client_.state());
  }

  StrictMock<provider::test::FakeTaskRunner> task_runner_;
  StrictMock<MockNetwork> network_;
  FakeXmppChannel xmpp_client_{&task_runner_, &network_};
};

TEST_F(XmppChannelTest, StartStream) {
  EXPECT_EQ(XmppChannel::XmppState::kNotStarted, xmpp_client_.state());
  xmpp_client_.ExpectWritePacketString({}, kStartStreamMessage);
  xmpp_client_.Start(nullptr);
  RunUntil(XmppChannel::XmppState::kConnected);
}

TEST_F(XmppChannelTest, HandleStartedResponse) {
  StartStream();
}

TEST_F(XmppChannelTest, HandleConnected) {
  StartWithState(XmppChannel::XmppState::kConnected);
  xmpp_client_.AddReadPacketString({}, kConnectedResponse);
  xmpp_client_.ExpectWritePacketString({}, kAuthenticationMessage);
  RunUntil(XmppChannel::XmppState::kAuthenticationStarted);
}

TEST_F(XmppChannelTest, HandleAuthenticationSucceededResponse) {
  StartWithState(XmppChannel::XmppState::kAuthenticationStarted);
  xmpp_client_.AddReadPacketString({}, kAuthenticationSucceededResponse);
  xmpp_client_.ExpectWritePacketString({}, kStartStreamMessage);
  RunUntil(XmppChannel::XmppState::kStreamRestartedPostAuthentication);
}

TEST_F(XmppChannelTest, HandleAuthenticationFailedResponse) {
  StartWithState(XmppChannel::XmppState::kAuthenticationStarted);
  xmpp_client_.AddReadPacketString({}, kAuthenticationFailedResponse);
  RunUntil(XmppChannel::XmppState::kAuthenticationFailed);
}

TEST_F(XmppChannelTest, HandleStreamRestartedResponse) {
  StartWithState(XmppChannel::XmppState::kStreamRestartedPostAuthentication);
  xmpp_client_.AddReadPacketString({}, kRestartStreamResponse);
  xmpp_client_.ExpectWritePacketString({}, kBindMessage);
  RunUntil(XmppChannel::XmppState::kBindSent);
  EXPECT_TRUE(xmpp_client_.jid().empty());

  xmpp_client_.AddReadPacketString({}, kBindResponse);
  xmpp_client_.ExpectWritePacketString({}, kSessionMessage);
  RunUntil(XmppChannel::XmppState::kSessionStarted);
  EXPECT_EQ(
      "110cc78f78d7032cc7bf2c6e14c1fa7d@clouddevices.gserviceaccount.com"
      "/19853128",
      xmpp_client_.jid());

  xmpp_client_.AddReadPacketString({}, kSessionResponse);
  xmpp_client_.ExpectWritePacketString({}, kSubscribeMessage);
  RunUntil(XmppChannel::XmppState::kSubscribeStarted);

  xmpp_client_.AddReadPacketString({}, kSubscribedResponse);
  RunUntil(XmppChannel::XmppState::kSubscribed);
}

}  // namespace weave
