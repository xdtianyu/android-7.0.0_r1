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

#include "shill/metrics.h"

#include <string>
#include <vector>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <metrics/metrics_library_mock.h>
#include <metrics/timer_mock.h>

#include "shill/mock_control.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_log.h"
#include "shill/mock_manager.h"
#include "shill/mock_service.h"

#if !defined(DISABLE_WIFI)
#include "shill/mock_eap_credentials.h"
#include "shill/wifi/mock_wifi_service.h"
#endif  // DISABLE_WIFI

using std::string;

using testing::_;
using testing::DoAll;
using testing::Ge;
using testing::Mock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::Test;

namespace shill {

class MetricsTest : public Test {
 public:
  MetricsTest()
      : manager_(&control_interface_,
                 &dispatcher_,
                 &metrics_),
        metrics_(&dispatcher_),
#if !defined(DISABLE_WIFI)
        open_wifi_service_(new MockWiFiService(&control_interface_,
                                               &dispatcher_,
                                               &metrics_,
                                               &manager_,
                                               manager_.wifi_provider(),
                                               ssid_,
                                               kModeManaged,
                                               kSecurityNone,
                                               false)),
        wep_wifi_service_(new MockWiFiService(&control_interface_,
                                              &dispatcher_,
                                              &metrics_,
                                              &manager_,
                                              manager_.wifi_provider(),
                                              ssid_,
                                              kModeManaged,
                                              kSecurityWep,
                                              false)),
        eap_wifi_service_(new MockWiFiService(&control_interface_,
                                              &dispatcher_,
                                              &metrics_,
                                              &manager_,
                                              manager_.wifi_provider(),
                                              ssid_,
                                              kModeManaged,
                                              kSecurity8021x,
                                              false)),
        eap_(new MockEapCredentials()),
#endif  // DISABLE_WIFI
        service_(new MockService(&control_interface_,
                                 &dispatcher_,
                                 &metrics_,
                                 &manager_)) {}

  virtual ~MetricsTest() {}

  virtual void SetUp() {
    metrics_.set_library(&library_);
#if !defined(DISABLE_WIFI)
    eap_wifi_service_->eap_.reset(eap_);  // Passes ownership.
#endif  // DISABLE_WIFI
    metrics_.collect_bootstats_ = false;
  }

 protected:
  void ExpectCommonPostReady(Metrics::WiFiApMode ap_mode,
                             Metrics::WiFiChannel channel,
                             Metrics::WiFiNetworkPhyMode mode,
                             Metrics::WiFiSecurity security,
                             int signal_strength) {
    EXPECT_CALL(library_, SendEnumToUMA("Network.Shill.Wifi.ApMode",
                                        ap_mode,
                                        Metrics::kWiFiApModeMax));
    EXPECT_CALL(library_, SendEnumToUMA("Network.Shill.Wifi.Channel",
                                        channel,
                                        Metrics::kMetricNetworkChannelMax));
    EXPECT_CALL(library_, SendEnumToUMA("Network.Shill.Wifi.PhyMode",
                                        mode,
                                        Metrics::kWiFiNetworkPhyModeMax));
    EXPECT_CALL(library_, SendEnumToUMA("Network.Shill.Wifi.Security",
                                        security,
                                        Metrics::kWiFiSecurityMax));
    EXPECT_CALL(library_,
                SendToUMA("Network.Shill.Wifi.SignalStrength",
                          signal_strength,
                          Metrics::kMetricNetworkSignalStrengthMin,
                          Metrics::kMetricNetworkSignalStrengthMax,
                          Metrics::kMetricNetworkSignalStrengthNumBuckets));
  }

  MockControl control_interface_;
  MockEventDispatcher dispatcher_;
  MockManager manager_;
  Metrics metrics_;  // This must be destroyed after all |service_|s.
  MetricsLibraryMock library_;
#if !defined(DISABLE_WIFI)
  const std::vector<uint8_t> ssid_;
  scoped_refptr<MockWiFiService> open_wifi_service_;
  scoped_refptr<MockWiFiService> wep_wifi_service_;
  scoped_refptr<MockWiFiService> eap_wifi_service_;
  MockEapCredentials* eap_;  // Owned by |eap_wifi_service_|.
#endif  // DISABLE_WIFI
  scoped_refptr<MockService> service_;
};

TEST_F(MetricsTest, TimeToConfig) {
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Unknown.TimeToConfig",
                                  Ge(0),
                                  Metrics::kTimerHistogramMillisecondsMin,
                                  Metrics::kTimerHistogramMillisecondsMax,
                                  Metrics::kTimerHistogramNumBuckets));
  metrics_.NotifyServiceStateChanged(*service_, Service::kStateConfiguring);
  metrics_.NotifyServiceStateChanged(*service_, Service::kStateConnected);
}

TEST_F(MetricsTest, TimeToPortal) {
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Unknown.TimeToPortal",
                                  Ge(0),
                                  Metrics::kTimerHistogramMillisecondsMin,
                                  Metrics::kTimerHistogramMillisecondsMax,
                                  Metrics::kTimerHistogramNumBuckets));
  metrics_.NotifyServiceStateChanged(*service_, Service::kStateConnected);
  metrics_.NotifyServiceStateChanged(*service_, Service::kStatePortal);
}

TEST_F(MetricsTest, TimeToOnline) {
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Unknown.TimeToOnline",
                                  Ge(0),
                                  Metrics::kTimerHistogramMillisecondsMin,
                                  Metrics::kTimerHistogramMillisecondsMax,
                                  Metrics::kTimerHistogramNumBuckets));
  metrics_.NotifyServiceStateChanged(*service_, Service::kStateConnected);
  metrics_.NotifyServiceStateChanged(*service_, Service::kStateOnline);
}

TEST_F(MetricsTest, ServiceFailure) {
  EXPECT_CALL(*service_, failure())
      .WillRepeatedly(Return(Service::kFailureBadPassphrase));
  EXPECT_CALL(library_,
      SendEnumToUMA(Metrics::kMetricNetworkServiceErrors,
                    Metrics::kNetworkServiceErrorBadPassphrase,
                    Metrics::kNetworkServiceErrorMax));
  metrics_.NotifyServiceStateChanged(*service_, Service::kStateFailure);
}

#if !defined(DISABLE_WIFI)
TEST_F(MetricsTest, WiFiServiceTimeToJoin) {
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Wifi.TimeToJoin",
                                  Ge(0),
                                  Metrics::kTimerHistogramMillisecondsMin,
                                  Metrics::kTimerHistogramMillisecondsMax,
                                  Metrics::kTimerHistogramNumBuckets));
  metrics_.NotifyServiceStateChanged(*open_wifi_service_,
                                     Service::kStateAssociating);
  metrics_.NotifyServiceStateChanged(*open_wifi_service_,
                                     Service::kStateConfiguring);
}

