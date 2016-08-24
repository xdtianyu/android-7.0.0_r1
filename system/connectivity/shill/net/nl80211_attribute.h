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

#ifndef SHILL_NET_NL80211_ATTRIBUTE_H_
#define SHILL_NET_NL80211_ATTRIBUTE_H_

#include <string>

#include <base/macros.h>

#include "shill/net/netlink_attribute.h"
#include "shill/net/netlink_message.h"

struct nlattr;

namespace shill {

// U8.

class Nl80211AttributeDfsRegion : public NetlinkU8Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeDfsRegion() : NetlinkU8Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeDfsRegion);
};

class Nl80211AttributeKeyIdx : public NetlinkU8Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeKeyIdx() : NetlinkU8Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeKeyIdx);
};

class Nl80211AttributeMaxMatchSets : public NetlinkU8Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeMaxMatchSets() : NetlinkU8Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeMaxMatchSets);
};

class Nl80211AttributeMaxNumPmkids : public NetlinkU8Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeMaxNumPmkids() : NetlinkU8Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeMaxNumPmkids);
};

class Nl80211AttributeMaxNumScanSsids : public NetlinkU8Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeMaxNumScanSsids() : NetlinkU8Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeMaxNumScanSsids);
};

class Nl80211AttributeMaxNumSchedScanSsids : public NetlinkU8Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeMaxNumSchedScanSsids()
      : NetlinkU8Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeMaxNumSchedScanSsids);
};

class Nl80211AttributeRegType : public NetlinkU8Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeRegType() : NetlinkU8Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeRegType);
};

class Nl80211AttributeWiphyCoverageClass : public NetlinkU8Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyCoverageClass()
      : NetlinkU8Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyCoverageClass);
};

class Nl80211AttributeWiphyRetryLong : public NetlinkU8Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyRetryLong() : NetlinkU8Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyRetryLong);
};

class Nl80211AttributeWiphyRetryShort : public NetlinkU8Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyRetryShort() : NetlinkU8Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyRetryShort);
};

// U16.

class Nl80211AttributeMaxScanIeLen : public NetlinkU16Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeMaxScanIeLen() : NetlinkU16Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeMaxScanIeLen);
};

class Nl80211AttributeMaxSchedScanIeLen : public NetlinkU16Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeMaxSchedScanIeLen()
      : NetlinkU16Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeMaxSchedScanIeLen);
};

class Nl80211AttributeReasonCode : public NetlinkU16Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeReasonCode() : NetlinkU16Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeReasonCode);
};

class Nl80211AttributeStatusCode : public NetlinkU16Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeStatusCode() : NetlinkU16Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeStatusCode);
};

// U32.

class Nl80211AttributeDuration : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeDuration() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeDuration);
};

class Nl80211AttributeDeviceApSme : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeDeviceApSme() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeDeviceApSme);
};

class Nl80211AttributeFeatureFlags : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeFeatureFlags() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeFeatureFlags);
};

class Nl80211AttributeGeneration : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeGeneration() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeGeneration);
};

class Nl80211AttributeIfindex : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeIfindex() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeIfindex);
};

class Nl80211AttributeIftype : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeIftype() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeIftype);
};

class Nl80211AttributeKeyType : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeKeyType() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeKeyType);
};

class Nl80211AttributeMaxRemainOnChannelDuration : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeMaxRemainOnChannelDuration()
      : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeMaxRemainOnChannelDuration);
};

class Nl80211AttributeProbeRespOffload : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeProbeRespOffload()
      : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeProbeRespOffload);
};

// Set SHILL_EXPORT to allow unit tests to instantiate these.
class SHILL_EXPORT Nl80211AttributeRegInitiator : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeRegInitiator() : NetlinkU32Attribute(kName, kNameString) {}
  bool InitFromValue(const ByteString& data) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeRegInitiator);
};

class Nl80211AttributeWiphy : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphy() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphy);
};

class Nl80211AttributeWiphyAntennaAvailRx : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyAntennaAvailRx()
      : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyAntennaAvailRx);
};

class Nl80211AttributeWiphyAntennaAvailTx : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyAntennaAvailTx()
      : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyAntennaAvailTx);
};

class Nl80211AttributeWiphyAntennaRx : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyAntennaRx() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyAntennaRx);
};

class Nl80211AttributeWiphyAntennaTx : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyAntennaTx() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyAntennaTx);
};

class Nl80211AttributeWiphyFragThreshold : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyFragThreshold()
      : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyFragThreshold);
};

class Nl80211AttributeWiphyFreq : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyFreq() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyFreq);
};

class Nl80211AttributeChannelType : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeChannelType() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeChannelType);
};

class Nl80211AttributeChannelWidth : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeChannelWidth() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeChannelWidth);
};

class Nl80211AttributeCenterFreq1 : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeCenterFreq1() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeCenterFreq1);
};

class Nl80211AttributeCenterFreq2 : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeCenterFreq2() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeCenterFreq2);
};

class Nl80211AttributeWiphyRtsThreshold : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyRtsThreshold()
      : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyRtsThreshold);
};

// U64.

class Nl80211AttributeCookie : public NetlinkU64Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeCookie() : NetlinkU64Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeCookie);
};

// Flag.

class Nl80211AttributeControlPortEthertype : public NetlinkFlagAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeControlPortEthertype()
      : NetlinkFlagAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeControlPortEthertype);
};

