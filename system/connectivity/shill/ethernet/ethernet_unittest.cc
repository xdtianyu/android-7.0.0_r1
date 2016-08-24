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

#include "shill/ethernet/ethernet.h"

#include <netinet/ether.h>
#include <linux/if.h>  // NOLINT - Needs definitions from netinet/ether.h
#include <linux/sockios.h>

#include <memory>
#include <utility>
#include <vector>

#include <base/memory/ref_counted.h>

#include "shill/dhcp/mock_dhcp_config.h"
#include "shill/dhcp/mock_dhcp_provider.h"
#include "shill/ethernet/mock_ethernet_service.h"
#include "shill/mock_device_info.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_log.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_service.h"
#include "shill/net/mock_rtnl_handler.h"
#include "shill/net/mock_sockets.h"
#include "shill/nice_mock_control.h"
#include "shill/testing.h"

#if !defined(DISABLE_WIRED_8021X)
#include "shill/ethernet/mock_ethernet_eap_provider.h"
#include "shill/mock_eap_credentials.h"
#include "shill/mock_eap_listener.h"
#include "shill/supplicant/mock_supplicant_interface_proxy.h"
#include "shill/supplicant/mock_supplicant_process_proxy.h"
#include "shill/supplicant/wpa_supplicant.h"
#endif  // DISABLE_WIRED_8021X

using std::pair;
using std::string;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::EndsWith;
using testing::Eq;
using testing::InSequence;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::SetArgumentPointee;
using testing::StrEq;
using testing::StrictMock;

namespace shill {

class EthernetTest : public testing::Test {
 public:
  EthernetTest()
      : metrics_(nullptr),
        manager_(&control_interface_, nullptr, &metrics_),
        device_info_(&control_interface_, &dispatcher_, &metrics_, &manager_),
        ethernet_(new Ethernet(&control_interface_,
                               &dispatcher_,
                               &metrics_,
                               &manager_,
                               kDeviceName,
                               kDeviceAddress,
                               kInterfaceIndex)),
        dhcp_config_(new MockDHCPConfig(&control_interface_,
                                        kDeviceName)),
#if !defined(DISABLE_WIRED_8021X)
        eap_listener_(new MockEapListener()),
        mock_eap_service_(new MockService(&control_interface_,
                                          &dispatcher_,
                                          &metrics_,
                                          &manager_)),
        supplicant_interface_proxy_(
            new NiceMock<MockSupplicantInterfaceProxy>()),
        supplicant_process_proxy_(new NiceMock<MockSupplicantProcessProxy>()),
#endif  // DISABLE_WIRED_8021X
        mock_sockets_(new StrictMock<MockSockets>()),
        mock_service_(new MockEthernetService(
            &control_interface_, &metrics_,
            ethernet_->weak_ptr_factory_.GetWeakPtr())) {}
  ~EthernetTest() override {}

  void SetUp() override {
    ethernet_->rtnl_handler_ = &rtnl_handler_;
    ethernet_->sockets_.reset(mock_sockets_);  // Transfers ownership.

    ethernet_->set_dhcp_provider(&dhcp_provider_);
    ON_CALL(manager_, device_info()).WillByDefault(Return(&device_info_));
    EXPECT_CALL(manager_, UpdateEnabledTechnologies()).Times(AnyNumber());

#if !defined(DISABLE_WIRED_8021X)
    ethernet_->eap_listener_.reset(eap_listener_);  // Transfers ownership.
    EXPECT_CALL(manager_, ethernet_eap_provider())
        .WillRepeatedly(Return(&ethernet_eap_provider_));
    ethernet_eap_provider_.set_service(mock_eap_service_);
    // Transfers ownership.
    ethernet_->supplicant_process_proxy_.reset(supplicant_process_proxy_);
#endif  // DISABLE_WIRED_8021X

    ON_CALL(*mock_service_, technology())
        .WillByDefault(Return(Technology::kEthernet));
  }

