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

#include "shill/cellular/cellular.h"

#include <sys/socket.h>
#include <linux/if.h>  // NOLINT - Needs typedefs from sys/socket.h.
#include <linux/netlink.h>

#include <base/bind.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/cellular/cellular_bearer.h"
#include "shill/cellular/cellular_capability_cdma.h"
#include "shill/cellular/cellular_capability_classic.h"
#include "shill/cellular/cellular_capability_gsm.h"
#include "shill/cellular/cellular_capability_universal.h"
#include "shill/cellular/cellular_service.h"
#include "shill/cellular/mock_cellular_service.h"
#include "shill/cellular/mock_mm1_modem_modem3gpp_proxy.h"
#include "shill/cellular/mock_mm1_modem_proxy.h"
#include "shill/cellular/mock_mm1_modem_simple_proxy.h"
#include "shill/cellular/mock_mobile_operator_info.h"
#include "shill/cellular/mock_modem_cdma_proxy.h"
#include "shill/cellular/mock_modem_gsm_card_proxy.h"
#include "shill/cellular/mock_modem_gsm_network_proxy.h"
#include "shill/cellular/mock_modem_info.h"
#include "shill/cellular/mock_modem_proxy.h"
#include "shill/cellular/mock_modem_simple_proxy.h"
#include "shill/dhcp/mock_dhcp_config.h"
#include "shill/dhcp/mock_dhcp_provider.h"
#include "shill/error.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_control.h"
#include "shill/mock_dbus_properties_proxy.h"
#include "shill/mock_device_info.h"
#include "shill/mock_external_task.h"
#include "shill/mock_ppp_device.h"
#include "shill/mock_ppp_device_factory.h"
#include "shill/mock_process_manager.h"
#include "shill/net/mock_rtnl_handler.h"
#include "shill/property_store_unittest.h"
#include "shill/rpc_task.h"  // for RpcTaskDelegate
#include "shill/test_event_dispatcher.h"
#include "shill/testing.h"

// mm/mm-modem.h must be included after cellular_capability_universal.h
// in order to allow MM_MODEM_CDMA_* to be defined properly.
#include <mm/mm-modem.h>

using base::Bind;
using base::Unretained;
using std::map;
using std::string;
using std::unique_ptr;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::DoAll;
using testing::Invoke;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::SaveArg;
using testing::SetArgumentPointee;
using testing::Unused;

namespace shill {

class CellularPropertyTest : public PropertyStoreTest {
 public:
  CellularPropertyTest()
      : modem_info_(control_interface(),
                    dispatcher(),
                    metrics(),
                    manager()),
        device_(new Cellular(&modem_info_,
                             "usb0",
                             "00:01:02:03:04:05",
                             3,
                             Cellular::kTypeCDMA,
                             "",
                             "")) {}
  virtual ~CellularPropertyTest() {}

 protected:
  MockModemInfo modem_info_;
  DeviceRefPtr device_;
};

TEST_F(CellularPropertyTest, Contains) {
  EXPECT_TRUE(device_->store().Contains(kNameProperty));
  EXPECT_FALSE(device_->store().Contains(""));
}

TEST_F(CellularPropertyTest, SetProperty) {
  {
    Error error;
    const bool allow_roaming = true;
    EXPECT_TRUE(device_->mutable_store()->SetAnyProperty(
        kCellularAllowRoamingProperty, allow_roaming, &error));
  }
  // Ensure that attempting to write a R/O property returns InvalidArgs error.
  {
    Error error;
    EXPECT_FALSE(device_->mutable_store()->SetAnyProperty(
        kAddressProperty, PropertyStoreTest::kStringV, &error));
    ASSERT_TRUE(error.IsFailure());  // name() may be invalid otherwise
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }
  {
    Error error;
    EXPECT_FALSE(device_->mutable_store()->SetAnyProperty(
        kCarrierProperty, PropertyStoreTest::kStringV, &error));
    ASSERT_TRUE(error.IsFailure());  // name() may be invalid otherwise
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }
}

class CellularTest : public testing::Test {
 public:
  CellularTest()
      : kHomeProviderCode("10001"),
        kHomeProviderCountry("us"),
        kHomeProviderName("HomeProviderName"),
        kServingOperatorCode("10002"),
        kServingOperatorCountry("ca"),
        kServingOperatorName("ServingOperatorName"),
        control_interface_(this),
        modem_info_(&control_interface_, &dispatcher_, nullptr, nullptr),
        device_info_(modem_info_.control_interface(), &dispatcher_,
                     modem_info_.metrics(), modem_info_.manager()),
        dhcp_config_(new MockDHCPConfig(modem_info_.control_interface(),
                                        kTestDeviceName)),
        create_gsm_card_proxy_from_factory_(false),
        mock_home_provider_info_(nullptr),
        mock_serving_operator_info_(nullptr),
        device_(new Cellular(&modem_info_,
                             kTestDeviceName,
                             kTestDeviceAddress,
                             3,
                             Cellular::kTypeGSM,
                             kDBusService,
                             kDBusPath)) {
    PopulateProxies();
    modem_info_.metrics()->RegisterDevice(device_->interface_index(),
                                          Technology::kCellular);
  }

  virtual void SetUp() {
    static_cast<Device*>(device_.get())->rtnl_handler_ = &rtnl_handler_;
    device_->set_dhcp_provider(&dhcp_provider_);
    device_->process_manager_ = &process_manager_;
    EXPECT_CALL(*modem_info_.mock_manager(), device_info())
        .WillRepeatedly(Return(&device_info_));
    EXPECT_CALL(*modem_info_.mock_manager(), DeregisterService(_))
        .Times(AnyNumber());
  }

  virtual void TearDown() {
    device_->DestroyIPConfig();
    device_->state_ = Cellular::kStateDisabled;
    device_->capability_->ReleaseProxies();
    device_->set_dhcp_provider(nullptr);
    // Break cycle between Cellular and CellularService.
    device_->service_ = nullptr;
    device_->SelectService(nullptr);
  }

  void PopulateProxies() {
    dbus_properties_proxy_.reset(new MockDBusPropertiesProxy());
    proxy_.reset(new MockModemProxy());
    simple_proxy_.reset(new MockModemSimpleProxy());
    cdma_proxy_.reset(new MockModemCDMAProxy());
    gsm_card_proxy_.reset(new MockModemGSMCardProxy());
    gsm_network_proxy_.reset(new MockModemGSMNetworkProxy());
    mm1_modem_3gpp_proxy_.reset(new mm1::MockModemModem3gppProxy());
    mm1_proxy_.reset(new mm1::MockModemProxy());
    mm1_simple_proxy_.reset(new mm1::MockModemSimpleProxy());
  }

  void SetMockMobileOperatorInfoObjects() {
    mock_home_provider_info_ =
        new MockMobileOperatorInfo(&dispatcher_, "HomeProvider");
    // Takes ownership.
    device_->set_home_provider_info(mock_home_provider_info_);

    mock_serving_operator_info_ =
        new MockMobileOperatorInfo(&dispatcher_, "ServingOperator");
    // Takes ownership.
    device_->set_serving_operator_info(mock_serving_operator_info_);
  }

  void InvokeEnable(bool enable, Error* error,
                    const ResultCallback& callback, int timeout) {
    callback.Run(Error());
  }
  void InvokeEnableReturningWrongState(
      bool enable, Error* error, const ResultCallback& callback, int timeout) {
    callback.Run(Error(Error::kWrongState));
  }
  void InvokeGetSignalQuality(Error* error,
                              const SignalQualityCallback& callback,
                              int timeout) {
    callback.Run(kStrength, Error());
  }
  void InvokeGetModemStatus(Error* error,
                            const KeyValueStoreCallback& callback,
                            int timeout) {
    KeyValueStore props;
    props.SetString("carrier", kTestCarrier);
    props.SetString("unknown-property", "irrelevant-value");
    callback.Run(props, Error());
  }
  void InvokeGetModemInfo(Error* error, const ModemInfoCallback& callback,
                            int timeout) {
    static const char kManufacturer[] = "Company";
    static const char kModelID[] = "Gobi 2000";
    static const char kHWRev[] = "A00B1234";
    callback.Run(kManufacturer, kModelID, kHWRev, Error());
  }
  void InvokeGetRegistrationState1X(Error* error,
                                    const RegistrationStateCallback& callback,
                                    int timeout) {
    callback.Run(MM_MODEM_CDMA_REGISTRATION_STATE_HOME,
                 MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN,
                 Error());
  }
  void InvokeGetIMEI(Error* error, const GSMIdentifierCallback& callback,
                     int timeout) {
    callback.Run(kIMEI, Error());
  }
  void InvokeGetIMSI(Error* error, const GSMIdentifierCallback& callback,
                     int timeout) {
    callback.Run(kIMSI, Error());
  }
  void InvokeGetMSISDN(Error* error, const GSMIdentifierCallback& callback,
                       int timeout) {
    callback.Run(kMSISDN, Error());
  }
  void InvokeGetSPN(Error* error, const GSMIdentifierCallback& callback,
                    int timeout) {
    callback.Run(kTestCarrierSPN, Error());
  }
  void InvokeGetRegistrationInfo(Error* error,
                                 const RegistrationInfoCallback& callback,
                                 int timeout) {
    static const char kNetworkID[] = "22803";
    callback.Run(MM_MODEM_GSM_NETWORK_REG_STATUS_ROAMING,
                 kNetworkID, kTestCarrier, Error());
  }
  void InvokeRegister(const string& network_id,
                      Error* error,
                      const ResultCallback& callback,
                      int timeout) {
    callback.Run(Error());
  }
  void InvokeGetRegistrationState(Error* error,
                                  const RegistrationStateCallback& callback,
                                  int timeout) {
    callback.Run(MM_MODEM_CDMA_REGISTRATION_STATE_REGISTERED,
                 MM_MODEM_CDMA_REGISTRATION_STATE_HOME,
                 Error());
  }
  void InvokeGetRegistrationStateUnregistered(
      Error* error,
      const RegistrationStateCallback& callback,
      int timeout) {
    callback.Run(MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN,
                 MM_MODEM_CDMA_REGISTRATION_STATE_UNKNOWN,
                 Error());
  }
  void InvokeConnect(KeyValueStore props, Error* error,
                     const ResultCallback& callback, int timeout) {
    EXPECT_EQ(Service::kStateAssociating, device_->service_->state());
    callback.Run(Error());
  }
  void InvokeConnectFail(KeyValueStore props, Error* error,
                         const ResultCallback& callback, int timeout) {
    EXPECT_EQ(Service::kStateAssociating, device_->service_->state());
    callback.Run(Error(Error::kNotOnHomeNetwork));
  }
  void InvokeConnectFailNoService(KeyValueStore props, Error* error,
                                  const ResultCallback& callback, int timeout) {
    device_->service_ = nullptr;
    callback.Run(Error(Error::kNotOnHomeNetwork));
  }
  void InvokeConnectSuccessNoService(KeyValueStore props, Error* error,
                                     const ResultCallback& callback,
                                     int timeout) {
    device_->service_ = nullptr;
    callback.Run(Error());
  }
  void InvokeDisconnect(Error* error, const ResultCallback& callback,
                        int timeout) {
    if (!callback.is_null())
      callback.Run(Error());
  }
  void InvokeDisconnectFail(Error* error, const ResultCallback& callback,
                            int timeout) {
    error->Populate(Error::kOperationFailed);
    if (!callback.is_null())
      callback.Run(*error);
  }
  void InvokeDisconnectMM1(const string& bearer, Error* error,
                           const ResultCallback& callback, int timeout) {
    if (!callback.is_null())
      callback.Run(Error());
  }
  void InvokeSetPowerState(const uint32_t& power_state,
                           Error* error,
                           const ResultCallback& callback,
                           int timeout) {
    callback.Run(Error());
  }
  void ExpectCdmaStartModem(string network_technology) {
    if (!device_->IsUnderlyingDeviceEnabled())
      EXPECT_CALL(*proxy_,
                  Enable(true, _, _, CellularCapability::kTimeoutEnable))
          .WillOnce(Invoke(this, &CellularTest::InvokeEnable));
    EXPECT_CALL(*simple_proxy_,
                GetModemStatus(_, _, CellularCapability::kTimeoutDefault))
        .WillOnce(Invoke(this, &CellularTest::InvokeGetModemStatus));
    EXPECT_CALL(*proxy_,
                GetModemInfo(_, _, CellularCapability::kTimeoutDefault))
        .WillOnce(Invoke(this, &CellularTest::InvokeGetModemInfo));
    if (network_technology == kNetworkTechnology1Xrtt)
      EXPECT_CALL(*cdma_proxy_, GetRegistrationState(nullptr, _, _))
          .WillOnce(Invoke(this, &CellularTest::InvokeGetRegistrationState1X));
    else
      EXPECT_CALL(*cdma_proxy_, GetRegistrationState(nullptr, _, _))
          .WillOnce(Invoke(this, &CellularTest::InvokeGetRegistrationState));
    EXPECT_CALL(*cdma_proxy_, GetSignalQuality(nullptr, _, _))
        .Times(2)
        .WillRepeatedly(Invoke(this, &CellularTest::InvokeGetSignalQuality));
    EXPECT_CALL(*this, TestCallback(IsSuccess()));
    EXPECT_CALL(*modem_info_.mock_manager(), RegisterService(_));
  }

