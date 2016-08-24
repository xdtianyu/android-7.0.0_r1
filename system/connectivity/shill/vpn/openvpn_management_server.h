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

#ifndef SHILL_VPN_OPENVPN_MANAGEMENT_SERVER_H_
#define SHILL_VPN_OPENVPN_MANAGEMENT_SERVER_H_

#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

namespace shill {

class Error;
class EventDispatcher;
struct InputData;
class IOHandler;
class OpenVPNDriver;
class Sockets;

class OpenVPNManagementServer {
 public:
  static const char kStateReconnecting[];
  static const char kStateResolve[];

  explicit OpenVPNManagementServer(OpenVPNDriver* driver);
  virtual ~OpenVPNManagementServer();

  // Returns false on failure. On success, returns true and appends management
  // interface openvpn options to |options|.
  virtual bool Start(EventDispatcher* dispatcher,
                     Sockets* sockets,
                     std::vector<std::vector<std::string>>* options);

  virtual void Stop();

  // Releases openvpn's hold if it's waiting for a hold release (i.e., if
  // |hold_waiting_| is true). Otherwise, sets |hold_release_| to true
  // indicating that the hold can be released as soon as openvpn requests.
  virtual void ReleaseHold();

  // Holds openvpn so that it doesn't connect or reconnect automatically (i.e.,
  // sets |hold_release_| to false). Note that this method neither drops an
  // existing connection, nor sends any commands to the openvpn client.
  virtual void Hold();

  // Restarts openvpn causing a disconnect followed by a reconnect attempt.
  virtual void Restart();

  // OpenVPN client state.
  const std::string& state() const { return state_; }

  // Method to get service identifier for logging.
  virtual std::string GetServiceRpcIdentifier();

 private:
  friend class OpenVPNDriverTest;
  friend class OpenVPNManagementServerTest;
  FRIEND_TEST(OpenVPNManagementServerTest, EscapeToQuote);
  FRIEND_TEST(OpenVPNManagementServerTest, Hold);
  FRIEND_TEST(OpenVPNManagementServerTest, OnInputStop);
  FRIEND_TEST(OpenVPNManagementServerTest, OnReady);
  FRIEND_TEST(OpenVPNManagementServerTest, OnReadyAcceptFail);
  FRIEND_TEST(OpenVPNManagementServerTest, PerformAuthentication);
  FRIEND_TEST(OpenVPNManagementServerTest, PerformAuthenticationNoCreds);
  FRIEND_TEST(OpenVPNManagementServerTest, PerformStaticChallengeNoCreds);
  FRIEND_TEST(OpenVPNManagementServerTest, PerformStaticChallengeOTP);
  FRIEND_TEST(OpenVPNManagementServerTest, PerformStaticChallengeToken);
  FRIEND_TEST(OpenVPNManagementServerTest, ProcessFailedPasswordMessage);
  FRIEND_TEST(OpenVPNManagementServerTest, ProcessHoldMessage);
  FRIEND_TEST(OpenVPNManagementServerTest, ProcessInfoMessage);
  FRIEND_TEST(OpenVPNManagementServerTest, ProcessNeedPasswordMessageAuth);
  FRIEND_TEST(OpenVPNManagementServerTest, ProcessNeedPasswordMessageAuthSC);
  FRIEND_TEST(OpenVPNManagementServerTest, ProcessNeedPasswordMessageTPMToken);
  FRIEND_TEST(OpenVPNManagementServerTest, ProcessNeedPasswordMessageUnknown);
  FRIEND_TEST(OpenVPNManagementServerTest, Send);
  FRIEND_TEST(OpenVPNManagementServerTest, SendHoldRelease);
  FRIEND_TEST(OpenVPNManagementServerTest, SendPassword);
  FRIEND_TEST(OpenVPNManagementServerTest, SendState);
  FRIEND_TEST(OpenVPNManagementServerTest, SendUsername);
  FRIEND_TEST(OpenVPNManagementServerTest, Start);
  FRIEND_TEST(OpenVPNManagementServerTest, Stop);
  FRIEND_TEST(OpenVPNManagementServerTest, SupplyTPMToken);
  FRIEND_TEST(OpenVPNManagementServerTest, SupplyTPMTokenNoPIN);

  // IO handler callbacks.
  void OnReady(int fd);
  void OnInput(InputData* data);
  void OnInputError(const std::string& error_msg);

  void Send(const std::string& data);
  void SendState(const std::string& state);
  void SendUsername(const std::string& tag, const std::string& username);
  void SendPassword(const std::string& tag, const std::string& password);
  void SendHoldRelease();
  void SendSignal(const std::string& signal);

  void ProcessMessage(const std::string& message);
  bool ProcessInfoMessage(const std::string& message);
  bool ProcessNeedPasswordMessage(const std::string& message);
  bool ProcessFailedPasswordMessage(const std::string& message);
  bool ProcessAuthTokenMessage(const std::string& message);
  bool ProcessStateMessage(const std::string& message);
  bool ProcessHoldMessage(const std::string& message);
  bool ProcessSuccessMessage(const std::string& message);

  void PerformStaticChallenge(const std::string& tag);
  void PerformAuthentication(const std::string& tag);
  void SupplyTPMToken(const std::string& tag);

  // Returns the first substring in |message| enclosed by the |start| and |end|
  // substrings. Note that the first |end| substring after the position of
  // |start| is matched.
  static std::string ParseSubstring(const std::string& message,
                                    const std::string& start,
                                    const std::string& end);

  // Password messages come in two forms:
  //
  // >PASSWORD:Need 'AUTH_TYPE' ...
  // >PASSWORD:Verification Failed: 'AUTH_TYPE' ['REASON_STRING']
  //
  // ParsePasswordTag parses AUTH_TYPE out of a password |message| and returns
  // it. ParsePasswordFailedReason parses REASON_STRING, if any, out of a
  // password |message| and returns it.
  static std::string ParsePasswordTag(const std::string& message);
  static std::string ParsePasswordFailedReason(const std::string& message);

  // Escapes |str| per OpenVPN's command parsing rules assuming |str| will be
  // sent over the management interface quoted (i.e., whitespace is not
  // escaped).
  static std::string EscapeToQuote(const std::string& str);

  bool IsStarted() const { return sockets_; }

  OpenVPNDriver* driver_;

  Sockets* sockets_;
  int socket_;
  std::unique_ptr<IOHandler> ready_handler_;
  EventDispatcher* dispatcher_;
  int connected_socket_;
  std::unique_ptr<IOHandler> input_handler_;

  std::string state_;

  bool hold_waiting_;
  bool hold_release_;

  DISALLOW_COPY_AND_ASSIGN(OpenVPNManagementServer);
};

}  // namespace shill

#endif  // SHILL_VPN_OPENVPN_MANAGEMENT_SERVER_H_