TEST_F(MetricsTest, WiFiServicePostReady) {
  base::TimeDelta non_zero_time_delta = base::TimeDelta::FromMilliseconds(1);
  chromeos_metrics::TimerMock* mock_time_resume_to_ready_timer =
      new chromeos_metrics::TimerMock;
  metrics_.set_time_resume_to_ready_timer(mock_time_resume_to_ready_timer);

  const int kStrength = -42;
  ExpectCommonPostReady(Metrics::kWiFiApModeManaged,
                        Metrics::kWiFiChannel2412,
                        Metrics::kWiFiNetworkPhyMode11a,
                        Metrics::kWiFiSecurityWep,
                        -kStrength);
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Wifi.TimeResumeToReady",
                                  _, _, _, _)).Times(0);
  EXPECT_CALL(library_, SendEnumToUMA("Network.Shill.Wifi.EapOuterProtocol",
                                       _, _)).Times(0);
  EXPECT_CALL(library_, SendEnumToUMA("Network.Shill.Wifi.EapInnerProtocol",
                                      _, _)).Times(0);
  wep_wifi_service_->frequency_ = 2412;
  wep_wifi_service_->physical_mode_ = Metrics::kWiFiNetworkPhyMode11a;
  wep_wifi_service_->raw_signal_strength_ = kStrength;
  metrics_.NotifyServiceStateChanged(*wep_wifi_service_,
                                     Service::kStateConnected);
  Mock::VerifyAndClearExpectations(&library_);

  // Simulate a system suspend, resume and an AP reconnect.
  ExpectCommonPostReady(Metrics::kWiFiApModeManaged,
                        Metrics::kWiFiChannel2412,
                        Metrics::kWiFiNetworkPhyMode11a,
                        Metrics::kWiFiSecurityWep,
                        -kStrength);
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Wifi.TimeResumeToReady",
                                  Ge(0),
                                  Metrics::kTimerHistogramMillisecondsMin,
                                  Metrics::kTimerHistogramMillisecondsMax,
                                  Metrics::kTimerHistogramNumBuckets));
  EXPECT_CALL(*mock_time_resume_to_ready_timer, GetElapsedTime(_)).
      WillOnce(DoAll(SetArgumentPointee<0>(non_zero_time_delta), Return(true)));
  metrics_.NotifySuspendDone();
  metrics_.NotifyServiceStateChanged(*wep_wifi_service_,
                                     Service::kStateConnected);
  Mock::VerifyAndClearExpectations(&library_);
  Mock::VerifyAndClearExpectations(mock_time_resume_to_ready_timer);

  // Make sure subsequent connects do not count towards TimeResumeToReady.
  ExpectCommonPostReady(Metrics::kWiFiApModeManaged,
                        Metrics::kWiFiChannel2412,
                        Metrics::kWiFiNetworkPhyMode11a,
                        Metrics::kWiFiSecurityWep,
                        -kStrength);
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Wifi.TimeResumeToReady",
                                  _, _, _, _)).Times(0);
  metrics_.NotifyServiceStateChanged(*wep_wifi_service_,
                                     Service::kStateConnected);
}

TEST_F(MetricsTest, WiFiServicePostReadyEAP) {
  const int kStrength = -42;
  ExpectCommonPostReady(Metrics::kWiFiApModeManaged,
                        Metrics::kWiFiChannel2412,
                        Metrics::kWiFiNetworkPhyMode11a,
                        Metrics::kWiFiSecurity8021x,
                        -kStrength);
  eap_wifi_service_->frequency_ = 2412;
  eap_wifi_service_->physical_mode_ = Metrics::kWiFiNetworkPhyMode11a;
  eap_wifi_service_->raw_signal_strength_ = kStrength;
  EXPECT_CALL(*eap_, OutputConnectionMetrics(&metrics_, Technology::kWifi));
  metrics_.NotifyServiceStateChanged(*eap_wifi_service_,
                                     Service::kStateConnected);
}

TEST_F(MetricsTest, WiFiServicePostReadyAdHoc) {
  auto adhoc_wifi_service(
      make_scoped_refptr(new MockWiFiService(&control_interface_,
                                             &dispatcher_,
                                             &metrics_,
                                             &manager_,
                                             manager_.wifi_provider(),
                                             ssid_,
                                             kModeAdhoc,
                                             kSecurityNone,
                                             false)));
  const int kStrength = -42;
  ExpectCommonPostReady(Metrics::kWiFiApModeAdHoc,
                        Metrics::kWiFiChannel2412,
                        Metrics::kWiFiNetworkPhyMode11b,
                        Metrics::kWiFiSecurityNone,
                        -kStrength);
  adhoc_wifi_service->frequency_ = 2412;
  adhoc_wifi_service->physical_mode_ = Metrics::kWiFiNetworkPhyMode11b;
  adhoc_wifi_service->raw_signal_strength_ = kStrength;
  metrics_.NotifyServiceStateChanged(*adhoc_wifi_service,
                                     Service::kStateConnected);
}
#endif  // DISABLE_WIFI

TEST_F(MetricsTest, FrequencyToChannel) {
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(2411));
  EXPECT_EQ(Metrics::kWiFiChannel2412, metrics_.WiFiFrequencyToChannel(2412));
  EXPECT_EQ(Metrics::kWiFiChannel2472, metrics_.WiFiFrequencyToChannel(2472));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(2473));
  EXPECT_EQ(Metrics::kWiFiChannel2484, metrics_.WiFiFrequencyToChannel(2484));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5169));
  EXPECT_EQ(Metrics::kWiFiChannel5170, metrics_.WiFiFrequencyToChannel(5170));
  EXPECT_EQ(Metrics::kWiFiChannel5190, metrics_.WiFiFrequencyToChannel(5190));
  EXPECT_EQ(Metrics::kWiFiChannel5180, metrics_.WiFiFrequencyToChannel(5180));
  EXPECT_EQ(Metrics::kWiFiChannel5200, metrics_.WiFiFrequencyToChannel(5200));
  EXPECT_EQ(Metrics::kWiFiChannel5230, metrics_.WiFiFrequencyToChannel(5230));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5231));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5239));
  EXPECT_EQ(Metrics::kWiFiChannel5240, metrics_.WiFiFrequencyToChannel(5240));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5241));
  EXPECT_EQ(Metrics::kWiFiChannel5320, metrics_.WiFiFrequencyToChannel(5320));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5321));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5499));
  EXPECT_EQ(Metrics::kWiFiChannel5500, metrics_.WiFiFrequencyToChannel(5500));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5501));
  EXPECT_EQ(Metrics::kWiFiChannel5700, metrics_.WiFiFrequencyToChannel(5700));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5701));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5744));
  EXPECT_EQ(Metrics::kWiFiChannel5745, metrics_.WiFiFrequencyToChannel(5745));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5746));
  EXPECT_EQ(Metrics::kWiFiChannel5825, metrics_.WiFiFrequencyToChannel(5825));
  EXPECT_EQ(Metrics::kWiFiChannelUndef, metrics_.WiFiFrequencyToChannel(5826));
}

