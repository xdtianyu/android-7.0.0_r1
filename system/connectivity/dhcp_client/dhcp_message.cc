//
// Copyright (C) 2015 The Android Open Source Project
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

#include "dhcp_client/dhcp_message.h"

#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <base/logging.h>

#include "dhcp_client/dhcp_options.h"
#include "dhcp_client/dhcp_options_writer.h"

using shill::ByteString;

namespace dhcp_client {

namespace {
const int kClientHardwareAddressLength = 16;
const int kServerNameLength = 64;
const int kBootFileLength = 128;
const uint32_t kMagicCookie = 0x63825363;
const size_t kDHCPMessageMaxLength = 548;
const size_t kDHCPMessageMinLength = 236;
const uint8_t kDHCPMessageBootRequest = 1;
const uint8_t kDHCPMessageBootReply = 2;

// Follow the naming in rfc2131 for this struct.
struct __attribute__((__packed__)) RawDHCPMessage {
  uint8_t op;
  uint8_t htype;
  uint8_t hlen;
  uint8_t hops;
  uint32_t xid;
  uint16_t secs;
  uint16_t flags;
  uint32_t ciaddr;
  uint32_t yiaddr;
  uint32_t siaddr;
  uint32_t giaddr;
  uint8_t chaddr[kClientHardwareAddressLength];
  uint8_t sname[kServerNameLength];
  uint8_t file[kBootFileLength];
  uint32_t cookie;
  uint8_t options[kDHCPOptionLength];
};
}  // namespace

DHCPMessage::DHCPMessage()
    : requested_ip_address_(0),
      lease_time_(0),
      message_type_(0),
      server_identifier_(0),
      renewal_time_(0),
      rebinding_time_(0) {
  options_map_.insert(std::make_pair(kDHCPOptionMessageType,
      ParserContext(new UInt8Parser(), &message_type_)));
  options_map_.insert(std::make_pair(kDHCPOptionLeaseTime,
      ParserContext(new UInt32Parser(), &lease_time_)));
  options_map_.insert(std::make_pair(kDHCPOptionMessage,
      ParserContext(new StringParser(), &error_message_)));
  options_map_.insert(std::make_pair(kDHCPOptionSubnetMask,
      ParserContext(new UInt32Parser(), &subnet_mask_)));
  options_map_.insert(std::make_pair(kDHCPOptionServerIdentifier,
      ParserContext(new UInt32Parser(), &server_identifier_)));
  options_map_.insert(std::make_pair(kDHCPOptionRenewalTime,
      ParserContext(new UInt32Parser(), &renewal_time_)));
  options_map_.insert(std::make_pair(kDHCPOptionRebindingTime,
      ParserContext(new UInt32Parser(), &rebinding_time_)));
  options_map_.insert(std::make_pair(kDHCPOptionDNSServer,
      ParserContext(new UInt32ListParser(), &dns_server_)));
  options_map_.insert(std::make_pair(kDHCPOptionRouter,
      ParserContext(new UInt32ListParser(), &router_)));
  options_map_.insert(std::make_pair(kDHCPOptionDomainName,
      ParserContext(new StringParser(), &domain_name_)));
  options_map_.insert(std::make_pair(kDHCPOptionVendorSpecificInformation,
      ParserContext(new ByteArrayParser(), &vendor_specific_info_)));
}

DHCPMessage::~DHCPMessage() {}

bool DHCPMessage::InitFromBuffer(const unsigned char* buffer,
                                 size_t length,
                                 DHCPMessage* message) {
  if (buffer == NULL) {
    LOG(ERROR) << "Invalid buffer address";
    return false;
  }
  if (length < kDHCPMessageMinLength || length > kDHCPMessageMaxLength) {
    LOG(ERROR) << "Invalid DHCP message length";
    return false;
  }
  const RawDHCPMessage* raw_message
      = reinterpret_cast<const RawDHCPMessage*>(buffer);
  size_t options_length = reinterpret_cast<const unsigned char*>(raw_message) +
      length - reinterpret_cast<const unsigned char*>(raw_message->options) + 1;
  message->opcode_ = raw_message->op;
  message->hardware_address_type_ = raw_message->htype;
  message->hardware_address_length_ = raw_message->hlen;
  if (message->hardware_address_length_ > kClientHardwareAddressLength) {
    LOG(ERROR) << "Invalid hardware address length";
    return false;
  }
  message->relay_hops_ = raw_message->hops;
  message->transaction_id_ = ntohl(raw_message->xid);
  message->seconds_ = ntohs(raw_message->secs);
  message->flags_ = ntohs(raw_message->flags);
  message->client_ip_address_ = ntohl(raw_message->ciaddr);
  message->your_ip_address_ = ntohl(raw_message->yiaddr);
  message->next_server_ip_address_ = ntohl(raw_message->siaddr);
  message->agent_ip_address_ = ntohl(raw_message->giaddr);
  message->cookie_ = ntohl(raw_message->cookie);
  message->client_hardware_address_ = ByteString(
      reinterpret_cast<const char*>(raw_message->chaddr),
      message->hardware_address_length_);
  message->servername_.assign(reinterpret_cast<const char*>(raw_message->sname),
                              kServerNameLength);
  message->bootfile_.assign(reinterpret_cast<const char*>(raw_message->file),
                            kBootFileLength);
  // Validate the DHCP Message
  if (!message->IsValid()) {
    return false;
  }
  if (!message->ParseDHCPOptions(raw_message->options, options_length)) {
    LOG(ERROR) << "Failed to parse DHCP options";
    return false;
  }
  return true;
}

bool DHCPMessage::ParseDHCPOptions(const uint8_t* options,
                                   size_t options_length) {
  // DHCP options are in TLV format.
  // T: tag, L: length, V: value(data)
  // RFC 1497, RFC 1533, RFC 2132
  const uint8_t* ptr = options;
  const uint8_t* end_ptr = options + options_length;
  std::set<uint8_t> options_set;
  while (ptr < end_ptr) {
    uint8_t option_code = *ptr++;
    int option_code_int = static_cast<int>(option_code);
    if (option_code == kDHCPOptionPad) {
      continue;
    } else if (option_code == kDHCPOptionEnd) {
      // We reach the end of the option field.
      // Validate the options before we return.
      return ContainsValidOptions(options_set);
    }
    if (ptr >= end_ptr) {
      LOG(ERROR) << "Failed to decode dhcp options, no option length field"
                    " for option: " << option_code_int;
      return false;
    }
    uint8_t option_length = *ptr++;
    if (ptr + option_length >= end_ptr) {
      LOG(ERROR) << "Failed to decode dhcp options, invalid option length field"
                    " for option: " << option_code_int;
      return false;
    }
    if (options_set.find(option_code) != options_set.end()) {
      LOG(ERROR) << "Found repeated DHCP option: " << option_code_int;
      return false;
    }
    // Here we find a valid DHCP option.
    auto it = options_map_.find(option_code);
    if (it != options_map_.end()) {
      ParserContext* context = &(it->second);
      if (!context->parser->GetOption(ptr, option_length, context->output)) {
        return false;
      }
      options_set.insert(option_code);
    } else {
      DLOG(INFO) << "Ignore DHCP option: " << option_code_int;
    }
    // Move to next tag.
    ptr += option_length;
  }
  // Reach the end of message without seeing kDHCPOptionEnd.
  LOG(ERROR) << "Broken DHCP options without END tag.";
  return false;
}

bool DHCPMessage::ContainsValidOptions(const std::set<uint8_t>& options_set) {
  // A DHCP message must contain option 53: DHCP Message Type.
  if (options_set.find(kDHCPOptionMessageType) == options_set.end()) {
    LOG(ERROR) << "Faied to find option 53: DHCP Message Type.";
    return false;
  }
  if (message_type_ != kDHCPMessageTypeOffer &&
      message_type_ != kDHCPMessageTypeAck &&
      message_type_ != kDHCPMessageTypeNak) {
    LOG(ERROR) << "Invalid DHCP Message Type: "
               << static_cast<int>(message_type_);
    return false;
  }
  // A DHCP Offer message must contain option 51: IP Address Lease Time.
  if (message_type_ == kDHCPMessageTypeOffer) {
    if (options_set.find(kDHCPOptionLeaseTime) == options_set.end()) {
      LOG(ERROR) << "Faied to find option 51: IP Address Lease Time";
      return false;
    }
  }
  // A message from DHCP server must contain option 54: Server Identifier.
  if (options_set.find(kDHCPOptionServerIdentifier) == options_set.end()) {
    LOG(ERROR) << "Faied to find option 54: Server Identifier.";
    return false;
  }
  return true;
}

bool DHCPMessage::IsValid() {
  if (opcode_ != kDHCPMessageBootReply) {
    LOG(ERROR) << "Invalid DHCP message op code";
    return false;
  }
  if (hardware_address_type_ != ARPHRD_ETHER) {
    LOG(ERROR) << "DHCP message device family id does not match";
    return false;
  }
  if (hardware_address_length_ != IFHWADDRLEN) {
    LOG(ERROR) <<
        "DHCP message device hardware address length does not match";
    return false;
  }
  // We have nothing to do with the 'hops' field.

  // The reply message from server should have the same xid we cached in client.
  // DHCP state machine will take charge of this checking.

  // According to RFC 2131, all secs field in reply messages should be 0.
  if (seconds_) {
    LOG(ERROR) << "Invalid DHCP message secs";
    return false;
  }

  // Check broadcast flags.
  // It should be 0 because we do not request broadcast reply.
  if (flags_) {
    LOG(ERROR) << "Invalid DHCP message flags";
    return false;
  }

  // We need to ensure the message contains the correct client hardware address.
  // DHCP state machine will take charge of this checking.

  // We do not use the bootfile field.
  if (cookie_ != kMagicCookie) {
    LOG(ERROR) << "DHCP message cookie does not match";
    return false;
  }
  return true;
}

bool DHCPMessage::Serialize(ByteString* data) const {
  RawDHCPMessage raw_message;
  raw_message.op = opcode_;
  raw_message.htype = hardware_address_type_;
  raw_message.hlen = hardware_address_length_;
  raw_message.hops = relay_hops_;
  raw_message.xid = htonl(transaction_id_);
  raw_message.secs = htons(seconds_);
  raw_message.flags = htons(flags_);
  raw_message.ciaddr = htonl(client_ip_address_);
  raw_message.yiaddr = htonl(your_ip_address_);
  raw_message.siaddr = htonl(next_server_ip_address_);
  raw_message.giaddr = htonl(agent_ip_address_);
  raw_message.cookie = htonl(cookie_);
  memcpy(raw_message.chaddr,
         client_hardware_address_.GetConstData(),
         hardware_address_length_);
  if (servername_.length() >= kServerNameLength) {
    LOG(ERROR) << "Invalid server name length: " << servername_.length();
    return false;
  }
  memcpy(raw_message.sname,
         servername_.c_str(),
         servername_.length());
  raw_message.sname[servername_.length()] = 0;
  if (bootfile_.length() >= kBootFileLength) {
    LOG(ERROR) << "Invalid boot file length: " << bootfile_.length();
    return false;
  }
  memcpy(raw_message.file,
         bootfile_.c_str(),
         bootfile_.length());
  raw_message.file[bootfile_.length()] = 0;
  data->Append(ByteString(reinterpret_cast<const char*>(&raw_message),
                          sizeof(raw_message) - kDHCPOptionLength));
  // Append DHCP options to the message.
  DHCPOptionsWriter* options_writer = DHCPOptionsWriter::GetInstance();
  if (options_writer->WriteUInt8Option(data,
                                       kDHCPOptionMessageType,
                                       message_type_) == -1) {
    LOG(ERROR) << "Failed to write message type option";
    return false;
  }
  if (requested_ip_address_ != 0) {
    if (options_writer->WriteUInt32Option(data,
                                          kDHCPOptionRequestedIPAddr,
                                          requested_ip_address_) == -1) {
      LOG(ERROR) << "Failed to write requested ip address option";
      return false;
    }
  }
  if (lease_time_ != 0) {
    if (options_writer->WriteUInt32Option(data,
                                          kDHCPOptionLeaseTime,
                                          lease_time_) == -1) {
      LOG(ERROR) << "Failed to write lease time option";
      return false;
    }
  }
  if (server_identifier_ != 0) {
    if (options_writer->WriteUInt32Option(data,
                                          kDHCPOptionServerIdentifier,
                                          server_identifier_) == -1) {
      LOG(ERROR) << "Failed to write server identifier option";
      return false;
    }
  }
  if (error_message_.size() != 0) {
    if (options_writer->WriteStringOption(data,
                                          kDHCPOptionMessage,
                                          error_message_) == -1) {
      LOG(ERROR) << "Failed to write error message option";
      return false;
    }
  }
  if (parameter_request_list_.size() != 0) {
    if (options_writer->WriteUInt8ListOption(data,
                                             kDHCPOptionParameterRequestList,
                                             parameter_request_list_) == -1) {
      LOG(ERROR) << "Failed to write parameter request list";
      return false;
    }
  }
  // TODO(nywang): Append other options.
  // Append end tag.
  if (options_writer->WriteEndTag(data) == -1) {
    LOG(ERROR) << "Failed to write DHCP options end tag";
    return false;
  }
  // Ensure we do not exceed the maximum length.
  if (data->GetLength() > kDHCPMessageMaxLength) {
    LOG(ERROR) << "DHCP message length exceeds the limit";
    return false;
  }
  return true;
}

uint16_t DHCPMessage::ComputeChecksum(const uint8_t* data, size_t len) {
  uint32_t sum = 0;

  while (len > 1) {
    sum += static_cast<uint32_t>(data[0]) << 8 | static_cast<uint32_t>(data[1]);
    data += 2;
    len -= 2;
  }
  if (len == 1) {
    sum += static_cast<uint32_t>(*data) << 8;
  }
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);