class Nl80211AttributeDisconnectedByAp : public NetlinkFlagAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeDisconnectedByAp() :
    NetlinkFlagAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeDisconnectedByAp);
};

class Nl80211AttributeOffchannelTxOk : public NetlinkFlagAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeOffchannelTxOk()
      : NetlinkFlagAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeOffchannelTxOk);
};

class Nl80211AttributeRoamSupport : public NetlinkFlagAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeRoamSupport() : NetlinkFlagAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeRoamSupport);
};

class Nl80211AttributeSupportApUapsd : public NetlinkFlagAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeSupportApUapsd()
      : NetlinkFlagAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeSupportApUapsd);
};

class Nl80211AttributeSupportIbssRsn : public NetlinkFlagAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeSupportIbssRsn()
      : NetlinkFlagAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeSupportIbssRsn);
};

class Nl80211AttributeSupportMeshAuth : public NetlinkFlagAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeSupportMeshAuth() :
    NetlinkFlagAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeSupportMeshAuth);
};

class Nl80211AttributeTdlsExternalSetup : public NetlinkFlagAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeTdlsExternalSetup()
      : NetlinkFlagAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeTdlsExternalSetup);
};

class Nl80211AttributeTdlsSupport : public NetlinkFlagAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeTdlsSupport() : NetlinkFlagAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeTdlsSupport);
};

class Nl80211AttributeTimedOut : public NetlinkFlagAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeTimedOut() : NetlinkFlagAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeTimedOut);
};

// String.

class Nl80211AttributeRegAlpha2 : public NetlinkStringAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeRegAlpha2() : NetlinkStringAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeRegAlpha2);
};

class Nl80211AttributeWiphyName : public NetlinkStringAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyName() : NetlinkStringAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyName);
};

// Nested.

class Nl80211AttributeBss : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  // These are sorted alphabetically.
  static const int kChallengeTextAttributeId;
  static const int kChannelsAttributeId;
  static const int kCountryInfoAttributeId;
  static const int kDSParameterSetAttributeId;
  static const int kErpAttributeId;
  static const int kExtendedRatesAttributeId;
  static const int kHtCapAttributeId;
  static const int kHtInfoAttributeId;
  static const int kPowerCapabilityAttributeId;
  static const int kPowerConstraintAttributeId;
  static const int kRequestAttributeId;
  static const int kRsnAttributeId;
  static const int kSsidAttributeId;
  static const int kSupportedRatesAttributeId;
  static const int kTpcReportAttributeId;
  static const int kVendorSpecificAttributeId;
  static const int kVhtCapAttributeId;
  static const int kVhtInfoAttributeId;

  Nl80211AttributeBss();

 private:
  static bool ParseInformationElements(AttributeList* attribute_list,
                                       size_t id,
                                       const std::string& attribute_name,
                                       ByteString data);

  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeBss);
};

class Nl80211AttributeCqm : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeCqm();

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeCqm);
};

class Nl80211AttributeRegRules : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeRegRules();

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeRegRules);
};

class Nl80211AttributeScanFrequencies : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeScanFrequencies();

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeScanFrequencies);
};

class Nl80211AttributeScanSsids : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeScanSsids();

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeScanSsids);
};

class Nl80211AttributeStaInfo : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeStaInfo();

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeStaInfo);
};

class Nl80211AttributeSupportedIftypes : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeSupportedIftypes();

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeSupportedIftypes);
};

class Nl80211AttributeWiphyBands : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWiphyBands();

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWiphyBands);
};

#if !defined(DISABLE_WAKE_ON_WIFI)
class Nl80211AttributeWowlanTriggers : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  explicit Nl80211AttributeWowlanTriggers(
      NetlinkMessage::MessageContext context);

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWowlanTriggers);
};

class Nl80211AttributeWowlanTriggersSupported : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeWowlanTriggersSupported();

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeWowlanTriggersSupported);
};
#endif  // DISABLE_WAKE_ON_WIFI

// Raw.

class Nl80211AttributeCipherSuites : public NetlinkRawAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeCipherSuites() : NetlinkRawAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeCipherSuites);
};

class Nl80211AttributeFrame : public NetlinkRawAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeFrame() : NetlinkRawAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeFrame);
};

class Nl80211AttributeHtCapabilityMask : public NetlinkRawAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeHtCapabilityMask()
      : NetlinkRawAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeHtCapabilityMask);
};

class Nl80211AttributeKeySeq : public NetlinkRawAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeKeySeq() : NetlinkRawAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeKeySeq);
};

class Nl80211AttributeMac : public NetlinkRawAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeMac() : NetlinkRawAttribute(kName, kNameString) {}
  virtual bool ToString(std::string* value) const;

  // Stringizes the MAC address found in 'arg'.  If there are problems (such
  // as a NULL |arg|), |value| is set to a bogus MAC address.
  static std::string StringFromMacAddress(const uint8_t* arg);

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeMac);
};

class Nl80211AttributeRespIe : public NetlinkRawAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeRespIe() : NetlinkRawAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeRespIe);
};

class Nl80211AttributeSurveyInfo : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  Nl80211AttributeSurveyInfo();

 private:
  DISALLOW_COPY_AND_ASSIGN(Nl80211AttributeSurveyInfo);
};

}  // namespace shill

#endif  // SHILL_NET_NL80211_ATTRIBUTE_H_