TEST_F(MetricsTest, TimeOnlineTimeToDrop) {
  chromeos_metrics::TimerMock* mock_time_online_timer =
      new chromeos_metrics::TimerMock;
  metrics_.set_time_online_timer(mock_time_online_timer);
  chromeos_metrics::TimerMock* mock_time_to_drop_timer =
      new chromeos_metrics::TimerMock;
  metrics_.set_time_to_drop_timer(mock_time_to_drop_timer);
  scoped_refptr<MockService> wifi_service =
      new MockService(&control_interface_, &dispatcher_, &metrics_, &manager_);
  EXPECT_CALL(*service_, technology()).
      WillOnce(Return(Technology::kEthernet));
  EXPECT_CALL(*wifi_service, technology()).
      WillOnce(Return(Technology::kWifi));
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Ethernet.TimeOnline",
                                  Ge(0),
                                  Metrics::kMetricTimeOnlineSecondsMin,
                                  Metrics::kMetricTimeOnlineSecondsMax,
                                  Metrics::kTimerHistogramNumBuckets));
  EXPECT_CALL(library_, SendToUMA(Metrics::kMetricTimeToDropSeconds,
                                  Ge(0),
                                  Metrics::kMetricTimeToDropSecondsMin,
                                  Metrics::kMetricTimeToDropSecondsMax,
                                  Metrics::kTimerHistogramNumBuckets)).Times(0);
  EXPECT_CALL(*mock_time_online_timer, Start()).Times(2);
  EXPECT_CALL(*mock_time_to_drop_timer, Start());
  metrics_.NotifyDefaultServiceChanged(service_.get());
  metrics_.NotifyDefaultServiceChanged(wifi_service.get());

  EXPECT_CALL(*mock_time_online_timer, Start());
  EXPECT_CALL(*mock_time_to_drop_timer, Start()).Times(0);
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Wifi.TimeOnline",
                                  Ge(0),
                                  Metrics::kMetricTimeOnlineSecondsMin,
                                  Metrics::kMetricTimeOnlineSecondsMax,
                                  Metrics::kTimerHistogramNumBuckets));
  EXPECT_CALL(library_, SendToUMA(Metrics::kMetricTimeToDropSeconds,
                                  Ge(0),
                                  Metrics::kMetricTimeToDropSecondsMin,
                                  Metrics::kMetricTimeToDropSecondsMax,
                                  Metrics::kTimerHistogramNumBuckets));
  metrics_.NotifyDefaultServiceChanged(nullptr);
}

TEST_F(MetricsTest, Disconnect) {
  EXPECT_CALL(*service_, technology()).
      WillRepeatedly(Return(Technology::kWifi));
  EXPECT_CALL(*service_, explicitly_disconnected()).
      WillOnce(Return(false));
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Wifi.Disconnect",
                                  false,
                                  Metrics::kMetricDisconnectMin,
                                  Metrics::kMetricDisconnectMax,
                                  Metrics::kMetricDisconnectNumBuckets));
  metrics_.NotifyServiceDisconnect(*service_);

  EXPECT_CALL(*service_, explicitly_disconnected()).
      WillOnce(Return(true));
  EXPECT_CALL(library_, SendToUMA("Network.Shill.Wifi.Disconnect",
                                  true,
                                  Metrics::kMetricDisconnectMin,
                                  Metrics::kMetricDisconnectMax,
                                  Metrics::kMetricDisconnectNumBuckets));
  metrics_.NotifyServiceDisconnect(*service_);
}

TEST_F(MetricsTest, PortalDetectionResultToEnum) {
  ConnectivityTrial::Result trial_result(ConnectivityTrial::kPhaseDNS,
                                ConnectivityTrial::kStatusFailure);
  PortalDetector::Result result(trial_result, 0, true);

  EXPECT_EQ(Metrics::kPortalResultDNSFailure,
            Metrics::PortalDetectionResultToEnum(result));

  result.trial_result.phase = ConnectivityTrial::kPhaseDNS;
  result.trial_result.status = ConnectivityTrial::kStatusTimeout;
  EXPECT_EQ(Metrics::kPortalResultDNSTimeout,
            Metrics::PortalDetectionResultToEnum(result));

  result.trial_result.phase = ConnectivityTrial::kPhaseConnection;
  result.trial_result.status = ConnectivityTrial::kStatusFailure;
  EXPECT_EQ(Metrics::kPortalResultConnectionFailure,
            Metrics::PortalDetectionResultToEnum(result));

  result.trial_result.phase = ConnectivityTrial::kPhaseConnection;
  result.trial_result.status = ConnectivityTrial::kStatusTimeout;
  EXPECT_EQ(Metrics::kPortalResultConnectionTimeout,
            Metrics::PortalDetectionResultToEnum(result));

  result.trial_result.phase = ConnectivityTrial::kPhaseHTTP;
  result.trial_result.status = ConnectivityTrial::kStatusFailure;
  EXPECT_EQ(Metrics::kPortalResultHTTPFailure,
            Metrics::PortalDetectionResultToEnum(result));

  result.trial_result.phase = ConnectivityTrial::kPhaseHTTP;
  result.trial_result.status = ConnectivityTrial::kStatusTimeout;
  EXPECT_EQ(Metrics::kPortalResultHTTPTimeout,
            Metrics::PortalDetectionResultToEnum(result));

  result.trial_result.phase = ConnectivityTrial::kPhaseContent;
  result.trial_result.status = ConnectivityTrial::kStatusSuccess;
  EXPECT_EQ(Metrics::kPortalResultSuccess,
            Metrics::PortalDetectionResultToEnum(result));

  result.trial_result.phase = ConnectivityTrial::kPhaseContent;
  result.trial_result.status = ConnectivityTrial::kStatusFailure;
  EXPECT_EQ(Metrics::kPortalResultContentFailure,
            Metrics::PortalDetectionResultToEnum(result));

  result.trial_result.phase = ConnectivityTrial::kPhaseContent;
  result.trial_result.status = ConnectivityTrial::kStatusTimeout;
  EXPECT_EQ(Metrics::kPortalResultContentTimeout,
            Metrics::PortalDetectionResultToEnum(result));

  result.trial_result.phase = ConnectivityTrial::kPhaseUnknown;
  result.trial_result.status = ConnectivityTrial::kStatusFailure;
  EXPECT_EQ(Metrics::kPortalResultUnknown,
            Metrics::PortalDetectionResultToEnum(result));
}

TEST_F(MetricsTest, TimeToConnect) {
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Cellular.TimeToConnect",
                Ge(0),
                Metrics::kMetricTimeToConnectMillisecondsMin,
                Metrics::kMetricTimeToConnectMillisecondsMax,
                Metrics::kMetricTimeToConnectMillisecondsNumBuckets));
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kCellular);
  metrics_.NotifyDeviceConnectStarted(kInterfaceIndex, false);
  metrics_.NotifyDeviceConnectFinished(kInterfaceIndex);
}