  return ~static_cast<uint16_t>(sum);
}

void DHCPMessage::SetClientIdentifier(
    const ByteString& client_identifier) {
  client_identifier_ = client_identifier;
}

void DHCPMessage::SetClientIPAddress(uint32_t client_ip_address) {
  client_ip_address_ = client_ip_address;
}

void DHCPMessage::SetClientHardwareAddress(
    const ByteString& client_hardware_address) {
  client_hardware_address_ = client_hardware_address;
}

void DHCPMessage::SetErrorMessage(const std::string& error_message) {
  error_message_ = error_message;
}

void DHCPMessage::SetLeaseTime(uint32_t lease_time) {
  lease_time_ = lease_time;
}

void DHCPMessage::SetMessageType(uint8_t message_type) {
  message_type_ = message_type;
}

void DHCPMessage::SetParameterRequestList(
    const std::vector<uint8_t>& parameter_request_list) {
  parameter_request_list_ = parameter_request_list;
}

void DHCPMessage::SetRequestedIpAddress(uint32_t requested_ip_address) {
  requested_ip_address_ = requested_ip_address;
}

void DHCPMessage::SetServerIdentifier(uint32_t server_identifier) {
  server_identifier_ = server_identifier;
}

void DHCPMessage::SetTransactionID(uint32_t transaction_id) {
  transaction_id_ = transaction_id;
}

void DHCPMessage::SetVendorSpecificInfo(
    const shill::ByteString& vendor_specific_info) {
  vendor_specific_info_ = vendor_specific_info;
}

void DHCPMessage::InitRequest(DHCPMessage* message) {
  message->opcode_ = kDHCPMessageBootRequest;
  message->hardware_address_type_ = ARPHRD_ETHER;
  message->hardware_address_length_ = IFHWADDRLEN;
  message->relay_hops_ = 0;
  // Seconds since DHCP process started.
  // 0 is also valid according to RFC 2131.
  message->seconds_ = 0;
  // Only firewire (IEEE 1394) and InfiniBand interfaces
  // require broadcast flag.
  message->flags_ =  0;
  // Should be zero in client's messages.
  message->your_ip_address_ = 0;
  // Should be zero in client's messages.
  message->next_server_ip_address_ = 0;
  // Should be zero in client's messages.
  message->agent_ip_address_ = 0;
  message->cookie_ = kMagicCookie;
}

}  // namespace dhcp_client
