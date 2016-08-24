//
// Copyright (C) 2012 The Android Open Source Project
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

#include "shill/net/rtnl_handler.h"

#include <string>

#include <gtest/gtest.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/netlink.h>  // Needs typedefs from sys/socket.h.
#include <linux/rtnetlink.h>
#include <sys/ioctl.h>

#include <base/bind.h>

#include "shill/mock_log.h"
#include "shill/net/mock_io_handler_factory.h"
#include "shill/net/mock_sockets.h"
#include "shill/net/rtnl_message.h"

using base::Bind;
using base::Callback;
using base::Unretained;
using std::string;
using testing::_;
using testing::A;
using testing::DoAll;
using testing::ElementsAre;
using testing::HasSubstr;
using testing::Return;
using testing::ReturnArg;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {

const int kTestInterfaceIndex = 4;

ACTION(SetInterfaceIndex) {
  if (arg2) {
    reinterpret_cast<struct ifreq*>(arg2)->ifr_ifindex = kTestInterfaceIndex;
  }
}

MATCHER_P(MessageType, message_type, "") {
  return std::get<0>(arg).type() == message_type;
}

}  // namespace

class RTNLHandlerTest : public Test {
 public:
  RTNLHandlerTest()
      : sockets_(new StrictMock<MockSockets>()),
        callback_(Bind(&RTNLHandlerTest::HandlerCallback, Unretained(this))),
        dummy_message_(RTNLMessage::kTypeLink,
                       RTNLMessage::kModeGet,
                       0,
                       0,
                       0,
                       0,
                       IPAddress::kFamilyUnknown) {
  }

  virtual void SetUp() {
    RTNLHandler::GetInstance()->io_handler_factory_ = &io_handler_factory_;
    RTNLHandler::GetInstance()->sockets_.reset(sockets_);
  }

  virtual void TearDown() {
    RTNLHandler::GetInstance()->Stop();
  }

  uint32_t GetRequestSequence() {
    return RTNLHandler::GetInstance()->request_sequence_;
  }

  void SetRequestSequence(uint32_t sequence) {
    RTNLHandler::GetInstance()->request_sequence_ = sequence;
  }

  bool IsSequenceInErrorMaskWindow(uint32_t sequence) {
    return RTNLHandler::GetInstance()->IsSequenceInErrorMaskWindow(sequence);
  }

  void SetErrorMask(uint32_t sequence,
                    const RTNLHandler::ErrorMask& error_mask) {
    return RTNLHandler::GetInstance()->SetErrorMask(sequence, error_mask);
  }

  RTNLHandler::ErrorMask GetAndClearErrorMask(uint32_t sequence) {
    return RTNLHandler::GetInstance()->GetAndClearErrorMask(sequence);
  }

  int GetErrorWindowSize() {
    return  RTNLHandler::kErrorWindowSize;
  }

  MOCK_METHOD1(HandlerCallback, void(const RTNLMessage&));

 protected:
  static const int kTestSocket;
  static const int kTestDeviceIndex;
  static const char kTestDeviceName[];

  void AddLink();
  void AddNeighbor();
  void StartRTNLHandler();
  void StopRTNLHandler();
  void ReturnError(uint32_t sequence, int error_number);

  MockSockets* sockets_;
  StrictMock<MockIOHandlerFactory> io_handler_factory_;
  Callback<void(const RTNLMessage&)> callback_;
  RTNLMessage dummy_message_;
};

const int RTNLHandlerTest::kTestSocket = 123;
const int RTNLHandlerTest::kTestDeviceIndex = 123456;
const char RTNLHandlerTest::kTestDeviceName[] = "test-device";

void RTNLHandlerTest::StartRTNLHandler() {
  EXPECT_CALL(*sockets_, Socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE))
      .WillOnce(Return(kTestSocket));
  EXPECT_CALL(*sockets_, Bind(kTestSocket, _, sizeof(sockaddr_nl)))
      .WillOnce(Return(0));
  EXPECT_CALL(*sockets_, SetReceiveBuffer(kTestSocket, _)).WillOnce(Return(0));
  EXPECT_CALL(io_handler_factory_, CreateIOInputHandler(kTestSocket, _, _));
  RTNLHandler::GetInstance()->Start(0);
}

void RTNLHandlerTest::StopRTNLHandler() {
  EXPECT_CALL(*sockets_, Close(kTestSocket)).WillOnce(Return(0));
  RTNLHandler::GetInstance()->Stop();
}

