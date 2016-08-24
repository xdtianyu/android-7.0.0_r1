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

#include "shill/net/netlink_manager.h"

#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>

#include <list>
#include <map>
#include <queue>

#include <base/location.h>
#include <base/logging.h>
#include <base/memory/weak_ptr.h>
#include <base/message_loop/message_loop.h>
#include <base/stl_util.h>

#include "shill/net/attribute_list.h"
#include "shill/net/generic_netlink_message.h"
#include "shill/net/io_handler.h"
#include "shill/net/netlink_message.h"
#include "shill/net/netlink_packet.h"
#include "shill/net/nl80211_message.h"
#include "shill/net/shill_time.h"
#include "shill/net/sockets.h"

using base::Bind;
using base::LazyInstance;
using base::MessageLoop;
using std::list;
using std::map;
using std::string;

namespace shill {

namespace {
LazyInstance<NetlinkManager> g_netlink_manager = LAZY_INSTANCE_INITIALIZER;
}  // namespace

const char NetlinkManager::kEventTypeConfig[] = "config";
const char NetlinkManager::kEventTypeScan[] = "scan";
const char NetlinkManager::kEventTypeRegulatory[] = "regulatory";
const char NetlinkManager::kEventTypeMlme[] = "mlme";
const long NetlinkManager::kMaximumNewFamilyWaitSeconds = 1;  // NOLINT
const long NetlinkManager::kMaximumNewFamilyWaitMicroSeconds = 0;  // NOLINT
const long NetlinkManager::kResponseTimeoutSeconds = 5;  // NOLINT
const long NetlinkManager::kResponseTimeoutMicroSeconds = 0;  // NOLINT
const long NetlinkManager::kPendingDumpTimeoutMilliseconds = 500;  // NOLINT
const long NetlinkManager::kNlMessageRetryDelayMilliseconds = 300;  // NOLINT
const int NetlinkManager::kMaxNlMessageRetries = 1;  // NOLINT

NetlinkManager::NetlinkResponseHandler::NetlinkResponseHandler(
    const NetlinkManager::NetlinkAckHandler& ack_handler,
    const NetlinkManager::NetlinkAuxilliaryMessageHandler& error_handler)
    : ack_handler_(ack_handler),
      error_handler_(error_handler) {}

NetlinkManager::NetlinkResponseHandler::~NetlinkResponseHandler() {}

void NetlinkManager::NetlinkResponseHandler::HandleError(
    AuxilliaryMessageType type, const NetlinkMessage* netlink_message) const {
  if (!error_handler_.is_null())
    error_handler_.Run(type, netlink_message);
}

bool NetlinkManager::NetlinkResponseHandler::HandleAck() const {
  if (!ack_handler_.is_null()) {
    // Default behavior is not to remove callbacks. In the case where the
    // callback is not successfully invoked, this is safe as it does not
    // prevent any further responses from behind handled.
    bool remove_callbacks = false;
    ack_handler_.Run(&remove_callbacks);
    // If there are no other handlers other than the Ack handler, then force
    // the callback to be removed after handling the Ack.
    return remove_callbacks || error_handler_.is_null();
  } else {
    // If there is no Ack handler, do not delete registered callbacks
    // for this function because we are not explicitly told to do so.
    return false;
  }
}

class ControlResponseHandler : public NetlinkManager::NetlinkResponseHandler {
 public:
  ControlResponseHandler(
      const NetlinkManager::NetlinkAckHandler& ack_handler,
      const NetlinkManager::NetlinkAuxilliaryMessageHandler& error_handler,
      const NetlinkManager::ControlNetlinkMessageHandler& handler)
    : NetlinkManager::NetlinkResponseHandler(ack_handler, error_handler),
      handler_(handler) {}