  void ExpectDisconnectCapabilityUniversal() {
    SetCellularType(Cellular::kTypeUniversal);
    device_->state_ = Cellular::kStateConnected;
    EXPECT_CALL(*mm1_simple_proxy_, Disconnect(_, _, _, _))
        .WillOnce(Invoke(this, &CellularTest::InvokeDisconnectMM1));
    GetCapabilityUniversal()->modem_simple_proxy_.reset(
        mm1_simple_proxy_.release());
  }

  void VerifyDisconnect() {
    EXPECT_EQ(Cellular::kStateRegistered, device_->state_);
  }

  void StartPPP(int pid) {
    EXPECT_CALL(process_manager_, StartProcess(_, _, _, _, _, _))
        .WillOnce(Return(pid));
    device_->StartPPP("fake_serial_device");
    EXPECT_FALSE(device_->ipconfig());  // No DHCP client.
    EXPECT_FALSE(device_->selected_service());
    EXPECT_FALSE(device_->is_ppp_authenticating_);
    EXPECT_NE(nullptr, device_->ppp_task_);
    Mock::VerifyAndClearExpectations(&process_manager_);
  }

  void FakeUpConnectedPPP() {
    const char kInterfaceName[] = "fake-ppp-device";
    const int kInterfaceIndex = -1;
    auto mock_ppp_device = make_scoped_refptr(
        new MockPPPDevice(modem_info_.control_interface(), nullptr, nullptr,
                          nullptr, kInterfaceName, kInterfaceIndex));
    device_->ppp_device_ = mock_ppp_device;
    device_->state_ = Cellular::kStateConnected;
  }

  void ExpectPPPStopped() {
    auto mock_ppp_device =
        static_cast<MockPPPDevice*>(device_->ppp_device_.get());
    EXPECT_CALL(*mock_ppp_device, DropConnection());
  }

  void VerifyPPPStopped() {
    EXPECT_EQ(nullptr, device_->ppp_task_);
    EXPECT_FALSE(device_->ppp_device_);
  }

  void SetCommonOnAfterResumeExpectations() {
    EXPECT_CALL(*dbus_properties_proxy_, GetAll(_))
        .WillRepeatedly(Return(KeyValueStore()));
    EXPECT_CALL(*mm1_proxy_, set_state_changed_callback(_)).Times(AnyNumber());
    EXPECT_CALL(*modem_info_.mock_metrics(), NotifyDeviceScanStarted(_))
        .Times(AnyNumber());
    EXPECT_CALL(*modem_info_.mock_manager(), UpdateEnabledTechnologies())
        .Times(AnyNumber());
    EXPECT_CALL(*static_cast<DeviceMockAdaptor*>(device_->adaptor()),
                EmitBoolChanged(_, _)).Times(AnyNumber());
  }

  mm1::MockModemProxy* SetupOnAfterResume() {
    SetCellularType(Cellular::kTypeUniversal);
    SetCommonOnAfterResumeExpectations();
    return mm1_proxy_.get();  // Before the capability snags it.
  }

  void VerifyOperatorMap(const Stringmap& operator_map,
                         const string& code,
                         const string& name,
                         const string& country) {
    Stringmap::const_iterator it;
    Stringmap::const_iterator endit = operator_map.end();

    it = operator_map.find(kOperatorCodeKey);
    if (code == "") {
      EXPECT_EQ(endit, it);
    } else {
      ASSERT_NE(endit, it);
      EXPECT_EQ(code, it->second);
    }
    it = operator_map.find(kOperatorNameKey);
    if (name == "") {
      EXPECT_EQ(endit, it);
    } else {
      ASSERT_NE(endit, it);
      EXPECT_EQ(name, it->second);
    }
    it = operator_map.find(kOperatorCountryKey);
    if (country == "") {
      EXPECT_EQ(endit, it);
    } else {
      ASSERT_NE(endit, it);
      EXPECT_EQ(country, it->second);
    }
  }

  MOCK_METHOD1(TestCallback, void(const Error& error));

 protected:
  static const char kTestDeviceName[];
  static const char kTestDeviceAddress[];
  static const char kDBusService[];
  static const char kDBusPath[];
  static const char kTestCarrier[];
  static const char kTestCarrierSPN[];
  static const char kMEID[];
  static const char kIMEI[];
  static const char kIMSI[];
  static const char kMSISDN[];
  static const char kTestMobileProviderDBPath[];
  static const Stringmaps kTestNetworksGSM;
  static const Stringmaps kTestNetworksCellular;
  static const int kStrength;

  // Must be std::string so that we can safely ReturnRef.
  const string kHomeProviderCode;
  const string kHomeProviderCountry;
  const string kHomeProviderName;
  const string kServingOperatorCode;
  const string kServingOperatorCountry;
  const string kServingOperatorName;

  class TestControl : public MockControl {
   public:
    explicit TestControl(CellularTest* test) : test_(test) {}

    virtual DBusPropertiesProxyInterface* CreateDBusPropertiesProxy(
        const std::string& path,
        const std::string& service) {
      CHECK(test_->dbus_properties_proxy_);
      return test_->dbus_properties_proxy_.release();
    }

    virtual ModemProxyInterface* CreateModemProxy(
        const string& /*path*/,
        const string& /*service*/) {
      CHECK(test_->proxy_);
      return test_->proxy_.release();
    }

    virtual ModemSimpleProxyInterface* CreateModemSimpleProxy(
        const string& /*path*/,
        const string& /*service*/) {
      CHECK(test_->simple_proxy_);
      return test_->simple_proxy_.release();
    }

    virtual ModemCDMAProxyInterface* CreateModemCDMAProxy(
        const string& /*path*/,
        const string& /*service*/) {
      CHECK(test_->cdma_proxy_);
      return test_->cdma_proxy_.release();
    }

    virtual ModemGSMCardProxyInterface* CreateModemGSMCardProxy(
        const string& /*path*/,
        const string& /*service*/) {
      // TODO(benchan): This code conditionally returns a nullptr to avoid
      // CellularCapabilityGSM::InitProperties (and thus
      // CellularCapabilityGSM::GetIMSI) from being called during the
      // construction. Remove this workaround after refactoring the tests.
      CHECK(!test_->create_gsm_card_proxy_from_factory_ ||
            test_->gsm_card_proxy_);
      return test_->create_gsm_card_proxy_from_factory_ ?
          test_->gsm_card_proxy_.release() : nullptr;
    }

    virtual ModemGSMNetworkProxyInterface* CreateModemGSMNetworkProxy(
        const string& /*path*/,
        const string& /*service*/) {
      CHECK(test_->gsm_network_proxy_);
      return test_->gsm_network_proxy_.release();
    }

    virtual mm1::ModemModem3gppProxyInterface* CreateMM1ModemModem3gppProxy(
      const std::string& path,
      const std::string& service) {
      CHECK(test_->mm1_modem_3gpp_proxy_);
      return test_->mm1_modem_3gpp_proxy_.release();
    }

    virtual mm1::ModemProxyInterface* CreateMM1ModemProxy(
      const std::string& path,
      const std::string& service) {
      CHECK(test_->mm1_proxy_);
      return test_->mm1_proxy_.release();
    }

    virtual mm1::ModemSimpleProxyInterface* CreateMM1ModemSimpleProxy(
        const string& /*path*/,
        const string& /*service*/) {
      CHECK(test_->mm1_simple_proxy_);
      return test_->mm1_simple_proxy_.release();
    }

   private:
    CellularTest* test_;
  };
  void StartRTNLHandler();
  void StopRTNLHandler();

  void AllowCreateGSMCardProxyFromFactory() {
    create_gsm_card_proxy_from_factory_ = true;
  }

  void SetCellularType(Cellular::Type type) {
    device_->InitCapability(type);
  }

  CellularCapabilityClassic* GetCapabilityClassic() {
    return static_cast<CellularCapabilityClassic*>(
        device_->capability_.get());
  }

  CellularCapabilityCDMA* GetCapabilityCDMA() {
    return static_cast<CellularCapabilityCDMA*>(device_->capability_.get());
  }

  CellularCapabilityGSM* GetCapabilityGSM() {
    return static_cast<CellularCapabilityGSM*>(device_->capability_.get());
  }

  CellularCapabilityUniversal* GetCapabilityUniversal() {
    return static_cast<CellularCapabilityUniversal*>(
        device_->capability_.get());
  }

  // Different tests simulate a cellular service being set using a real /mock
  // service.
  CellularService* SetService() {
    device_->service_ = new CellularService(&modem_info_, device_);
    return device_->service_.get();
  }
  MockCellularService* SetMockService() {
    device_->service_ = new MockCellularService(&modem_info_, device_);
    return static_cast<MockCellularService*>(device_->service_.get());
  }

  void set_enabled_persistent(bool new_value) {
    device_->enabled_persistent_ = new_value;
  }

  void SetCapabilityUniversalActiveBearer(unique_ptr<CellularBearer> bearer) {
    SetCellularType(Cellular::kTypeUniversal);
    CellularCapabilityUniversal* capability = GetCapabilityUniversal();
    capability->active_bearer_ = std::move(bearer);
  }