  void TearDown() override {
#if !defined(DISABLE_WIRED_8021X)
    ethernet_eap_provider_.set_service(nullptr);
    ethernet_->eap_listener_.reset();
#endif  // DISABLE_WIRED_8021X
    ethernet_->set_dhcp_provider(nullptr);
    ethernet_->sockets_.reset();
    Mock::VerifyAndClearExpectations(&manager_);
    ethernet_->Stop(nullptr, EnabledStateChangedCallback());
  }

 protected:
  static const char kDeviceName[];
  static const char kDeviceAddress[];
  static const char kInterfacePath[];
  static const int kInterfaceIndex;

  bool GetLinkUp() { return ethernet_->link_up_; }
  void SetLinkUp(bool link_up) { ethernet_->link_up_ = link_up; }
  const ServiceRefPtr& GetSelectedService() {
    return ethernet_->selected_service();
  }
  ServiceRefPtr GetService() { return ethernet_->service_; }
  void SetService(const EthernetServiceRefPtr& service) {
    ethernet_->service_ = service;
  }
  const PropertyStore& GetStore() { return ethernet_->store(); }
  void StartEthernet() {
    EXPECT_CALL(rtnl_handler_,
                SetInterfaceFlags(kInterfaceIndex, IFF_UP, IFF_UP));
    ethernet_->Start(nullptr, EnabledStateChangedCallback());
  }

#if !defined(DISABLE_WIRED_8021X)
  bool GetIsEapAuthenticated() { return ethernet_->is_eap_authenticated_; }
  void SetIsEapAuthenticated(bool is_eap_authenticated) {
    ethernet_->is_eap_authenticated_ = is_eap_authenticated;
  }
  bool GetIsEapDetected() { return ethernet_->is_eap_detected_; }
  void SetIsEapDetected(bool is_eap_detected) {
    ethernet_->is_eap_detected_ = is_eap_detected;
  }
  const SupplicantInterfaceProxyInterface* GetSupplicantInterfaceProxy() {
    return ethernet_->supplicant_interface_proxy_.get();
  }
  const string& GetSupplicantInterfacePath() {
    return ethernet_->supplicant_interface_path_;
  }
  const string& GetSupplicantNetworkPath() {
    return ethernet_->supplicant_network_path_;
  }
  void SetSupplicantNetworkPath(const string& network_path) {
    ethernet_->supplicant_network_path_ = network_path;
  }
  bool InvokeStartSupplicant() {
    return ethernet_->StartSupplicant();
  }
  void InvokeStopSupplicant() {
    return ethernet_->StopSupplicant();
  }
  bool InvokeStartEapAuthentication() {
    return ethernet_->StartEapAuthentication();
  }
  void StartSupplicant() {
    MockSupplicantInterfaceProxy* interface_proxy =
        ExpectCreateSupplicantInterfaceProxy();
    EXPECT_CALL(*supplicant_process_proxy_, CreateInterface(_, _))
        .WillOnce(DoAll(SetArgumentPointee<1>(string(kInterfacePath)),
                        Return(true)));
    EXPECT_TRUE(InvokeStartSupplicant());
    EXPECT_EQ(interface_proxy, GetSupplicantInterfaceProxy());
    EXPECT_EQ(kInterfacePath, GetSupplicantInterfacePath());
  }
  void TriggerOnEapDetected() { ethernet_->OnEapDetected(); }
  void TriggerCertification(const string& subject, uint32_t depth) {
    ethernet_->CertificationTask(subject, depth);
  }
  void TriggerTryEapAuthentication() {
    ethernet_->TryEapAuthenticationTask();
  }

  MockSupplicantInterfaceProxy* ExpectCreateSupplicantInterfaceProxy() {
    EXPECT_CALL(control_interface_,
                CreateSupplicantInterfaceProxy(_, kInterfacePath))
        .WillOnce(ReturnAndReleasePointee(&supplicant_interface_proxy_));
    return supplicant_interface_proxy_.get();
  }
#endif  // DISABLE_WIRED_8021X

