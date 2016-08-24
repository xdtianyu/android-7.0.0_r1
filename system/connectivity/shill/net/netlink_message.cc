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

#include "shill/net/netlink_message.h"

#include <limits.h>

#include <algorithm>
#include <map>
#include <memory>
#include <string>

#include <base/format_macros.h>
#include <base/logging.h>
#include <base/stl_util.h>
#include <base/strings/stringprintf.h>

#include "shill/net/netlink_packet.h"

using base::StringAppendF;
using base::StringPrintf;
using std::map;
using std::min;
using std::string;

namespace shill {

const uint32_t NetlinkMessage::kBroadcastSequenceNumber = 0;
const uint16_t NetlinkMessage::kIllegalMessageType = UINT16_MAX;

// NetlinkMessage

ByteString NetlinkMessage::EncodeHeader(uint32_t sequence_number) {
  ByteString result;
  if (message_type_ == kIllegalMessageType) {
    LOG(ERROR) << "Message type not set";
    return result;
  }
  sequence_number_ = sequence_number;
  if (sequence_number_ == kBroadcastSequenceNumber) {
    LOG(ERROR) << "Couldn't get a legal sequence number";
    return result;
  }

  // Build netlink header.
  nlmsghdr header;
  size_t nlmsghdr_with_pad = NLMSG_ALIGN(sizeof(header));
  header.nlmsg_len = nlmsghdr_with_pad;
  header.nlmsg_type = message_type_;
  header.nlmsg_flags = NLM_F_REQUEST | flags_;
  header.nlmsg_seq = sequence_number_;
  header.nlmsg_pid = getpid();

  // Netlink header + pad.
  result.Append(ByteString(reinterpret_cast<unsigned char*>(&header),
                           sizeof(header)));
  result.Resize(nlmsghdr_with_pad);  // Zero-fill pad space (if any).
  return result;
}

bool NetlinkMessage::InitAndStripHeader(NetlinkPacket* packet) {
  const nlmsghdr& header = packet->GetNlMsgHeader();
  message_type_ = header.nlmsg_type;
  flags_ = header.nlmsg_flags;
  sequence_number_ = header.nlmsg_seq;

  return true;
}

bool NetlinkMessage::InitFromPacket(NetlinkPacket* packet,
                                    NetlinkMessage::MessageContext context) {
  if (!packet) {
    LOG(ERROR) << "Null |packet| parameter";
    return false;
  }
  if (!InitAndStripHeader(packet)) {
    return false;
  }
  return true;
}

// static
void NetlinkMessage::PrintBytes(int log_level, const unsigned char* buf,
                                size_t num_bytes) {
  VLOG(log_level) << "Netlink Message -- Examining Bytes";
  if (!buf) {
    VLOG(log_level) << "<NULL Buffer>";
    return;
  }

  if (num_bytes >= sizeof(nlmsghdr)) {
      PrintHeader(log_level, reinterpret_cast<const nlmsghdr*>(buf));
      buf += sizeof(nlmsghdr);
      num_bytes -= sizeof(nlmsghdr);
  } else {
    VLOG(log_level) << "Not enough bytes (" << num_bytes
                    << ") for a complete nlmsghdr (requires "
                    << sizeof(nlmsghdr) << ").";
  }

  PrintPayload(log_level, buf, num_bytes);
}

// static
void NetlinkMessage::PrintPacket(int log_level, const NetlinkPacket& packet) {
  VLOG(log_level) << "Netlink Message -- Examining Packet";
  if (!packet.IsValid()) {
    VLOG(log_level) << "<Invalid Buffer>";
    return;
  }

  PrintHeader(log_level, &packet.GetNlMsgHeader());
  const ByteString& payload = packet.GetPayload();
  PrintPayload(log_level, payload.GetConstData(), payload.GetLength());
}

// static
void NetlinkMessage::PrintHeader(int log_level, const nlmsghdr* header) {
  const unsigned char* buf = reinterpret_cast<const unsigned char*>(header);
  VLOG(log_level) << StringPrintf(
      "len:          %02x %02x %02x %02x = %u bytes",
      buf[0], buf[1], buf[2], buf[3], header->nlmsg_len);

  VLOG(log_level) << StringPrintf(
      "type | flags: %02x %02x %02x %02x - type:%u flags:%s%s%s%s%s",
      buf[4], buf[5], buf[6], buf[7], header->nlmsg_type,
      ((header->nlmsg_flags & NLM_F_REQUEST) ? " REQUEST" : ""),
      ((header->nlmsg_flags & NLM_F_MULTI) ? " MULTI" : ""),
      ((header->nlmsg_flags & NLM_F_ACK) ? " ACK" : ""),
      ((header->nlmsg_flags & NLM_F_ECHO) ? " ECHO" : ""),
      ((header->nlmsg_flags & NLM_F_DUMP_INTR) ? " BAD-SEQ" : ""));

  VLOG(log_level) << StringPrintf(
      "sequence:     %02x %02x %02x %02x = %u",
      buf[8], buf[9], buf[10], buf[11], header->nlmsg_seq);
  VLOG(log_level) << StringPrintf(
      "pid:          %02x %02x %02x %02x = %u",
      buf[12], buf[13], buf[14], buf[15], header->nlmsg_pid);
}

// static
void NetlinkMessage::PrintPayload(int log_level, const unsigned char* buf,
                                  size_t num_bytes) {
  while (num_bytes) {
    string output;
    size_t bytes_this_row = min(num_bytes, static_cast<size_t>(32));
    for (size_t i = 0; i < bytes_this_row; ++i) {
      StringAppendF(&output, " %02x", *buf++);
    }
    VLOG(log_level) << output;
    num_bytes -= bytes_this_row;
  }
}

// ErrorAckMessage.

const uint16_t ErrorAckMessage::kMessageType = NLMSG_ERROR;

bool ErrorAckMessage::InitFromPacket(NetlinkPacket* packet,
                                     NetlinkMessage::MessageContext context) {
  if (!packet) {
    LOG(ERROR) << "Null |const_msg| parameter";
    return false;
  }
  if (!InitAndStripHeader(packet)) {
    return false;
  }

  // Get the error code from the payload.
  return packet->ConsumeData(sizeof(error_), &error_);
}

ByteString ErrorAckMessage::Encode(uint32_t sequence_number) {
  LOG(ERROR) << "We're not supposed to send errors or Acks to the kernel";
  return ByteString();
}

string ErrorAckMessage::ToString() const {
  string output;
  if (error()) {
    StringAppendF(&output, "NETLINK_ERROR 0x%" PRIx32 ": %s",
                  -error_, strerror(-error_));
  } else {
    StringAppendF(&output, "ACK");
  }
  return output;
}

void ErrorAckMessage::Print(int header_log_level,
                            int /*detail_log_level*/) const {
  VLOG(header_log_level) << ToString();
}

// NoopMessage.

const uint16_t NoopMessage::kMessageType = NLMSG_NOOP;

ByteString NoopMessage::Encode(uint32_t sequence_number) {
  LOG(ERROR) << "We're not supposed to send NOOP to the kernel";
  return ByteString();
}

void NoopMessage::Print(int header_log_level, int /*detail_log_level*/) const {
  VLOG(header_log_level) << ToString();
}

// DoneMessage.

const uint16_t DoneMessage::kMessageType = NLMSG_DONE;

ByteString DoneMessage::Encode(uint32_t sequence_number) {
  return EncodeHeader(sequence_number);
}

void DoneMessage::Print(int header_log_level, int /*detail_log_level*/) const {
  VLOG(header_log_level) << ToString();
}

// OverrunMessage.

const uint16_t OverrunMessage::kMessageType = NLMSG_OVERRUN;

ByteString OverrunMessage::Encode(uint32_t sequence_number) {
  LOG(ERROR) << "We're not supposed to send Overruns to the kernel";
  return ByteString();
}

void OverrunMessage::Print(int header_log_level,
                           int /*detail_log_level*/) const {
  VLOG(header_log_level) << ToString();
}

// UnknownMessage.

ByteString UnknownMessage::Encode(uint32_t sequence_number) {
  LOG(ERROR) << "We're not supposed to send UNKNOWN messages to the kernel";
  return ByteString();
}

void UnknownMessage::Print(int header_log_level,
                           int /*detail_log_level*/) const {
  int total_bytes = message_body_.GetLength();
  const uint8_t* const_data = message_body_.GetConstData();

  string output = StringPrintf("%d bytes:", total_bytes);
  for (int i = 0; i < total_bytes; ++i) {
    StringAppendF(&output, " 0x%02x", const_data[i]);
  }
  VLOG(header_log_level) << output;
}

//
// Factory class.
//

bool NetlinkMessageFactory::AddFactoryMethod(uint16_t message_type,
                                             FactoryMethod factory) {
  if (ContainsKey(factories_, message_type)) {
    LOG(WARNING) << "Message type " << message_type << " already exists.";
    return false;
  }
  if (message_type == NetlinkMessage::kIllegalMessageType) {
    LOG(ERROR) << "Not installing factory for illegal message type.";
    return false;
  }
  factories_[message_type] = factory;
  return true;
}

NetlinkMessage* NetlinkMessageFactory::CreateMessage(
    NetlinkPacket* packet, NetlinkMessage::MessageContext context) const {
  std::unique_ptr<NetlinkMessage> message;

  auto message_type = packet->GetMessageType();
  if (message_type == NoopMessage::kMessageType) {
    message.reset(new NoopMessage());
  } else if (message_type == DoneMessage::kMessageType) {
    message.reset(new DoneMessage());
  } else if (message_type == OverrunMessage::kMessageType) {
    message.reset(new OverrunMessage());
  } else if (message_type == ErrorAckMessage::kMessageType) {
    message.reset(new ErrorAckMessage());
  } else if (ContainsKey(factories_, message_type)) {
    map<uint16_t, FactoryMethod>::const_iterator factory;
    factory = factories_.find(message_type);
    message.reset(factory->second.Run(*packet));
  }

  // If no factory exists for this message _or_ if a factory exists but it
  // failed, there'll be no message.  Handle either of those cases, by
  // creating an |UnknownMessage|.
  if (!message) {
    message.reset(new UnknownMessage(message_type, packet->GetPayload()));
  }

  if (!message->InitFromPacket(packet, context)) {
    LOG(ERROR) << "Message did not initialize properly";
    return nullptr;
  }

  return message.release();
}

}  // namespace shill.