  EventDispatcherForTest dispatcher_;
  TestControl control_interface_;
  MockModemInfo modem_info_;
  MockDeviceInfo device_info_;
  MockProcessManager process_manager_;
  NiceMock<MockRTNLHandler> rtnl_handler_;

  MockDHCPProvider dhcp_provider_;
  scoped_refptr<MockDHCPConfig> dhcp_config_;

  bool create_gsm_card_proxy_from_factory_;
  unique_ptr<MockDBusPropertiesProxy> dbus_properties_proxy_;
  unique_ptr<MockModemProxy> proxy_;
  unique_ptr<MockModemSimpleProxy> simple_proxy_;
  unique_ptr<MockModemCDMAProxy> cdma_proxy_;
  unique_ptr<MockModemGSMCardProxy> gsm_card_proxy_;
  unique_ptr<MockModemGSMNetworkProxy> gsm_network_proxy_;
  unique_ptr<mm1::MockModemModem3gppProxy> mm1_modem_3gpp_proxy_;
  unique_ptr<mm1::MockModemProxy> mm1_proxy_;
  unique_ptr<mm1::MockModemSimpleProxy> mm1_simple_proxy_;
  MockMobileOperatorInfo* mock_home_provider_info_;
  MockMobileOperatorInfo* mock_serving_operator_info_;
  CellularRefPtr device_;
};

const char CellularTest::kTestDeviceName[] = "usb0";
const char CellularTest::kTestDeviceAddress[] = "00:01:02:03:04:05";
const char CellularTest::kDBusService[] = "org.chromium.ModemManager";
const char CellularTest::kDBusPath[] = "/org/chromium/ModemManager/Gobi/0";
const char CellularTest::kTestCarrier[] = "The Cellular Carrier";
const char CellularTest::kTestCarrierSPN[] = "Home Provider";
const char CellularTest::kMEID[] = "01234567EF8901";
const char CellularTest::kIMEI[] = "987654321098765";
const char CellularTest::kIMSI[] = "123456789012345";
const char CellularTest::kMSISDN[] = "12345678901";
const char CellularTest::kTestMobileProviderDBPath[] =
    "provider_db_unittest.bfd";
const Stringmaps CellularTest::kTestNetworksGSM =
    {{{CellularCapabilityGSM::kNetworkPropertyStatus, "1"},
      {CellularCapabilityGSM::kNetworkPropertyID, "0000"},
      {CellularCapabilityGSM::kNetworkPropertyLongName, "some_long_name"},
      {CellularCapabilityGSM::kNetworkPropertyShortName, "short"}}};
const Stringmaps CellularTest::kTestNetworksCellular =
    {{{kStatusProperty, "available"},
      {kNetworkIdProperty, "0000"},
      {kLongNameProperty, "some_long_name"},
      {kShortNameProperty, "short"}}};
const int CellularTest::kStrength = 90;

TEST_F(CellularTest, GetStateString) {
  EXPECT_EQ("CellularStateDisabled",
            Cellular::GetStateString(Cellular::kStateDisabled));
  EXPECT_EQ("CellularStateEnabled",
            Cellular::GetStateString(Cellular::kStateEnabled));
  EXPECT_EQ("CellularStateRegistered",
            Cellular::GetStateString(Cellular::kStateRegistered));
  EXPECT_EQ("CellularStateConnected",
            Cellular::GetStateString(Cellular::kStateConnected));
  EXPECT_EQ("CellularStateLinked",
            Cellular::GetStateString(Cellular::kStateLinked));
}

TEST_F(CellularTest, GetModemStateString) {
  EXPECT_EQ("CellularModemStateFailed",
            Cellular::GetModemStateString(Cellular::kModemStateFailed));
  EXPECT_EQ("CellularModemStateUnknown",
            Cellular::GetModemStateString(Cellular::kModemStateUnknown));
  EXPECT_EQ("CellularModemStateInitializing",
            Cellular::GetModemStateString(Cellular::kModemStateInitializing));
  EXPECT_EQ("CellularModemStateLocked",
            Cellular::GetModemStateString(Cellular::kModemStateLocked));
  EXPECT_EQ("CellularModemStateDisabled",
            Cellular::GetModemStateString(Cellular::kModemStateDisabled));
  EXPECT_EQ("CellularModemStateDisabling",
            Cellular::GetModemStateString(Cellular::kModemStateDisabling));
  EXPECT_EQ("CellularModemStateEnabling",
            Cellular::GetModemStateString(Cellular::kModemStateEnabling));
  EXPECT_EQ("CellularModemStateEnabled",
            Cellular::GetModemStateString(Cellular::kModemStateEnabled));
  EXPECT_EQ("CellularModemStateSearching",
            Cellular::GetModemStateString(Cellular::kModemStateSearching));
  EXPECT_EQ("CellularModemStateRegistered",
            Cellular::GetModemStateString(Cellular::kModemStateRegistered));
  EXPECT_EQ("CellularModemStateDisconnecting",
            Cellular::GetModemStateString(Cellular::kModemStateDisconnecting));
  EXPECT_EQ("CellularModemStateConnecting",
            Cellular::GetModemStateString(Cellular::kModemStateConnecting));
  EXPECT_EQ("CellularModemStateConnected",
            Cellular::GetModemStateString(Cellular::kModemStateConnected));
}

TEST_F(CellularTest, StartCDMARegister) {
  SetCellularType(Cellular::kTypeCDMA);
  ExpectCdmaStartModem(kNetworkTechnology1Xrtt);
  EXPECT_CALL(*cdma_proxy_, MEID()).WillOnce(Return(kMEID));
  Error error;
  device_->Start(&error, Bind(&CellularTest::TestCallback, Unretained(this)));
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(kMEID, device_->meid());
  EXPECT_EQ(kTestCarrier, device_->carrier());
  EXPECT_EQ(Cellular::kStateRegistered, device_->state_);
  ASSERT_TRUE(device_->service_.get());
  EXPECT_EQ(kNetworkTechnology1Xrtt, device_->service_->network_technology());
  EXPECT_EQ(kStrength, device_->service_->strength());
  EXPECT_EQ(kRoamingStateHome, device_->service_->roaming_state());
}

TEST_F(CellularTest, StartGSMRegister) {
  SetMockMobileOperatorInfoObjects();
  EXPECT_CALL(*proxy_, Enable(true, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(Invoke(this, &CellularTest::InvokeEnable));
  EXPECT_CALL(*gsm_card_proxy_,
              GetIMEI(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularTest::InvokeGetIMEI));
  EXPECT_CALL(*gsm_card_proxy_,
              GetIMSI(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularTest::InvokeGetIMSI));
  EXPECT_CALL(*gsm_card_proxy_,
              GetSPN(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularTest::InvokeGetSPN));
  EXPECT_CALL(*gsm_card_proxy_,
              GetMSISDN(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularTest::InvokeGetMSISDN));
  EXPECT_CALL(*gsm_network_proxy_, AccessTechnology())
      .WillOnce(Return(MM_MODEM_GSM_ACCESS_TECH_EDGE));
  EXPECT_CALL(*gsm_card_proxy_, EnabledFacilityLocks())
      .WillOnce(Return(MM_MODEM_GSM_FACILITY_SIM));
  EXPECT_CALL(*proxy_, GetModemInfo(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularTest::InvokeGetModemInfo));
  EXPECT_CALL(*gsm_network_proxy_,
              GetRegistrationInfo(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularTest::InvokeGetRegistrationInfo));
  EXPECT_CALL(*gsm_network_proxy_, GetSignalQuality(nullptr, _, _))
      .Times(2)
      .WillRepeatedly(Invoke(this,
                             &CellularTest::InvokeGetSignalQuality));
  EXPECT_CALL(*mock_serving_operator_info_, UpdateMCCMNC(_));
  EXPECT_CALL(*mock_serving_operator_info_, UpdateOperatorName(_));
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  EXPECT_CALL(*modem_info_.mock_manager(), RegisterService(_));
  AllowCreateGSMCardProxyFromFactory();

  Error error;
  device_->Start(&error, Bind(&CellularTest::TestCallback, Unretained(this)));
  EXPECT_TRUE(error.IsSuccess());
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(kIMEI, device_->imei());
  EXPECT_EQ(kIMSI, device_->imsi());
  EXPECT_EQ(kTestCarrierSPN, GetCapabilityGSM()->spn_);
  EXPECT_EQ(kMSISDN, device_->mdn());
  EXPECT_EQ(Cellular::kStateRegistered, device_->state_);
  ASSERT_TRUE(device_->service_.get());
  EXPECT_EQ(kNetworkTechnologyEdge, device_->service_->network_technology());
  EXPECT_TRUE(GetCapabilityGSM()->sim_lock_status_.enabled);
  EXPECT_EQ(kStrength, device_->service_->strength());
  EXPECT_EQ(kRoamingStateRoaming, device_->service_->roaming_state());
}

TEST_F(CellularTest, StartConnected) {
  EXPECT_CALL(device_info_, GetFlags(device_->interface_index(), _))
      .WillOnce(Return(true));
  SetCellularType(Cellular::kTypeCDMA);
  device_->set_modem_state(Cellular::kModemStateConnected);
  device_->set_meid(kMEID);
  ExpectCdmaStartModem(kNetworkTechnologyEvdo);
  Error error;
  device_->Start(&error, Bind(&CellularTest::TestCallback, Unretained(this)));
  EXPECT_TRUE(error.IsSuccess());
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(Cellular::kStateConnected, device_->state_);
}

TEST_F(CellularTest, StartLinked) {
  EXPECT_CALL(device_info_, GetFlags(device_->interface_index(), _))
      .WillOnce(DoAll(SetArgumentPointee<1>(IFF_UP), Return(true)));
  SetCellularType(Cellular::kTypeCDMA);
  device_->set_modem_state(Cellular::kModemStateConnected);
  device_->set_meid(kMEID);
  ExpectCdmaStartModem(kNetworkTechnologyEvdo);
  EXPECT_CALL(dhcp_provider_, CreateIPv4Config(kTestDeviceName, _, _, _))
      .WillOnce(Return(dhcp_config_));
  EXPECT_CALL(*dhcp_config_, RequestIP()).WillOnce(Return(true));
  EXPECT_CALL(*modem_info_.mock_manager(), UpdateService(_)).Times(3);
  Error error;
  device_->Start(&error, Bind(&CellularTest::TestCallback, Unretained(this)));
  EXPECT_TRUE(error.IsSuccess());
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(Cellular::kStateLinked, device_->state_);
  EXPECT_EQ(Service::kStateConfiguring, device_->service_->state());
  device_->SelectService(nullptr);
}

TEST_F(CellularTest, FriendlyServiceName) {
  // Test that the name created for the service is sensible under different
  // scenarios w.r.t. information about the mobile network operator.
  SetMockMobileOperatorInfoObjects();
  CHECK(mock_home_provider_info_);
  CHECK(mock_serving_operator_info_);

  SetCellularType(Cellular::kTypeCDMA);
  // We are not testing the behaviour of capabilities here.
  device_->mobile_operator_info_observer_->set_capability(nullptr);

  // (1) Service created, MNO not known => Default name.
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  device_->CreateService();
  // Compare substrings explicitly using EXPECT_EQ for better error message.
  size_t prefix_len = strlen(Cellular::kGenericServiceNamePrefix);
  EXPECT_EQ(Cellular::kGenericServiceNamePrefix,
            device_->service_->friendly_name().substr(0, prefix_len));
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // (2) Service created, then home provider determined => Name provided by
  //     home provider.
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  device_->CreateService();
  // Now emulate an event for updated home provider information.
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  mock_home_provider_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, operator_name())
      .WillRepeatedly(ReturnRef(kHomeProviderName));
  device_->mobile_operator_info_observer_->OnOperatorChanged();
  EXPECT_EQ(kHomeProviderName, device_->service_->friendly_name());
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // (3) Service created, then serving operator determined => Name provided by
  //     serving operator.
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  device_->CreateService();
  // Now emulate an event for updated serving operator information.
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, operator_name())
      .WillRepeatedly(ReturnRef(kServingOperatorName));
  device_->mobile_operator_info_observer_->OnOperatorChanged();
  EXPECT_EQ(kServingOperatorName, device_->service_->friendly_name());
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // (4) Service created, then home provider determined, then serving operator
  // determined => final name is serving operator.
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  device_->CreateService();
  // Now emulate an event for updated home provider information.
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  mock_home_provider_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, operator_name())
      .WillRepeatedly(ReturnRef(kHomeProviderName));
  device_->mobile_operator_info_observer_->OnOperatorChanged();
  // Now emulate an event for updated serving operator information.
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, operator_name())
      .WillRepeatedly(ReturnRef(kServingOperatorName));
  device_->mobile_operator_info_observer_->OnOperatorChanged();
  EXPECT_EQ(kServingOperatorName, device_->service_->friendly_name());
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // (5) Service created, then serving operator determined, then home provider
  // determined => final name is serving operator.
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  device_->CreateService();
  // Now emulate an event for updated serving operator information.
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, operator_name())
      .WillRepeatedly(ReturnRef(kServingOperatorName));
  device_->mobile_operator_info_observer_->OnOperatorChanged();
  // Now emulate an event for updated home provider information.
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  mock_home_provider_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, operator_name())
      .WillRepeatedly(ReturnRef(kHomeProviderName));
  device_->mobile_operator_info_observer_->OnOperatorChanged();
  EXPECT_EQ(kServingOperatorName, device_->service_->friendly_name());
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // (6) Serving operator known, home provider known, and then service created
  //     => Name is serving operator.
  mock_home_provider_info_->SetEmptyDefaultsForProperties();
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, operator_name())
      .WillRepeatedly(ReturnRef(kHomeProviderName));
  EXPECT_CALL(*mock_serving_operator_info_, operator_name())
      .WillRepeatedly(ReturnRef(kServingOperatorName));
  device_->CreateService();
  EXPECT_EQ(kServingOperatorName, device_->service_->friendly_name());
}