  StrictMock<MockEventDispatcher> dispatcher_;
  NiceMockControl control_interface_;
  NiceMock<MockMetrics> metrics_;
  MockManager manager_;
  MockDeviceInfo device_info_;
  EthernetRefPtr ethernet_;
  MockDHCPProvider dhcp_provider_;
  scoped_refptr<MockDHCPConfig> dhcp_config_;

#if !defined(DISABLE_WIRED_8021X)
  MockEthernetEapProvider ethernet_eap_provider_;

  // Owned by Ethernet instance, but tracked here for expectations.
  MockEapListener* eap_listener_;

  scoped_refptr<MockService> mock_eap_service_;
  std::unique_ptr<MockSupplicantInterfaceProxy> supplicant_interface_proxy_;
  MockSupplicantProcessProxy* supplicant_process_proxy_;
#endif  // DISABLE_WIRED_8021X

  // Owned by Ethernet instance, but tracked here for expectations.
  MockSockets* mock_sockets_;

  MockRTNLHandler rtnl_handler_;
  scoped_refptr<MockEthernetService> mock_service_;
};

// static
const char EthernetTest::kDeviceName[] = "eth0";
const char EthernetTest::kDeviceAddress[] = "000102030405";
const char EthernetTest::kInterfacePath[] = "/interface/path";
const int EthernetTest::kInterfaceIndex = 123;

TEST_F(EthernetTest, Construct) {
  EXPECT_FALSE(GetLinkUp());
#if !defined(DISABLE_WIRED_8021X)
  EXPECT_FALSE(GetIsEapAuthenticated());
  EXPECT_FALSE(GetIsEapDetected());
  EXPECT_TRUE(GetStore().Contains(kEapAuthenticationCompletedProperty));
  EXPECT_TRUE(GetStore().Contains(kEapAuthenticatorDetectedProperty));
#endif  // DISABLE_WIRED_8021X
  EXPECT_NE(nullptr, GetService().get());
}

TEST_F(EthernetTest, StartStop) {
  Service* service = GetService().get();
  EXPECT_CALL(manager_, RegisterService(Eq(service)));
  StartEthernet();

  EXPECT_CALL(manager_, DeregisterService(Eq(service)));
  ethernet_->Stop(nullptr, EnabledStateChangedCallback());

  // Ethernet device retains its service.
  EXPECT_NE(nullptr, GetService());
}

TEST_F(EthernetTest, LinkEvent) {
  StartEthernet();
  SetService(mock_service_);

  // Link-down event while already down.
  EXPECT_CALL(manager_, DeregisterService(_)).Times(0);
#if !defined(DISABLE_WIRED_8021X)
  EXPECT_CALL(*eap_listener_, Start()).Times(0);
#endif  // DISABLE_WIRED_8021X
  ethernet_->LinkEvent(0, IFF_LOWER_UP);
  EXPECT_FALSE(GetLinkUp());
#if !defined(DISABLE_WIRED_8021X)
  EXPECT_FALSE(GetIsEapDetected());
#endif  // DISABLE_WIRED_8021X
  Mock::VerifyAndClearExpectations(&manager_);

  // Link-up event while down.
  int kFakeFd = 789;
  EXPECT_CALL(manager_, UpdateService(IsRefPtrTo(mock_service_)));
  EXPECT_CALL(*mock_service_, OnVisibilityChanged());
#if !defined(DISABLE_WIRED_8021X)
  EXPECT_CALL(*eap_listener_, Start());
#endif  // DISABLE_WIRED_8021X
  EXPECT_CALL(*mock_sockets_, Socket(_, _, _)).WillOnce(Return(kFakeFd));
  EXPECT_CALL(*mock_sockets_, Ioctl(kFakeFd, SIOCETHTOOL, _));
  EXPECT_CALL(*mock_sockets_, Close(kFakeFd));
  ethernet_->LinkEvent(IFF_LOWER_UP, 0);
  EXPECT_TRUE(GetLinkUp());
#if !defined(DISABLE_WIRED_8021X)
  EXPECT_FALSE(GetIsEapDetected());
#endif  // DISABLE_WIRED_8021X
  Mock::VerifyAndClearExpectations(&manager_);
  Mock::VerifyAndClearExpectations(mock_service_.get());

  // Link-up event while already up.
  EXPECT_CALL(manager_, UpdateService(_)).Times(0);
  EXPECT_CALL(*mock_service_, OnVisibilityChanged()).Times(0);
#if !defined(DISABLE_WIRED_8021X)
  EXPECT_CALL(*eap_listener_, Start()).Times(0);
#endif  // DISABLE_WIRED_8021X
  ethernet_->LinkEvent(IFF_LOWER_UP, 0);
  EXPECT_TRUE(GetLinkUp());
#if !defined(DISABLE_WIRED_8021X)
  EXPECT_FALSE(GetIsEapDetected());
#endif  // DISABLE_WIRED_8021X
  Mock::VerifyAndClearExpectations(&manager_);
  Mock::VerifyAndClearExpectations(mock_service_.get());

  // Link-down event while up.
#if !defined(DISABLE_WIRED_8021X)
  SetIsEapDetected(true);
  // This is done in SetUp, but we have to reestablish this after calling
  // VerifyAndClearExpectations() above.
  EXPECT_CALL(manager_, ethernet_eap_provider())
      .WillRepeatedly(Return(&ethernet_eap_provider_));
  EXPECT_CALL(ethernet_eap_provider_,
              ClearCredentialChangeCallback(ethernet_.get()));
  EXPECT_CALL(*eap_listener_, Stop());
#endif  // DISABLE_WIRED_8021X
  EXPECT_CALL(manager_, UpdateService(IsRefPtrTo(GetService().get())));
  EXPECT_CALL(*mock_service_, OnVisibilityChanged());
  ethernet_->LinkEvent(0, IFF_LOWER_UP);
  EXPECT_FALSE(GetLinkUp());
#if !defined(DISABLE_WIRED_8021X)
  EXPECT_FALSE(GetIsEapDetected());
#endif  // DISABLE_WIRED_8021X

  // Restore this expectation during shutdown.
  EXPECT_CALL(manager_, UpdateEnabledTechnologies()).Times(AnyNumber());
}

TEST_F(EthernetTest, ConnectToLinkDown) {
  StartEthernet();
  SetService(mock_service_);
  SetLinkUp(false);
  EXPECT_EQ(nullptr, GetSelectedService().get());
  EXPECT_CALL(dhcp_provider_, CreateIPv4Config(_, _, _, _)).Times(0);
  EXPECT_CALL(*dhcp_config_.get(), RequestIP()).Times(0);
  EXPECT_CALL(dispatcher_, PostTask(_)).Times(0);
  EXPECT_CALL(*mock_service_, SetState(_)).Times(0);
  ethernet_->ConnectTo(mock_service_.get());
  EXPECT_EQ(nullptr, GetSelectedService().get());
}

TEST_F(EthernetTest, ConnectToFailure) {
  StartEthernet();
  SetService(mock_service_);
  SetLinkUp(true);
  EXPECT_EQ(nullptr, GetSelectedService().get());
  EXPECT_CALL(dhcp_provider_, CreateIPv4Config(_, _, _, _)).
      WillOnce(Return(dhcp_config_));
  EXPECT_CALL(*dhcp_config_.get(), RequestIP()).WillOnce(Return(false));
  EXPECT_CALL(dispatcher_, PostTask(_));  // Posts ConfigureStaticIPTask.
  EXPECT_CALL(*mock_service_, SetState(Service::kStateFailure));
  ethernet_->ConnectTo(mock_service_.get());
  EXPECT_EQ(mock_service_, GetSelectedService().get());
}

TEST_F(EthernetTest, ConnectToSuccess) {
  StartEthernet();
  SetService(mock_service_);
  SetLinkUp(true);
  EXPECT_EQ(nullptr, GetSelectedService().get());
  EXPECT_CALL(dhcp_provider_, CreateIPv4Config(_, _, _, _)).
      WillOnce(Return(dhcp_config_));
  EXPECT_CALL(*dhcp_config_.get(), RequestIP()).WillOnce(Return(true));
  EXPECT_CALL(dispatcher_, PostTask(_));  // Posts ConfigureStaticIPTask.
  EXPECT_CALL(*mock_service_, SetState(Service::kStateConfiguring));
  ethernet_->ConnectTo(mock_service_.get());
  EXPECT_EQ(GetService().get(), GetSelectedService().get());
  Mock::VerifyAndClearExpectations(mock_service_.get());

  EXPECT_CALL(*mock_service_, SetState(Service::kStateIdle));
  ethernet_->DisconnectFrom(mock_service_.get());
  EXPECT_EQ(nullptr, GetSelectedService().get());
}

#if !defined(DISABLE_WIRED_8021X)
TEST_F(EthernetTest, OnEapDetected) {
  EXPECT_FALSE(GetIsEapDetected());
  EXPECT_CALL(*eap_listener_, Stop());
  EXPECT_CALL(ethernet_eap_provider_,
              SetCredentialChangeCallback(ethernet_.get(), _));
  EXPECT_CALL(dispatcher_, PostTask(_));  // Posts TryEapAuthenticationTask.
  TriggerOnEapDetected();
  EXPECT_TRUE(GetIsEapDetected());
}

TEST_F(EthernetTest, TryEapAuthenticationNotConnectableNotAuthenticated) {
  SetService(mock_service_);
  EXPECT_CALL(*mock_eap_service_, Is8021xConnectable()).WillOnce(Return(false));
  NiceScopedMockLog log;
  EXPECT_CALL(log, Log(logging::LOG_INFO, _,
                       EndsWith("EAP Service lacks 802.1X credentials; "
                                "not doing EAP authentication.")));
  TriggerTryEapAuthentication();
  SetService(nullptr);
}

TEST_F(EthernetTest, TryEapAuthenticationNotConnectableAuthenticated) {
  SetService(mock_service_);
  SetIsEapAuthenticated(true);
  EXPECT_CALL(*mock_eap_service_, Is8021xConnectable()).WillOnce(Return(false));
  NiceScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(logging::LOG_INFO, _,
                       EndsWith("EAP Service lost 802.1X credentials; "
                                "terminating EAP authentication.")));
  TriggerTryEapAuthentication();
  EXPECT_FALSE(GetIsEapAuthenticated());
}