TEST_F(MetricsTest, TimeToDisable) {
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Cellular.TimeToDisable",
                Ge(0),
                Metrics::kMetricTimeToDisableMillisecondsMin,
                Metrics::kMetricTimeToDisableMillisecondsMax,
                Metrics::kMetricTimeToDisableMillisecondsNumBuckets));
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kCellular);
  metrics_.NotifyDeviceDisableStarted(kInterfaceIndex);
  metrics_.NotifyDeviceDisableFinished(kInterfaceIndex);
}

TEST_F(MetricsTest, TimeToEnable) {
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Cellular.TimeToEnable",
                Ge(0),
                Metrics::kMetricTimeToEnableMillisecondsMin,
                Metrics::kMetricTimeToEnableMillisecondsMax,
                Metrics::kMetricTimeToEnableMillisecondsNumBuckets));
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kCellular);
  metrics_.NotifyDeviceEnableStarted(kInterfaceIndex);
  metrics_.NotifyDeviceEnableFinished(kInterfaceIndex);
}

TEST_F(MetricsTest, TimeToInitialize) {
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Cellular.TimeToInitialize",
                Ge(0),
                Metrics::kMetricTimeToInitializeMillisecondsMin,
                Metrics::kMetricTimeToInitializeMillisecondsMax,
                Metrics::kMetricTimeToInitializeMillisecondsNumBuckets));
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kCellular);
  metrics_.NotifyDeviceInitialized(kInterfaceIndex);
}

TEST_F(MetricsTest, TimeToScan) {
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Cellular.TimeToScan",
                Ge(0),
                Metrics::kMetricTimeToScanMillisecondsMin,
                Metrics::kMetricTimeToScanMillisecondsMax,
                Metrics::kMetricTimeToScanMillisecondsNumBuckets));
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kCellular);
  metrics_.NotifyDeviceScanStarted(kInterfaceIndex);
  metrics_.NotifyDeviceScanFinished(kInterfaceIndex);
}

TEST_F(MetricsTest, TimeToScanAndConnect) {
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Wifi.TimeToScan",
                Ge(0),
                Metrics::kMetricTimeToScanMillisecondsMin,
                Metrics::kMetricTimeToScanMillisecondsMax,
                Metrics::kMetricTimeToScanMillisecondsNumBuckets));
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kWifi);
  metrics_.NotifyDeviceScanStarted(kInterfaceIndex);
  metrics_.NotifyDeviceScanFinished(kInterfaceIndex);

  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Wifi.TimeToConnect",
                Ge(0),
                Metrics::kMetricTimeToConnectMillisecondsMin,
                Metrics::kMetricTimeToConnectMillisecondsMax,
                Metrics::kMetricTimeToConnectMillisecondsNumBuckets));
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Wifi.TimeToScanAndConnect",
                Ge(0),
                Metrics::kMetricTimeToScanMillisecondsMin,
                Metrics::kMetricTimeToScanMillisecondsMax +
                    Metrics::kMetricTimeToConnectMillisecondsMax,
                Metrics::kMetricTimeToScanMillisecondsNumBuckets +
                    Metrics::kMetricTimeToConnectMillisecondsNumBuckets));
  metrics_.NotifyDeviceConnectStarted(kInterfaceIndex, false);
  metrics_.NotifyDeviceConnectFinished(kInterfaceIndex);
}

TEST_F(MetricsTest, SpontaneousConnect) {
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kWifi);
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Wifi.TimeToConnect",
                Ge(0),
                Metrics::kMetricTimeToConnectMillisecondsMin,
                Metrics::kMetricTimeToConnectMillisecondsMax,
                Metrics::kMetricTimeToConnectMillisecondsNumBuckets)).Times(0);
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Wifi.TimeToScanAndConnect",
                Ge(0),
                Metrics::kMetricTimeToScanMillisecondsMin,
                Metrics::kMetricTimeToScanMillisecondsMax +
                    Metrics::kMetricTimeToConnectMillisecondsMax,
                Metrics::kMetricTimeToScanMillisecondsNumBuckets +
                    Metrics::kMetricTimeToConnectMillisecondsNumBuckets)).
      Times(0);
  // This simulates a connection that is not scan-based.
  metrics_.NotifyDeviceConnectFinished(kInterfaceIndex);
}

TEST_F(MetricsTest, ResetConnectTimer) {
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kWifi);
  chromeos_metrics::TimerReporterMock* mock_scan_timer =
      new chromeos_metrics::TimerReporterMock;
  metrics_.set_time_to_scan_timer(kInterfaceIndex, mock_scan_timer);
  chromeos_metrics::TimerReporterMock* mock_connect_timer =
      new chromeos_metrics::TimerReporterMock;
  metrics_.set_time_to_connect_timer(kInterfaceIndex, mock_connect_timer);
  chromeos_metrics::TimerReporterMock* mock_scan_connect_timer =
      new chromeos_metrics::TimerReporterMock;
  metrics_.set_time_to_scan_connect_timer(kInterfaceIndex,
                                          mock_scan_connect_timer);
  EXPECT_CALL(*mock_scan_timer, Reset()).Times(0);
  EXPECT_CALL(*mock_connect_timer, Reset());
  EXPECT_CALL(*mock_scan_connect_timer, Reset());
  metrics_.ResetConnectTimer(kInterfaceIndex);
}

TEST_F(MetricsTest, TimeToScanNoStart) {
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Cellular.TimeToScan", _, _, _, _)).Times(0);
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kCellular);
  metrics_.NotifyDeviceScanFinished(kInterfaceIndex);
}

TEST_F(MetricsTest, TimeToScanIgnore) {
  // Make sure TimeToScan is not sent if the elapsed time exceeds the max
  // value.  This simulates the case where the device is in an area with no
  // service.
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kCellular);
  base::TimeDelta large_time_delta =
      base::TimeDelta::FromMilliseconds(
          Metrics::kMetricTimeToScanMillisecondsMax + 1);
  chromeos_metrics::TimerReporterMock* mock_time_to_scan_timer =
      new chromeos_metrics::TimerReporterMock;
  metrics_.set_time_to_scan_timer(kInterfaceIndex, mock_time_to_scan_timer);
  EXPECT_CALL(*mock_time_to_scan_timer, Stop()).WillOnce(Return(true));
  EXPECT_CALL(*mock_time_to_scan_timer, GetElapsedTime(_)).
      WillOnce(DoAll(SetArgumentPointee<0>(large_time_delta), Return(true)));
  EXPECT_CALL(library_, SendToUMA(_, _, _, _, _)).Times(0);
  metrics_.NotifyDeviceScanStarted(kInterfaceIndex);
  metrics_.NotifyDeviceScanFinished(kInterfaceIndex);
}