TEST_F(CellularTest, HomeProviderServingOperator) {
  // Test that the the home provider information is correctly updated under
  // different scenarios w.r.t. information about the mobile network operators.
  SetMockMobileOperatorInfoObjects();
  CHECK(mock_home_provider_info_);
  CHECK(mock_serving_operator_info_);
  Stringmap home_provider;
  Stringmap serving_operator;


  // (1) Neither home provider nor serving operator known.
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));

  device_->CreateService();

  home_provider = device_->home_provider();
  VerifyOperatorMap(home_provider, "", "", "");
  serving_operator = device_->service_->serving_operator();
  VerifyOperatorMap(serving_operator, "", "", "");
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // (2) serving operator known.
  // When home provider is not known, serving operator proxies in.
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, mccmnc())
      .WillRepeatedly(ReturnRef(kServingOperatorCode));
  EXPECT_CALL(*mock_serving_operator_info_, operator_name())
      .WillRepeatedly(ReturnRef(kServingOperatorName));
  EXPECT_CALL(*mock_serving_operator_info_, country())
      .WillRepeatedly(ReturnRef(kServingOperatorCountry));

  device_->CreateService();

  home_provider = device_->home_provider();
  VerifyOperatorMap(home_provider,
                    kServingOperatorCode,
                    kServingOperatorName,
                    kServingOperatorCountry);
  serving_operator = device_->service_->serving_operator();
  VerifyOperatorMap(serving_operator,
                    kServingOperatorCode,
                    kServingOperatorName,
                    kServingOperatorCountry);
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // (3) home provider known.
  // When serving operator is not known, home provider proxies in.
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  mock_home_provider_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, mccmnc())
      .WillRepeatedly(ReturnRef(kHomeProviderCode));
  EXPECT_CALL(*mock_home_provider_info_, operator_name())
      .WillRepeatedly(ReturnRef(kHomeProviderName));
  EXPECT_CALL(*mock_home_provider_info_, country())
      .WillRepeatedly(ReturnRef(kHomeProviderCountry));

  device_->CreateService();

  home_provider = device_->home_provider();
  VerifyOperatorMap(home_provider,
                    kHomeProviderCode,
                    kHomeProviderName,
                    kHomeProviderCountry);
  serving_operator = device_->service_->serving_operator();
  VerifyOperatorMap(serving_operator,
                    kHomeProviderCode,
                    kHomeProviderName,
                    kHomeProviderCountry);
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // (4) Serving operator known, home provider known.
  mock_home_provider_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, mccmnc())
      .WillRepeatedly(ReturnRef(kHomeProviderCode));
  EXPECT_CALL(*mock_home_provider_info_, operator_name())
      .WillRepeatedly(ReturnRef(kHomeProviderName));
  EXPECT_CALL(*mock_home_provider_info_, country())
      .WillRepeatedly(ReturnRef(kHomeProviderCountry));
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, mccmnc())
      .WillRepeatedly(ReturnRef(kServingOperatorCode));
  EXPECT_CALL(*mock_serving_operator_info_, operator_name())
      .WillRepeatedly(ReturnRef(kServingOperatorName));
  EXPECT_CALL(*mock_serving_operator_info_, country())
      .WillRepeatedly(ReturnRef(kServingOperatorCountry));

  device_->CreateService();

  home_provider = device_->home_provider();
  VerifyOperatorMap(home_provider,
                    kHomeProviderCode,
                    kHomeProviderName,
                    kHomeProviderCountry);
  serving_operator = device_->service_->serving_operator();
  VerifyOperatorMap(serving_operator,
                    kServingOperatorCode,
                    kServingOperatorName,
                    kServingOperatorCountry);
}

static bool IllegalChar(char a) {
  return !(isalnum(a) || a == '_');
}

TEST_F(CellularTest, StorageIdentifier) {
  // Test that the storage identifier name used by the service is sensible under
  // different scenarios w.r.t. information about the mobile network operator.
  SetMockMobileOperatorInfoObjects();
  mock_home_provider_info_->SetEmptyDefaultsForProperties();
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  CHECK(mock_home_provider_info_);
  CHECK(mock_serving_operator_info_);

  // See cellular_service.cc
  string prefix = string(kTypeCellular) + "_" +
                  string(kTestDeviceAddress) + "_";
  // Service replaces ':' with '_'
  std::replace_if(prefix.begin(), prefix.end(), &IllegalChar, '_');
  const string kUuidHomeProvider = "uuidHomeProvider";
  const string kUuidServingOperator = "uuidServingOperator";
  const string kSimIdentifier = "12345123451234512345";

  SetCellularType(Cellular::kTypeCDMA);
  // We are not testing the behaviour of capabilities here.
  device_->mobile_operator_info_observer_->set_capability(nullptr);
  ON_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillByDefault(Return(false));

  // (1) Service created, both home provider and serving operator known =>
  // home provider used.
  mock_home_provider_info_->SetEmptyDefaultsForProperties();
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_home_provider_info_, uuid())
      .WillRepeatedly(ReturnRef(kUuidHomeProvider));
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, uuid())
      .WillRepeatedly(ReturnRef(kUuidServingOperator));
  device_->CreateService();
  EXPECT_EQ(prefix + kUuidHomeProvider,
            device_->service()->GetStorageIdentifier());
  Mock::VerifyAndClearExpectations(mock_home_provider_info_);
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // Common expectation for following tests:
  EXPECT_CALL(*mock_home_provider_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));

  // (2) Service created, no extra information => Default storage_id;
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(false));
  device_->CreateService();
  EXPECT_EQ(prefix + device_->service()->friendly_name(),
            device_->service()->GetStorageIdentifier());
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // (3) Service created, serving operator known, uuid known.
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_serving_operator_info_, uuid())
      .WillRepeatedly(ReturnRef(kUuidServingOperator));
  device_->CreateService();
  EXPECT_EQ(prefix + kUuidServingOperator,
            device_->service()->GetStorageIdentifier());
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();

  // (4) Service created, serving operator known, uuid not known, iccid known.
  mock_serving_operator_info_->SetEmptyDefaultsForProperties();
  EXPECT_CALL(*mock_serving_operator_info_, IsMobileNetworkOperatorKnown())
      .WillRepeatedly(Return(true));
  device_->set_sim_identifier(kSimIdentifier);
  device_->CreateService();
  EXPECT_EQ(prefix + kSimIdentifier,
            device_->service()->GetStorageIdentifier());
  Mock::VerifyAndClearExpectations(mock_serving_operator_info_);
  device_->DestroyService();
}

namespace {

MATCHER(ContainsPhoneNumber, "") {
  return arg.ContainsString(
      CellularCapabilityClassic::kConnectPropertyPhoneNumber);
}

}  // namespace

TEST_F(CellularTest, Connect) {
  Error error;
  EXPECT_CALL(device_info_, GetFlags(device_->interface_index(), _))
      .Times(2)
      .WillRepeatedly(Return(true));
  device_->state_ = Cellular::kStateConnected;
  device_->Connect(&error);
  EXPECT_EQ(Error::kAlreadyConnected, error.type());
  error.Populate(Error::kSuccess);

  device_->state_ = Cellular::kStateLinked;
  device_->Connect(&error);
  EXPECT_EQ(Error::kAlreadyConnected, error.type());

  device_->state_ = Cellular::kStateEnabled;
  device_->Connect(&error);
  EXPECT_EQ(Error::kNotRegistered, error.type());

  error.Reset();
  device_->state_ = Cellular::kStateDisabled;
  device_->Connect(&error);
  EXPECT_EQ(Error::kNotRegistered, error.type());

  device_->state_ = Cellular::kStateRegistered;
  SetService();

  device_->allow_roaming_ = false;
  device_->service_->roaming_state_ = kRoamingStateRoaming;
  device_->Connect(&error);
  EXPECT_EQ(Error::kNotOnHomeNetwork, error.type());

  error.Populate(Error::kSuccess);
  EXPECT_CALL(*simple_proxy_,
              Connect(ContainsPhoneNumber(), _, _,
                      CellularCapability::kTimeoutConnect))
                .Times(2)
                .WillRepeatedly(Invoke(this, &CellularTest::InvokeConnect));
  GetCapabilityClassic()->simple_proxy_.reset(simple_proxy_.release());
  device_->service_->roaming_state_ = kRoamingStateHome;
  device_->state_ = Cellular::kStateRegistered;
  device_->Connect(&error);
  EXPECT_TRUE(error.IsSuccess());
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(Cellular::kStateConnected, device_->state_);

  device_->allow_roaming_ = true;
  device_->service_->roaming_state_ = kRoamingStateRoaming;
  device_->state_ = Cellular::kStateRegistered;
  device_->Connect(&error);
  EXPECT_TRUE(error.IsSuccess());
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(Cellular::kStateConnected, device_->state_);
}