TEST_F(EthernetTest, TryEapAuthenticationEapNotDetected) {
  SetService(mock_service_);
  EXPECT_CALL(*mock_eap_service_, Is8021xConnectable()).WillOnce(Return(true));
  NiceScopedMockLog log;
  EXPECT_CALL(log, Log(logging::LOG_WARNING, _,
                       EndsWith("EAP authenticator not detected; "
                                "not doing EAP authentication.")));
  TriggerTryEapAuthentication();
}

TEST_F(EthernetTest, StartSupplicant) {
  // Save the mock proxy pointers before the Ethernet instance accepts it.
  MockSupplicantInterfaceProxy* interface_proxy =
      supplicant_interface_proxy_.get();
  MockSupplicantProcessProxy* process_proxy = supplicant_process_proxy_;

  StartSupplicant();

  // Starting it again should not invoke another call to create an interface.
  Mock::VerifyAndClearExpectations(process_proxy);
  EXPECT_CALL(*process_proxy, CreateInterface(_, _)).Times(0);
  EXPECT_TRUE(InvokeStartSupplicant());

  // Also, the mock pointers should remain; if the MockProxyFactory was
  // invoked again, they would be nullptr.
  EXPECT_EQ(interface_proxy, GetSupplicantInterfaceProxy());
  EXPECT_EQ(kInterfacePath, GetSupplicantInterfacePath());
}