TEST_F(MetricsTest, Cellular3GPPRegistrationDelayedDropPosted) {
  EXPECT_CALL(library_,
      SendEnumToUMA(Metrics::kMetricCellular3GPPRegistrationDelayedDrop,
                    Metrics::kCellular3GPPRegistrationDelayedDropPosted,
                    Metrics::kCellular3GPPRegistrationDelayedDropMax));
  metrics_.Notify3GPPRegistrationDelayedDropPosted();
  Mock::VerifyAndClearExpectations(&library_);

  EXPECT_CALL(library_,
      SendEnumToUMA(Metrics::kMetricCellular3GPPRegistrationDelayedDrop,
                    Metrics::kCellular3GPPRegistrationDelayedDropCanceled,
                    Metrics::kCellular3GPPRegistrationDelayedDropMax));
  metrics_.Notify3GPPRegistrationDelayedDropCanceled();
}

TEST_F(MetricsTest, CellularAutoConnect) {
  EXPECT_CALL(library_,
      SendToUMA("Network.Shill.Cellular.TimeToConnect",
                Ge(0),
                Metrics::kMetricTimeToConnectMillisecondsMin,
                Metrics::kMetricTimeToConnectMillisecondsMax,
                Metrics::kMetricTimeToConnectMillisecondsNumBuckets));
  EXPECT_CALL(library_,
      SendToUMA(Metrics::kMetricCellularAutoConnectTotalTime,
                Ge(0),
                Metrics::kMetricCellularAutoConnectTotalTimeMin,
                Metrics::kMetricCellularAutoConnectTotalTimeMax,
                Metrics::kMetricCellularAutoConnectTotalTimeNumBuckets));
  EXPECT_CALL(library_,
      SendToUMA(Metrics::kMetricCellularAutoConnectTries,
                2,
                Metrics::kMetricCellularAutoConnectTriesMin,
                Metrics::kMetricCellularAutoConnectTriesMax,
                Metrics::kMetricCellularAutoConnectTriesNumBuckets));
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kCellular);
  metrics_.NotifyDeviceConnectStarted(kInterfaceIndex, true);
  metrics_.NotifyDeviceConnectStarted(kInterfaceIndex, true);
  metrics_.NotifyDeviceConnectFinished(kInterfaceIndex);
}

TEST_F(MetricsTest, CellularDrop) {
  const char* kUMATechnologyStrings[] = {
      kNetworkTechnology1Xrtt,
      kNetworkTechnologyEdge,
      kNetworkTechnologyEvdo,
      kNetworkTechnologyGprs,
      kNetworkTechnologyGsm,
      kNetworkTechnologyHspa,
      kNetworkTechnologyHspaPlus,
      kNetworkTechnologyLte,
      kNetworkTechnologyUmts,
      "Unknown" };

  const uint16_t signal_strength = 100;
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kCellular);
  for (size_t index = 0; index < arraysize(kUMATechnologyStrings); ++index) {
    EXPECT_CALL(library_,
        SendEnumToUMA(Metrics::kMetricCellularDrop,
                      index,
                      Metrics::kCellularDropTechnologyMax));
    EXPECT_CALL(library_,
        SendToUMA(Metrics::kMetricCellularSignalStrengthBeforeDrop,
                  signal_strength,
                  Metrics::kMetricCellularSignalStrengthBeforeDropMin,
                  Metrics::kMetricCellularSignalStrengthBeforeDropMax,
                  Metrics::kMetricCellularSignalStrengthBeforeDropNumBuckets));
    metrics_.NotifyCellularDeviceDrop(kUMATechnologyStrings[index],
                                      signal_strength);
    Mock::VerifyAndClearExpectations(&library_);
  }
}

TEST_F(MetricsTest, CellularDeviceFailure) {
  EXPECT_CALL(library_, SendEnumToUMA(Metrics::kMetricCellularFailure,
                                      Metrics::kMetricCellularConnectionFailure,
                                      Metrics::kMetricCellularMaxFailure));
  metrics_.NotifyCellularDeviceConnectionFailure();
}

TEST_F(MetricsTest, CellularOutOfCreditsReason) {
  EXPECT_CALL(library_,
      SendEnumToUMA(Metrics::kMetricCellularOutOfCreditsReason,
                    Metrics::kCellularOutOfCreditsReasonConnectDisconnectLoop,
                    Metrics::kCellularOutOfCreditsReasonMax));
  metrics_.NotifyCellularOutOfCredits(
      Metrics::kCellularOutOfCreditsReasonConnectDisconnectLoop);
  Mock::VerifyAndClearExpectations(&library_);

  EXPECT_CALL(library_,
      SendEnumToUMA(Metrics::kMetricCellularOutOfCreditsReason,
                    Metrics::kCellularOutOfCreditsReasonTxCongested,
                    Metrics::kCellularOutOfCreditsReasonMax));
  metrics_.NotifyCellularOutOfCredits(
      Metrics::kCellularOutOfCreditsReasonTxCongested);
  Mock::VerifyAndClearExpectations(&library_);

  EXPECT_CALL(library_,
      SendEnumToUMA(Metrics::kMetricCellularOutOfCreditsReason,
                    Metrics::kCellularOutOfCreditsReasonElongatedTimeWait,
                    Metrics::kCellularOutOfCreditsReasonMax));
  metrics_.NotifyCellularOutOfCredits(
      Metrics::kCellularOutOfCreditsReasonElongatedTimeWait);
}

TEST_F(MetricsTest, CorruptedProfile) {
  EXPECT_CALL(library_, SendEnumToUMA(Metrics::kMetricCorruptedProfile,
                                      Metrics::kCorruptedProfile,
                                      Metrics::kCorruptedProfileMax));
  metrics_.NotifyCorruptedProfile();
}

TEST_F(MetricsTest, Logging) {
  NiceScopedMockLog log;
  const int kVerboseLevel5 = -5;
  ScopeLogger::GetInstance()->EnableScopesByName("+metrics");
  ScopeLogger::GetInstance()->set_verbose_level(-kVerboseLevel5);

  const string kEnumName("fake-enum");
  const int kEnumValue = 1;
  const int kEnumMax = 12;
  EXPECT_CALL(log, Log(kVerboseLevel5, _,
                       "(metrics) Sending enum fake-enum with value 1."));
  EXPECT_CALL(library_, SendEnumToUMA(kEnumName, kEnumValue, kEnumMax));
  metrics_.SendEnumToUMA(kEnumName, kEnumValue, kEnumMax);

  const string kMetricName("fake-metric");
  const int kMetricValue = 2;
  const int kHistogramMin = 0;
  const int kHistogramMax = 100;
  const int kHistogramBuckets = 10;
  EXPECT_CALL(log, Log(kVerboseLevel5, _,
                       "(metrics) Sending metric fake-metric with value 2."));
  EXPECT_CALL(library_, SendToUMA(kMetricName, kMetricValue, kHistogramMin,
                                  kHistogramMax, kHistogramBuckets));
  metrics_.SendToUMA(kMetricName, kMetricValue,
                     kHistogramMin, kHistogramMax, kHistogramBuckets);

  ScopeLogger::GetInstance()->EnableScopesByName("-metrics");
  ScopeLogger::GetInstance()->set_verbose_level(0);
}