TEST_F(CellularTest, Disconnect) {
  Error error;
  device_->state_ = Cellular::kStateRegistered;
  device_->Disconnect(&error, "in test");
  EXPECT_EQ(Error::kNotConnected, error.type());
  error.Reset();

  device_->state_ = Cellular::kStateConnected;
  EXPECT_CALL(*proxy_,
              Disconnect(_, _, CellularCapability::kTimeoutDisconnect))
      .WillOnce(Invoke(this, &CellularTest::InvokeDisconnect));
  GetCapabilityClassic()->proxy_.reset(proxy_.release());
  device_->Disconnect(&error, "in test");
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(Cellular::kStateRegistered, device_->state_);
}

TEST_F(CellularTest, DisconnectFailure) {
  // Test the case where the underlying modem state is set
  // to disconnecting, but shill thinks it's still connected
  Error error;
  device_->state_ = Cellular::kStateConnected;
  EXPECT_CALL(*proxy_,
              Disconnect(_, _, CellularCapability::kTimeoutDisconnect))
       .Times(2)
       .WillRepeatedly(Invoke(this, &CellularTest::InvokeDisconnectFail));
  GetCapabilityClassic()->proxy_.reset(proxy_.release());
  device_->modem_state_ = Cellular::kModemStateDisconnecting;
  device_->Disconnect(&error, "in test");
  EXPECT_TRUE(error.IsFailure());
  EXPECT_EQ(Cellular::kStateConnected, device_->state_);

  device_->modem_state_ = Cellular::kModemStateConnected;
  device_->Disconnect(&error, "in test");
  EXPECT_TRUE(error.IsFailure());
  EXPECT_EQ(Cellular::kStateRegistered, device_->state_);
}

TEST_F(CellularTest, ConnectFailure) {
  SetCellularType(Cellular::kTypeCDMA);
  device_->state_ = Cellular::kStateRegistered;
  SetService();
  ASSERT_EQ(Service::kStateIdle, device_->service_->state());
  EXPECT_CALL(*simple_proxy_,
              Connect(_, _, _, CellularCapability::kTimeoutConnect))
                .WillOnce(Invoke(this, &CellularTest::InvokeConnectFail));
  GetCapabilityClassic()->simple_proxy_.reset(simple_proxy_.release());
  Error error;
  device_->Connect(&error);
  EXPECT_EQ(Service::kStateFailure, device_->service_->state());
}

TEST_F(CellularTest, ConnectFailureNoService) {
  // Make sure we don't crash if the connect failed and there is no
  // CellularService object.  This can happen if the modem is enabled and
  // then quick disabled.
  SetCellularType(Cellular::kTypeCDMA);
  device_->state_ = Cellular::kStateRegistered;
  SetService();
  EXPECT_CALL(
      *simple_proxy_,
      Connect(_, _, _, CellularCapability::kTimeoutConnect))
      .WillOnce(Invoke(this, &CellularTest::InvokeConnectFailNoService));
  EXPECT_CALL(*modem_info_.mock_manager(), UpdateService(_));
  GetCapabilityClassic()->simple_proxy_.reset(simple_proxy_.release());
  Error error;
  device_->Connect(&error);
}

TEST_F(CellularTest, ConnectSuccessNoService) {
  // Make sure we don't crash if the connect succeeds but the service was
  // destroyed before the connect request completes.
  SetCellularType(Cellular::kTypeCDMA);
  device_->state_ = Cellular::kStateRegistered;
  SetService();
  EXPECT_CALL(
      *simple_proxy_,
      Connect(_, _, _, CellularCapability::kTimeoutConnect))
      .WillOnce(Invoke(this, &CellularTest::InvokeConnectSuccessNoService));
  EXPECT_CALL(*modem_info_.mock_manager(), UpdateService(_));
  GetCapabilityClassic()->simple_proxy_.reset(simple_proxy_.release());
  Error error;
  device_->Connect(&error);
}

TEST_F(CellularTest, LinkEventWontDestroyService) {
  // If the network interface goes down, Cellular::LinkEvent should
  // drop the connection but the service object should persist.
  device_->state_ = Cellular::kStateLinked;
  CellularService* service = SetService();
  device_->LinkEvent(0, 0);  // flags doesn't contain IFF_UP
  EXPECT_EQ(device_->state_, Cellular::kStateConnected);
  EXPECT_EQ(device_->service_, service);
}

TEST_F(CellularTest, UseNoArpGateway) {
  EXPECT_CALL(dhcp_provider_, CreateIPv4Config(kTestDeviceName, _, false, _))
      .WillOnce(Return(dhcp_config_));
  device_->AcquireIPConfig();
}

TEST_F(CellularTest, ModemStateChangeEnable) {
  EXPECT_CALL(*simple_proxy_,
              GetModemStatus(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularTest::InvokeGetModemStatus));
  EXPECT_CALL(*cdma_proxy_, MEID()).WillOnce(Return(kMEID));
  EXPECT_CALL(*proxy_,
              GetModemInfo(_, _, CellularCapability::kTimeoutDefault))
      .WillOnce(Invoke(this, &CellularTest::InvokeGetModemInfo));
  EXPECT_CALL(*cdma_proxy_, GetRegistrationState(nullptr, _, _))
      .WillOnce(Invoke(this,
                       &CellularTest::InvokeGetRegistrationStateUnregistered));
  EXPECT_CALL(*cdma_proxy_, GetSignalQuality(nullptr, _, _))
      .WillOnce(Invoke(this, &CellularTest::InvokeGetSignalQuality));
  EXPECT_CALL(*modem_info_.mock_manager(), UpdateEnabledTechnologies());
  device_->state_ = Cellular::kStateDisabled;
  device_->set_modem_state(Cellular::kModemStateDisabled);
  SetCellularType(Cellular::kTypeCDMA);

  KeyValueStore props;
  props.SetBool(CellularCapabilityClassic::kModemPropertyEnabled, true);
  device_->OnPropertiesChanged(MM_MODEM_INTERFACE, props, vector<string>());
  dispatcher_.DispatchPendingEvents();

  EXPECT_EQ(Cellular::kModemStateEnabled, device_->modem_state());
  EXPECT_EQ(Cellular::kStateEnabled, device_->state());
  EXPECT_TRUE(device_->enabled());
}

TEST_F(CellularTest, ModemStateChangeDisable) {
  EXPECT_CALL(*proxy_,
              Disconnect(_, _, CellularCapability::kTimeoutDisconnect))
      .WillOnce(Invoke(this, &CellularTest::InvokeDisconnect));
  EXPECT_CALL(*proxy_,
              Enable(false, _, _, CellularCapability::kTimeoutEnable))
      .WillOnce(Invoke(this, &CellularTest::InvokeEnable));
  EXPECT_CALL(*modem_info_.mock_manager(), UpdateEnabledTechnologies());
  device_->enabled_ = true;
  device_->enabled_pending_ = true;
  device_->state_ = Cellular::kStateEnabled;
  device_->set_modem_state(Cellular::kModemStateEnabled);
  SetCellularType(Cellular::kTypeCDMA);
  GetCapabilityClassic()->InitProxies();

  GetCapabilityClassic()->OnModemStateChangedSignal(kModemClassicStateEnabled,
                                                    kModemClassicStateDisabled,
                                                    0);
  dispatcher_.DispatchPendingEvents();

  EXPECT_EQ(Cellular::kModemStateDisabled, device_->modem_state());
  EXPECT_EQ(Cellular::kStateDisabled, device_->state());
  EXPECT_FALSE(device_->enabled());
}

TEST_F(CellularTest, ModemStateChangeStaleConnected) {
  // Test to make sure that we ignore stale modem Connected state transitions.
  // When a modem is asked to connect and before the connect completes, the
  // modem is disabled, it may send a stale Connected state transition after
  // it has been disabled.
  AllowCreateGSMCardProxyFromFactory();
  device_->state_ = Cellular::kStateDisabled;
  device_->modem_state_ = Cellular::kModemStateEnabling;
  device_->OnModemStateChanged(Cellular::kModemStateConnected);
  dispatcher_.DispatchPendingEvents();
  EXPECT_EQ(Cellular::kStateDisabled, device_->state());
}

TEST_F(CellularTest, ModemStateChangeValidConnected) {
  device_->state_ = Cellular::kStateEnabled;
  device_->modem_state_ = Cellular::kModemStateConnecting;
  SetService();
  device_->OnModemStateChanged(Cellular::kModemStateConnected);
  EXPECT_EQ(Cellular::kStateConnected, device_->state());
}

TEST_F(CellularTest, ModemStateChangeLostRegistration) {
  SetCellularType(Cellular::kTypeUniversal);
  CellularCapabilityUniversal* capability = GetCapabilityUniversal();
  capability->registration_state_ = MM_MODEM_3GPP_REGISTRATION_STATE_HOME;
  EXPECT_TRUE(capability->IsRegistered());
  device_->set_modem_state(Cellular::kModemStateRegistered);
  device_->OnModemStateChanged(Cellular::kModemStateEnabled);
  EXPECT_FALSE(capability->IsRegistered());
}

TEST_F(CellularTest, StartModemCallback) {
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  EXPECT_EQ(device_->state_, Cellular::kStateDisabled);
  device_->StartModemCallback(Bind(&CellularTest::TestCallback,
                                   Unretained(this)),
                              Error(Error::kSuccess));
  EXPECT_EQ(device_->state_, Cellular::kStateEnabled);
}

TEST_F(CellularTest, StartModemCallbackFail) {
  EXPECT_CALL(*this, TestCallback(IsFailure()));
  EXPECT_EQ(device_->state_, Cellular::kStateDisabled);
  device_->StartModemCallback(Bind(&CellularTest::TestCallback,
                                   Unretained(this)),
                              Error(Error::kOperationFailed));
  EXPECT_EQ(device_->state_, Cellular::kStateDisabled);
}

TEST_F(CellularTest, StopModemCallback) {
  EXPECT_CALL(*this, TestCallback(IsSuccess()));
  SetMockService();
  device_->StopModemCallback(Bind(&CellularTest::TestCallback,
                                  Unretained(this)),
                             Error(Error::kSuccess));
  EXPECT_EQ(device_->state_, Cellular::kStateDisabled);
  EXPECT_FALSE(device_->service_.get());
}

TEST_F(CellularTest, StopModemCallbackFail) {
  EXPECT_CALL(*this, TestCallback(IsFailure()));
  SetMockService();
  device_->StopModemCallback(Bind(&CellularTest::TestCallback,
                                  Unretained(this)),
                             Error(Error::kOperationFailed));
  EXPECT_EQ(device_->state_, Cellular::kStateDisabled);
  EXPECT_FALSE(device_->service_.get());
}