TEST_F(EthernetTest, StartSupplicantWithInterfaceExistsException) {
  MockSupplicantProcessProxy* process_proxy = supplicant_process_proxy_;
  MockSupplicantInterfaceProxy* interface_proxy =
      ExpectCreateSupplicantInterfaceProxy();
  EXPECT_CALL(*process_proxy, CreateInterface(_, _)).WillOnce(Return(false));
  EXPECT_CALL(*process_proxy, GetInterface(kDeviceName, _))
      .WillOnce(
          DoAll(SetArgumentPointee<1>(string(kInterfacePath)), Return(true)));
  EXPECT_TRUE(InvokeStartSupplicant());
  EXPECT_EQ(interface_proxy, GetSupplicantInterfaceProxy());
  EXPECT_EQ(kInterfacePath, GetSupplicantInterfacePath());
}

TEST_F(EthernetTest, StartSupplicantWithUnknownException) {
  MockSupplicantProcessProxy* process_proxy = supplicant_process_proxy_;
  EXPECT_CALL(*process_proxy, CreateInterface(_, _)).WillOnce(Return(false));
  EXPECT_CALL(*process_proxy, GetInterface(kDeviceName, _))
      .WillOnce(Return(false));
  EXPECT_FALSE(InvokeStartSupplicant());
  EXPECT_EQ(nullptr, GetSupplicantInterfaceProxy());
  EXPECT_EQ("", GetSupplicantInterfacePath());
}