TEST_F(MetricsTest, NotifyServicesOnSameNetwork) {
  EXPECT_CALL(library_,
      SendToUMA(Metrics::kMetricServicesOnSameNetwork,
                1,
                Metrics::kMetricServicesOnSameNetworkMin,
                Metrics::kMetricServicesOnSameNetworkMax,
                Metrics::kMetricServicesOnSameNetworkNumBuckets));
  metrics_.NotifyServicesOnSameNetwork(1);
}

TEST_F(MetricsTest, NotifyUserInitiatedEvent) {
  EXPECT_CALL(library_,
      SendEnumToUMA(Metrics::kMetricUserInitiatedEvents,
                    Metrics::kUserInitiatedEventWifiScan,
                    Metrics::kUserInitiatedEventMax));
  metrics_.NotifyUserInitiatedEvent(Metrics::kUserInitiatedEventWifiScan);
}

TEST_F(MetricsTest, NotifyWifiTxBitrate) {
  EXPECT_CALL(library_,
      SendToUMA(Metrics::kMetricWifiTxBitrate,
                1,
                Metrics::kMetricWifiTxBitrateMin,
                Metrics::kMetricWifiTxBitrateMax,
                Metrics::kMetricWifiTxBitrateNumBuckets));
  metrics_.NotifyWifiTxBitrate(1);
}

TEST_F(MetricsTest, NotifyUserInitiatedConnectionResult) {
  EXPECT_CALL(library_,
      SendEnumToUMA(Metrics::kMetricWifiUserInitiatedConnectionResult,
                    Metrics::kUserInitiatedConnectionResultSuccess,
                    Metrics::kUserInitiatedConnectionResultMax));
  metrics_.NotifyUserInitiatedConnectionResult(
      Metrics::kMetricWifiUserInitiatedConnectionResult,
      Metrics::kUserInitiatedConnectionResultSuccess);
}

TEST_F(MetricsTest, NotifyFallbackDNSTestResult) {
  EXPECT_CALL(library_,
      SendEnumToUMA("Network.Shill.Wifi.FallbackDNSTestResult",
                    Metrics::kFallbackDNSTestResultSuccess,
                    Metrics::kFallbackDNSTestResultMax));
  metrics_.NotifyFallbackDNSTestResult(Technology::kWifi,
                                       Metrics::kFallbackDNSTestResultSuccess);
}

TEST_F(MetricsTest, NotifyNetworkProblemDetected) {
  EXPECT_CALL(library_,
      SendEnumToUMA("Network.Shill.Wifi.NetworkProblemDetected",
                    Metrics::kNetworkProblemDNSFailure,
                    Metrics::kNetworkProblemMax));
  metrics_.NotifyNetworkProblemDetected(Technology::kWifi,
                                        Metrics::kNetworkProblemDNSFailure);
}

TEST_F(MetricsTest, NotifyDhcpClientStatus) {
  EXPECT_CALL(library_,
      SendEnumToUMA("Network.Shill.DHCPClientStatus",
                    Metrics::kDhcpClientStatusReboot,
                    Metrics::kDhcpClientStatusMax));
  metrics_.NotifyDhcpClientStatus(Metrics::kDhcpClientStatusReboot);
}

TEST_F(MetricsTest, DeregisterDevice) {
  const int kInterfaceIndex = 1;
  metrics_.RegisterDevice(kInterfaceIndex, Technology::kCellular);

  EXPECT_CALL(library_,
      SendEnumToUMA("Network.Shill.DeviceRemovedEvent",
                    Metrics::kDeviceTechnologyTypeCellular,
                    Metrics::kDeviceTechnologyTypeMax));
  metrics_.DeregisterDevice(kInterfaceIndex);
}

TEST_F(MetricsTest, NotifyWakeOnWiFiFeaturesEnabledState) {
  const Metrics::WakeOnWiFiFeaturesEnabledState state =
      Metrics::kWakeOnWiFiFeaturesEnabledStateNone;
  EXPECT_CALL(
      library_,
      SendEnumToUMA("Network.Shill.WiFi.WakeOnWiFiFeaturesEnabledState", state,
                    Metrics::kWakeOnWiFiFeaturesEnabledStateMax));
  metrics_.NotifyWakeOnWiFiFeaturesEnabledState(state);
}

TEST_F(MetricsTest, NotifyVerifyWakeOnWiFiSettingsResult) {
  const Metrics::VerifyWakeOnWiFiSettingsResult result =
      Metrics::kVerifyWakeOnWiFiSettingsResultSuccess;
  EXPECT_CALL(
      library_,
      SendEnumToUMA("Network.Shill.WiFi.VerifyWakeOnWiFiSettingsResult", result,
                    Metrics::kVerifyWakeOnWiFiSettingsResultMax));
  metrics_.NotifyVerifyWakeOnWiFiSettingsResult(result);
}

TEST_F(MetricsTest, NotifyConnectedToServiceAfterWake) {
  const Metrics::WiFiConnectionStatusAfterWake status =
      Metrics::kWiFiConnetionStatusAfterWakeOnWiFiEnabledWakeConnected;
  EXPECT_CALL(library_,
              SendEnumToUMA("Network.Shill.WiFi.WiFiConnectionStatusAfterWake",
                            status, Metrics::kWiFiConnetionStatusAfterWakeMax));
  metrics_.NotifyConnectedToServiceAfterWake(status);
}

TEST_F(MetricsTest, NotifyWakeOnWiFiThrottled) {
  EXPECT_FALSE(metrics_.wake_on_wifi_throttled_);
  metrics_.NotifyWakeOnWiFiThrottled();
  EXPECT_TRUE(metrics_.wake_on_wifi_throttled_);
}

TEST_F(MetricsTest, NotifySuspendWithWakeOnWiFiEnabledDone) {
  const Metrics::WakeOnWiFiThrottled result_true =
      Metrics::kWakeOnWiFiThrottledTrue;
  metrics_.wake_on_wifi_throttled_ = true;
  EXPECT_CALL(library_,
              SendEnumToUMA("Network.Shill.WiFi.WakeOnWiFiThrottled",
                            result_true, Metrics::kWakeOnWiFiThrottledMax));
  metrics_.NotifySuspendWithWakeOnWiFiEnabledDone();

  const Metrics::WakeOnWiFiThrottled result_false =
      Metrics::kWakeOnWiFiThrottledFalse;
  metrics_.wake_on_wifi_throttled_ = false;
  EXPECT_CALL(library_,
              SendEnumToUMA("Network.Shill.WiFi.WakeOnWiFiThrottled",
                            result_false, Metrics::kWakeOnWiFiThrottledMax));
  metrics_.NotifySuspendWithWakeOnWiFiEnabledDone();
}