TEST_F(CellularTest, SetAllowRoaming) {
  EXPECT_FALSE(device_->allow_roaming_);
  EXPECT_CALL(*modem_info_.mock_manager(), UpdateDevice(_));
  Error error;
  device_->SetAllowRoaming(true, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_TRUE(device_->allow_roaming_);
}

class TestRPCTaskDelegate :
      public RPCTaskDelegate,
      public base::SupportsWeakPtr<TestRPCTaskDelegate> {
 public:
  virtual void GetLogin(std::string* user, std::string* password) {}
  virtual void Notify(const std::string& reason,
                      const std::map<std::string, std::string>& dict) {}
};

TEST_F(CellularTest, LinkEventUpWithPPP) {
  // If PPP is running, don't run DHCP as well.
  TestRPCTaskDelegate task_delegate;
  base::Callback<void(pid_t, int)> death_callback;
  unique_ptr<NiceMock<MockExternalTask>> mock_task(
      new NiceMock<MockExternalTask>(modem_info_.control_interface(),
                                     &process_manager_,
                                     task_delegate.AsWeakPtr(),
                                     death_callback));
  EXPECT_CALL(*mock_task, OnDelete()).Times(AnyNumber());
  device_->ppp_task_ = std::move(mock_task);
  device_->state_ = Cellular::kStateConnected;
  EXPECT_CALL(dhcp_provider_, CreateIPv4Config(kTestDeviceName, _, _, _))
      .Times(0);
  EXPECT_CALL(*dhcp_config_, RequestIP()).Times(0);
  device_->LinkEvent(IFF_UP, 0);
}

TEST_F(CellularTest, LinkEventUpWithoutPPP) {
  // If PPP is not running, fire up DHCP.
  device_->state_ = Cellular::kStateConnected;
  EXPECT_CALL(dhcp_provider_, CreateIPv4Config(kTestDeviceName, _, _, _))
      .WillOnce(Return(dhcp_config_));
  EXPECT_CALL(*dhcp_config_, RequestIP());
  EXPECT_CALL(*dhcp_config_, ReleaseIP(_)).Times(AnyNumber());
  device_->LinkEvent(IFF_UP, 0);
}

TEST_F(CellularTest, StartPPP) {
  const int kPID = 234;
  EXPECT_EQ(nullptr, device_->ppp_task_);
  StartPPP(kPID);
}

TEST_F(CellularTest, StartPPPAlreadyStarted) {
  const int kPID = 234;
  StartPPP(kPID);

  const int kPID2 = 235;
  StartPPP(kPID2);
}

TEST_F(CellularTest, StartPPPAfterEthernetUp) {
  CellularService* service(SetService());
  device_->state_ = Cellular::kStateLinked;
  device_->set_ipconfig(dhcp_config_);
  device_->SelectService(service);
  EXPECT_CALL(*dhcp_config_, ReleaseIP(_))
      .Times(AnyNumber())
      .WillRepeatedly(Return(true));
  const int kPID = 234;
  EXPECT_EQ(nullptr, device_->ppp_task_);
  StartPPP(kPID);
  EXPECT_EQ(Cellular::kStateLinked, device_->state());
}

TEST_F(CellularTest, GetLogin) {
  // Doesn't crash when there is no service.
  string username_to_pppd;
  string password_to_pppd;
  EXPECT_FALSE(device_->service());
  device_->GetLogin(&username_to_pppd, &password_to_pppd);

  // Provides expected username and password in normal case.
  const char kFakeUsername[] = "fake-user";
  const char kFakePassword[] = "fake-password";
  CellularService& service(*SetService());
  service.ppp_username_ = kFakeUsername;
  service.ppp_password_ = kFakePassword;
  device_->GetLogin(&username_to_pppd, &password_to_pppd);
}

TEST_F(CellularTest, Notify) {
  // Common setup.
  MockPPPDeviceFactory* ppp_device_factory =
      MockPPPDeviceFactory::GetInstance();
  const int kPID = 91;
  device_->ppp_device_factory_ = ppp_device_factory;
  SetMockService();
  StartPPP(kPID);

  const map<string, string> kEmptyArgs;
  device_->Notify(kPPPReasonAuthenticating, kEmptyArgs);
  EXPECT_TRUE(device_->is_ppp_authenticating_);
  device_->Notify(kPPPReasonAuthenticated, kEmptyArgs);
  EXPECT_FALSE(device_->is_ppp_authenticating_);

  // Normal connect.
  const string kInterfaceName("fake-device");
  const int kInterfaceIndex = 1;
  scoped_refptr<MockPPPDevice> ppp_device;
  map<string, string> ppp_config;
  ppp_device =
      new MockPPPDevice(modem_info_.control_interface(), nullptr, nullptr,
                        nullptr, kInterfaceName, kInterfaceIndex);
  ppp_config[kPPPInterfaceName] = kInterfaceName;
  EXPECT_CALL(device_info_, GetIndex(kInterfaceName))
      .WillOnce(Return(kInterfaceIndex));
  EXPECT_CALL(device_info_, RegisterDevice(_));
  EXPECT_CALL(*ppp_device_factory,
              CreatePPPDevice(_, _, _, _, kInterfaceName, kInterfaceIndex))
      .WillOnce(Return(ppp_device.get()));
  EXPECT_CALL(*ppp_device, SetEnabled(true));
  EXPECT_CALL(*ppp_device, SelectService(_));
  EXPECT_CALL(*ppp_device, UpdateIPConfigFromPPP(ppp_config, false));
  device_->Notify(kPPPReasonConnect, ppp_config);
  Mock::VerifyAndClearExpectations(&device_info_);
  Mock::VerifyAndClearExpectations(ppp_device.get());

  // Re-connect on same network device: if pppd sends us multiple connect
  // events, we behave sanely.
  EXPECT_CALL(device_info_, GetIndex(kInterfaceName))
      .WillOnce(Return(kInterfaceIndex));
  EXPECT_CALL(*ppp_device, SetEnabled(true));
  EXPECT_CALL(*ppp_device, SelectService(_));
  EXPECT_CALL(*ppp_device, UpdateIPConfigFromPPP(ppp_config, false));
  device_->Notify(kPPPReasonConnect, ppp_config);
  Mock::VerifyAndClearExpectations(&device_info_);
  Mock::VerifyAndClearExpectations(ppp_device.get());

  // Re-connect on new network device: if we still have the PPPDevice
  // from a prior connect, this new connect should DTRT. This is
  // probably an unlikely case.
  const string kInterfaceName2("fake-device2");
  const int kInterfaceIndex2 = 2;
  scoped_refptr<MockPPPDevice> ppp_device2;
  map<string, string> ppp_config2;
  ppp_device2 =
      new MockPPPDevice(modem_info_.control_interface(), nullptr, nullptr,
                        nullptr, kInterfaceName2, kInterfaceIndex2);
  ppp_config2[kPPPInterfaceName] = kInterfaceName2;
  EXPECT_CALL(device_info_, GetIndex(kInterfaceName2))
      .WillOnce(Return(kInterfaceIndex2));
  EXPECT_CALL(device_info_,
              RegisterDevice(static_cast<DeviceRefPtr>(ppp_device2)));
  EXPECT_CALL(*ppp_device_factory,
              CreatePPPDevice(_, _, _, _, kInterfaceName2, kInterfaceIndex2))
      .WillOnce(Return(ppp_device2.get()));
  EXPECT_CALL(*ppp_device, SelectService(ServiceRefPtr(nullptr)));
  EXPECT_CALL(*ppp_device2, SetEnabled(true));
  EXPECT_CALL(*ppp_device2, SelectService(_));
  EXPECT_CALL(*ppp_device2, UpdateIPConfigFromPPP(ppp_config2, false));
  device_->Notify(kPPPReasonConnect, ppp_config2);
  Mock::VerifyAndClearExpectations(&device_info_);
  Mock::VerifyAndClearExpectations(ppp_device.get());
  Mock::VerifyAndClearExpectations(ppp_device2.get());

  // Disconnect should report unknown failure, since we had a
  // Notify(kPPPReasonAuthenticated, ...).
  EXPECT_CALL(*ppp_device2, SetServiceFailure(Service::kFailureUnknown));
  device_->Notify(kPPPReasonDisconnect, kEmptyArgs);
  EXPECT_EQ(nullptr, device_->ppp_task_);

  // |Cellular::ppp_task_| is destroyed on the task loop. Must dispatch once to
  // cleanup.
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularTest, PPPConnectionFailedBeforeAuth) {
  // Test that we properly set Service state in the case where pppd
  // disconnects before authenticating (as opposed to the Notify test,
  // where pppd disconnects after connecting).
  const int kPID = 52;
  const map<string, string> kEmptyArgs;
  MockCellularService* service = SetMockService();
  StartPPP(kPID);

  ExpectDisconnectCapabilityUniversal();
  EXPECT_CALL(*service, SetFailure(Service::kFailureUnknown));
  device_->Notify(kPPPReasonDisconnect, kEmptyArgs);
  EXPECT_EQ(nullptr, device_->ppp_task_);
  VerifyDisconnect();

  // |Cellular::ppp_task_| is destroyed on the task loop. Must dispatch once to
  // cleanup.
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularTest, PPPConnectionFailedDuringAuth) {
  // Test that we properly set Service state in the case where pppd
  // disconnects during authentication (as opposed to the Notify test,
  // where pppd disconnects after connecting).
  const int kPID = 52;
  const map<string, string> kEmptyArgs;
  MockCellularService* service = SetMockService();
  StartPPP(kPID);

  ExpectDisconnectCapabilityUniversal();
  EXPECT_CALL(*service, SetFailure(Service::kFailurePPPAuth));
  device_->Notify(kPPPReasonAuthenticating, kEmptyArgs);
  device_->Notify(kPPPReasonDisconnect, kEmptyArgs);
  EXPECT_EQ(nullptr, device_->ppp_task_);
  VerifyDisconnect();

  // |Cellular::ppp_task_| is destroyed on the task loop. Must dispatch once to
  // cleanup.
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularTest, PPPConnectionFailedAfterAuth) {
  // Test that we properly set Service state in the case where pppd
  // disconnects after authenticating, but before connecting (as
  // opposed to the Notify test, where pppd disconnects after
  // connecting).
  const int kPID = 52;
  const map<string, string> kEmptyArgs;
  MockCellularService* service = SetMockService();
  StartPPP(kPID);

  EXPECT_CALL(*service, SetFailure(Service::kFailureUnknown));
  ExpectDisconnectCapabilityUniversal();
  device_->Notify(kPPPReasonAuthenticating, kEmptyArgs);
  device_->Notify(kPPPReasonAuthenticated, kEmptyArgs);
  device_->Notify(kPPPReasonDisconnect, kEmptyArgs);
  EXPECT_EQ(nullptr, device_->ppp_task_);
  VerifyDisconnect();

  // |Cellular::ppp_task_| is destroyed on the task loop. Must dispatch once to
  // cleanup.
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularTest, OnPPPDied) {
  const int kPID = 1234;
  const int kExitStatus = 5;
  ExpectDisconnectCapabilityUniversal();
  device_->OnPPPDied(kPID, kExitStatus);
  VerifyDisconnect();
}

TEST_F(CellularTest, OnPPPDiedCleanupDevice) {
  // Test that OnPPPDied causes the ppp_device_ reference to be dropped.
  const int kPID = 123;
  const int kExitStatus = 5;
  StartPPP(kPID);
  FakeUpConnectedPPP();
  ExpectDisconnectCapabilityUniversal();
  device_->OnPPPDied(kPID, kExitStatus);
  VerifyPPPStopped();

  // |Cellular::ppp_task_| is destroyed on the task loop. Must dispatch once to
  // cleanup.
  dispatcher_.DispatchPendingEvents();
}

TEST_F(CellularTest, DropConnection) {
  device_->set_ipconfig(dhcp_config_);
  EXPECT_CALL(*dhcp_config_, ReleaseIP(_));
  device_->DropConnection();
  Mock::VerifyAndClearExpectations(dhcp_config_.get());  // verify before dtor
  EXPECT_FALSE(device_->ipconfig());
}

TEST_F(CellularTest, DropConnectionPPP) {
  scoped_refptr<MockPPPDevice> ppp_device(
      new MockPPPDevice(modem_info_.control_interface(),
                        nullptr, nullptr, nullptr, "fake_ppp0", -1));
  EXPECT_CALL(*ppp_device, DropConnection());
  device_->ppp_device_ = ppp_device;
  device_->DropConnection();
}

TEST_F(CellularTest, ChangeServiceState) {
  MockCellularService* service(SetMockService());
  EXPECT_CALL(*service, SetState(_));
  EXPECT_CALL(*service, SetFailure(_));
  EXPECT_CALL(*service, SetFailureSilent(_));
  ON_CALL(*service, state()).WillByDefault(Return(Service::kStateUnknown));

  // Without PPP, these should be handled by our selected_service().
  device_->SelectService(service);
  device_->SetServiceState(Service::kStateConfiguring);
  device_->SetServiceFailure(Service::kFailurePPPAuth);
  device_->SetServiceFailureSilent(Service::kFailureUnknown);
  Mock::VerifyAndClearExpectations(service);  // before Cellular dtor
}

TEST_F(CellularTest, ChangeServiceStatePPP) {
  MockCellularService* service(SetMockService());
  scoped_refptr<MockPPPDevice> ppp_device(
      new MockPPPDevice(modem_info_.control_interface(),
                        nullptr, nullptr, nullptr, "fake_ppp0", -1));
  EXPECT_CALL(*ppp_device, SetServiceState(_));
  EXPECT_CALL(*ppp_device, SetServiceFailure(_));
  EXPECT_CALL(*ppp_device, SetServiceFailureSilent(_));
  EXPECT_CALL(*service, SetState(_)).Times(0);
  EXPECT_CALL(*service, SetFailure(_)).Times(0);
  EXPECT_CALL(*service, SetFailureSilent(_)).Times(0);
  device_->ppp_device_ = ppp_device;

  // With PPP, these should all be punted over to the |ppp_device|.
  // Note in particular that Cellular does not manipulate |service| in
  // this case.
  device_->SetServiceState(Service::kStateConfiguring);
  device_->SetServiceFailure(Service::kFailurePPPAuth);
  device_->SetServiceFailureSilent(Service::kFailureUnknown);
}

TEST_F(CellularTest, StopPPPOnDisconnect) {
  const int kPID = 123;
  Error error;
  StartPPP(kPID);
  FakeUpConnectedPPP();
  ExpectPPPStopped();
  device_->Disconnect(&error, "in test");
  VerifyPPPStopped();
}

TEST_F(CellularTest, StopPPPOnSuspend) {
  const int kPID = 123;
  StartPPP(kPID);
  FakeUpConnectedPPP();
  ExpectPPPStopped();
  device_->OnBeforeSuspend(ResultCallback());
  VerifyPPPStopped();
}

TEST_F(CellularTest, OnAfterResumeDisabledWantDisabled) {
  // The Device was disabled prior to resume, and the profile settings
  // indicate that the device should be disabled. We should leave
  // things alone.

  // Initial state.
  mm1::MockModemProxy* mm1_proxy = SetupOnAfterResume();
  set_enabled_persistent(false);
  EXPECT_FALSE(device_->running());
  EXPECT_FALSE(device_->enabled_persistent());
  EXPECT_EQ(Cellular::kStateDisabled, device_->state_);

  // Resume, while device is disabled.
  EXPECT_CALL(*mm1_proxy, Enable(_, _, _, _)).Times(0);
  device_->OnAfterResume();
  EXPECT_FALSE(device_->running());
  EXPECT_FALSE(device_->enabled_persistent());
  EXPECT_EQ(Cellular::kStateDisabled, device_->state_);
}

TEST_F(CellularTest, OnAfterResumeDisableInProgressWantDisabled) {
  // The Device was not disabled prior to resume, but the profile
  // settings indicate that the device _should be_ disabled. Most
  // likely, we started disabling the device, but that did not
  // complete before we suspended. We should leave things alone.

  // Initial state.
  mm1::MockModemProxy* mm1_proxy = SetupOnAfterResume();
  Error error;
  EXPECT_CALL(*mm1_proxy, Enable(true, _, _, _))
      .WillOnce(Invoke(this, &CellularTest::InvokeEnable));
  device_->SetEnabled(true);
  EXPECT_TRUE(device_->running());
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);

  // Start disable.
  EXPECT_CALL(*modem_info_.mock_manager(), UpdateDevice(_));
  device_->SetEnabledPersistent(false, &error, ResultCallback());
  EXPECT_FALSE(device_->running());  // changes immediately
  EXPECT_FALSE(device_->enabled_persistent());  // changes immediately
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);  // changes on completion

  // Resume, with disable still in progress.
  device_->OnAfterResume();
  EXPECT_FALSE(device_->running());
  EXPECT_FALSE(device_->enabled_persistent());
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);

  // Finish the disable operation.
  EXPECT_CALL(*mm1_proxy, Enable(false, _, _, _))
      .WillOnce(Invoke(this, &CellularTest::InvokeEnable));
  EXPECT_CALL(*mm1_proxy, SetPowerState(_, _, _, _))
      .WillOnce(Invoke(this, &CellularTest::InvokeSetPowerState));
  dispatcher_.DispatchPendingEvents();
  EXPECT_FALSE(device_->running());
  EXPECT_FALSE(device_->enabled_persistent());
  EXPECT_EQ(Cellular::kStateDisabled, device_->state_);
}

