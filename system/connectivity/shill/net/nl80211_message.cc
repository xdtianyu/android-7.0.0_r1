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
//
// This code is derived from the 'iw' source code.  The copyright and license
// of that code is as follows:
//
// Copyright (c) 2007, 2008  Johannes Berg
// Copyright (c) 2007  Andy Lutomirski
// Copyright (c) 2007  Mike Kershaw
// Copyright (c) 2008-2009  Luis R. Rodriguez
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "shill/net/nl80211_message.h"

#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <endian.h>

#include "shill/net/attribute_list.h"
#include "shill/net/ieee80211.h"
#include "shill/net/netlink_attribute.h"
#include "shill/net/netlink_packet.h"
#include "shill/net/nl80211_attribute.h"  // For Nl80211AttributeMac

using base::Bind;
using base::LazyInstance;
using base::StringAppendF;
using std::map;
using std::string;
using std::vector;

namespace shill {

namespace {
LazyInstance<Nl80211MessageDataCollector> g_datacollector =
    LAZY_INSTANCE_INITIALIZER;
}  // namespace

const uint8_t Nl80211Frame::kMinimumFrameByteCount = 26;
const uint8_t Nl80211Frame::kFrameTypeMask = 0xfc;

const char Nl80211Message::kMessageTypeString[] = "nl80211";
map<uint16_t, string>* Nl80211Message::reason_code_string_ = nullptr;
map<uint16_t, string>* Nl80211Message::status_code_string_ = nullptr;
uint16_t Nl80211Message::nl80211_message_type_ = kIllegalMessageType;

// static
uint16_t Nl80211Message::GetMessageType() {
  return nl80211_message_type_;
}

// static
void Nl80211Message::SetMessageType(uint16_t message_type) {
  if (message_type == NetlinkMessage::kIllegalMessageType) {
    LOG(FATAL) << "Absolutely need a legal message type for Nl80211 messages.";
  }
  nl80211_message_type_ = message_type;
}

bool Nl80211Message::InitFromPacket(NetlinkPacket* packet,
                                    NetlinkMessage::MessageContext context) {
  if (!packet) {
    LOG(ERROR) << "Null |packet| parameter";
    return false;
  }

  if (!InitAndStripHeader(packet)) {
    return false;
  }

  return packet->ConsumeAttributes(
      Bind(&NetlinkAttribute::NewNl80211AttributeFromId, context), attributes_);

  // Convert integer values provided by the kernel (for example, from the
  // NL80211_ATTR_STATUS_CODE or NL80211_ATTR_REASON_CODE attribute) into
  // strings describing the status.
  if (!reason_code_string_) {
    reason_code_string_ = new map<uint16_t, string>;
    (*reason_code_string_)[IEEE_80211::kReasonCodeUnspecified] =
        "Unspecified reason";
    (*reason_code_string_)[
        IEEE_80211::kReasonCodePreviousAuthenticationInvalid] =
        "Previous authentication no longer valid";
    (*reason_code_string_)[IEEE_80211::kReasonCodeSenderHasLeft] =
        "Deauthentcated because sending STA is leaving (or has left) IBSS or "
        "ESS";
    (*reason_code_string_)[IEEE_80211::kReasonCodeInactivity] =
        "Disassociated due to inactivity";
    (*reason_code_string_)[IEEE_80211::kReasonCodeTooManySTAs] =
        "Disassociated because AP is unable to handle all currently associated "
        "STAs";
    (*reason_code_string_)[IEEE_80211::kReasonCodeNonAuthenticated] =
        "Class 2 frame received from nonauthenticated STA";
    (*reason_code_string_)[IEEE_80211::kReasonCodeNonAssociated] =
        "Class 3 frame received from nonassociated STA";
    (*reason_code_string_)[IEEE_80211::kReasonCodeDisassociatedHasLeft] =
        "Disassociated because sending STA is leaving (or has left) BSS";
    (*reason_code_string_)[
        IEEE_80211::kReasonCodeReassociationNotAuthenticated] =
        "STA requesting (re)association is not authenticated with responding "
        "STA";
    (*reason_code_string_)[IEEE_80211::kReasonCodeUnacceptablePowerCapability] =
        "Disassociated because the information in the Power Capability "
        "element is unacceptable";
    (*reason_code_string_)[
        IEEE_80211::kReasonCodeUnacceptableSupportedChannelInfo] =
        "Disassociated because the information in the Supported Channels "
        "element is unacceptable";
    (*reason_code_string_)[IEEE_80211::kReasonCodeInvalidInfoElement] =
        "Invalid information element, i.e., an information element defined in "
        "this standard for which the content does not meet the specifications "
        "in Clause 7";
    (*reason_code_string_)[IEEE_80211::kReasonCodeMICFailure] =
        "Message integrity code (MIC) failure";
    (*reason_code_string_)[IEEE_80211::kReasonCode4WayTimeout] =
        "4-Way Handshake timeout";
    (*reason_code_string_)[IEEE_80211::kReasonCodeGroupKeyHandshakeTimeout] =
        "Group Key Handshake timeout";
    (*reason_code_string_)[IEEE_80211::kReasonCodeDifferenIE] =
        "Information element in 4-Way Handshake different from "
        "(Re)Association Request/Probe Response/Beacon frame";
    (*reason_code_string_)[IEEE_80211::kReasonCodeGroupCipherInvalid] =
        "Invalid group cipher";
    (*reason_code_string_)[IEEE_80211::kReasonCodePairwiseCipherInvalid] =
        "Invalid pairwise cipher";
    (*reason_code_string_)[IEEE_80211::kReasonCodeAkmpInvalid] =
        "Invalid AKMP";
    (*reason_code_string_)[IEEE_80211::kReasonCodeUnsupportedRsnIeVersion] =
        "Unsupported RSN information element version";
    (*reason_code_string_)[IEEE_80211::kReasonCodeInvalidRsnIeCaps] =
        "Invalid RSN information element capabilities";
    (*reason_code_string_)[IEEE_80211::kReasonCode8021XAuth] =
        "IEEE 802.1X authentication failed";
    (*reason_code_string_)[IEEE_80211::kReasonCodeCipherSuiteRejected] =
        "Cipher suite rejected because of the security policy";
    (*reason_code_string_)[IEEE_80211::kReasonCodeUnspecifiedQoS] =
        "Disassociated for unspecified, QoS-related reason";
    (*reason_code_string_)[IEEE_80211::kReasonCodeQoSBandwidth] =
        "Disassociated because QoS AP lacks sufficient bandwidth for this "
        "QoS STA";
    (*reason_code_string_)[IEEE_80211::kReasonCodeiPoorConditions] =
        "Disassociated because excessive number of frames need to be "
        "acknowledged, but are not acknowledged due to AP transmissions "
        "and/or poor channel conditions";
    (*reason_code_string_)[IEEE_80211::kReasonCodeOutsideTxop] =
        "Disassociated because STA is transmitting outside the limits of its "
        "TXOPs";
    (*reason_code_string_)[IEEE_80211::kReasonCodeStaLeaving] =
        "Requested from peer STA as the STA is leaving the BSS (or resetting)";
    (*reason_code_string_)[IEEE_80211::kReasonCodeUnacceptableMechanism] =
        "Requested from peer STA as it does not want to use the mechanism";
    (*reason_code_string_)[IEEE_80211::kReasonCodeSetupRequired] =
        "Requested from peer STA as the STA received frames using the "
        "mechanism for which a setup is required";
    (*reason_code_string_)[IEEE_80211::kReasonCodeTimeout] =
        "Requested from peer STA due to timeout";
    (*reason_code_string_)[IEEE_80211::kReasonCodeCipherSuiteNotSupported] =
        "Peer STA does not support the requested cipher suite";
    (*reason_code_string_)[IEEE_80211::kReasonCodeInvalid] = "<INVALID REASON>";
  }

  if (!status_code_string_) {
    status_code_string_ = new map<uint16_t, string>;
    (*status_code_string_)[IEEE_80211::kStatusCodeSuccessful] = "Successful";
    (*status_code_string_)[IEEE_80211::kStatusCodeFailure] =
        "Unspecified failure";
    (*status_code_string_)[IEEE_80211::kStatusCodeAllCapabilitiesNotSupported] =
        "Cannot support all requested capabilities in the capability "
        "information field";
    (*status_code_string_)[IEEE_80211::kStatusCodeCantConfirmAssociation] =
        "Reassociation denied due to inability to confirm that association "
        "exists";
    (*status_code_string_)[IEEE_80211::kStatusCodeAssociationDenied] =
        "Association denied due to reason outside the scope of this standard";
    (*status_code_string_)[
        IEEE_80211::kStatusCodeAuthenticationUnsupported] =
        "Responding station does not support the specified authentication "
        "algorithm";
    (*status_code_string_)[IEEE_80211::kStatusCodeOutOfSequence] =
        "Received an authentication frame with authentication transaction "
        "sequence number out of expected sequence";
    (*status_code_string_)[IEEE_80211::kStatusCodeChallengeFailure] =
        "Authentication rejected because of challenge failure";
    (*status_code_string_)[IEEE_80211::kStatusCodeFrameTimeout] =
        "Authentication rejected due to timeout waiting for next frame in "
        "sequence";
    (*status_code_string_)[IEEE_80211::kStatusCodeMaxSta] =
        "Association denied because AP is unable to handle additional "
        "associated STA";
    (*status_code_string_)[IEEE_80211::kStatusCodeDataRateUnsupported] =
        "Association denied due to requesting station not supporting all of "
        "the data rates in the BSSBasicRateSet parameter";
    (*status_code_string_)[IEEE_80211::kStatusCodeShortPreambleUnsupported] =
        "Association denied due to requesting station not supporting the "
        "short preamble option";
    (*status_code_string_)[IEEE_80211::kStatusCodePbccUnsupported] =
        "Association denied due to requesting station not supporting the PBCC "
        "modulation option";
    (*status_code_string_)[
        IEEE_80211::kStatusCodeChannelAgilityUnsupported] =
        "Association denied due to requesting station not supporting the "
        "channel agility option";
    (*status_code_string_)[IEEE_80211::kStatusCodeNeedSpectrumManagement] =
        "Association request rejected because Spectrum Management capability "
        "is required";
    (*status_code_string_)[
        IEEE_80211::kStatusCodeUnacceptablePowerCapability] =
        "Association request rejected because the information in the Power "
        "Capability element is unacceptable";
    (*status_code_string_)[
        IEEE_80211::kStatusCodeUnacceptableSupportedChannelInfo] =
        "Association request rejected because the information in the "
        "Supported Channels element is unacceptable";
    (*status_code_string_)[IEEE_80211::kStatusCodeShortTimeSlotRequired] =
        "Association request rejected due to requesting station not "
        "supporting the Short Slot Time option";
    (*status_code_string_)[IEEE_80211::kStatusCodeDssOfdmRequired] =
        "Association request rejected due to requesting station not "
        "supporting the DSSS-OFDM option";
    (*status_code_string_)[IEEE_80211::kStatusCodeQosFailure] =
        "Unspecified, QoS related failure";
    (*status_code_string_)[
        IEEE_80211::kStatusCodeInsufficientBandwithForQsta] =
        "Association denied due to QAP having insufficient bandwidth to handle "
        "another QSTA";
    (*status_code_string_)[IEEE_80211::kStatusCodePoorConditions] =
        "Association denied due to poor channel conditions";
    (*status_code_string_)[IEEE_80211::kStatusCodeQosNotSupported] =
        "Association (with QoS BSS) denied due to requesting station not "
        "supporting the QoS facility";
    (*status_code_string_)[IEEE_80211::kStatusCodeDeclined] =
        "The request has been declined";
    (*status_code_string_)[IEEE_80211::kStatusCodeInvalidParameterValues] =
        "The request has not been successful as one or more parameters have "
        "invalid values";
    (*status_code_string_)[IEEE_80211::kStatusCodeCannotBeHonored] =
        "The TS has not been created because the request cannot be honored. "
        "However, a suggested Tspec is provided so that the initiating QSTA "
        "may attempt to send another TS with the suggested changes to the "
        "TSpec";
    (*status_code_string_)[IEEE_80211::kStatusCodeInvalidInfoElement] =
        "Invalid Information Element";
    (*status_code_string_)[IEEE_80211::kStatusCodeGroupCipherInvalid] =
        "Invalid Group Cipher";
    (*status_code_string_)[IEEE_80211::kStatusCodePairwiseCipherInvalid] =
        "Invalid Pairwise Cipher";
    (*status_code_string_)[IEEE_80211::kStatusCodeAkmpInvalid] = "Invalid AKMP";
    (*status_code_string_)[IEEE_80211::kStatusCodeUnsupportedRsnIeVersion] =
        "Unsupported RSN Information Element version";
    (*status_code_string_)[IEEE_80211::kStatusCodeInvalidRsnIeCaps] =
        "Invalid RSN Information Element Capabilities";
    (*status_code_string_)[IEEE_80211::kStatusCodeCipherSuiteRejected] =
        "Cipher suite is rejected per security policy";
    (*status_code_string_)[IEEE_80211::kStatusCodeTsDelayNotMet] =
        "The TS has not been created. However, the HC may be capable of "
        "creating a TS, in response to a request, after the time indicated in "
        "the TS Delay element";
    (*status_code_string_)[IEEE_80211::kStatusCodeDirectLinkIllegal] =
        "Direct link is not allowed in the BSS by policy";
    (*status_code_string_)[IEEE_80211::kStatusCodeStaNotInBss] =
        "Destination STA is not present within this BSS";
    (*status_code_string_)[IEEE_80211::kStatusCodeStaNotInQsta] =
        "The destination STA is not a QoS STA";
    (*status_code_string_)[IEEE_80211::kStatusCodeExcessiveListenInterval] =
        "Association denied because Listen Interval is too large";
    (*status_code_string_)[IEEE_80211::kStatusCodeInvalid] = "<INVALID STATUS>";
  }

  return true;
}

// static
string Nl80211Message::StringFromReason(uint16_t status) {
  map<uint16_t, string>::const_iterator match;
  match = reason_code_string_->find(status);
  if (match == reason_code_string_->end()) {
    string output;
    if (status < IEEE_80211::kReasonCodeMax) {
      StringAppendF(&output, "<Reserved Reason:%u>", status);
    } else {
      StringAppendF(&output, "<Unknown Reason:%u>", status);
    }
    return output;
  }
  return match->second;
}

// static
string Nl80211Message::StringFromStatus(uint16_t status) {
  map<uint16_t, string>::const_iterator match;
  match = status_code_string_->find(status);
  if (match == status_code_string_->end()) {
    string output;
    if (status < IEEE_80211::kStatusCodeMax) {
      StringAppendF(&output, "<Reserved Status:%u>", status);
    } else {
      StringAppendF(&output, "<Unknown Status:%u>", status);
    }
    return output;
  }
  return match->second;
}

// Nl80211Frame

Nl80211Frame::Nl80211Frame(const ByteString& raw_frame)
  : frame_type_(kIllegalFrameType),
    reason_(std::numeric_limits<uint16_t>::max()),
    status_(std::numeric_limits<uint16_t>::max()),
    frame_(raw_frame) {
  const IEEE_80211::ieee80211_frame* frame =
      reinterpret_cast<const IEEE_80211::ieee80211_frame*>(
          frame_.GetConstData());

  // Now, let's populate the other stuff.
  if (frame_.GetLength() >= kMinimumFrameByteCount) {
    mac_from_ =
        Nl80211AttributeMac::StringFromMacAddress(&frame->destination_mac[0]);
    mac_to_ = Nl80211AttributeMac::StringFromMacAddress(&frame->source_mac[0]);
    frame_type_ = frame->frame_control & kFrameTypeMask;

    switch (frame_type_) {
    case kAssocResponseFrameType:
    case kReassocResponseFrameType:
      status_ = le16toh(frame->u.associate_response.status_code);
      break;

    case kAuthFrameType:
      status_ = le16toh(frame->u.authentiate_message.status_code);
      break;

    case kDisassocFrameType:
    case kDeauthFrameType:
      reason_ = le16toh(frame->u.deauthentiate_message.reason_code);
      break;

    default:
      break;
    }
  }
}

bool Nl80211Frame::ToString(string* output) const {
  if (!output) {
    LOG(ERROR) << "NULL |output|";
    return false;
  }

  if (frame_.IsEmpty()) {
    output->append(" [no frame]");
    return true;
  }

  if (frame_.GetLength() < kMinimumFrameByteCount) {
    output->append(" [invalid frame: ");
  } else {
    StringAppendF(output, " %s -> %s", mac_from_.c_str(), mac_to_.c_str());

    switch (frame_.GetConstData()[0] & kFrameTypeMask) {
    case kAssocResponseFrameType:
      StringAppendF(output, "; AssocResponse status: %u: %s",
                    status_,
                    Nl80211Message::StringFromStatus(status_).c_str());
      break;
    case kReassocResponseFrameType:
      StringAppendF(output, "; ReassocResponse status: %u: %s",
                    status_,
                    Nl80211Message::StringFromStatus(status_).c_str());
      break;
    case kAuthFrameType:
      StringAppendF(output, "; Auth status: %u: %s",
                    status_,
                    Nl80211Message::StringFromStatus(status_).c_str());
      break;

    case kDisassocFrameType:
      StringAppendF(output, "; Disassoc reason %u: %s",
                    reason_,
                    Nl80211Message::StringFromReason(reason_).c_str());
      break;
    case kDeauthFrameType:
      StringAppendF(output, "; Deauth reason %u: %s",
                    reason_,
                    Nl80211Message::StringFromReason(reason_).c_str());
      break;

    default:
      break;
    }
    output->append(" [frame: ");
  }

  const unsigned char* frame = frame_.GetConstData();
  for (size_t i = 0; i < frame_.GetLength(); ++i) {
    StringAppendF(output, "%02x, ", frame[i]);
  }
  output->append("]");

  return true;
}

bool Nl80211Frame::IsEqual(const Nl80211Frame& other) const {
  return frame_.Equals(other.frame_);
}

//
// Specific Nl80211Message types.
//

const uint8_t AssociateMessage::kCommand = NL80211_CMD_ASSOCIATE;
const char AssociateMessage::kCommandString[] = "NL80211_CMD_ASSOCIATE";

const uint8_t AuthenticateMessage::kCommand = NL80211_CMD_AUTHENTICATE;
const char AuthenticateMessage::kCommandString[] = "NL80211_CMD_AUTHENTICATE";

const uint8_t CancelRemainOnChannelMessage::kCommand =
  NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL;
const char CancelRemainOnChannelMessage::kCommandString[] =
  "NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL";

const uint8_t ConnectMessage::kCommand = NL80211_CMD_CONNECT;
const char ConnectMessage::kCommandString[] = "NL80211_CMD_CONNECT";

const uint8_t DeauthenticateMessage::kCommand = NL80211_CMD_DEAUTHENTICATE;
const char DeauthenticateMessage::kCommandString[] =
    "NL80211_CMD_DEAUTHENTICATE";

const uint8_t DeleteStationMessage::kCommand = NL80211_CMD_DEL_STATION;
const char DeleteStationMessage::kCommandString[] = "NL80211_CMD_DEL_STATION";

const uint8_t DisassociateMessage::kCommand = NL80211_CMD_DISASSOCIATE;
const char DisassociateMessage::kCommandString[] = "NL80211_CMD_DISASSOCIATE";

const uint8_t DisconnectMessage::kCommand = NL80211_CMD_DISCONNECT;
const char DisconnectMessage::kCommandString[] = "NL80211_CMD_DISCONNECT";

const uint8_t FrameTxStatusMessage::kCommand = NL80211_CMD_FRAME_TX_STATUS;
const char FrameTxStatusMessage::kCommandString[] =
    "NL80211_CMD_FRAME_TX_STATUS";

const uint8_t GetRegMessage::kCommand = NL80211_CMD_GET_REG;
const char GetRegMessage::kCommandString[] = "NL80211_CMD_GET_REG";

const uint8_t GetStationMessage::kCommand = NL80211_CMD_GET_STATION;
const char GetStationMessage::kCommandString[] = "NL80211_CMD_GET_STATION";

GetStationMessage::GetStationMessage()
    : Nl80211Message(kCommand, kCommandString) {
  attributes()->CreateAttribute(
      NL80211_ATTR_IFINDEX, Bind(&NetlinkAttribute::NewNl80211AttributeFromId,
                                 NetlinkMessage::MessageContext()));
  attributes()->CreateAttribute(
      NL80211_ATTR_MAC, Bind(&NetlinkAttribute::NewNl80211AttributeFromId,
                             NetlinkMessage::MessageContext()));
}

const uint8_t SetWakeOnPacketConnMessage::kCommand = NL80211_CMD_SET_WOWLAN;
const char SetWakeOnPacketConnMessage::kCommandString[] =
    "NL80211_CMD_SET_WOWLAN";

const uint8_t GetWakeOnPacketConnMessage::kCommand = NL80211_CMD_GET_WOWLAN;
const char GetWakeOnPacketConnMessage::kCommandString[] =
    "NL80211_CMD_GET_WOWLAN";

const uint8_t GetWiphyMessage::kCommand = NL80211_CMD_GET_WIPHY;
const char GetWiphyMessage::kCommandString[] = "NL80211_CMD_GET_WIPHY";

GetWiphyMessage::GetWiphyMessage() : Nl80211Message(kCommand, kCommandString) {
  attributes()->CreateAttribute(
      NL80211_ATTR_IFINDEX, Bind(&NetlinkAttribute::NewNl80211AttributeFromId,
                                 NetlinkMessage::MessageContext()));
}

const uint8_t JoinIbssMessage::kCommand = NL80211_CMD_JOIN_IBSS;
const char JoinIbssMessage::kCommandString[] = "NL80211_CMD_JOIN_IBSS";

const uint8_t MichaelMicFailureMessage::kCommand =
    NL80211_CMD_MICHAEL_MIC_FAILURE;
const char MichaelMicFailureMessage::kCommandString[] =
    "NL80211_CMD_MICHAEL_MIC_FAILURE";

const uint8_t NewScanResultsMessage::kCommand = NL80211_CMD_NEW_SCAN_RESULTS;
const char NewScanResultsMessage::kCommandString[] =
    "NL80211_CMD_NEW_SCAN_RESULTS";

const uint8_t NewStationMessage::kCommand = NL80211_CMD_NEW_STATION;
const char NewStationMessage::kCommandString[] = "NL80211_CMD_NEW_STATION";

const uint8_t NewWiphyMessage::kCommand = NL80211_CMD_NEW_WIPHY;
const char NewWiphyMessage::kCommandString[] = "NL80211_CMD_NEW_WIPHY";

const uint8_t NotifyCqmMessage::kCommand = NL80211_CMD_NOTIFY_CQM;
const char NotifyCqmMessage::kCommandString[] = "NL80211_CMD_NOTIFY_CQM";

const uint8_t PmksaCandidateMessage::kCommand = NL80211_ATTR_PMKSA_CANDIDATE;
const char PmksaCandidateMessage::kCommandString[] =
  "NL80211_ATTR_PMKSA_CANDIDATE";

const uint8_t RegBeaconHintMessage::kCommand = NL80211_CMD_REG_BEACON_HINT;
const char RegBeaconHintMessage::kCommandString[] =
    "NL80211_CMD_REG_BEACON_HINT";

const uint8_t RegChangeMessage::kCommand = NL80211_CMD_REG_CHANGE;
const char RegChangeMessage::kCommandString[] = "NL80211_CMD_REG_CHANGE";

const uint8_t RemainOnChannelMessage::kCommand = NL80211_CMD_REMAIN_ON_CHANNEL;
const char RemainOnChannelMessage::kCommandString[] =
    "NL80211_CMD_REMAIN_ON_CHANNEL";

const uint8_t RoamMessage::kCommand = NL80211_CMD_ROAM;
const char RoamMessage::kCommandString[] = "NL80211_CMD_ROAM";

const uint8_t ScanAbortedMessage::kCommand = NL80211_CMD_SCAN_ABORTED;
const char ScanAbortedMessage::kCommandString[] = "NL80211_CMD_SCAN_ABORTED";

const uint8_t GetScanMessage::kCommand = NL80211_CMD_GET_SCAN;
const char GetScanMessage::kCommandString[] = "NL80211_CMD_GET_SCAN";

GetScanMessage::GetScanMessage()
    : Nl80211Message(kCommand, kCommandString) {
  attributes()->CreateAttribute(
      NL80211_ATTR_IFINDEX, Bind(&NetlinkAttribute::NewNl80211AttributeFromId,
                                 NetlinkMessage::MessageContext()));
}

const uint8_t TriggerScanMessage::kCommand = NL80211_CMD_TRIGGER_SCAN;
const char TriggerScanMessage::kCommandString[] = "NL80211_CMD_TRIGGER_SCAN";

TriggerScanMessage::TriggerScanMessage()
    : Nl80211Message(kCommand, kCommandString) {
  attributes()->CreateAttribute(
      NL80211_ATTR_IFINDEX, Bind(&NetlinkAttribute::NewNl80211AttributeFromId,
                                 NetlinkMessage::MessageContext()));
}

const uint8_t UnprotDeauthenticateMessage::kCommand =
    NL80211_CMD_UNPROT_DEAUTHENTICATE;
const char UnprotDeauthenticateMessage::kCommandString[] =
    "NL80211_CMD_UNPROT_DEAUTHENTICATE";

const uint8_t UnprotDisassociateMessage::kCommand =
    NL80211_CMD_UNPROT_DISASSOCIATE;
const char UnprotDisassociateMessage::kCommandString[] =
    "NL80211_CMD_UNPROT_DISASSOCIATE";

GetInterfaceMessage::GetInterfaceMessage()
    : Nl80211Message(kCommand, kCommandString) {
  attributes()->CreateAttribute(
      NL80211_ATTR_IFINDEX, Bind(&NetlinkAttribute::NewNl80211AttributeFromId,
                                 NetlinkMessage::MessageContext()));
}

const uint8_t GetInterfaceMessage::kCommand = NL80211_CMD_GET_INTERFACE;
const char GetInterfaceMessage::kCommandString[] = "NL80211_CMD_GET_INTERFACE";

const uint8_t NewInterfaceMessage::kCommand = NL80211_CMD_NEW_INTERFACE;
const char NewInterfaceMessage::kCommandString[] = "NL80211_CMD_NEW_INTERFACE";

const uint8_t GetSurveyMessage::kCommand = NL80211_CMD_GET_SURVEY;
const char GetSurveyMessage::kCommandString[] = "NL80211_CMD_GET_SURVEY";

GetSurveyMessage::GetSurveyMessage()
    : Nl80211Message(kCommand, kCommandString) {
  attributes()->CreateAttribute(
      NL80211_ATTR_IFINDEX, Bind(&NetlinkAttribute::NewNl80211AttributeFromId,
                                 NetlinkMessage::MessageContext()));
  AddFlag(NLM_F_DUMP);
}

const uint8_t SurveyResultsMessage::kCommand = NL80211_CMD_NEW_SURVEY_RESULTS;
const char SurveyResultsMessage::kCommandString[] =
    "NL80211_CMD_NEW_SURVEY_RESULTS";

// static
NetlinkMessage* Nl80211Message::CreateMessage(const NetlinkPacket& packet) {
  genlmsghdr header;
  if (!packet.GetGenlMsgHdr(&header)) {
    LOG(ERROR) << "Could not read genl header.";
    return nullptr;
  }
  std::unique_ptr<NetlinkMessage> message;

  switch (header.cmd) {
    case AssociateMessage::kCommand:
      return new AssociateMessage();
    case AuthenticateMessage::kCommand:
      return new AuthenticateMessage();
    case CancelRemainOnChannelMessage::kCommand:
      return new CancelRemainOnChannelMessage();
    case ConnectMessage::kCommand:
      return new ConnectMessage();
    case DeauthenticateMessage::kCommand:
      return new DeauthenticateMessage();
    case DeleteStationMessage::kCommand:
      return new DeleteStationMessage();
    case DisassociateMessage::kCommand:
      return new DisassociateMessage();
    case DisconnectMessage::kCommand:
      return new DisconnectMessage();
    case FrameTxStatusMessage::kCommand:
      return new FrameTxStatusMessage();
    case GetInterfaceMessage::kCommand:
      return new GetInterfaceMessage();
    case GetWakeOnPacketConnMessage::kCommand:
      return new GetWakeOnPacketConnMessage();
    case GetRegMessage::kCommand:
      return new GetRegMessage();
    case GetStationMessage::kCommand:
      return new GetStationMessage();
    case GetWiphyMessage::kCommand:
      return new GetWiphyMessage();
    case JoinIbssMessage::kCommand:
      return new JoinIbssMessage();
    case MichaelMicFailureMessage::kCommand:
      return new MichaelMicFailureMessage();
    case NewInterfaceMessage::kCommand:
      return new NewInterfaceMessage();
    case NewScanResultsMessage::kCommand:
      return new NewScanResultsMessage();
    case NewStationMessage::kCommand:
      return new NewStationMessage();
    case NewWiphyMessage::kCommand:
      return new NewWiphyMessage();
    case NotifyCqmMessage::kCommand:
      return new NotifyCqmMessage();
    case PmksaCandidateMessage::kCommand:
      return new PmksaCandidateMessage();
    case RegBeaconHintMessage::kCommand:
      return new RegBeaconHintMessage();
    case RegChangeMessage::kCommand:
      return new RegChangeMessage();
    case RemainOnChannelMessage::kCommand:
      return new RemainOnChannelMessage();
    case RoamMessage::kCommand:
      return new RoamMessage();
    case SetWakeOnPacketConnMessage::kCommand:
      return new SetWakeOnPacketConnMessage();
    case ScanAbortedMessage::kCommand:
      return new ScanAbortedMessage();
    case TriggerScanMessage::kCommand:
      return new TriggerScanMessage();
    case UnprotDeauthenticateMessage::kCommand:
      return new UnprotDeauthenticateMessage();
    case UnprotDisassociateMessage::kCommand:
      return new UnprotDisassociateMessage();
    case GetSurveyMessage::kCommand:
      return new GetSurveyMessage();
    case SurveyResultsMessage::kCommand:
      return new SurveyResultsMessage();
    default:
      LOG(WARNING) << base::StringPrintf(
          "Unknown/unhandled netlink nl80211 message 0x%02x", header.cmd);
      return new UnknownNl80211Message(header.cmd);
      break;
  }
  return nullptr;
}

//
// Data Collector
//

Nl80211MessageDataCollector *
    Nl80211MessageDataCollector::GetInstance() {
  return g_datacollector.Pointer();
}

Nl80211MessageDataCollector::Nl80211MessageDataCollector() {
  need_to_print[NL80211_ATTR_PMKSA_CANDIDATE] = true;
  need_to_print[NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL] = true;
  need_to_print[NL80211_CMD_DEL_STATION] = true;
  need_to_print[NL80211_CMD_FRAME_TX_STATUS] = true;
  need_to_print[NL80211_CMD_JOIN_IBSS] = true;
  need_to_print[NL80211_CMD_MICHAEL_MIC_FAILURE] = true;
  need_to_print[NL80211_CMD_NEW_WIPHY] = true;
  need_to_print[NL80211_CMD_REG_BEACON_HINT] = true;
  need_to_print[NL80211_CMD_REG_CHANGE] = true;
  need_to_print[NL80211_CMD_REMAIN_ON_CHANNEL] = true;
  need_to_print[NL80211_CMD_ROAM] = true;
  need_to_print[NL80211_CMD_SCAN_ABORTED] = true;
  need_to_print[NL80211_CMD_UNPROT_DEAUTHENTICATE] = true;
  need_to_print[NL80211_CMD_UNPROT_DISASSOCIATE] = true;
}

void Nl80211MessageDataCollector::CollectDebugData(
    const Nl80211Message& message, const NetlinkPacket& packet) {
  map<uint8_t, bool>::const_iterator node;
  node = need_to_print.find(message.command());
  if (node == need_to_print.end() || !node->second)
    return;

  LOG(INFO) << "@@const unsigned char "
             << "k" << message.command_string()
             << "[] = {";

  const unsigned char* rawdata =
      reinterpret_cast<const unsigned char*>(&packet.GetNlMsgHeader());
  for (size_t i = 0; i < sizeof(nlmsghdr); ++i) {
    LOG(INFO) << "  0x"
               << std::hex << std::setfill('0') << std::setw(2)
               << + rawdata[i] << ",";
  }
  rawdata = packet.GetPayload().GetConstData();
  for (size_t i = 0; i < packet.GetPayload().GetLength(); ++i) {
    LOG(INFO) << "  0x"
               << std::hex << std::setfill('0') << std::setw(2)
               << + rawdata[i] << ",";
  }
  LOG(INFO) << "};";
  need_to_print[message.command()] = false;
}

}  // namespace shill.