void RTNLHandlerTest::AddLink() {
  RTNLMessage message(RTNLMessage::kTypeLink,
                      RTNLMessage::kModeAdd,
                      0,
                      0,
                      0,
                      kTestDeviceIndex,
                      IPAddress::kFamilyIPv4);
  message.SetAttribute(static_cast<uint16_t>(IFLA_IFNAME),
                       ByteString(string(kTestDeviceName), true));
  ByteString b(message.Encode());
  InputData data(b.GetData(), b.GetLength());
  RTNLHandler::GetInstance()->ParseRTNL(&data);
}

void RTNLHandlerTest::AddNeighbor() {
  RTNLMessage message(RTNLMessage::kTypeNeighbor,
                      RTNLMessage::kModeAdd,
                      0,
                      0,
                      0,
                      kTestDeviceIndex,
                      IPAddress::kFamilyIPv4);
  ByteString encoded(message.Encode());
  InputData data(encoded.GetData(), encoded.GetLength());
  RTNLHandler::GetInstance()->ParseRTNL(&data);
}

void RTNLHandlerTest::ReturnError(uint32_t sequence, int error_number) {
  struct {
    struct nlmsghdr hdr;
    struct nlmsgerr err;
  } errmsg;

  memset(&errmsg, 0, sizeof(errmsg));
  errmsg.hdr.nlmsg_type = NLMSG_ERROR;
  errmsg.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(errmsg.err));
  errmsg.hdr.nlmsg_seq = sequence;
  errmsg.err.error = -error_number;

  InputData data(reinterpret_cast<unsigned char*>(&errmsg), sizeof(errmsg));
  RTNLHandler::GetInstance()->ParseRTNL(&data);
}

TEST_F(RTNLHandlerTest, ListenersInvoked) {
  StartRTNLHandler();

  std::unique_ptr<RTNLListener> link_listener(
      new RTNLListener(RTNLHandler::kRequestLink, callback_));
  std::unique_ptr<RTNLListener> neighbor_listener(
      new RTNLListener(RTNLHandler::kRequestNeighbor, callback_));

  EXPECT_CALL(*this, HandlerCallback(A<const RTNLMessage&>()))
      .With(MessageType(RTNLMessage::kTypeLink));
  EXPECT_CALL(*this, HandlerCallback(A<const RTNLMessage&>()))
      .With(MessageType(RTNLMessage::kTypeNeighbor));

  AddLink();
  AddNeighbor();

  StopRTNLHandler();
}

TEST_F(RTNLHandlerTest, GetInterfaceName) {
  EXPECT_EQ(-1, RTNLHandler::GetInstance()->GetInterfaceIndex(""));
  {
    struct ifreq ifr;
    string name(sizeof(ifr.ifr_name), 'x');
    EXPECT_EQ(-1, RTNLHandler::GetInstance()->GetInterfaceIndex(name));
  }

  const int kTestSocket = 123;
  EXPECT_CALL(*sockets_, Socket(PF_INET, SOCK_DGRAM, 0))
      .Times(3)
      .WillOnce(Return(-1))
      .WillRepeatedly(Return(kTestSocket));
  EXPECT_CALL(*sockets_, Ioctl(kTestSocket, SIOCGIFINDEX, _))
      .WillOnce(Return(-1))
      .WillOnce(DoAll(SetInterfaceIndex(), Return(0)));
  EXPECT_CALL(*sockets_, Close(kTestSocket))
      .Times(2)
      .WillRepeatedly(Return(0));
  EXPECT_EQ(-1, RTNLHandler::GetInstance()->GetInterfaceIndex("eth0"));
  EXPECT_EQ(-1, RTNLHandler::GetInstance()->GetInterfaceIndex("wlan0"));
  EXPECT_EQ(kTestInterfaceIndex,
            RTNLHandler::GetInstance()->GetInterfaceIndex("usb0"));
}

TEST_F(RTNLHandlerTest, IsSequenceInErrorMaskWindow) {
  const uint32_t kRequestSequence = 1234;
  SetRequestSequence(kRequestSequence);
  EXPECT_FALSE(IsSequenceInErrorMaskWindow(kRequestSequence + 1));
  EXPECT_TRUE(IsSequenceInErrorMaskWindow(kRequestSequence));
  EXPECT_TRUE(IsSequenceInErrorMaskWindow(kRequestSequence - 1));
  EXPECT_TRUE(IsSequenceInErrorMaskWindow(kRequestSequence -
                                          GetErrorWindowSize() + 1));
  EXPECT_FALSE(IsSequenceInErrorMaskWindow(kRequestSequence -
                                           GetErrorWindowSize()));
  EXPECT_FALSE(IsSequenceInErrorMaskWindow(kRequestSequence -
                                           GetErrorWindowSize() - 1));
}