TEST_F(MetricsTest, NotifySuspendActionsCompleted_Success) {
  base::TimeDelta non_zero_time_delta = base::TimeDelta::FromMilliseconds(1);
  chromeos_metrics::TimerMock* mock_time_suspend_actions_timer =
      new chromeos_metrics::TimerMock;
  metrics_.set_time_suspend_actions_timer(mock_time_suspend_actions_timer);
  metrics_.wake_reason_received_ = true;
  EXPECT_CALL(*mock_time_suspend_actions_timer, GetElapsedTime(_))
      .WillOnce(
          DoAll(SetArgumentPointee<0>(non_zero_time_delta), Return(true)));
  EXPECT_CALL(*mock_time_suspend_actions_timer, HasStarted())
      .WillOnce(Return(true));
  EXPECT_CALL(library_,
              SendToUMA(Metrics::kMetricSuspendActionTimeTaken,
                        non_zero_time_delta.InMilliseconds(),
                        Metrics::kMetricSuspendActionTimeTakenMillisecondsMin,
                        Metrics::kMetricSuspendActionTimeTakenMillisecondsMax,
                        Metrics::kTimerHistogramNumBuckets));
  EXPECT_CALL(library_, SendEnumToUMA(Metrics::kMetricSuspendActionResult,
                                      Metrics::kSuspendActionResultSuccess,
                                      Metrics::kSuspendActionResultMax));
  metrics_.NotifySuspendActionsCompleted(true);
  EXPECT_FALSE(metrics_.wake_reason_received_);
}

TEST_F(MetricsTest, NotifySuspendActionsCompleted_Failure) {
  base::TimeDelta non_zero_time_delta = base::TimeDelta::FromMilliseconds(1);
  chromeos_metrics::TimerMock* mock_time_suspend_actions_timer =
      new chromeos_metrics::TimerMock;
  metrics_.set_time_suspend_actions_timer(mock_time_suspend_actions_timer);
  metrics_.wake_reason_received_ = true;
  EXPECT_CALL(*mock_time_suspend_actions_timer, GetElapsedTime(_))
      .WillOnce(
          DoAll(SetArgumentPointee<0>(non_zero_time_delta), Return(true)));
  EXPECT_CALL(*mock_time_suspend_actions_timer, HasStarted())
      .WillOnce(Return(true));
  EXPECT_CALL(library_,
              SendToUMA(Metrics::kMetricSuspendActionTimeTaken,
                        non_zero_time_delta.InMilliseconds(),
                        Metrics::kMetricSuspendActionTimeTakenMillisecondsMin,
                        Metrics::kMetricSuspendActionTimeTakenMillisecondsMax,
                        Metrics::kTimerHistogramNumBuckets));
  EXPECT_CALL(library_, SendEnumToUMA(Metrics::kMetricSuspendActionResult,
                                      Metrics::kSuspendActionResultFailure,
                                      Metrics::kSuspendActionResultMax));
  metrics_.NotifySuspendActionsCompleted(false);
  EXPECT_FALSE(metrics_.wake_reason_received_);
}

TEST_F(MetricsTest, NotifyDarkResumeActionsCompleted_Success) {
  metrics_.num_scan_results_expected_in_dark_resume_ = 0;
  base::TimeDelta non_zero_time_delta = base::TimeDelta::FromMilliseconds(1);
  chromeos_metrics::TimerMock* mock_time_dark_resume_actions_timer =
      new chromeos_metrics::TimerMock;
  metrics_.set_time_dark_resume_actions_timer(
      mock_time_dark_resume_actions_timer);
  metrics_.wake_reason_received_ = true;
  const int non_zero_num_retries = 3;
  metrics_.dark_resume_scan_retries_ = non_zero_num_retries;
  EXPECT_CALL(*mock_time_dark_resume_actions_timer, GetElapsedTime(_))
      .WillOnce(
          DoAll(SetArgumentPointee<0>(non_zero_time_delta), Return(true)));
  EXPECT_CALL(*mock_time_dark_resume_actions_timer, HasStarted())
      .WillOnce(Return(true));
  EXPECT_CALL(
      library_,
      SendToUMA(Metrics::kMetricDarkResumeActionTimeTaken,
                non_zero_time_delta.InMilliseconds(),
                Metrics::kMetricDarkResumeActionTimeTakenMillisecondsMin,
                Metrics::kMetricDarkResumeActionTimeTakenMillisecondsMax,
                Metrics::kTimerHistogramNumBuckets));
  EXPECT_CALL(library_, SendEnumToUMA(Metrics::kMetricDarkResumeActionResult,
                                      Metrics::kDarkResumeActionResultSuccess,
                                      Metrics::kDarkResumeActionResultMax));
  EXPECT_CALL(
      library_,
      SendEnumToUMA(Metrics::kMetricDarkResumeUnmatchedScanResultReceived,
                    Metrics::kDarkResumeUnmatchedScanResultsReceivedFalse,
                    Metrics::kDarkResumeUnmatchedScanResultsReceivedMax));
  EXPECT_CALL(library_, SendToUMA(Metrics::kMetricDarkResumeScanNumRetries,
                                  non_zero_num_retries,
                                  Metrics::kMetricDarkResumeScanNumRetriesMin,
                                  Metrics::kMetricDarkResumeScanNumRetriesMax,
                                  Metrics::kTimerHistogramNumBuckets));
  metrics_.NotifyDarkResumeActionsCompleted(true);
  EXPECT_FALSE(metrics_.wake_reason_received_);
}

TEST_F(MetricsTest, NotifyDarkResumeActionsCompleted_Failure) {
  metrics_.num_scan_results_expected_in_dark_resume_ = 0;
  base::TimeDelta non_zero_time_delta = base::TimeDelta::FromMilliseconds(1);
  chromeos_metrics::TimerMock* mock_time_dark_resume_actions_timer =
      new chromeos_metrics::TimerMock;
  metrics_.set_time_dark_resume_actions_timer(
      mock_time_dark_resume_actions_timer);
  metrics_.wake_reason_received_ = true;
  const int non_zero_num_retries = 3;
  metrics_.dark_resume_scan_retries_ = non_zero_num_retries;
  EXPECT_CALL(*mock_time_dark_resume_actions_timer, GetElapsedTime(_))
      .WillOnce(
          DoAll(SetArgumentPointee<0>(non_zero_time_delta), Return(true)));
  EXPECT_CALL(*mock_time_dark_resume_actions_timer, HasStarted())
      .WillOnce(Return(true));
  EXPECT_CALL(
      library_,
      SendToUMA(Metrics::kMetricDarkResumeActionTimeTaken,
                non_zero_time_delta.InMilliseconds(),
                Metrics::kMetricDarkResumeActionTimeTakenMillisecondsMin,
                Metrics::kMetricDarkResumeActionTimeTakenMillisecondsMax,
                Metrics::kTimerHistogramNumBuckets));
  EXPECT_CALL(library_, SendEnumToUMA(Metrics::kMetricDarkResumeActionResult,
                                      Metrics::kDarkResumeActionResultFailure,
                                      Metrics::kDarkResumeActionResultMax));
  EXPECT_CALL(
      library_,
      SendEnumToUMA(Metrics::kMetricDarkResumeUnmatchedScanResultReceived,
                    Metrics::kDarkResumeUnmatchedScanResultsReceivedFalse,
                    Metrics::kDarkResumeUnmatchedScanResultsReceivedMax));
  EXPECT_CALL(library_, SendToUMA(Metrics::kMetricDarkResumeScanNumRetries,
                                  non_zero_num_retries,
                                  Metrics::kMetricDarkResumeScanNumRetriesMin,
                                  Metrics::kMetricDarkResumeScanNumRetriesMax,
                                  Metrics::kTimerHistogramNumBuckets));
  metrics_.NotifyDarkResumeActionsCompleted(false);
  EXPECT_FALSE(metrics_.wake_reason_received_);
}

