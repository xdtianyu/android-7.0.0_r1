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

#include "shill/net/generic_netlink_message.h"

#include <base/bind.h>
#include <base/logging.h>
#include <base/strings/stringprintf.h>

#include "shill/net/netlink_attribute.h"
#include "shill/net/netlink_packet.h"

using base::Bind;
using base::StringPrintf;

namespace shill {

ByteString GenericNetlinkMessage::EncodeHeader(uint32_t sequence_number) {
  // Build nlmsghdr.
  ByteString result(NetlinkMessage::EncodeHeader(sequence_number));
  if (result.GetLength() == 0) {
    LOG(ERROR) << "Couldn't encode message header.";
    return result;
  }

  // Build and append the genl message header.
  genlmsghdr genl_header;
  genl_header.cmd = command();
  genl_header.version = 1;
  genl_header.reserved = 0;

  ByteString genl_header_string(
      reinterpret_cast<unsigned char*>(&genl_header), sizeof(genl_header));
  size_t genlmsghdr_with_pad = NLMSG_ALIGN(sizeof(genl_header));
  genl_header_string.Resize(genlmsghdr_with_pad);  // Zero-fill.

  nlmsghdr* pheader = reinterpret_cast<nlmsghdr*>(result.GetData());
  pheader->nlmsg_len += genlmsghdr_with_pad;
  result.Append(genl_header_string);
  return result;
}

ByteString GenericNetlinkMessage::Encode(uint32_t sequence_number) {
  ByteString result(EncodeHeader(sequence_number));
  if (result.GetLength() == 0) {
    LOG(ERROR) << "Couldn't encode message header.";
    return result;
  }

  // Build and append attributes (padding is included by
  // AttributeList::Encode).
  ByteString attribute_string = attributes_->Encode();

  // Need to re-calculate |header| since |Append|, above, moves the data.
  nlmsghdr* pheader = reinterpret_cast<nlmsghdr*>(result.GetData());
  pheader->nlmsg_len += attribute_string.GetLength();
  result.Append(attribute_string);

  return result;
}

bool GenericNetlinkMessage::InitAndStripHeader(NetlinkPacket* packet) {
  if (!packet) {
    LOG(ERROR) << "NULL packet";
    return false;
  }
  if (!NetlinkMessage::InitAndStripHeader(packet)) {
    return false;
  }

  genlmsghdr gnlh;
  if (!packet->ConsumeData(sizeof(gnlh), &gnlh)) {
    return false;
  }

  if (command_ != gnlh.cmd) {
    LOG(WARNING) << "This object thinks it's a " << command_
                 << " but the message thinks it's a " << gnlh.cmd;
  }

  return true;
}

void GenericNetlinkMessage::Print(int header_log_level,
                                  int detail_log_level) const {
  VLOG(header_log_level) << StringPrintf("Message %s (%d)",
                                         command_string(),
                                         command());
  attributes_->Print(detail_log_level, 1);
}

// Control Message

const uint16_t ControlNetlinkMessage::kMessageType = GENL_ID_CTRL;

bool ControlNetlinkMessage::InitFromPacket(
    NetlinkPacket* packet, NetlinkMessage::MessageContext context) {
  if (!packet) {
    LOG(ERROR) << "Null |packet| parameter";
    return false;
  }

  if (!InitAndStripHeader(packet)) {
    return false;
  }

  return packet->ConsumeAttributes(
      Bind(&NetlinkAttribute::NewControlAttributeFromId), attributes_);
}

// Specific Control types.

const uint8_t NewFamilyMessage::kCommand = CTRL_CMD_NEWFAMILY;
const char NewFamilyMessage::kCommandString[] = "CTRL_CMD_NEWFAMILY";

const uint8_t GetFamilyMessage::kCommand = CTRL_CMD_GETFAMILY;
const char GetFamilyMessage::kCommandString[] = "CTRL_CMD_GETFAMILY";

GetFamilyMessage::GetFamilyMessage()
    : ControlNetlinkMessage(kCommand, kCommandString) {
  attributes()->CreateStringAttribute(CTRL_ATTR_FAMILY_NAME,
                                      "CTRL_ATTR_FAMILY_NAME");
}

// static
NetlinkMessage* ControlNetlinkMessage::CreateMessage(
    const NetlinkPacket& packet) {
  genlmsghdr header;
  if (!packet.GetGenlMsgHdr(&header)) {
    LOG(ERROR) << "Could not read genl header.";
    return nullptr;
  }

  switch (header.cmd) {
    case NewFamilyMessage::kCommand:
      return new NewFamilyMessage();
    case GetFamilyMessage::kCommand:
      return new GetFamilyMessage();
    default:
      LOG(WARNING) << "Unknown/unhandled netlink control message "
                   << header.cmd;
      return new UnknownControlMessage(header.cmd);
      break;
  }
  return nullptr;
}

}  // namespace shill.