TEST_F(RTNLHandlerTest, SendMessageReturnsErrorAndAdvancesSequenceNumber) {
  StartRTNLHandler();
  const uint32_t kSequenceNumber = 123;
  SetRequestSequence(kSequenceNumber);
  EXPECT_CALL(*sockets_, Send(kTestSocket, _, _, 0)).WillOnce(Return(-1));
  EXPECT_FALSE(RTNLHandler::GetInstance()->SendMessage(&dummy_message_));

  // Sequence number should still increment even if there was a failure.
  EXPECT_EQ(kSequenceNumber + 1, GetRequestSequence());
  StopRTNLHandler();
}

TEST_F(RTNLHandlerTest, SendMessageWithEmptyMask) {
  StartRTNLHandler();
  const uint32_t kSequenceNumber = 123;
  SetRequestSequence(kSequenceNumber);
  SetErrorMask(kSequenceNumber, {1, 2, 3});
  EXPECT_CALL(*sockets_, Send(kTestSocket, _, _, 0)).WillOnce(ReturnArg<2>());
  EXPECT_TRUE(RTNLHandler::GetInstance()->SendMessageWithErrorMask(
      &dummy_message_, {}));
  EXPECT_EQ(kSequenceNumber + 1, GetRequestSequence());
  EXPECT_TRUE(GetAndClearErrorMask(kSequenceNumber).empty());
  StopRTNLHandler();
}

TEST_F(RTNLHandlerTest, SendMessageWithErrorMask) {
  StartRTNLHandler();
  const uint32_t kSequenceNumber = 123;
  SetRequestSequence(kSequenceNumber);
  EXPECT_CALL(*sockets_, Send(kTestSocket, _, _, 0)).WillOnce(ReturnArg<2>());
  EXPECT_TRUE(RTNLHandler::GetInstance()->SendMessageWithErrorMask(
      &dummy_message_, {1, 2, 3}));
  EXPECT_EQ(kSequenceNumber + 1, GetRequestSequence());
  EXPECT_TRUE(GetAndClearErrorMask(kSequenceNumber + 1).empty());
  EXPECT_THAT(GetAndClearErrorMask(kSequenceNumber), ElementsAre(1, 2, 3));

  // A second call to GetAndClearErrorMask() returns an empty vector.
  EXPECT_TRUE(GetAndClearErrorMask(kSequenceNumber).empty());
  StopRTNLHandler();
}

TEST_F(RTNLHandlerTest, SendMessageInferredErrorMasks) {
  struct {
    RTNLMessage::Type type;
    RTNLMessage::Mode mode;
    RTNLHandler::ErrorMask mask;
  } expectations[] = {
    { RTNLMessage::kTypeLink, RTNLMessage::kModeGet, {} },
    { RTNLMessage::kTypeLink, RTNLMessage::kModeAdd, {EEXIST} },
    { RTNLMessage::kTypeLink, RTNLMessage::kModeDelete, {ESRCH, ENODEV} },
    { RTNLMessage::kTypeAddress, RTNLMessage::kModeDelete,
         {ESRCH, ENODEV, EADDRNOTAVAIL} }
  };
  const uint32_t kSequenceNumber = 123;
  EXPECT_CALL(*sockets_, Send(_, _, _, 0)).WillRepeatedly(ReturnArg<2>());
  for (const auto& expectation : expectations) {
    SetRequestSequence(kSequenceNumber);
    RTNLMessage message(expectation.type,
                        expectation.mode,
                        0,
                        0,
                        0,
                        0,
                        IPAddress::kFamilyUnknown);
    EXPECT_TRUE(RTNLHandler::GetInstance()->SendMessage(&message));
    EXPECT_EQ(expectation.mask, GetAndClearErrorMask(kSequenceNumber));
  }
}

TEST_F(RTNLHandlerTest, MaskedError) {
  StartRTNLHandler();
  const uint32_t kSequenceNumber = 123;
  SetRequestSequence(kSequenceNumber);
  EXPECT_CALL(*sockets_, Send(kTestSocket, _, _, 0)).WillOnce(ReturnArg<2>());
  EXPECT_TRUE(RTNLHandler::GetInstance()->SendMessageWithErrorMask(
      &dummy_message_, {1, 2, 3}));
  ScopedMockLog log;

  // This error will be not be masked since this sequence number has no mask.
  EXPECT_CALL(log, Log(logging::LOG_ERROR, _, HasSubstr("error 1"))).Times(1);
  ReturnError(kSequenceNumber - 1, 1);

  // This error will be masked.
  EXPECT_CALL(log, Log(logging::LOG_ERROR, _, HasSubstr("error 2"))).Times(0);
  ReturnError(kSequenceNumber, 2);

  // This second error will be not be masked since the error mask was removed.
  EXPECT_CALL(log, Log(logging::LOG_ERROR, _, HasSubstr("error 3"))).Times(1);
  ReturnError(kSequenceNumber, 3);

  StopRTNLHandler();
}

}  // namespace shill