TEST_F(CellularTest, OnAfterResumeDisableQueuedWantEnabled) {
  // The Device was not disabled prior to resume, and the profile
  // settings indicate that the device should be enabled. In
  // particular, we went into suspend before we actually processed the
  // task queued by CellularCapabilityUniversal::StopModem.
  //
  // This is unlikely, and a case where we fail to do the right thing.
  // The tests exists to document this corner case, which we get wrong.

  // Initial state.
  mm1::MockModemProxy* mm1_proxy = SetupOnAfterResume();
  EXPECT_CALL(*mm1_proxy, Enable(true, _, _, _))
      .WillOnce(Invoke(this, &CellularTest::InvokeEnable));
  device_->SetEnabled(true);
  EXPECT_TRUE(device_->running());
  EXPECT_TRUE(device_->enabled_persistent());
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);

  // Start disable.
  device_->SetEnabled(false);
  EXPECT_FALSE(device_->running());  // changes immediately
  EXPECT_TRUE(device_->enabled_persistent());  // no change
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);  // changes on completion

  // Refresh proxies, since CellularCapabilityUniversal::StartModem wants
  // new proxies. Also, stash away references for later.
  PopulateProxies();
  SetCommonOnAfterResumeExpectations();
  mm1_proxy = mm1_proxy_.get();
  auto dbus_properties_proxy = dbus_properties_proxy_.get();

  // Resume, with disable still in progress.
  EXPECT_CALL(*mm1_proxy, Enable(true, _, _, _))
      .WillOnce(Invoke(this, &CellularTest::InvokeEnableReturningWrongState));
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);  // disable still pending
  device_->OnAfterResume();
  EXPECT_TRUE(device_->running());  // changes immediately
  EXPECT_TRUE(device_->enabled_persistent());  // no change
  EXPECT_EQ(Cellular::kStateDisabled, device_->state_);  // by OnAfterResume

  // Set up state that we need.
  KeyValueStore modem_properties;
  modem_properties.SetInt(MM_MODEM_PROPERTY_STATE,
                          Cellular::kModemStateDisabled);

  // Let the disable complete.
  EXPECT_CALL(*mm1_proxy, Enable(false, _, _, _))
      .WillOnce(Invoke(this, &CellularTest::InvokeEnable));
  EXPECT_CALL(*mm1_proxy, SetPowerState(_, _, _, _))
      .WillOnce(Invoke(this, &CellularTest::InvokeSetPowerState));
  EXPECT_CALL(*dbus_properties_proxy, GetAll(_))
      .WillRepeatedly(Return(modem_properties));
  dispatcher_.DispatchPendingEvents();
  EXPECT_TRUE(device_->running());  // last changed by OnAfterResume
  EXPECT_TRUE(device_->enabled_persistent());  // last changed by OnAfterResume
  EXPECT_EQ(Cellular::kStateDisabled, device_->state_);

  // There's nothing queued up to restart the modem. Even though we
  // want to be running, we're stuck in the disabled state.
  dispatcher_.DispatchPendingEvents();
  EXPECT_TRUE(device_->running());
  EXPECT_TRUE(device_->enabled_persistent());
  EXPECT_EQ(Cellular::kStateDisabled, device_->state_);
}