  bool HandleMessage(const NetlinkMessage& netlink_message) const override {
    if (netlink_message.message_type() !=
        ControlNetlinkMessage::GetMessageType()) {
      LOG(ERROR) << "Message is type " << netlink_message.message_type()
                 << ", not " << ControlNetlinkMessage::GetMessageType()
                 << " (Control).";
      return false;
    }
    if (!handler_.is_null()) {
      const ControlNetlinkMessage* message =
          static_cast<const ControlNetlinkMessage*>(&netlink_message);
      handler_.Run(*message);
    }
    return true;
  }

  bool HandleAck() const override {
    if (handler_.is_null()) {
      return NetlinkManager::NetlinkResponseHandler::HandleAck();
    } else {
      bool remove_callbacks = false;
      NetlinkManager::NetlinkResponseHandler::ack_handler_.Run(
          &remove_callbacks);
      return remove_callbacks;
    }
  }

 private:
  NetlinkManager::ControlNetlinkMessageHandler handler_;

  DISALLOW_COPY_AND_ASSIGN(ControlResponseHandler);
};

class Nl80211ResponseHandler : public NetlinkManager::NetlinkResponseHandler {
 public:
  Nl80211ResponseHandler(
      const NetlinkManager::NetlinkAckHandler& ack_handler,
      const NetlinkManager::NetlinkAuxilliaryMessageHandler& error_handler,
      const NetlinkManager::Nl80211MessageHandler& handler)
    : NetlinkManager::NetlinkResponseHandler(ack_handler, error_handler),
      handler_(handler) {}

  bool HandleMessage(const NetlinkMessage& netlink_message) const override {
    if (netlink_message.message_type() != Nl80211Message::GetMessageType()) {
      LOG(ERROR) << "Message is type " << netlink_message.message_type()
                 << ", not " << Nl80211Message::GetMessageType()
                 << " (Nl80211).";
      return false;
    }
    if (!handler_.is_null()) {
      const Nl80211Message* message =
          static_cast<const Nl80211Message*>(&netlink_message);
      handler_.Run(*message);
    }
    return true;
  }

  bool HandleAck() const override {
    if (handler_.is_null()) {
      return NetlinkManager::NetlinkResponseHandler::HandleAck();
    } else {
      bool remove_callbacks = false;
      NetlinkManager::NetlinkResponseHandler::ack_handler_.Run(
          &remove_callbacks);
      return remove_callbacks;
    }
  }

 private:
  NetlinkManager::Nl80211MessageHandler handler_;