TEST_F(EthernetTest, StartEapAuthentication) {
  MockSupplicantInterfaceProxy* interface_proxy =
      supplicant_interface_proxy_.get();

  StartSupplicant();
  SetService(mock_service_);

  EXPECT_CALL(*mock_service_, ClearEAPCertification());
  MockEapCredentials mock_eap_credentials;
  EXPECT_CALL(*mock_eap_service_, eap())
      .WillOnce(Return(&mock_eap_credentials));
  EXPECT_CALL(mock_eap_credentials, PopulateSupplicantProperties(_, _));
  EXPECT_CALL(*interface_proxy, RemoveNetwork(_)).Times(0);
  EXPECT_CALL(*interface_proxy, AddNetwork(_, _))
      .WillOnce(Return(false));
  EXPECT_CALL(*interface_proxy, SelectNetwork(_)).Times(0);
  EXPECT_CALL(*interface_proxy, EAPLogon()).Times(0);
  EXPECT_FALSE(InvokeStartEapAuthentication());
  Mock::VerifyAndClearExpectations(mock_service_.get());
  Mock::VerifyAndClearExpectations(mock_eap_service_.get());
  Mock::VerifyAndClearExpectations(interface_proxy);
  EXPECT_EQ("", GetSupplicantNetworkPath());

  EXPECT_CALL(*mock_service_, ClearEAPCertification());
  EXPECT_CALL(*interface_proxy, RemoveNetwork(_)).Times(0);
  EXPECT_CALL(*mock_eap_service_, eap())
      .WillOnce(Return(&mock_eap_credentials));
  EXPECT_CALL(mock_eap_credentials, PopulateSupplicantProperties(_, _));
  const char kFirstNetworkPath[] = "/network/first-path";
  EXPECT_CALL(*interface_proxy, AddNetwork(_, _))
      .WillOnce(
          DoAll(SetArgumentPointee<1>(string(kFirstNetworkPath)),
                Return(true)));
  EXPECT_CALL(*interface_proxy, SelectNetwork(StrEq(kFirstNetworkPath)));
  EXPECT_CALL(*interface_proxy, EAPLogon());
  EXPECT_TRUE(InvokeStartEapAuthentication());
  Mock::VerifyAndClearExpectations(mock_service_.get());
  Mock::VerifyAndClearExpectations(mock_eap_service_.get());
  Mock::VerifyAndClearExpectations(&mock_eap_credentials);
  Mock::VerifyAndClearExpectations(interface_proxy);
  EXPECT_EQ(kFirstNetworkPath, GetSupplicantNetworkPath());

  EXPECT_CALL(*mock_service_, ClearEAPCertification());
  EXPECT_CALL(*interface_proxy, RemoveNetwork(StrEq(kFirstNetworkPath)))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_eap_service_, eap())
      .WillOnce(Return(&mock_eap_credentials));
  EXPECT_CALL(mock_eap_credentials, PopulateSupplicantProperties(_, _));
  const char kSecondNetworkPath[] = "/network/second-path";
  EXPECT_CALL(*interface_proxy, AddNetwork(_, _))
      .WillOnce(
          DoAll(SetArgumentPointee<1>(string(kSecondNetworkPath)),
                Return(true)));
  EXPECT_CALL(*interface_proxy, SelectNetwork(StrEq(kSecondNetworkPath)));
  EXPECT_CALL(*interface_proxy, EAPLogon());
  EXPECT_TRUE(InvokeStartEapAuthentication());
  EXPECT_EQ(kSecondNetworkPath, GetSupplicantNetworkPath());
}