TEST_F(CellularTest, OnAfterResumePowerDownInProgressWantEnabled) {
  // The Device was not fully disabled prior to resume, and the
  // profile settings indicate that the device should be enabled. In
  // this case, we have disabled the device, but are waiting for the
  // power-down (switch to low power) to complete.
  //
  // This test emulates the behavior of the Huawei E303 dongle, when
  // Manager::kTerminationActionsTimeoutMilliseconds is 9500
  // msec. (The dongle takes 10-11 seconds to go through the whole
  // disable, power-down sequence).
  //
  // Eventually, the power-down would complete, and the device would
  // be stuck in the disabled state. To counter-act that,
  // OnAfterResume tries to enable the device now, even though the
  // device is currently enabled.

  // Initial state.
  mm1::MockModemProxy* mm1_proxy = SetupOnAfterResume();
  EXPECT_CALL(*mm1_proxy, Enable(true, _, _, _))
      .WillOnce(Invoke(this, &CellularTest::InvokeEnable));
  device_->SetEnabled(true);
  EXPECT_TRUE(device_->running());
  EXPECT_TRUE(device_->enabled_persistent());
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);

  // Start disable.
  ResultCallback modem_proxy_enable_callback;
  EXPECT_CALL(*mm1_proxy, Enable(false, _, _, _))
      .WillOnce(SaveArg<2>(&modem_proxy_enable_callback));
  device_->SetEnabled(false);
  dispatcher_.DispatchPendingEvents();  // SetEnabled yields a deferred task
  EXPECT_FALSE(device_->running());  // changes immediately
  EXPECT_TRUE(device_->enabled_persistent());  // no change
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);  // changes on completion

  // Let the disable complete. That will trigger power-down.
  //
  // Note that, unlike for mm1_proxy->Enable, we don't save the
  // callback for mm1_proxy->SetPowerState. We expect the callback not
  // to be executed, as explained in the comment about having a fresh
  // proxy OnAfterResume, below.
  Error error;
  ASSERT_TRUE(error.IsSuccess());
  EXPECT_CALL(*mm1_proxy, SetPowerState(MM_MODEM_POWER_STATE_LOW, _, _, _))
      .WillOnce(SetErrorTypeInArgument<1>(Error::kOperationInitiated));
  modem_proxy_enable_callback.Run(error);

  // No response to power-down yet. It probably completed while the host
  // was asleep, and so the reply from the modem was lost.

  // Refresh proxies, since CellularCapabilityUniversal::StartModem wants
  // new proxies. Also, stash away references for later.
  PopulateProxies();
  SetCommonOnAfterResumeExpectations();
  auto new_mm1_proxy = mm1_proxy_.get();
  auto dbus_properties_proxy = dbus_properties_proxy_.get();

  // Resume.
  ResultCallback new_callback;
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);  // disable still pending
  EXPECT_CALL(*new_mm1_proxy, Enable(true, _, _, _))
      .WillOnce(SaveArg<2>(&modem_proxy_enable_callback));
  device_->OnAfterResume();
  EXPECT_TRUE(device_->running());  // changes immediately
  EXPECT_TRUE(device_->enabled_persistent());  // no change
  EXPECT_EQ(Cellular::kStateDisabled, device_->state_);  // by OnAfterResume

  // We should have a fresh proxy OnAfterResume. Otherwise, we may get
  // confused when the SetPowerState call completes (either naturally,
  // or via a time-out from dbus-c++).
  //
  // The pointers must differ, because the new proxy is constructed
  // before the old one is destructed.
  EXPECT_FALSE(new_mm1_proxy == mm1_proxy);

  // Set up state that we need.
  KeyValueStore modem_properties;
  modem_properties.SetInt(MM_MODEM_PROPERTY_STATE,
                          Cellular::kModemStateEnabled);

  // Let the enable complete.
  ASSERT_TRUE(error.IsSuccess());
  EXPECT_CALL(*dbus_properties_proxy, GetAll(_))
      .WillRepeatedly(Return(modem_properties));
  ASSERT_TRUE(!modem_proxy_enable_callback.is_null());
  modem_proxy_enable_callback.Run(error);
  EXPECT_TRUE(device_->running());
  EXPECT_TRUE(device_->enabled_persistent());
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);
}

TEST_F(CellularTest, OnAfterResumeDisabledWantEnabled) {
  // This is the ideal case. The disable process completed before
  // going into suspend.
  mm1::MockModemProxy* mm1_proxy = SetupOnAfterResume();
  EXPECT_FALSE(device_->running());
  EXPECT_TRUE(device_->enabled_persistent());
  EXPECT_EQ(Cellular::kStateDisabled, device_->state_);

  // Resume.
  ResultCallback modem_proxy_enable_callback;
  EXPECT_CALL(*mm1_proxy, Enable(true, _, _, _))
      .WillOnce(SaveArg<2>(&modem_proxy_enable_callback));
  device_->OnAfterResume();

  // Complete enable.
  Error error;
  ASSERT_TRUE(error.IsSuccess());
  modem_proxy_enable_callback.Run(error);
  EXPECT_TRUE(device_->running());
  EXPECT_TRUE(device_->enabled_persistent());
  EXPECT_EQ(Cellular::kStateEnabled, device_->state_);
}

// Custom property setters should return false, and make no changes, if
// the new value is the same as the old value.
TEST_F(CellularTest, CustomSetterNoopChange) {
  Error error;
  EXPECT_FALSE(device_->allow_roaming_);
  EXPECT_FALSE(device_->SetAllowRoaming(false, &error));
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(CellularTest, ScanImmediateFailure) {
  Error error;

  device_->set_found_networks(kTestNetworksCellular);
  EXPECT_FALSE(device_->scanning_);
  // |InitProxies| must be called before calling any functions on the
  // Capability*, to set up the modem proxies.
  // Warning: The test loses all references to the proxies when |InitProxies| is
  // called.
  GetCapabilityGSM()->InitProxies();
  device_->Scan(Device::kFullScan, &error, "");
  EXPECT_TRUE(error.IsFailure());
  EXPECT_FALSE(device_->scanning_);
  EXPECT_EQ(kTestNetworksCellular, device_->found_networks());
}

TEST_F(CellularTest, ScanAsynchronousFailure) {
  Error error;
  ScanResultsCallback results_callback;

  device_->set_found_networks(kTestNetworksCellular);
  EXPECT_CALL(*gsm_network_proxy_, Scan(&error, _, _))
      .WillOnce(DoAll(SetErrorTypeInArgument<0>(Error::kOperationInitiated),
                      SaveArg<1>(&results_callback)));
  EXPECT_FALSE(device_->scanning_);
  // |InitProxies| must be called before calling any functions on the
  // Capability*, to set up the modem proxies.
  // Warning: The test loses all references to the proxies when |InitProxies| is
  // called.
  GetCapabilityGSM()->InitProxies();
  device_->Scan(Device::kFullScan, &error, "");
  EXPECT_TRUE(error.IsOngoing());
  EXPECT_TRUE(device_->scanning_);

  // Asynchronously fail the scan.
  error.Populate(Error::kOperationFailed);
  results_callback.Run(kTestNetworksGSM, error);
  EXPECT_FALSE(device_->scanning_);
  EXPECT_TRUE(device_->found_networks().empty());
}

TEST_F(CellularTest, ScanSuccess) {
  Error error;
  ScanResultsCallback results_callback;

  device_->clear_found_networks();
  EXPECT_CALL(*gsm_network_proxy_, Scan(&error, _, _))
      .WillOnce(DoAll(SetErrorTypeInArgument<0>(Error::kOperationInitiated),
                      SaveArg<1>(&results_callback)));
  EXPECT_FALSE(device_->scanning_);
  // |InitProxies| must be called before calling any functions on the
  // Capability*, to set up the modem proxies.
  // Warning: The test loses all references to the proxies when |InitProxies| is
  // called.
  GetCapabilityGSM()->InitProxies();
  device_->Scan(Device::kFullScan, &error, "");
  EXPECT_TRUE(error.IsOngoing());
  EXPECT_TRUE(device_->scanning_);

  // Successfully complete the scan.
  const GSMScanResults gsm_results{};
  error.Populate(Error::kSuccess);
  results_callback.Run(kTestNetworksGSM, error);
  EXPECT_FALSE(device_->scanning_);
  EXPECT_EQ(kTestNetworksCellular, device_->found_networks());
}

TEST_F(CellularTest, EstablishLinkDHCP) {
  unique_ptr<CellularBearer> bearer(
      new CellularBearer(&control_interface_, "", ""));
  bearer->set_ipv4_config_method(IPConfig::kMethodDHCP);
  SetCapabilityUniversalActiveBearer(std::move(bearer));
  device_->state_ = Cellular::kStateConnected;

  MockCellularService* service = SetMockService();
  ON_CALL(*service, state()).WillByDefault(Return(Service::kStateUnknown));

  EXPECT_CALL(device_info_, GetFlags(device_->interface_index(), _))
      .WillOnce(DoAll(SetArgumentPointee<1>(IFF_UP), Return(true)));
  EXPECT_CALL(dhcp_provider_, CreateIPv4Config(kTestDeviceName, _, _, _))
      .WillOnce(Return(dhcp_config_));
  EXPECT_CALL(*dhcp_config_, RequestIP()).WillOnce(Return(true));
  EXPECT_CALL(*service, SetState(Service::kStateConfiguring));
  device_->EstablishLink();
  EXPECT_EQ(service, device_->selected_service());
  Mock::VerifyAndClearExpectations(service);  // before Cellular dtor
}

TEST_F(CellularTest, EstablishLinkPPP) {
  unique_ptr<CellularBearer> bearer(
      new CellularBearer(&control_interface_, "", ""));
  bearer->set_ipv4_config_method(IPConfig::kMethodPPP);
  SetCapabilityUniversalActiveBearer(std::move(bearer));
  device_->state_ = Cellular::kStateConnected;

  const int kPID = 123;
  EXPECT_CALL(process_manager_, StartProcess(_, _, _, _, _, _))
      .WillOnce(Return(kPID));
  device_->EstablishLink();
  EXPECT_FALSE(device_->ipconfig());  // No DHCP client.
  EXPECT_FALSE(device_->selected_service());
  EXPECT_FALSE(device_->is_ppp_authenticating_);
  EXPECT_NE(nullptr, device_->ppp_task_);
}

TEST_F(CellularTest, EstablishLinkStatic) {
  IPAddress::Family kAddressFamily = IPAddress::kFamilyIPv4;
  const char kAddress[] = "10.0.0.1";
  const char kGateway[] = "10.0.0.254";
  const int32_t kSubnetPrefix = 16;
  const char* const kDNS[] = {"10.0.0.2", "8.8.4.4", "8.8.8.8"};

  unique_ptr<IPConfig::Properties> ipconfig_properties(
      new IPConfig::Properties);
  ipconfig_properties->address_family = kAddressFamily;
  ipconfig_properties->address = kAddress;
  ipconfig_properties->gateway = kGateway;
  ipconfig_properties->subnet_prefix = kSubnetPrefix;
  ipconfig_properties->dns_servers = vector<string>{kDNS[0], kDNS[1], kDNS[2]};

  unique_ptr<CellularBearer> bearer(
      new CellularBearer(&control_interface_, "", ""));
  bearer->set_ipv4_config_method(IPConfig::kMethodStatic);
  bearer->set_ipv4_config_properties(std::move(ipconfig_properties));
  SetCapabilityUniversalActiveBearer(std::move(bearer));
  device_->state_ = Cellular::kStateConnected;

  MockCellularService* service = SetMockService();
  ON_CALL(*service, state()).WillByDefault(Return(Service::kStateUnknown));

  EXPECT_CALL(device_info_, GetFlags(device_->interface_index(), _))
      .WillOnce(DoAll(SetArgumentPointee<1>(IFF_UP), Return(true)));
  EXPECT_CALL(*service, SetState(Service::kStateConfiguring));
  device_->EstablishLink();
  EXPECT_EQ(service, device_->selected_service());
  ASSERT_TRUE(device_->ipconfig());
  EXPECT_EQ(kAddressFamily, device_->ipconfig()->properties().address_family);
  EXPECT_EQ(kAddress, device_->ipconfig()->properties().address);
  EXPECT_EQ(kGateway, device_->ipconfig()->properties().gateway);
  EXPECT_EQ(kSubnetPrefix, device_->ipconfig()->properties().subnet_prefix);
  ASSERT_EQ(3, device_->ipconfig()->properties().dns_servers.size());
  EXPECT_EQ(kDNS[0], device_->ipconfig()->properties().dns_servers[0]);
  EXPECT_EQ(kDNS[1], device_->ipconfig()->properties().dns_servers[1]);
  EXPECT_EQ(kDNS[2], device_->ipconfig()->properties().dns_servers[2]);
  Mock::VerifyAndClearExpectations(service);  // before Cellular dtor
}

}  // namespace shill