  DISALLOW_COPY_AND_ASSIGN(Nl80211ResponseHandler);
};


NetlinkManager::MessageType::MessageType() :
  family_id(NetlinkMessage::kIllegalMessageType) {}

NetlinkManager::NetlinkManager()
    : weak_ptr_factory_(this),
      dispatcher_callback_(Bind(&NetlinkManager::OnRawNlMessageReceived,
                                weak_ptr_factory_.GetWeakPtr())),
      time_(Time::GetInstance()),
      io_handler_factory_(
          IOHandlerFactoryContainer::GetInstance()->GetIOHandlerFactory()),
          dump_pending_(false) {}

NetlinkManager::~NetlinkManager() {}

NetlinkManager* NetlinkManager::GetInstance() {
  return g_netlink_manager.Pointer();
}

void NetlinkManager::Reset(bool full) {
  ClearBroadcastHandlers();
  message_handlers_.clear();
  message_types_.clear();
  while (!pending_messages_.empty()) {
    pending_messages_.pop();
  }
  pending_dump_timeout_callback_.Cancel();
  resend_dump_message_callback_.Cancel();
  dump_pending_ = false;
  if (full) {
    sock_.reset();
  }
}

void NetlinkManager::OnNewFamilyMessage(const ControlNetlinkMessage& message) {
  uint16_t family_id;
  string family_name;

  if (!message.const_attributes()->GetU16AttributeValue(CTRL_ATTR_FAMILY_ID,
                                                         &family_id)) {
    LOG(ERROR) << __func__ << ": Couldn't get family_id attribute";
    return;
  }

  if (!message.const_attributes()->GetStringAttributeValue(
      CTRL_ATTR_FAMILY_NAME, &family_name)) {
    LOG(ERROR) << __func__ << ": Couldn't get family_name attribute";
    return;
  }

  VLOG(3) << "Socket family '" << family_name << "' has id=" << family_id;

  // Extract the available multicast groups from the message.
  AttributeListConstRefPtr multicast_groups;
  if (message.const_attributes()->ConstGetNestedAttributeList(
      CTRL_ATTR_MCAST_GROUPS, &multicast_groups)) {
    AttributeListConstRefPtr current_group;

    for (int i = 1;
         multicast_groups->ConstGetNestedAttributeList(i, &current_group);
         ++i) {
      string group_name;
      uint32_t group_id;
      if (!current_group->GetStringAttributeValue(CTRL_ATTR_MCAST_GRP_NAME,
                                                  &group_name)) {
        LOG(WARNING) << "Expected CTRL_ATTR_MCAST_GRP_NAME, found none";
        continue;
      }
      if (!current_group->GetU32AttributeValue(CTRL_ATTR_MCAST_GRP_ID,
                                               &group_id)) {
        LOG(WARNING) << "Expected CTRL_ATTR_MCAST_GRP_ID, found none";
        continue;
      }
      VLOG(3) << "  Adding group '" << group_name << "' = " << group_id;
      message_types_[family_name].groups[group_name] = group_id;
    }
  }

  message_types_[family_name].family_id = family_id;
}

// static
void NetlinkManager::OnNetlinkMessageError(AuxilliaryMessageType type,
                                           const NetlinkMessage* raw_message) {
  switch (type) {
    case kErrorFromKernel:
      if (!raw_message) {
        LOG(ERROR) << "Unknown error from kernel.";
        break;
      }
      if (raw_message->message_type() == ErrorAckMessage::GetMessageType()) {
        const ErrorAckMessage* error_ack_message =
            static_cast<const ErrorAckMessage*>(raw_message);
        // error_ack_message->error() should be non-zero (i.e. not an ACK),
        // since ACKs would be routed to a NetlinkAckHandler in
        // NetlinkManager::OnNlMessageReceived.
        LOG(ERROR) << __func__
                   << ": Message (seq: " << error_ack_message->sequence_number()
                   << ") failed: " << error_ack_message->ToString();
      }
      break;

    case kUnexpectedResponseType:
      LOG(ERROR) << "Message not handled by regular message handler:";
      if (raw_message) {
        raw_message->Print(0, 0);
      }
      break;

    case kTimeoutWaitingForResponse:
      LOG(WARNING) << "Timeout waiting for response";
      break;

    default:
      LOG(ERROR) << "Unexpected auxilliary message type: " << type;
      break;
  }
}

bool NetlinkManager::Init() {
  // Install message factory for control class of messages, which has
  // statically-known message type.
  message_factory_.AddFactoryMethod(
      ControlNetlinkMessage::kMessageType,
      Bind(&ControlNetlinkMessage::CreateMessage));
  if (!sock_) {
    sock_.reset(new NetlinkSocket);
    if (!sock_) {
      LOG(ERROR) << "No memory";
      return false;
    }

    if (!sock_->Init()) {
      return false;
    }
  }
  return true;
}

void NetlinkManager::Start() {
  // Create an IO handler for receiving messages on the netlink socket.
  // IO handler will be installed to the current message loop.
  dispatcher_handler_.reset(io_handler_factory_->CreateIOInputHandler(
      file_descriptor(),
      dispatcher_callback_,
      Bind(&NetlinkManager::OnReadError, weak_ptr_factory_.GetWeakPtr())));
}

int NetlinkManager::file_descriptor() const {
  return (sock_ ? sock_->file_descriptor() : Sockets::kInvalidFileDescriptor);
}

uint16_t NetlinkManager::GetFamily(const string& name,
    const NetlinkMessageFactory::FactoryMethod& message_factory) {
  MessageType& message_type = message_types_[name];
  if (message_type.family_id != NetlinkMessage::kIllegalMessageType) {
    return message_type.family_id;
  }
  if (!sock_) {
    LOG(FATAL) << "Must call |Init| before this method.";
    return false;
  }

  GetFamilyMessage msg;
  if (!msg.attributes()->SetStringAttributeValue(CTRL_ATTR_FAMILY_NAME, name)) {
    LOG(ERROR) << "Couldn't set string attribute";
    return false;
  }
  SendControlMessage(&msg,
                     Bind(&NetlinkManager::OnNewFamilyMessage,
                          weak_ptr_factory_.GetWeakPtr()),
                     Bind(&NetlinkManager::OnAckDoNothing),
                     Bind(&NetlinkManager::OnNetlinkMessageError));

  // Wait for a response.  The code absolutely needs family_ids for its
  // message types so we do a synchronous wait.  It's OK to do this because
  // a) libnl does a synchronous wait (so there's prior art), b) waiting
  // asynchronously would add significant and unnecessary complexity to the
  // code that deals with pending messages that could, potentially, be waiting
  // for a message type, and c) it really doesn't take very long for the
  // GETFAMILY / NEWFAMILY transaction to transpire (this transaction was timed
  // over 20 times and found a maximum duration of 11.1 microseconds and an
  // average of 4.0 microseconds).
  struct timeval now, end_time;
  struct timeval maximum_wait_duration = {kMaximumNewFamilyWaitSeconds,
                                          kMaximumNewFamilyWaitMicroSeconds};
  time_->GetTimeMonotonic(&now);
  timeradd(&now, &maximum_wait_duration, &end_time);

  do {
    // Wait with timeout for a message from the netlink socket.
    fd_set read_fds;
    FD_ZERO(&read_fds);

    int socket = file_descriptor();
    if (socket >= FD_SETSIZE)
       LOG(FATAL) << "Invalid file_descriptor.";
    FD_SET(socket, &read_fds);

    struct timeval wait_duration;
    timersub(&end_time, &now, &wait_duration);
    int result = sock_->sockets()->Select(file_descriptor() + 1,
                                          &read_fds,
                                          nullptr,
                                          nullptr,
                                          &wait_duration);
    if (result < 0) {
      PLOG(ERROR) << "Select failed";
      return NetlinkMessage::kIllegalMessageType;
    }
    if (result == 0) {
      LOG(WARNING) << "Timed out waiting for family_id for family '"
                   << name << "'.";
      return NetlinkMessage::kIllegalMessageType;
    }

    // Read and process any messages.
    ByteString received;
    sock_->RecvMessage(&received);
    InputData input_data(received.GetData(), received.GetLength());
    OnRawNlMessageReceived(&input_data);
    if (message_type.family_id != NetlinkMessage::kIllegalMessageType) {
      uint16_t family_id = message_type.family_id;
      if (family_id != NetlinkMessage::kIllegalMessageType) {
        message_factory_.AddFactoryMethod(family_id, message_factory);
      }
      return message_type.family_id;
    }
    time_->GetTimeMonotonic(&now);
  } while (timercmp(&now, &end_time, <));

  LOG(ERROR) << "Timed out waiting for family_id for family '" << name << "'.";
  return NetlinkMessage::kIllegalMessageType;
}

bool NetlinkManager::AddBroadcastHandler(const NetlinkMessageHandler& handler) {
  if (FindBroadcastHandler(handler)) {
    LOG(WARNING) << "Trying to re-add a handler";
    return false;  // Should only be one copy in the list.
  }
  if (handler.is_null()) {
    LOG(WARNING) << "Trying to add a NULL handler";
    return false;
  }
  // And add the handler to the list.
  VLOG(3) << "NetlinkManager::" << __func__ << " - adding handler";
  broadcast_handlers_.push_back(handler);
  return true;
}

bool NetlinkManager::RemoveBroadcastHandler(
    const NetlinkMessageHandler& handler) {
  list<NetlinkMessageHandler>::iterator i;
  for (i = broadcast_handlers_.begin(); i != broadcast_handlers_.end(); ++i) {
    if ((*i).Equals(handler)) {
      broadcast_handlers_.erase(i);
      // Should only be one copy in the list so we don't have to continue
      // looking for another one.
      return true;
    }
  }
  LOG(WARNING) << "NetlinkMessageHandler not found.";
  return false;
}

bool NetlinkManager::FindBroadcastHandler(const NetlinkMessageHandler& handler)
    const {
  for (const auto& broadcast_handler : broadcast_handlers_) {
    if (broadcast_handler.Equals(handler)) {
      return true;
    }
  }
  return false;
}

void NetlinkManager::ClearBroadcastHandlers() {
  broadcast_handlers_.clear();
}

bool NetlinkManager::SendControlMessage(
    ControlNetlinkMessage* message,
    const ControlNetlinkMessageHandler& message_handler,
    const NetlinkAckHandler& ack_handler,
    const NetlinkAuxilliaryMessageHandler& error_handler) {
  return SendOrPostMessage(message,
                           new ControlResponseHandler(ack_handler,
                                                      error_handler,
                                                      message_handler));
}

bool NetlinkManager::SendNl80211Message(
    Nl80211Message* message,
    const Nl80211MessageHandler& message_handler,
    const NetlinkAckHandler& ack_handler,
    const NetlinkAuxilliaryMessageHandler& error_handler) {
  return SendOrPostMessage(message,
                           new Nl80211ResponseHandler(ack_handler,
                                                      error_handler,
                                                      message_handler));
}

bool NetlinkManager::SendOrPostMessage(
    NetlinkMessage* message,
    NetlinkManager::NetlinkResponseHandler* response_handler) {
  if (!message) {
    LOG(ERROR) << "Message is NULL.";
    return false;
  }

  const uint32_t sequence_number = this->GetSequenceNumber();
  const bool is_dump_msg = message->flags() & NLM_F_DUMP;
  NetlinkPendingMessage pending_message(
      sequence_number, is_dump_msg, message->Encode(sequence_number),
      NetlinkResponseHandlerRefPtr(response_handler));

  // TODO(samueltan): print this debug message above the actual call to
  // NetlinkSocket::SendMessage in NetlinkManager::SendMessageInternal.
  VLOG(5) << "NL Message " << pending_message.sequence_number << " to send ("
          << pending_message.message_string.GetLength() << " bytes) ===>";
  message->Print(6, 7);
  NetlinkMessage::PrintBytes(8, pending_message.message_string.GetConstData(),
                             pending_message.message_string.GetLength());

  if (is_dump_msg) {
    pending_messages_.push(pending_message);
    if (IsDumpPending()) {
      VLOG(5) << "Dump pending -- will send message after dump is complete";
      return true;
    }
  }
  return RegisterHandlersAndSendMessage(pending_message);
}

bool NetlinkManager::RegisterHandlersAndSendMessage(
    const NetlinkPendingMessage& pending_message) {
  // Clean out timed-out message handlers.  The list of outstanding messages
  // should be small so the time wasted by looking through all of them should
  // be small.
  struct timeval now;
  time_->GetTimeMonotonic(&now);
  map<uint32_t, NetlinkResponseHandlerRefPtr>::iterator handler_it =
      message_handlers_.begin();
  while (handler_it != message_handlers_.end()) {
    if (timercmp(&now, &handler_it->second->delete_after(), >)) {
      // A timeout isn't always unexpected so this is not a warning.
      VLOG(3) << "Removing timed-out handler for sequence number "
              << handler_it->first;
      handler_it->second->HandleError(kTimeoutWaitingForResponse, nullptr);
      handler_it = message_handlers_.erase(handler_it);
    } else {
      ++handler_it;
    }
  }

  // Register handlers for replies to this message.
  if (!pending_message.handler) {
    VLOG(3) << "Handler for message was null.";
  } else if (ContainsKey(message_handlers_, pending_message.sequence_number)) {
    LOG(ERROR) << "A handler already existed for sequence: "
               << pending_message.sequence_number;
    return false;
  } else {
    struct timeval response_timeout = {kResponseTimeoutSeconds,
                                       kResponseTimeoutMicroSeconds};
    struct timeval delete_after;
    timeradd(&now, &response_timeout, &delete_after);
    pending_message.handler->set_delete_after(delete_after);

    message_handlers_[pending_message.sequence_number] =
        pending_message.handler;
  }
  return SendMessageInternal(pending_message);
}

bool NetlinkManager::SendMessageInternal(
    const NetlinkPendingMessage& pending_message) {
  VLOG(5) << "Sending NL message " << pending_message.sequence_number;

  if (!sock_->SendMessage(pending_message.message_string)) {
    LOG(ERROR) << "Failed to send Netlink message.";
    return false;
  }
  if (pending_message.is_dump_request) {
    VLOG(5) << "Waiting for replies to NL dump message "
            << pending_message.sequence_number;
    dump_pending_ = true;
    pending_dump_timeout_callback_.Reset(Bind(
        &NetlinkManager::OnPendingDumpTimeout, weak_ptr_factory_.GetWeakPtr()));
    MessageLoop::current()->PostDelayedTask(
        FROM_HERE, pending_dump_timeout_callback_.callback(),
        base::TimeDelta::FromMilliseconds(kPendingDumpTimeoutMilliseconds));
  }
  return true;
}

NetlinkMessage::MessageContext NetlinkManager::InferMessageContext(
    const NetlinkPacket& packet) {
  NetlinkMessage::MessageContext context;

  const uint32_t sequence_number = packet.GetMessageSequence();
  if (!ContainsKey(message_handlers_, sequence_number) &&
      packet.GetMessageType() != ErrorAckMessage::kMessageType) {
    context.is_broadcast = true;
  }

  genlmsghdr genl_header;
  if (packet.GetMessageType() == Nl80211Message::GetMessageType() &&
      packet.GetGenlMsgHdr(&genl_header)) {
    context.nl80211_cmd = genl_header.cmd;
  }

  return context;
}

void NetlinkManager::OnPendingDumpTimeout() {
  VLOG(3) << "Timed out waiting for replies to NL dump message "
          << PendingDumpSequenceNumber();
  CallErrorHandler(PendingDumpSequenceNumber(), kTimeoutWaitingForResponse,
                   nullptr);
  OnPendingDumpComplete();
}

void NetlinkManager::OnPendingDumpComplete() {
  VLOG(3) << __func__;
  dump_pending_ = false;
  pending_dump_timeout_callback_.Cancel();
  resend_dump_message_callback_.Cancel();
  pending_messages_.pop();
  if (!pending_messages_.empty()) {
    VLOG(3) << "Sending next pending message";
    NetlinkPendingMessage to_send = pending_messages_.front();
    RegisterHandlersAndSendMessage(to_send);
  }
}

bool NetlinkManager::IsDumpPending() {
  return dump_pending_ && !pending_messages_.empty();
}

uint32_t NetlinkManager::PendingDumpSequenceNumber() {
  if (!IsDumpPending()) {
    LOG(ERROR) << __func__ << ": no pending dump";
    return 0;
  }
  return pending_messages_.front().sequence_number;
}

bool NetlinkManager::RemoveMessageHandler(const NetlinkMessage& message) {
  if (!ContainsKey(message_handlers_, message.sequence_number())) {
    return false;
  }
  message_handlers_.erase(message.sequence_number());
  return true;
}

uint32_t NetlinkManager::GetSequenceNumber() {
  return sock_ ?
      sock_->GetSequenceNumber() : NetlinkMessage::kBroadcastSequenceNumber;
}

bool NetlinkManager::SubscribeToEvents(const string& family_id,
                                       const string& group_name) {
  if (!ContainsKey(message_types_, family_id)) {
    LOG(ERROR) << "Family '" << family_id << "' doesn't exist";
    return false;
  }

  if (!ContainsKey(message_types_[family_id].groups, group_name)) {
    LOG(ERROR) << "Group '" << group_name << "' doesn't exist in family '"
               << family_id << "'";
    return false;
  }

  uint32_t group_id = message_types_[family_id].groups[group_name];
  if (!sock_) {
    LOG(FATAL) << "Need to call |Init| first.";
  }
  return sock_->SubscribeToEvents(group_id);
}

void NetlinkManager::OnRawNlMessageReceived(InputData* data) {
  if (!data) {
    LOG(ERROR) << __func__ << "() called with null header.";
    return;
  }
  unsigned char* buf = data->buf;
  unsigned char* end = buf + data->len;
  while (buf < end) {
    NetlinkPacket packet(buf, end - buf);
    if (!packet.IsValid()) {
      break;
    }
    buf += packet.GetLength();
    OnNlMessageReceived(&packet);
  }
}

void NetlinkManager::OnNlMessageReceived(NetlinkPacket* packet) {
  if (!packet) {
    LOG(ERROR) << __func__ << "() called with null packet.";
    return;
  }
  const uint32_t sequence_number = packet->GetMessageSequence();

  std::unique_ptr<NetlinkMessage> message(
      message_factory_.CreateMessage(packet, InferMessageContext(*packet)));
  if (message == nullptr) {
    VLOG(3) << "NL Message " << sequence_number << " <===";
    VLOG(3) << __func__ << "(msg:NULL)";
    return;  // Skip current message, continue parsing buffer.
  }
  VLOG(5) << "NL Message " << sequence_number << " Received ("
          << packet->GetLength() << " bytes) <===";
  message->Print(6, 7);
  NetlinkMessage::PrintPacket(8, *packet);

  bool is_error_ack_message = false;
  uint32_t error_code = 0;
  if (message->message_type() == ErrorAckMessage::GetMessageType()) {
    is_error_ack_message = true;
    const ErrorAckMessage* error_ack_message =
        static_cast<const ErrorAckMessage*>(message.get());
    error_code = error_ack_message->error();
  }

  // Note: assumes we only receive one reply to a dump request: an error
  // message, an ACK, or a single multi-part reply. If we receive two replies,
  // then we will stop waiting for replies after the first reply is processed
  // here. This assumption should hold unless the NLM_F_ACK or NLM_F_ECHO
  // flags are explicitly added to the dump request.
  if (IsDumpPending() &&
      (message->sequence_number() == PendingDumpSequenceNumber()) &&
      !((message->flags() & NLM_F_MULTI) &&
        (message->message_type() != NLMSG_DONE))) {
    // Dump currently in progress, this message's sequence number matches that
    // of the pending dump request, and we are not in the middle of receiving a
    // multi-part reply.
    if (is_error_ack_message && (error_code == static_cast<uint32_t>(-EBUSY))) {
      VLOG(3) << "EBUSY reply received for NL dump message "
              << PendingDumpSequenceNumber();
      if (pending_messages_.front().retries_left) {
        pending_messages_.front().last_received_error = error_code;
        pending_dump_timeout_callback_.Cancel();
        ResendPendingDumpMessageAfterDelay();
        // Since we will resend the message, do not invoke error handler.
        return;
      } else {
        VLOG(3) << "No more resend attempts left for NL dump message "
                << PendingDumpSequenceNumber() << " -- stop waiting "
                                                  "for replies";
        OnPendingDumpComplete();
      }
    } else {
      VLOG(3) << "Reply received for NL dump message "
              << PendingDumpSequenceNumber() << " -- stop waiting for replies";
      OnPendingDumpComplete();
    }
  }

  if (is_error_ack_message) {
    VLOG(3) << "Error/ACK response to message " << sequence_number;
    if (error_code) {
      CallErrorHandler(sequence_number, kErrorFromKernel, message.get());
    } else {
      if (ContainsKey(message_handlers_, sequence_number)) {
        VLOG(6) << "Found message-specific ACK handler";
        if (message_handlers_[sequence_number]->HandleAck()) {
          VLOG(6) << "ACK handler invoked -- removing callback";
          message_handlers_.erase(sequence_number);
        } else {
          VLOG(6) << "ACK handler invoked -- not removing callback";
        }
      }
    }
    return;
  }

  if (ContainsKey(message_handlers_, sequence_number)) {
    VLOG(6) << "Found message-specific handler";
    if ((message->flags() & NLM_F_MULTI) &&
        (message->message_type() == NLMSG_DONE)) {
      message_handlers_[sequence_number]->HandleError(kDone, message.get());
    } else if (!message_handlers_[sequence_number]->HandleMessage(*message)) {
      LOG(ERROR) << "Couldn't call message handler for " << sequence_number;
      // Call the error handler but, since we don't have an |ErrorAckMessage|,
      // we'll have to pass a nullptr.
      message_handlers_[sequence_number]->HandleError(kUnexpectedResponseType,
                                                      nullptr);
    }
    if ((message->flags() & NLM_F_MULTI) &&
        (message->message_type() != NLMSG_DONE)) {
      VLOG(6) << "Multi-part message -- not removing callback";
    } else {
      VLOG(6) << "Removing callbacks";
      message_handlers_.erase(sequence_number);
    }
    return;
  }

  for (const auto& handler : broadcast_handlers_) {
    VLOG(6) << "Calling broadcast handler";
    if (!handler.is_null()) {
      handler.Run(*message);
    }
  }
}

void NetlinkManager::ResendPendingDumpMessage() {
  if (!IsDumpPending()) {
    VLOG(3) << "No pending dump, so do not resend dump message";
    return;
  }
  --pending_messages_.front().retries_left;
  if (SendMessageInternal(pending_messages_.front())) {
    VLOG(3) << "NL message " << PendingDumpSequenceNumber()
            << " sent again successfully";
    return;
  }
  VLOG(3) << "Failed to resend NL message " << PendingDumpSequenceNumber();
  if (pending_messages_.front().retries_left) {
    ResendPendingDumpMessageAfterDelay();
  } else {
    VLOG(3) << "No more resend attempts left for NL dump message "
            << PendingDumpSequenceNumber() << " -- stop waiting "
                                              "for replies";
    ErrorAckMessage err_message(pending_messages_.front().last_received_error);
    CallErrorHandler(PendingDumpSequenceNumber(), kErrorFromKernel,
                     &err_message);
    OnPendingDumpComplete();
  }
}

void NetlinkManager::CallErrorHandler(uint32_t sequence_number,
                                      AuxilliaryMessageType type,
                                      const NetlinkMessage* netlink_message) {
  if (ContainsKey(message_handlers_, sequence_number)) {
    VLOG(6) << "Found message-specific error handler";
    message_handlers_[sequence_number]->HandleError(type, netlink_message);
    message_handlers_.erase(sequence_number);
  }
}

void NetlinkManager::OnReadError(const string& error_msg) {
  // TODO(wdg): When netlink_manager is used for scan, et al., this should
  // either be LOG(FATAL) or the code should properly deal with errors,
  // e.g., dropped messages due to the socket buffer being full.
  LOG(ERROR) << "NetlinkManager's netlink Socket read returns error: "
             << error_msg;
}

void NetlinkManager::ResendPendingDumpMessageAfterDelay() {
  VLOG(3) << "Resending NL dump message " << PendingDumpSequenceNumber()
          << " after " << kNlMessageRetryDelayMilliseconds << " ms";
  resend_dump_message_callback_.Reset(
      Bind(&NetlinkManager::ResendPendingDumpMessage,
           weak_ptr_factory_.GetWeakPtr()));
  MessageLoop::current()->PostDelayedTask(
      FROM_HERE, resend_dump_message_callback_.callback(),
      base::TimeDelta::FromMilliseconds(kNlMessageRetryDelayMilliseconds));
}

}  // namespace shill.