TEST_F(EthernetTest, StopSupplicant) {
  MockSupplicantProcessProxy* process_proxy = supplicant_process_proxy_;
  MockSupplicantInterfaceProxy* interface_proxy =
      supplicant_interface_proxy_.get();
  StartSupplicant();
  SetIsEapAuthenticated(true);
  SetSupplicantNetworkPath("/network/1");
  EXPECT_CALL(*interface_proxy, EAPLogoff()).WillOnce(Return(true));
  EXPECT_CALL(*process_proxy, RemoveInterface(StrEq(kInterfacePath)))
      .WillOnce(Return(true));
  InvokeStopSupplicant();
  EXPECT_EQ(nullptr, GetSupplicantInterfaceProxy());
  EXPECT_EQ("", GetSupplicantInterfacePath());
  EXPECT_EQ("", GetSupplicantNetworkPath());
  EXPECT_FALSE(GetIsEapAuthenticated());
}

TEST_F(EthernetTest, Certification) {
  const string kSubjectName("subject-name");
  const uint32_t kDepth = 123;
  // Should not crash due to no service_.
  TriggerCertification(kSubjectName, kDepth);
  EXPECT_CALL(*mock_service_, AddEAPCertification(kSubjectName, kDepth));
  SetService(mock_service_);
  TriggerCertification(kSubjectName, kDepth);
}
#endif  // DISABLE_WIRED_8021X

#if !defined(DISABLE_PPPOE)

MATCHER_P(TechnologyEq, technology, "") {
  return arg->technology() == technology;
}

TEST_F(EthernetTest, TogglePPPoE) {
  SetService(mock_service_);

  EXPECT_CALL(*mock_service_, technology())
      .WillRepeatedly(Return(Technology::kEthernet));
  EXPECT_CALL(*mock_service_, Disconnect(_, _));

  InSequence sequence;
  EXPECT_CALL(manager_, DeregisterService(Eq(mock_service_)));
  EXPECT_CALL(manager_, RegisterService(TechnologyEq(Technology::kPPPoE)));
  EXPECT_CALL(manager_, DeregisterService(TechnologyEq(Technology::kPPPoE)));
  EXPECT_CALL(manager_, RegisterService(_));

  const vector<pair<bool, Technology::Identifier>> transitions = {
    {false, Technology::kEthernet},
    {true,  Technology::kPPPoE},
    {false, Technology::kEthernet},
  };
  for (const auto transition : transitions) {
    Error error;
    ethernet_->mutable_store()->SetBoolProperty(
        kPPPoEProperty, transition.first, &error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(GetService()->technology(), transition.second);
  }
}

#else

TEST_F(EthernetTest, PPPoEDisabled) {
  Error error;
  ethernet_->mutable_store()->SetBoolProperty(kPPPoEProperty, true, &error);
  EXPECT_FALSE(error.IsSuccess());
}

#endif  // DISABLE_PPPOE

}  // namespace shill