TEST_F(MetricsTest, NotifySuspendActionsStarted) {
  metrics_.time_suspend_actions_timer->Stop();
  metrics_.wake_on_wifi_throttled_ = true;
  metrics_.NotifySuspendActionsStarted();
  EXPECT_TRUE(metrics_.time_suspend_actions_timer->HasStarted());
  EXPECT_FALSE(metrics_.wake_on_wifi_throttled_);
}

TEST_F(MetricsTest, NotifyDarkResumeActionsStarted) {
  metrics_.time_dark_resume_actions_timer->Stop();
  metrics_.num_scan_results_expected_in_dark_resume_ = 2;
  metrics_.dark_resume_scan_retries_ = 3;
  metrics_.NotifyDarkResumeActionsStarted();
  EXPECT_TRUE(metrics_.time_dark_resume_actions_timer->HasStarted());
  EXPECT_EQ(0, metrics_.num_scan_results_expected_in_dark_resume_);
  EXPECT_EQ(0, metrics_.dark_resume_scan_retries_);
}

TEST_F(MetricsTest, NotifyDarkResumeInitiateScan) {
  metrics_.num_scan_results_expected_in_dark_resume_ = 0;
  metrics_.NotifyDarkResumeInitiateScan();
  EXPECT_EQ(1, metrics_.num_scan_results_expected_in_dark_resume_);
}

TEST_F(MetricsTest, NotifyDarkResumeScanResultsReceived) {
  metrics_.num_scan_results_expected_in_dark_resume_ = 1;
  metrics_.NotifyDarkResumeScanResultsReceived();
  EXPECT_EQ(0, metrics_.num_scan_results_expected_in_dark_resume_);
}

TEST_F(MetricsTest, NotifyDarkResumeScanRetry) {
  const int initial_num_retries = 2;
  metrics_.dark_resume_scan_retries_ = initial_num_retries;
  metrics_.NotifyDarkResumeScanRetry();
  EXPECT_EQ(initial_num_retries + 1, metrics_.dark_resume_scan_retries_);
}

TEST_F(MetricsTest, NotifyBeforeSuspendActions_InDarkResume) {
  const bool in_dark_resume = true;
  bool is_connected;
  metrics_.dark_resume_scan_retries_ = 1;

  is_connected = true;
  EXPECT_CALL(library_,
              SendEnumToUMA(Metrics::kMetricDarkResumeScanRetryResult,
                            Metrics::kDarkResumeScanRetryResultConnected,
                            Metrics::kDarkResumeScanRetryResultMax));
  metrics_.NotifyBeforeSuspendActions(is_connected, in_dark_resume);

  is_connected = false;
  EXPECT_CALL(library_,
              SendEnumToUMA(Metrics::kMetricDarkResumeScanRetryResult,
                            Metrics::kDarkResumeScanRetryResultNotConnected,
                            Metrics::kDarkResumeScanRetryResultMax));
  metrics_.NotifyBeforeSuspendActions(is_connected, in_dark_resume);
}

TEST_F(MetricsTest, NotifyBeforeSuspendActions_NotInDarkResume) {
  const bool in_dark_resume = false;
  bool is_connected;
  metrics_.dark_resume_scan_retries_ = 1;

  is_connected = true;
  EXPECT_CALL(library_, SendEnumToUMA(_, _, _)).Times(0);
  metrics_.NotifyBeforeSuspendActions(is_connected, in_dark_resume);

  is_connected = false;
  EXPECT_CALL(library_, SendEnumToUMA(_, _, _)).Times(0);
  metrics_.NotifyBeforeSuspendActions(is_connected, in_dark_resume);
}

TEST_F(MetricsTest, NotifyConnectionDiagnosticsIssue_Success) {
  const string& issue = ConnectionDiagnostics::kIssueIPCollision;
  EXPECT_CALL(library_,
              SendEnumToUMA(Metrics::kMetricConnectionDiagnosticsIssue,
                            Metrics::kConnectionDiagnosticsIssueIPCollision,
                            Metrics::kConnectionDiagnosticsIssueMax));
  metrics_.NotifyConnectionDiagnosticsIssue(issue);
}

TEST_F(MetricsTest, NotifyConnectionDiagnosticsIssue_Failure) {
  const string& invalid_issue = "Invalid issue string.";
  EXPECT_CALL(library_, SendEnumToUMA(_, _, _)).Times(0);
  metrics_.NotifyConnectionDiagnosticsIssue(invalid_issue);
}

#ifndef NDEBUG

typedef MetricsTest MetricsDeathTest;

TEST_F(MetricsDeathTest, PortalDetectionResultToEnumDNSSuccess) {
  PortalDetector::Result result(
      ConnectivityTrial::Result(ConnectivityTrial::kPhaseDNS,
                                ConnectivityTrial::kStatusSuccess),
      0, true);
  EXPECT_DEATH(Metrics::PortalDetectionResultToEnum(result),
               "Final result status 1 is not allowed in the DNS phase");
}

TEST_F(MetricsDeathTest, PortalDetectionResultToEnumConnectionSuccess) {
  PortalDetector::Result result(
      ConnectivityTrial::Result(ConnectivityTrial::kPhaseConnection,
                                ConnectivityTrial::kStatusSuccess),
      0, true);
  EXPECT_DEATH(Metrics::PortalDetectionResultToEnum(result),
               "Final result status 1 is not allowed in the Connection phase");
}

TEST_F(MetricsDeathTest, PortalDetectionResultToEnumHTTPSuccess) {
  PortalDetector::Result result(
      ConnectivityTrial::Result(ConnectivityTrial::kPhaseHTTP,
                                ConnectivityTrial::kStatusSuccess),
      0, true);
  EXPECT_DEATH(Metrics::PortalDetectionResultToEnum(result),
               "Final result status 1 is not allowed in the HTTP phase");
}

#endif  // NDEBUG

}  // namespace shill
