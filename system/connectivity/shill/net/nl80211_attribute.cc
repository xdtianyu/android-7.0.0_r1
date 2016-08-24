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

#include "shill/net/nl80211_attribute.h"

#include <string>
#include <utility>

#include <base/bind.h>
#include <base/format_macros.h>
#include <base/logging.h>
#include <base/strings/stringprintf.h>

#include "shill/net/ieee80211.h"
#include "shill/net/netlink_message.h"

using base::Bind;
using base::StringAppendF;
using base::StringPrintf;
using std::string;

namespace shill {

const int Nl80211AttributeCookie::kName = NL80211_ATTR_COOKIE;
const char Nl80211AttributeCookie::kNameString[] = "NL80211_ATTR_COOKIE";

const int Nl80211AttributeBss::kName = NL80211_ATTR_BSS;
const char Nl80211AttributeBss::kNameString[] = "NL80211_ATTR_BSS";
const int Nl80211AttributeBss::kChannelsAttributeId =
    IEEE_80211::kElemIdChannels;
const int Nl80211AttributeBss::kChallengeTextAttributeId =
    IEEE_80211::kElemIdChallengeText;
const int Nl80211AttributeBss::kCountryInfoAttributeId =
    IEEE_80211::kElemIdCountry;
const int Nl80211AttributeBss::kDSParameterSetAttributeId =
    IEEE_80211::kElemIdDSParameterSet;
const int Nl80211AttributeBss::kErpAttributeId =
    IEEE_80211::kElemIdErp;
const int Nl80211AttributeBss::kExtendedRatesAttributeId =
    IEEE_80211::kElemIdExtendedRates;
const int Nl80211AttributeBss::kHtCapAttributeId =
    IEEE_80211::kElemIdHTCap;
const int Nl80211AttributeBss::kHtInfoAttributeId =
    IEEE_80211::kElemIdHTInfo;
const int Nl80211AttributeBss::kPowerCapabilityAttributeId =
    IEEE_80211::kElemIdPowerCapability;
const int Nl80211AttributeBss::kPowerConstraintAttributeId =
    IEEE_80211::kElemIdPowerConstraint;
const int Nl80211AttributeBss::kRequestAttributeId =
    IEEE_80211::kElemIdRequest;
const int Nl80211AttributeBss::kRsnAttributeId =
    IEEE_80211::kElemIdRSN;
const int Nl80211AttributeBss::kSsidAttributeId =
    IEEE_80211::kElemIdSsid;
const int Nl80211AttributeBss::kSupportedRatesAttributeId =
    IEEE_80211::kElemIdSupportedRates;
const int Nl80211AttributeBss::kTpcReportAttributeId =
    IEEE_80211::kElemIdTpcReport;
const int Nl80211AttributeBss::kVendorSpecificAttributeId =
    IEEE_80211::kElemIdVendor;
const int Nl80211AttributeBss::kVhtCapAttributeId =
    IEEE_80211::kElemIdVHTCap;
const int Nl80211AttributeBss::kVhtInfoAttributeId =
    IEEE_80211::kElemIdVHTOperation;

static const char kSsidString[] = "SSID";
static const char kRatesString[] = "Rates";
static const char kHtCapString[] = "HTCapabilities";
static const char kHtOperString[] = "HTOperation";
static const char kVhtCapString[] = "VHTCapabilities";
static const char kVhtOperString[] = "VHTOperation";

Nl80211AttributeBss::Nl80211AttributeBss()
    : NetlinkNestedAttribute(kName, kNameString) {
  nested_template_.insert(
      AttrDataPair(__NL80211_BSS_INVALID,
                   NestedData(kTypeU32, "__NL80211_BSS_INVALID", false)));
  nested_template_.insert(AttrDataPair(
      NL80211_BSS_BSSID, NestedData(kTypeRaw, "NL80211_BSS_BSSID", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_BSS_FREQUENCY,
                   NestedData(kTypeU32, "NL80211_BSS_FREQUENCY", false)));
  nested_template_.insert(AttrDataPair(
      NL80211_BSS_TSF, NestedData(kTypeU64, "NL80211_BSS_TSF", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_BSS_BEACON_INTERVAL,
                   NestedData(kTypeU16, "NL80211_BSS_BEACON_INTERVAL", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_BSS_CAPABILITY,
                   NestedData(kTypeU16, "NL80211_BSS_CAPABILITY", false)));
  nested_template_.insert(AttrDataPair(
      NL80211_BSS_INFORMATION_ELEMENTS,
      NestedData(kTypeRaw, "NL80211_BSS_INFORMATION_ELEMENTS", false,
                 Bind(&Nl80211AttributeBss::ParseInformationElements))));
  nested_template_.insert(
      AttrDataPair(NL80211_BSS_SIGNAL_MBM,
                   NestedData(kTypeU32, "NL80211_BSS_SIGNAL_MBM", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_BSS_SIGNAL_UNSPEC,
                   NestedData(kTypeU8, "NL80211_BSS_SIGNAL_UNSPEC", false)));
  nested_template_.insert(AttrDataPair(
      NL80211_BSS_STATUS, NestedData(kTypeU32, "NL80211_BSS_STATUS", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_BSS_SEEN_MS_AGO,
                   NestedData(kTypeU32, "NL80211_BSS_SEEN_MS_AGO", false)));
}

bool Nl80211AttributeBss::ParseInformationElements(
    AttributeList* attribute_list, size_t id, const string& attribute_name,
    ByteString data) {
  if (!attribute_list) {
    LOG(ERROR) << "NULL |attribute_list| parameter";
    return false;
  }
  attribute_list->CreateNestedAttribute(id, attribute_name.c_str());

  // Now, handle the nested data.
  AttributeListRefPtr ie_attribute;
  if (!attribute_list->GetNestedAttributeList(id, &ie_attribute) ||
      !ie_attribute) {
    LOG(ERROR) << "Couldn't get attribute " << attribute_name
               << " which we just created.";
    return false;
  }
  const uint8_t* sub_attribute = data.GetConstData();
  const uint8_t* end = sub_attribute + data.GetLength();
  const int kHeaderBytes = 2;
  while (end - sub_attribute > kHeaderBytes) {
    uint8_t type = sub_attribute[0];
    uint8_t payload_bytes = sub_attribute[1];
    const uint8_t* payload = &sub_attribute[kHeaderBytes];
    if (payload + payload_bytes > end) {
      LOG(ERROR) << "Found malformed IE data.";
      return false;
    }
    // See http://dox.ipxe.org/ieee80211_8h_source.html for more info on types
    // and data inside information elements.
    switch (type) {
      case kSsidAttributeId: {
        ie_attribute->CreateSsidAttribute(type, kSsidString);
        if (payload_bytes == 0) {
          ie_attribute->SetStringAttributeValue(type, "");
        } else {
          ie_attribute->SetStringAttributeValue(
              type,
              string(reinterpret_cast<const char*>(payload), payload_bytes));
        }
        break;
      }
      case kSupportedRatesAttributeId:
      case kExtendedRatesAttributeId: {
        ie_attribute->CreateNestedAttribute(type, kRatesString);
        AttributeListRefPtr rates_attribute;
        if (!ie_attribute->GetNestedAttributeList(type, &rates_attribute) ||
            !rates_attribute) {
          LOG(ERROR) << "Couldn't get attribute " << attribute_name
                     << " which we just created.";
          break;
        }
        // Extract each rate, add it to the list.
        for (size_t i = 0; i < payload_bytes; ++i) {
          string rate_name = StringPrintf("Rate-%zu", i);
          rates_attribute->CreateU8Attribute(i, rate_name.c_str());
          rates_attribute->SetU8AttributeValue(i, payload[i]);
        }
        ie_attribute->SetNestedAttributeHasAValue(type);
        break;
      }
      case kHtCapAttributeId: {
        ie_attribute->CreateRawAttribute(type, kHtCapString);
        ie_attribute->SetRawAttributeValue(
            type,
            ByteString(
                reinterpret_cast<const char*>(payload), payload_bytes));
        break;
      }
      case kHtInfoAttributeId: {
        ie_attribute->CreateRawAttribute(type, kHtOperString);
        ie_attribute->SetRawAttributeValue(
            type,
            ByteString(
                reinterpret_cast<const char*>(payload), payload_bytes));
        break;
      }
      case kVhtCapAttributeId: {
        ie_attribute->CreateRawAttribute(type, kVhtCapString);
        ie_attribute->SetRawAttributeValue(
            type,
            ByteString(
                reinterpret_cast<const char*>(payload), payload_bytes));
        break;
      }
      case kVhtInfoAttributeId: {
        ie_attribute->CreateRawAttribute(type, kVhtOperString);
        ie_attribute->SetRawAttributeValue(
            type,
            ByteString(
                reinterpret_cast<const char*>(payload), payload_bytes));
        break;
      }
      case kDSParameterSetAttributeId:
      case kCountryInfoAttributeId:
      case kRequestAttributeId:
      case kChallengeTextAttributeId:
      case kPowerConstraintAttributeId:
      case kPowerCapabilityAttributeId:
      case kTpcReportAttributeId:
      case kChannelsAttributeId:
      case kErpAttributeId:
      case kRsnAttributeId:
      case kVendorSpecificAttributeId:
      default:
        break;
    }
    sub_attribute += kHeaderBytes + payload_bytes;
  }
  attribute_list->SetNestedAttributeHasAValue(id);
  return true;
}

const int Nl80211AttributeWiphyBands::kName = NL80211_ATTR_WIPHY_BANDS;
const char Nl80211AttributeWiphyBands::kNameString[] =
    "NL80211_ATTR_WIPHY_BANDS";

Nl80211AttributeWiphyBands::Nl80211AttributeWiphyBands()
    : NetlinkNestedAttribute(kName, kNameString) {
  // Frequencies
  NestedData freq(kTypeNested, "NL80211_BAND_ATTR_FREQ", true);
  freq.deeper_nesting.insert(AttrDataPair(
      __NL80211_FREQUENCY_ATTR_INVALID,
      NestedData(kTypeU32, "__NL80211_FREQUENCY_ATTR_INVALID", false)));
  freq.deeper_nesting.insert(
      AttrDataPair(NL80211_FREQUENCY_ATTR_FREQ,
                   NestedData(kTypeU32, "NL80211_FREQUENCY_ATTR_FREQ", false)));
  freq.deeper_nesting.insert(AttrDataPair(
      NL80211_FREQUENCY_ATTR_DISABLED,
      NestedData(kTypeFlag, "NL80211_FREQUENCY_ATTR_DISABLED", false)));
  freq.deeper_nesting.insert(AttrDataPair(
      NL80211_FREQUENCY_ATTR_PASSIVE_SCAN,
      NestedData(kTypeFlag, "NL80211_FREQUENCY_ATTR_PASSIVE_SCAN", false)));
  freq.deeper_nesting.insert(AttrDataPair(
      NL80211_FREQUENCY_ATTR_NO_IBSS,
      NestedData(kTypeFlag, "NL80211_FREQUENCY_ATTR_NO_IBSS", false)));
  freq.deeper_nesting.insert(AttrDataPair(
      NL80211_FREQUENCY_ATTR_RADAR,
      NestedData(kTypeFlag, "NL80211_FREQUENCY_ATTR_RADAR", false)));
  freq.deeper_nesting.insert(AttrDataPair(
      NL80211_FREQUENCY_ATTR_MAX_TX_POWER,
      NestedData(kTypeU32, "NL80211_FREQUENCY_ATTR_MAX_TX_POWER", false)));

  NestedData freqs(kTypeNested, "NL80211_BAND_ATTR_FREQS", false);
  freqs.deeper_nesting.insert(AttrDataPair(kArrayAttrEnumVal, freq));

  // Rates
  NestedData rate(kTypeNested, "NL80211_BAND_ATTR_RATE", true);
  rate.deeper_nesting.insert(AttrDataPair(
      __NL80211_BITRATE_ATTR_INVALID,
      NestedData(kTypeU32, "__NL80211_BITRATE_ATTR_INVALID", false)));
  rate.deeper_nesting.insert(
      AttrDataPair(NL80211_BITRATE_ATTR_RATE,
                   NestedData(kTypeU32, "NL80211_BITRATE_ATTR_RATE", false)));
  rate.deeper_nesting.insert(AttrDataPair(
      NL80211_BITRATE_ATTR_2GHZ_SHORTPREAMBLE,
      NestedData(kTypeFlag, "NL80211_BITRATE_ATTR_2GHZ_SHORTPREAMBLE", false)));

  NestedData rates(kTypeNested, "NL80211_BAND_ATTR_RATES", true);
  rates.deeper_nesting.insert(AttrDataPair(kArrayAttrEnumVal, rate));

  // Main body of attribute
  NestedData bands(kTypeNested, "NL80211_ATTR_BANDS", true);
  bands.deeper_nesting.insert(
      AttrDataPair(
          __NL80211_BAND_ATTR_INVALID,
          NestedData(kTypeU32, "__NL80211_BAND_ATTR_INVALID,", false)));
  bands.deeper_nesting.insert(AttrDataPair(NL80211_BAND_ATTR_FREQS, freqs));
  bands.deeper_nesting.insert(AttrDataPair(NL80211_BAND_ATTR_RATES, rates));
  bands.deeper_nesting.insert(AttrDataPair(
      NL80211_BAND_ATTR_HT_MCS_SET,
      NestedData(kTypeRaw, "NL80211_BAND_ATTR_HT_MCS_SET", false)));
  bands.deeper_nesting.insert(
      AttrDataPair(NL80211_BAND_ATTR_HT_CAPA,
                   NestedData(kTypeU16, "NL80211_BAND_ATTR_HT_CAPA", false)));
  bands.deeper_nesting.insert(AttrDataPair(
      NL80211_BAND_ATTR_HT_AMPDU_FACTOR,
      NestedData(kTypeU8, "NL80211_BAND_ATTR_HT_AMPDU_FACTOR", false)));
  bands.deeper_nesting.insert(AttrDataPair(
      NL80211_BAND_ATTR_HT_AMPDU_DENSITY,
      NestedData(kTypeU8, "NL80211_BAND_ATTR_HT_AMPDU_DENSITY", false)));

  nested_template_.insert(AttrDataPair(kArrayAttrEnumVal, bands));
}

#if !defined(DISABLE_WAKE_ON_WIFI)
const int Nl80211AttributeWowlanTriggers::kName = NL80211_ATTR_WOWLAN_TRIGGERS;
const char Nl80211AttributeWowlanTriggers::kNameString[] =
    "NL80211_ATTR_WOWLAN_TRIGGERS";

Nl80211AttributeWowlanTriggers::Nl80211AttributeWowlanTriggers(
    NetlinkMessage::MessageContext context)
    : NetlinkNestedAttribute(kName, kNameString) {
  // Pattern matching trigger attribute.
  if (context.nl80211_cmd == NL80211_CMD_SET_WOWLAN && context.is_broadcast) {
    // If this attribute occurs in a wakeup report, parse
    // NL80211_WOWLAN_TRIG_PKT_PATTERN as a U32 reporting the index of the
    // pattern that caused the wake.
    nested_template_.insert(AttrDataPair(
        NL80211_WOWLAN_TRIG_PKT_PATTERN,
        NestedData(kTypeU32, "NL80211_WOWLAN_TRIG_PKT_PATTERN", false)));
  } else {
    // Otherwise, this attribute is meant to program the NIC, so parse it as
    // a nested attribute.
    NestedData patterns(kTypeNested, "NL80211_WOWLAN_TRIG_PKT_PATTERN", false);
    NestedData individual_pattern(kTypeNested, "Pattern Match Info", true);
    individual_pattern.deeper_nesting.insert(
        AttrDataPair(NL80211_PKTPAT_MASK,
                     NestedData(kTypeRaw, "NL80211_PKTPAT_MASK", false)));
    individual_pattern.deeper_nesting.insert(
        AttrDataPair(NL80211_PKTPAT_PATTERN,
                     NestedData(kTypeRaw, "NL80211_PKTPAT_PATTERN", false)));
    individual_pattern.deeper_nesting.insert(
        AttrDataPair(NL80211_PKTPAT_OFFSET,
                     NestedData(kTypeU32, "NL80211_PKTPAT_OFFSET", false)));
    patterns.deeper_nesting.insert(
        AttrDataPair(kArrayAttrEnumVal, individual_pattern));
    nested_template_.insert(
        AttrDataPair(NL80211_WOWLAN_TRIG_PKT_PATTERN, patterns));
  }

  // Net detect SSID matching trigger attribute.
  NestedData net_detect(kTypeNested, "NL80211_WOWLAN_TRIG_NET_DETECT", false);
  NestedData scan_freqs(kTypeNested, "NL80211_ATTR_SCAN_FREQUENCIES", true);
  scan_freqs.deeper_nesting.insert(
      AttrDataPair(kArrayAttrEnumVal,
                   NestedData(kTypeU32, "Frequency match", false)));
  net_detect.deeper_nesting.insert(
      AttrDataPair(NL80211_ATTR_SCAN_FREQUENCIES, scan_freqs));
  net_detect.deeper_nesting.insert(AttrDataPair(
      NL80211_ATTR_SCHED_SCAN_INTERVAL,
      NestedData(kTypeU32, "NL80211_ATTR_SCHED_SCAN_INTERVAL", false)));
  NestedData scan_matches(kTypeNested, "NL80211_ATTR_SCHED_SCAN_MATCH", false);
  NestedData individual_scan_match(
      kTypeNested, "NL80211_ATTR_SCHED_SCAN_MATCH_SINGLE", true);
  individual_scan_match.deeper_nesting.insert(AttrDataPair(
      NL80211_SCHED_SCAN_MATCH_ATTR_SSID,
      NestedData(kTypeRaw, "NL80211_SCHED_SCAN_MATCH_ATTR_SSID", false)));
  scan_matches.deeper_nesting.insert(
      AttrDataPair(kArrayAttrEnumVal, individual_scan_match));
  net_detect.deeper_nesting.insert(
      AttrDataPair(NL80211_ATTR_SCHED_SCAN_MATCH, scan_matches));

  // Net detect results attribute
  NestedData net_detect_results(
      kTypeNested, "NL80211_WOWLAN_TRIG_NET_DETECT_RESULTS", false);
  NestedData single_result(kTypeNested, "NL80211_WOWLAN_TRIG_NET_DETECT_RESULT",
                           true);
  NestedData freq_list(kTypeNested, "NL80211_ATTR_SCAN_FREQUENCIES", false);
  freq_list.deeper_nesting.insert(
      AttrDataPair(kArrayAttrEnumVal,
                   NestedData(kTypeU32, "Frequency match", true)));
  single_result.deeper_nesting.insert(
      AttrDataPair(NL80211_ATTR_SCAN_FREQUENCIES, freq_list));
  single_result.deeper_nesting.insert(AttrDataPair(
      NL80211_ATTR_SSID, NestedData(kTypeRaw, "NL80211_ATTR_SSID", false)));
  net_detect_results.deeper_nesting.insert(
      AttrDataPair(kArrayAttrEnumVal, single_result));

  // Main body of the triggers attribute.
  nested_template_.insert(AttrDataPair(
      NL80211_WOWLAN_TRIG_DISCONNECT,
      NestedData(kTypeFlag, "NL80211_WOWLAN_TRIG_DISCONNECT", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_WOWLAN_TRIG_NET_DETECT, net_detect));
  nested_template_.insert(
      AttrDataPair(NL80211_WOWLAN_TRIG_NET_DETECT_RESULTS, net_detect_results));
}

const int Nl80211AttributeWowlanTriggersSupported::kName =
    NL80211_ATTR_WOWLAN_TRIGGERS_SUPPORTED;
const char Nl80211AttributeWowlanTriggersSupported::kNameString[] =
    "NL80211_ATTR_WOWLAN_TRIGGERS_SUPPORTED";

Nl80211AttributeWowlanTriggersSupported::
    Nl80211AttributeWowlanTriggersSupported()
    : NetlinkNestedAttribute(kName, kNameString) {
  nested_template_.insert(AttrDataPair(
      NL80211_WOWLAN_TRIG_DISCONNECT,
      NestedData(kTypeFlag, "NL80211_WOWLAN_TRIG_DISCONNECT", false)));
  nested_template_.insert(AttrDataPair(
      NL80211_WOWLAN_TRIG_PKT_PATTERN,
      NestedData(kTypeRaw, "NL80211_WOWLAN_TRIG_PKT_PATTERN", false)));
  nested_template_.insert(AttrDataPair(
      NL80211_WOWLAN_TRIG_NET_DETECT,
      NestedData(kTypeU32, "NL80211_WOWLAN_TRIG_NET_DETECT", false)));
}
#endif  // DISABLE_WAKE_ON_WIFI

const int Nl80211AttributeCipherSuites::kName = NL80211_ATTR_CIPHER_SUITES;
const char Nl80211AttributeCipherSuites::kNameString[] =
    "NL80211_ATTR_CIPHER_SUITES";

const int Nl80211AttributeControlPortEthertype::kName =
    NL80211_ATTR_CONTROL_PORT_ETHERTYPE;
const char Nl80211AttributeControlPortEthertype::kNameString[] =
    "NL80211_ATTR_CONTROL_PORT_ETHERTYPE";

const int Nl80211AttributeCqm::kName = NL80211_ATTR_CQM;
const char Nl80211AttributeCqm::kNameString[] = "NL80211_ATTR_CQM";

Nl80211AttributeCqm::Nl80211AttributeCqm()
    : NetlinkNestedAttribute(kName, kNameString) {
  nested_template_.insert(
      AttrDataPair(__NL80211_ATTR_CQM_INVALID,
                   NestedData(kTypeU32, "__NL80211_ATTR_CQM_INVALID", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_ATTR_CQM_RSSI_THOLD,
                   NestedData(kTypeU32, "NL80211_ATTR_CQM_RSSI_THOLD", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_ATTR_CQM_RSSI_HYST,
                   NestedData(kTypeU32, "NL80211_ATTR_CQM_RSSI_HYST", false)));
  nested_template_.insert(AttrDataPair(
      NL80211_ATTR_CQM_RSSI_THRESHOLD_EVENT,
      NestedData(kTypeU32, "NL80211_ATTR_CQM_RSSI_THRESHOLD_EVENT", false)));
  nested_template_.insert(AttrDataPair(
      NL80211_ATTR_CQM_PKT_LOSS_EVENT,
      NestedData(kTypeU32, "NL80211_ATTR_CQM_PKT_LOSS_EVENT", false)));
}

const int Nl80211AttributeDeviceApSme::kName = NL80211_ATTR_DEVICE_AP_SME;
const char Nl80211AttributeDeviceApSme::kNameString[] =
    "NL80211_ATTR_DEVICE_AP_SME";

const int Nl80211AttributeDfsRegion::kName = NL80211_ATTR_DFS_REGION;
const char Nl80211AttributeDfsRegion::kNameString[] = "NL80211_ATTR_DFS_REGION";

const int Nl80211AttributeDisconnectedByAp::kName =
    NL80211_ATTR_DISCONNECTED_BY_AP;
const char Nl80211AttributeDisconnectedByAp::kNameString[] =
    "NL80211_ATTR_DISCONNECTED_BY_AP";

const int Nl80211AttributeDuration::kName = NL80211_ATTR_DURATION;
const char Nl80211AttributeDuration::kNameString[] = "NL80211_ATTR_DURATION";

const int Nl80211AttributeFeatureFlags::kName = NL80211_ATTR_FEATURE_FLAGS;
const char Nl80211AttributeFeatureFlags::kNameString[] =
    "NL80211_ATTR_FEATURE_FLAGS";

const int Nl80211AttributeFrame::kName = NL80211_ATTR_FRAME;
const char Nl80211AttributeFrame::kNameString[] = "NL80211_ATTR_FRAME";

const int Nl80211AttributeGeneration::kName = NL80211_ATTR_GENERATION;
const char Nl80211AttributeGeneration::kNameString[] =
    "NL80211_ATTR_GENERATION";

const int Nl80211AttributeHtCapabilityMask::kName =
    NL80211_ATTR_HT_CAPABILITY_MASK;
const char Nl80211AttributeHtCapabilityMask::kNameString[] =
    "NL80211_ATTR_HT_CAPABILITY_MASK";

const int Nl80211AttributeIfindex::kName = NL80211_ATTR_IFINDEX;
const char Nl80211AttributeIfindex::kNameString[] = "NL80211_ATTR_IFINDEX";

const int Nl80211AttributeIftype::kName = NL80211_ATTR_IFTYPE;
const char Nl80211AttributeIftype::kNameString[] = "NL80211_ATTR_IFTYPE";

const int Nl80211AttributeKeyIdx::kName = NL80211_ATTR_KEY_IDX;
const char Nl80211AttributeKeyIdx::kNameString[] = "NL80211_ATTR_KEY_IDX";

const int Nl80211AttributeKeySeq::kName = NL80211_ATTR_KEY_SEQ;
const char Nl80211AttributeKeySeq::kNameString[] = "NL80211_ATTR_KEY_SEQ";

const int Nl80211AttributeKeyType::kName = NL80211_ATTR_KEY_TYPE;
const char Nl80211AttributeKeyType::kNameString[] = "NL80211_ATTR_KEY_TYPE";

const int Nl80211AttributeMac::kName = NL80211_ATTR_MAC;
const char Nl80211AttributeMac::kNameString[] = "NL80211_ATTR_MAC";

bool Nl80211AttributeMac::ToString(std::string* value) const {
  if (!value) {
    LOG(ERROR) << "Null |value| parameter";
    return false;
  }
  *value = StringFromMacAddress(data_.GetConstData());
  return true;
}

// static
string Nl80211AttributeMac::StringFromMacAddress(const uint8_t* arg) {
  string output;

  if (!arg) {
    static const char kBogusMacAddress[] = "XX:XX:XX:XX:XX:XX";
    output = kBogusMacAddress;
    LOG(ERROR) << "|arg| parameter is NULL.";
  } else {
    output = StringPrintf("%02x:%02x:%02x:%02x:%02x:%02x", arg[0], arg[1],
                          arg[2], arg[3], arg[4], arg[5]);
  }
  return output;
}

const int Nl80211AttributeMaxMatchSets::kName = NL80211_ATTR_MAX_MATCH_SETS;
const char Nl80211AttributeMaxMatchSets::kNameString[] =
    "NL80211_ATTR_MAX_MATCH_SETS";

const int Nl80211AttributeMaxNumPmkids::kName = NL80211_ATTR_MAX_NUM_PMKIDS;
const char Nl80211AttributeMaxNumPmkids::kNameString[] =
    "NL80211_ATTR_MAX_NUM_PMKIDS";

const int Nl80211AttributeMaxNumScanSsids::kName =
    NL80211_ATTR_MAX_NUM_SCAN_SSIDS;
const char Nl80211AttributeMaxNumScanSsids::kNameString[] =
    "NL80211_ATTR_MAX_NUM_SCAN_SSIDS";

const int Nl80211AttributeMaxNumSchedScanSsids::kName =
    NL80211_ATTR_MAX_NUM_SCHED_SCAN_SSIDS;
const char Nl80211AttributeMaxNumSchedScanSsids::kNameString[] =
    "NL80211_ATTR_MAX_NUM_SCHED_SCAN_SSIDS";

const int Nl80211AttributeMaxRemainOnChannelDuration::kName =
    NL80211_ATTR_MAX_REMAIN_ON_CHANNEL_DURATION;
const char Nl80211AttributeMaxRemainOnChannelDuration::kNameString[] =
    "NL80211_ATTR_MAX_REMAIN_ON_CHANNEL_DURATION";

const int Nl80211AttributeMaxScanIeLen::kName = NL80211_ATTR_MAX_SCAN_IE_LEN;
const char Nl80211AttributeMaxScanIeLen::kNameString[] =
    "NL80211_ATTR_MAX_SCAN_IE_LEN";

const int Nl80211AttributeMaxSchedScanIeLen::kName =
    NL80211_ATTR_MAX_SCHED_SCAN_IE_LEN;
const char Nl80211AttributeMaxSchedScanIeLen::kNameString[] =
    "NL80211_ATTR_MAX_SCHED_SCAN_IE_LEN";

const int Nl80211AttributeOffchannelTxOk::kName = NL80211_ATTR_OFFCHANNEL_TX_OK;
const char Nl80211AttributeOffchannelTxOk::kNameString[] =
    "NL80211_ATTR_OFFCHANNEL_TX_OK";

const int Nl80211AttributeProbeRespOffload::kName =
    NL80211_ATTR_PROBE_RESP_OFFLOAD;
const char Nl80211AttributeProbeRespOffload::kNameString[] =
    "NL80211_ATTR_PROBE_RESP_OFFLOAD";

const int Nl80211AttributeReasonCode::kName = NL80211_ATTR_REASON_CODE;
const char Nl80211AttributeReasonCode::kNameString[] =
    "NL80211_ATTR_REASON_CODE";

const int Nl80211AttributeRegAlpha2::kName = NL80211_ATTR_REG_ALPHA2;
const char Nl80211AttributeRegAlpha2::kNameString[] = "NL80211_ATTR_REG_ALPHA2";

const int Nl80211AttributeRegInitiator::kName = NL80211_ATTR_REG_INITIATOR;
const char Nl80211AttributeRegInitiator::kNameString[] =
    "NL80211_ATTR_REG_INITIATOR";

// The RegInitiator type can be interpreted as either a U8 or U32 depending
// on context.  Override the default InitFromValue implementation to be
// flexible to either encoding.
bool Nl80211AttributeRegInitiator::InitFromValue(const ByteString& input) {
  uint8_t u8_data;
  if (input.GetLength() != sizeof(u8_data))
    return NetlinkU32Attribute::InitFromValue(input);

  if (!input.CopyData(sizeof(u8_data), &u8_data)) {
    LOG(ERROR) << "Invalid |input| parameter.";
    return false;
  }

  SetU32Value(static_cast<uint32_t>(u8_data));
  return NetlinkAttribute::InitFromValue(input);
}

const int Nl80211AttributeRegRules::kName = NL80211_ATTR_REG_RULES;
const char Nl80211AttributeRegRules::kNameString[] = "NL80211_ATTR_REG_RULES";

Nl80211AttributeRegRules::Nl80211AttributeRegRules()
    : NetlinkNestedAttribute(kName, kNameString) {
  NestedData reg_rules(kTypeNested, "NL80211_REG_RULES", true);
  reg_rules.deeper_nesting.insert(
      AttrDataPair(__NL80211_REG_RULE_ATTR_INVALID,
                   NestedData(kTypeU32, "__NL80211_ATTR_REG_RULE_INVALID",
                              false)));
  reg_rules.deeper_nesting.insert(
      AttrDataPair(NL80211_ATTR_REG_RULE_FLAGS,
                   NestedData(kTypeU32, "NL80211_ATTR_REG_RULE_FLAGS",
                              false)));
  reg_rules.deeper_nesting.insert(
      AttrDataPair(NL80211_ATTR_FREQ_RANGE_START,
                   NestedData(kTypeU32, "NL80211_ATTR_FREQ_RANGE_START",
                              false)));
  reg_rules.deeper_nesting.insert(
      AttrDataPair(NL80211_ATTR_FREQ_RANGE_END,
                   NestedData(kTypeU32, "NL80211_ATTR_FREQ_RANGE_END",
                              false)));
  reg_rules.deeper_nesting.insert(
      AttrDataPair(NL80211_ATTR_FREQ_RANGE_MAX_BW,
                   NestedData(kTypeU32, "NL80211_ATTR_FREQ_RANGE_MAX_BW",
                              false)));
  reg_rules.deeper_nesting.insert(
      AttrDataPair(NL80211_ATTR_POWER_RULE_MAX_ANT_GAIN,
                   NestedData(kTypeU32, "NL80211_ATTR_POWER_RULE_MAX_ANT_GAIN",
                              false)));
  reg_rules.deeper_nesting.insert(
      AttrDataPair(NL80211_ATTR_POWER_RULE_MAX_EIRP,
                   NestedData(kTypeU32, "NL80211_ATTR_POWER_RULE_MAX_EIRP",
                              false)));

  nested_template_.insert(AttrDataPair(kArrayAttrEnumVal, reg_rules));
}

const int Nl80211AttributeRegType::kName = NL80211_ATTR_REG_TYPE;
const char Nl80211AttributeRegType::kNameString[] = "NL80211_ATTR_REG_TYPE";

const int Nl80211AttributeRespIe::kName = NL80211_ATTR_RESP_IE;
const char Nl80211AttributeRespIe::kNameString[] = "NL80211_ATTR_RESP_IE";

const int Nl80211AttributeRoamSupport::kName = NL80211_ATTR_ROAM_SUPPORT;
const char Nl80211AttributeRoamSupport::kNameString[] =
    "NL80211_ATTR_ROAM_SUPPORT";

const int Nl80211AttributeScanFrequencies::kName =
    NL80211_ATTR_SCAN_FREQUENCIES;
const char Nl80211AttributeScanFrequencies::kNameString[] =
    "NL80211_ATTR_SCAN_FREQUENCIES";

Nl80211AttributeScanFrequencies::Nl80211AttributeScanFrequencies()
    : NetlinkNestedAttribute(kName, kNameString) {
  nested_template_.insert(AttrDataPair(
      kArrayAttrEnumVal, NestedData(kTypeU32, "NL80211_SCAN_FREQ", true)));
}

const int Nl80211AttributeScanSsids::kName = NL80211_ATTR_SCAN_SSIDS;
const char Nl80211AttributeScanSsids::kNameString[] = "NL80211_ATTR_SCAN_SSIDS";

Nl80211AttributeScanSsids::Nl80211AttributeScanSsids()
    : NetlinkNestedAttribute(kName, kNameString) {
  nested_template_.insert(AttrDataPair(
      kArrayAttrEnumVal, NestedData(kTypeString, "NL80211_SCAN_SSID", true)));
}

const int Nl80211AttributeStaInfo::kName = NL80211_ATTR_STA_INFO;
const char Nl80211AttributeStaInfo::kNameString[] = "NL80211_ATTR_STA_INFO";

Nl80211AttributeStaInfo::Nl80211AttributeStaInfo()
    : NetlinkNestedAttribute(kName, kNameString) {
  NestedData tx_rates(kTypeNested, "NL80211_STA_INFO_TX_BITRATE", false);
  tx_rates.deeper_nesting.insert(
      AttrDataPair(__NL80211_RATE_INFO_INVALID,
                   NestedData(kTypeU32, "__NL80211_RATE_INFO_INVALID", false)));
  tx_rates.deeper_nesting.insert(
      AttrDataPair(NL80211_RATE_INFO_BITRATE,
                   NestedData(kTypeU16, "NL80211_RATE_INFO_BITRATE", false)));
  tx_rates.deeper_nesting.insert(
      AttrDataPair(NL80211_RATE_INFO_MCS,
                   NestedData(kTypeU8, "NL80211_RATE_INFO_MCS", false)));
  tx_rates.deeper_nesting.insert(AttrDataPair(
      NL80211_RATE_INFO_40_MHZ_WIDTH,
      NestedData(kTypeFlag, "NL80211_RATE_INFO_40_MHZ_WIDTH", false)));
  tx_rates.deeper_nesting.insert(
      AttrDataPair(NL80211_RATE_INFO_SHORT_GI,
                   NestedData(kTypeFlag, "NL80211_RATE_INFO_SHORT_GI", false)));
  tx_rates.deeper_nesting.insert(
      AttrDataPair(NL80211_RATE_INFO_BITRATE32,
                   NestedData(kTypeU32, "NL80211_RATE_INFO_BITRATE32", false)));
  tx_rates.deeper_nesting.insert(
      AttrDataPair(NL80211_RATE_INFO_VHT_MCS,
                   NestedData(kTypeU8, "NL80211_RATE_INFO_VHT_MCS", false)));
  tx_rates.deeper_nesting.insert(
      AttrDataPair(NL80211_RATE_INFO_VHT_NSS,
                   NestedData(kTypeU8, "NL80211_RATE_INFO_VHT_NSS", false)));
  tx_rates.deeper_nesting.insert(AttrDataPair(
      NL80211_RATE_INFO_80_MHZ_WIDTH,
      NestedData(kTypeFlag, "NL80211_RATE_INFO_80_MHZ_WIDTH", false)));
  tx_rates.deeper_nesting.insert(AttrDataPair(
      NL80211_RATE_INFO_80P80_MHZ_WIDTH,
      NestedData(kTypeFlag, "NL80211_RATE_INFO_80P80_MHZ_WIDTH", false)));
  tx_rates.deeper_nesting.insert(AttrDataPair(
      NL80211_RATE_INFO_160_MHZ_WIDTH,
      NestedData(kTypeFlag, "NL80211_RATE_INFO_160_MHZ_WIDTH", false)));

  NestedData rx_rates(kTypeNested, "NL80211_STA_INFO_RX_BITRATE", false);
  rx_rates.deeper_nesting = tx_rates.deeper_nesting;

  NestedData bss(kTypeNested, "NL80211_STA_INFO_BSS_PARAM", false);
  bss.deeper_nesting.insert(AttrDataPair(
      __NL80211_STA_BSS_PARAM_INVALID,
      NestedData(kTypeU32, "__NL80211_STA_BSS_PARAM_INVALID", false)));
  bss.deeper_nesting.insert(AttrDataPair(
      NL80211_STA_BSS_PARAM_CTS_PROT,
      NestedData(kTypeFlag, "NL80211_STA_BSS_PARAM_CTS_PROT", false)));
  bss.deeper_nesting.insert(AttrDataPair(
      NL80211_STA_BSS_PARAM_SHORT_PREAMBLE,
      NestedData(kTypeFlag, "NL80211_STA_BSS_PARAM_SHORT_PREAMBLE", false)));
  bss.deeper_nesting.insert(AttrDataPair(
      NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME,
      NestedData(kTypeFlag, "NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME", false)));
  bss.deeper_nesting.insert(AttrDataPair(
      NL80211_STA_BSS_PARAM_DTIM_PERIOD,
      NestedData(kTypeU8, "NL80211_STA_BSS_PARAM_DTIM_PERIOD", false)));
  bss.deeper_nesting.insert(AttrDataPair(
      NL80211_STA_BSS_PARAM_BEACON_INTERVAL,
      NestedData(kTypeU16, "NL80211_STA_BSS_PARAM_BEACON_INTERVAL", false)));

  nested_template_.insert(
      AttrDataPair(__NL80211_STA_INFO_INVALID,
                   NestedData(kTypeU32, "__NL80211_STA_INFO_INVALID", false)));
  nested_template_.insert(AttrDataPair(
      NL80211_STA_INFO_INACTIVE_TIME,
      NestedData(kTypeU32, "NL80211_STA_INFO_INACTIVE_TIME", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_RX_BYTES,
                   NestedData(kTypeU32, "NL80211_STA_INFO_RX_BYTES", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_TX_BYTES,
                   NestedData(kTypeU32, "NL80211_STA_INFO_TX_BYTES", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_RX_BYTES64,
                   NestedData(kTypeU64, "NL80211_STA_INFO_RX_BYTES64", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_TX_BYTES64,
                   NestedData(kTypeU64, "NL80211_STA_INFO_TX_BYTES64", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_LLID,
                   NestedData(kTypeU16, "NL80211_STA_INFO_LLID", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_PLID,
                   NestedData(kTypeU16, "NL80211_STA_INFO_PLID", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_PLINK_STATE,
                   NestedData(kTypeU8, "NL80211_STA_INFO_PLINK_STATE", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_SIGNAL,
                   NestedData(kTypeU8, "NL80211_STA_INFO_SIGNAL", false)));
  nested_template_.insert(AttrDataPair(NL80211_STA_INFO_TX_BITRATE, tx_rates));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_RX_PACKETS,
                   NestedData(kTypeU32, "NL80211_STA_INFO_RX_PACKETS", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_TX_PACKETS,
                   NestedData(kTypeU32, "NL80211_STA_INFO_TX_PACKETS", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_TX_RETRIES,
                   NestedData(kTypeU32, "NL80211_STA_INFO_TX_RETRIES", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_TX_FAILED,
                   NestedData(kTypeU32, "NL80211_STA_INFO_TX_FAILED", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_SIGNAL_AVG,
                   NestedData(kTypeU8, "NL80211_STA_INFO_SIGNAL_AVG", false)));
  nested_template_.insert(AttrDataPair(NL80211_STA_INFO_RX_BITRATE, rx_rates));
  nested_template_.insert(AttrDataPair(NL80211_STA_INFO_BSS_PARAM, bss));
  nested_template_.insert(AttrDataPair(
      NL80211_STA_INFO_CONNECTED_TIME,
      NestedData(kTypeU32, "NL80211_STA_INFO_CONNECTED_TIME", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_STA_FLAGS,
                   NestedData(kTypeU64, "NL80211_STA_INFO_STA_FLAGS", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_STA_INFO_BEACON_LOSS,
                   NestedData(kTypeU32, "NL80211_STA_INFO_BEACON_LOSS",
                   false)));
}

const int Nl80211AttributeSurveyInfo::kName = NL80211_ATTR_SURVEY_INFO;
const char Nl80211AttributeSurveyInfo::kNameString[] =
                                 "NL80211_ATTR_SURVEY_INFO";

Nl80211AttributeSurveyInfo::Nl80211AttributeSurveyInfo()
    : NetlinkNestedAttribute(kName, kNameString) {
  nested_template_.insert(
      AttrDataPair(NL80211_SURVEY_INFO_FREQUENCY,
         NestedData(kTypeU32, "NL80211_SURVEY_INFO_FREQUENCY", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_SURVEY_INFO_NOISE,
         NestedData(kTypeU8, "NL80211_SURVEY_INFO_NOISE", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_SURVEY_INFO_IN_USE,
         NestedData(kTypeFlag, "NL80211_SURVEY_INFO_IN_USE", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_SURVEY_INFO_CHANNEL_TIME,
         NestedData(kTypeU64, "NL80211_SURVEY_INFO_CHANNEL_TIME", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY,
         NestedData(kTypeU64, "NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY,
         NestedData(kTypeU64,
             "NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_SURVEY_INFO_CHANNEL_TIME_RX,
         NestedData(kTypeU64, "NL80211_SURVEY_INFO_CHANNEL_TIME_RX", false)));
  nested_template_.insert(
      AttrDataPair(NL80211_SURVEY_INFO_CHANNEL_TIME_TX,
         NestedData(kTypeU64, "NL80211_SURVEY_INFO_CHANNEL_TIME_TX", false)));
}

const int Nl80211AttributeSupportedIftypes::kName =
    NL80211_ATTR_SUPPORTED_IFTYPES;
const char Nl80211AttributeSupportedIftypes::kNameString[] =
    "NL80211_ATTR_SUPPORTED_IFTYPES";
Nl80211AttributeSupportedIftypes::Nl80211AttributeSupportedIftypes()
    : NetlinkNestedAttribute(kName, kNameString) {
  nested_template_.insert(AttrDataPair(
      kArrayAttrEnumVal,
      NestedData(kTypeFlag, "NL80211_SUPPORTED_IFTYPES_IFTYPE", true)));
}

const int Nl80211AttributeStatusCode::kName = NL80211_ATTR_STATUS_CODE;
const char Nl80211AttributeStatusCode::kNameString[] =
    "NL80211_ATTR_STATUS_CODE";

const int Nl80211AttributeSupportApUapsd::kName = NL80211_ATTR_SUPPORT_AP_UAPSD;
const char Nl80211AttributeSupportApUapsd::kNameString[] =
    "NL80211_ATTR_SUPPORT_AP_UAPSD";

const int Nl80211AttributeSupportIbssRsn::kName = NL80211_ATTR_SUPPORT_IBSS_RSN;
const char Nl80211AttributeSupportIbssRsn::kNameString[] =
    "NL80211_ATTR_SUPPORT_IBSS_RSN";

const int Nl80211AttributeSupportMeshAuth::kName =
    NL80211_ATTR_SUPPORT_MESH_AUTH;
const char Nl80211AttributeSupportMeshAuth::kNameString[] =
    "NL80211_ATTR_SUPPORT_MESH_AUTH";

const int Nl80211AttributeTdlsExternalSetup::kName =
    NL80211_ATTR_TDLS_EXTERNAL_SETUP;
const char Nl80211AttributeTdlsExternalSetup::kNameString[] =
    "NL80211_ATTR_TDLS_EXTERNAL_SETUP";

const int Nl80211AttributeTdlsSupport::kName = NL80211_ATTR_TDLS_SUPPORT;
const char Nl80211AttributeTdlsSupport::kNameString[] =
    "NL80211_ATTR_TDLS_SUPPORT";

const int Nl80211AttributeTimedOut::kName = NL80211_ATTR_TIMED_OUT;
const char Nl80211AttributeTimedOut::kNameString[] = "NL80211_ATTR_TIMED_OUT";

const int Nl80211AttributeWiphyAntennaAvailRx::kName =
    NL80211_ATTR_WIPHY_ANTENNA_AVAIL_RX;
const char Nl80211AttributeWiphyAntennaAvailRx::kNameString[] =
    "NL80211_ATTR_WIPHY_ANTENNA_AVAIL_RX";

const int Nl80211AttributeWiphyAntennaAvailTx::kName =
    NL80211_ATTR_WIPHY_ANTENNA_AVAIL_TX;
const char Nl80211AttributeWiphyAntennaAvailTx::kNameString[] =
    "NL80211_ATTR_WIPHY_ANTENNA_AVAIL_TX";

const int Nl80211AttributeWiphyAntennaRx::kName = NL80211_ATTR_WIPHY_ANTENNA_RX;
const char Nl80211AttributeWiphyAntennaRx::kNameString[] =
    "NL80211_ATTR_WIPHY_ANTENNA_RX";

const int Nl80211AttributeWiphyAntennaTx::kName = NL80211_ATTR_WIPHY_ANTENNA_TX;
const char Nl80211AttributeWiphyAntennaTx::kNameString[] =
    "NL80211_ATTR_WIPHY_ANTENNA_TX";

const int Nl80211AttributeWiphyCoverageClass::kName =
    NL80211_ATTR_WIPHY_COVERAGE_CLASS;
const char Nl80211AttributeWiphyCoverageClass::kNameString[] =
    "NL80211_ATTR_WIPHY_COVERAGE_CLASS";

const int Nl80211AttributeWiphyFragThreshold::kName =
    NL80211_ATTR_WIPHY_FRAG_THRESHOLD;
const char Nl80211AttributeWiphyFragThreshold::kNameString[] =
    "NL80211_ATTR_WIPHY_FRAG_THRESHOLD";

const int Nl80211AttributeWiphyFreq::kName = NL80211_ATTR_WIPHY_FREQ;
const char Nl80211AttributeWiphyFreq::kNameString[] = "NL80211_ATTR_WIPHY_FREQ";

const int Nl80211AttributeChannelType::kName = NL80211_ATTR_WIPHY_CHANNEL_TYPE;
const char Nl80211AttributeChannelType::kNameString[] =
    "NL80211_ATTR_WIPHY_CHANNEL_TYPE";

const int Nl80211AttributeChannelWidth::kName = NL80211_ATTR_CHANNEL_WIDTH;
const char Nl80211AttributeChannelWidth::kNameString[] =
    "NL80211_ATTR_CHANNEL_WIDTH";

const int Nl80211AttributeCenterFreq1::kName = NL80211_ATTR_CENTER_FREQ1;
const char Nl80211AttributeCenterFreq1::kNameString[] =
    "NL80211_ATTR_CENTER_FREQ1";

const int Nl80211AttributeCenterFreq2::kName = NL80211_ATTR_CENTER_FREQ2;
const char Nl80211AttributeCenterFreq2::kNameString[] =
    "NL80211_ATTR_CENTER_FREQ2";

const int Nl80211AttributeWiphy::kName = NL80211_ATTR_WIPHY;
const char Nl80211AttributeWiphy::kNameString[] = "NL80211_ATTR_WIPHY";

const int Nl80211AttributeWiphyName::kName = NL80211_ATTR_WIPHY_NAME;
const char Nl80211AttributeWiphyName::kNameString[] = "NL80211_ATTR_WIPHY_NAME";

const int Nl80211AttributeWiphyRetryLong::kName = NL80211_ATTR_WIPHY_RETRY_LONG;
const char Nl80211AttributeWiphyRetryLong::kNameString[] =
    "NL80211_ATTR_WIPHY_RETRY_LONG";

const int Nl80211AttributeWiphyRetryShort::kName =
    NL80211_ATTR_WIPHY_RETRY_SHORT;
const char Nl80211AttributeWiphyRetryShort::kNameString[] =
    "NL80211_ATTR_WIPHY_RETRY_SHORT";

const int Nl80211AttributeWiphyRtsThreshold::kName =
    NL80211_ATTR_WIPHY_RTS_THRESHOLD;
const char Nl80211AttributeWiphyRtsThreshold::kNameString[] =
    "NL80211_ATTR_WIPHY_RTS_THRESHOLD";

}  // namespace shill
