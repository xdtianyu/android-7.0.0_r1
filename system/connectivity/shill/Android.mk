#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

# Common variables
# ========================================================

# Definitions applying to all targets built from C++ source files.
# Be sure to $(eval) this last.
define shill_cpp_common
  LOCAL_CPP_EXTENSION := .cc
  LOCAL_CLANG := true
  LOCAL_CFLAGS := \
      -Wextra \
      -Werror \
      -Wno-unused-parameter \
      -DRUNDIR=\"/data/misc/shill\" \
      -DSHIMDIR=\"/system/lib/shill/shims\" \
      -DDISABLE_CELLULAR \
      -DDISABLE_VPN \
      -DDISABLE_WAKE_ON_WIFI \
      -DDISABLE_WIMAX \
      -DENABLE_CHROMEOS_DBUS \
      -DENABLE_JSON_STORE
  ifeq ($(SHILL_USE_BINDER), true)
    LOCAL_CFLAGS += -DENABLE_BINDER
  endif # SHILL_USE_BINDER
  ifneq ($(SHILL_USE_WIFI), true)
    LOCAL_CFLAGS += -DDISABLE_WIFI
  endif
  ifneq ($(SHILL_USE_DHCPV6), true)
    LOCAL_CFLAGS += -DDISABLE_DHCPV6
  endif
  ifneq ($(SHILL_USE_PPPOE), true)
    LOCAL_CFLAGS += -DDISABLE_PPPOE
  endif
  ifneq ($(SHILL_USE_WIRED_8021X), true)
    LOCAL_CFLAGS += -DDISABLE_WIRED_8021X
  endif
  # The following flags ensure that shill builds with the same compiler
  # warnings disabled in CrOS and Android.
  LOCAL_CFLAGS +=  \
      -Wmultichar \
      -Wunused
endef

shill_parent_dir := $(LOCAL_PATH)/../

shill_c_includes := \
    $(shill_parent_dir) \
    external/gtest/include/

shill_shared_libraries := \
    libbrillo \
    libchrome \
    libdbus

shill_cpp_flags := \
    -fno-strict-aliasing \
    -Woverloaded-virtual \
    -Wno-missing-field-initializers  # for LAZY_INSTANCE_INITIALIZER

# libshill-net (shared library)
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libshill-net
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_CPPFLAGS := $(shill_cpp_flags)
LOCAL_SHARED_LIBRARIES := $(shill_shared_libraries)
LOCAL_C_INCLUDES := $(shill_c_includes)
LOCAL_SRC_FILES := \
    net/attribute_list.cc \
    net/byte_string.cc \
    net/control_netlink_attribute.cc \
    net/event_history.cc \
    net/generic_netlink_message.cc \
    net/io_handler_factory.cc \
    net/io_handler_factory_container.cc \
    net/io_input_handler.cc \
    net/io_ready_handler.cc \
    net/ip_address.cc \
    net/netlink_attribute.cc \
    net/netlink_manager.cc \
    net/netlink_message.cc \
    net/netlink_packet.cc \
    net/netlink_socket.cc \
    net/nl80211_attribute.cc \
    net/nl80211_message.cc \
    net/rtnl_handler.cc \
    net/rtnl_listener.cc \
    net/rtnl_message.cc \
    net/shill_time.cc \
    net/sockets.cc
$(eval $(shill_cpp_common))
include $(BUILD_SHARED_LIBRARY)

# libshill-client (shared library)
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libshill-client
LOCAL_DBUS_PROXY_PREFIX := shill
# TODO(samueltan): do not build these dbus-xml files when shill is using
# its Binder interface. All of shill's D-Bus clients will have to accommodate
# to this change.
LOCAL_SRC_FILES := \
    dbus_bindings/dbus-service-config.json \
    dbus_bindings/org.chromium.flimflam.Device.dbus-xml \
    dbus_bindings/org.chromium.flimflam.IPConfig.dbus-xml \
    dbus_bindings/org.chromium.flimflam.Manager.dbus-xml \
    dbus_bindings/org.chromium.flimflam.Profile.dbus-xml \
    dbus_bindings/org.chromium.flimflam.Service.dbus-xml \
    dbus_bindings/org.chromium.flimflam.Task.dbus-xml \
    dbus_bindings/org.chromium.flimflam.ThirdPartyVpn.dbus-xml
ifeq ($(SHILL_USE_BINDER), true)
LOCAL_AIDL_INCLUDES := \
    system/connectivity/shill/binder \
    frameworks/native/aidl/binder
LOCAL_SHARED_LIBRARIES := libbinder libutils
LOCAL_SRC_FILES += \
    binder/android/system/connectivity/shill/IDevice.aidl \
    binder/android/system/connectivity/shill/IManager.aidl \
    binder/android/system/connectivity/shill/IService.aidl \
    binder/android/system/connectivity/shill/IPropertyChangedCallback.aidl
endif # SHILL_USE_BINDER
LOCAL_EXPORT_C_INCLUDE_DIRS := external/cros/system_api/
include $(BUILD_SHARED_LIBRARY)

# supplicant-proxies (static library)
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := supplicant-proxies
LOCAL_DBUS_PROXY_PREFIX := supplicant
LOCAL_SRC_FILES := \
    dbus_bindings/supplicant-bss.dbus-xml \
    dbus_bindings/supplicant-interface.dbus-xml \
    dbus_bindings/supplicant-network.dbus-xml \
    dbus_bindings/supplicant-process.dbus-xml
include $(BUILD_STATIC_LIBRARY)

# dhcpcd-proxies (static library)
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := dhcpcd-proxies
LOCAL_DBUS_PROXY_PREFIX := dhcpcd
LOCAL_SRC_FILES := dbus_bindings/dhcpcd.dbus-xml
include $(BUILD_STATIC_LIBRARY)

# libshill (static library)
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libshill
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_CPPFLAGS := $(shill_cpp_flags) -Wno-sign-compare
LOCAL_STATIC_LIBRARIES := \
    supplicant-proxies \
    dhcpcd-proxies
LOCAL_SHARED_LIBRARIES := \
    $(shill_shared_libraries) \
    libshill-net \
    libcares \
    libmetrics \
    libprotobuf-cpp-lite \
    libminijail \
    libfirewalld-client
proto_header_dir := $(call local-generated-sources-dir)/proto/$(shill_parent_dir)
LOCAL_C_INCLUDES := \
    $(shill_c_includes) \
    $(proto_header_dir) \
    external/cros/system_api/
LOCAL_SRC_FILES := \
    shims/protos/crypto_util.proto \
    json_store.cc \
    active_link_monitor.cc \
    arp_client.cc \
    arp_packet.cc \
    async_connection.cc \
    certificate_file.cc \
    connection.cc \
    connection_diagnostics.cc \
    connection_health_checker.cc \
    connection_info.cc \
    connection_info_reader.cc \
    connection_tester.cc \
    connectivity_trial.cc \
    crypto_rot47.cc \
    crypto_util_proxy.cc \
    daemon_task.cc \
    dbus/chromeos_dbus_service_watcher.cc \
    dbus/chromeos_dhcpcd_listener.cc \
    dbus/chromeos_dhcpcd_proxy.cc \
    dbus/chromeos_firewalld_proxy.cc \
    default_profile.cc \
    device.cc \
    device_claimer.cc \
    device_info.cc \
    dhcp_properties.cc \
    dhcp/dhcp_config.cc \
    dhcp/dhcp_provider.cc \
    dhcp/dhcpv4_config.cc \
    dns_client.cc \
    dns_client_factory.cc \
    dns_server_proxy.cc \
    dns_server_proxy_factory.cc \
    dns_server_tester.cc \
    ephemeral_profile.cc \
    error.cc \
    ethernet/ethernet.cc \
    ethernet/ethernet_service.cc \
    ethernet/ethernet_temporary_service.cc \
    ethernet/virtio_ethernet.cc \
    event_dispatcher.cc \
    external_task.cc \
    file_io.cc \
    file_reader.cc \
    geolocation_info.cc \
    hook_table.cc \
    http_proxy.cc \
    http_request.cc \
    http_url.cc \
    icmp.cc \
    icmp_session.cc \
    icmp_session_factory.cc \
    ip_address_store.cc \
    ipconfig.cc \
    key_value_store.cc \
    link_monitor.cc \
    logging.cc \
    manager.cc \
    metrics.cc \
    passive_link_monitor.cc \
    pending_activation_store.cc \
    portal_detector.cc \
    power_manager.cc \
    power_manager_proxy_stub.cc \
    ppp_daemon.cc \
    ppp_device.cc \
    ppp_device_factory.cc \
    pppoe/pppoe_service.cc \
    process_manager.cc \
    profile.cc \
    property_store.cc \
    resolver.cc \
    result_aggregator.cc \
    routing_table.cc \
    rpc_task.cc \
    scope_logger.cc \
    scoped_umask.cc \
    service.cc \
    service_property_change_notifier.cc \
    shill_ares.cc \
    shill_config.cc \
    shill_daemon.cc \
    shill_test_config.cc \
    socket_info.cc \
    socket_info_reader.cc \
    static_ip_parameters.cc \
    store_factory.cc \
    technology.cc \
    tethering.cc \
    traffic_monitor.cc \
    upstart/upstart.cc \
    upstart/upstart_proxy_stub.cc \
    virtual_device.cc \
    vpn/vpn_driver.cc \
    vpn/vpn_provider.cc \
    vpn/vpn_service.cc
ifeq ($(SHILL_USE_BINDER), true)
LOCAL_AIDL_INCLUDES := \
    system/connectivity/shill/binder \
    frameworks/native/aidl/binder
LOCAL_SHARED_LIBRARIES += libbinder libbinderwrapper libutils libbrillo-binder
LOCAL_SRC_FILES += \
    adaptor_stub.cc \
    binder/android/system/connectivity/shill/IDevice.aidl \
    binder/android/system/connectivity/shill/IManager.aidl \
    binder/android/system/connectivity/shill/IPropertyChangedCallback.aidl \
    binder/android/system/connectivity/shill/IService.aidl \
    binder/binder_adaptor.cc \
    binder/binder_control.cc \
    binder/device_binder_adaptor.cc \
    binder/manager_binder_adaptor.cc \
    binder/service_binder_adaptor.cc \
    ipconfig_adaptor_stub.cc \
    profile_adaptor_stub.cc \
    rpc_task_adaptor_stub.cc \
    third_party_vpn_adaptor_stub.cc
else
LOCAL_SRC_FILES += \
    dbus/chromeos_dbus_adaptor.cc \
    dbus/chromeos_dbus_control.cc \
    dbus/chromeos_device_dbus_adaptor.cc \
    dbus/chromeos_ipconfig_dbus_adaptor.cc \
    dbus/chromeos_manager_dbus_adaptor.cc \
    dbus/chromeos_profile_dbus_adaptor.cc \
    dbus/chromeos_rpc_task_dbus_adaptor.cc \
    dbus/chromeos_service_dbus_adaptor.cc \
    dbus/chromeos_third_party_vpn_dbus_adaptor.cc \
    dbus/dbus_service_watcher_factory.cc \
    dbus_bindings/org.chromium.flimflam.Device.dbus-xml \
    dbus_bindings/org.chromium.flimflam.IPConfig.dbus-xml \
    dbus_bindings/org.chromium.flimflam.Manager.dbus-xml \
    dbus_bindings/org.chromium.flimflam.Profile.dbus-xml \
    dbus_bindings/org.chromium.flimflam.Service.dbus-xml \
    dbus_bindings/org.chromium.flimflam.Task.dbus-xml \
    dbus_bindings/org.chromium.flimflam.ThirdPartyVpn.dbus-xml
endif # SHILL_USE_BINDER
ifeq ($(SHILL_USE_WIFI), true)
LOCAL_SRC_FILES += \
    wifi/callback80211_metrics.cc \
    wifi/mac80211_monitor.cc \
    wifi/scan_session.cc \
    wifi/tdls_manager.cc \
    wifi/wake_on_wifi.cc \
    wifi/wifi.cc \
    wifi/wifi_endpoint.cc \
    wifi/wifi_provider.cc \
    wifi/wifi_service.cc
endif
ifeq ($(SHILL_USE_WIRED_8021X), true)
LOCAL_SRC_FILES += \
    ethernet/ethernet_eap_provider.cc \
    ethernet/ethernet_eap_service.cc
endif
ifeq ($(SHILL_USE_DHCPV6), true)
LOCAL_SRC_FILES += dhcp/dhcpv6_config.cc
endif
ifneq (,$(filter true, $(SHILL_USE_WIRED_8021X) $(SHILL_USE_WIFI)))
LOCAL_SRC_FILES += \
    dbus/chromeos_supplicant_bss_proxy.cc \
    dbus/chromeos_supplicant_interface_proxy.cc \
    dbus/chromeos_supplicant_network_proxy.cc \
    dbus/chromeos_supplicant_process_proxy.cc \
    supplicant/supplicant_eap_state_handler.cc \
    supplicant/wpa_supplicant.cc \
    eap_credentials.cc \
    eap_listener.cc
endif
ifdef BRILLO
LOCAL_SHARED_LIBRARIES += libhardware
LOCAL_C_INCLUDES += device/generic/brillo/wifi_driver_hal/include
LOCAL_SRC_FILES += wifi/wifi_driver_hal.cc
endif # BRILLO
$(eval $(shill_cpp_common))
include $(BUILD_STATIC_LIBRARY)

# shill
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := shill
LOCAL_CPPFLAGS := $(shill_cpp_flags)
LOCAL_SHARED_LIBRARIES := \
    $(shill_shared_libraries) \
    libbrillo-minijail \
    libminijail \
    libcares \
    libbrillo-dbus \
    libchrome-dbus \
    libshill-net \
    libmetrics \
    libprotobuf-cpp-lite
ifeq ($(SHILL_USE_BINDER), true)
LOCAL_SHARED_LIBRARIES += libbinder libbinderwrapper libutils libbrillo-binder
endif # SHILL_USE_BINDER
ifdef BRILLO
LOCAL_SHARED_LIBRARIES += libhardware
LOCAL_REQUIRED_MODULES := $(WIFI_DRIVER_HAL_MODULE)
endif # BRILLO
LOCAL_STATIC_LIBRARIES := libshill
LOCAL_C_INCLUDES := $(shill_c_includes)
LOCAL_SRC_FILES := shill_main.cc
LOCAL_INIT_RC := shill.rc
$(eval $(shill_cpp_common))
include $(BUILD_EXECUTABLE)

# shill_test (native test)
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := shill_test
LOCAL_MODULE_CLASS := EXECUTABLES
ifdef BRILLO
LOCAL_MODULE_TAGS := eng
endif # BRILLO
LOCAL_CPPFLAGS := $(shill_cpp_flags) -Wno-sign-compare -Wno-return-stack-address
LOCAL_SHARED_LIBRARIES := \
    $(shill_shared_libraries) \
    libshill-net \
    libminijail \
    libmetrics \
    libcares \
    libbrillo-minijail \
    libbrillo-dbus \
    libchrome-dbus \
    libprotobuf-cpp-lite
LOCAL_STATIC_LIBRARIES := libshill libgmock libchrome_test_helpers
proto_header_dir := $(call local-generated-sources-dir)/proto/$(shill_parent_dir)
LOCAL_C_INCLUDES := \
    $(shill_c_includes) \
    $(proto_header_dir) \
    external/cros/system_api/
LOCAL_SRC_FILES := \
    shims/protos/crypto_util.proto \
    active_link_monitor_unittest.cc \
    arp_client_test_helper.cc \
    arp_client_unittest.cc \
    arp_packet_unittest.cc \
    async_connection_unittest.cc \
    certificate_file_unittest.cc \
    connection_diagnostics_unittest.cc \
    connection_health_checker_unittest.cc \
    connection_info_reader_unittest.cc \
    connection_info_unittest.cc \
    connection_tester_unittest.cc \
    connection_unittest.cc \
    connectivity_trial_unittest.cc \
    crypto_rot47_unittest.cc \
    crypto_util_proxy_unittest.cc \
    daemon_task_unittest.cc \
    default_profile_unittest.cc \
    device_claimer_unittest.cc \
    device_info_unittest.cc \
    device_unittest.cc \
    dhcp/dhcp_config_unittest.cc \
    dhcp/dhcp_provider_unittest.cc \
    dhcp/dhcpv4_config_unittest.cc \
    dhcp/mock_dhcp_config.cc \
    dhcp/mock_dhcp_provider.cc \
    dhcp/mock_dhcp_proxy.cc \
    dhcp_properties_unittest.cc \
    dns_client_unittest.cc \
    dns_server_tester_unittest.cc \
    error_unittest.cc \
    ethernet/ethernet_service_unittest.cc \
    ethernet/ethernet_unittest.cc \
    ethernet/mock_ethernet.cc \
    ethernet/mock_ethernet_service.cc \
    external_task_unittest.cc \
    fake_store.cc \
    file_reader_unittest.cc \
    hook_table_unittest.cc \
    http_proxy_unittest.cc \
    http_request_unittest.cc \
    http_url_unittest.cc \
    icmp_unittest.cc \
    icmp_session_unittest.cc \
    ip_address_store_unittest.cc \
    ipconfig_unittest.cc \
    key_value_store_unittest.cc \
    link_monitor_unittest.cc \
    manager_unittest.cc \
    metrics_unittest.cc \
    mock_active_link_monitor.cc \
    mock_adaptors.cc \
    mock_ares.cc \
    mock_arp_client.cc \
    mock_async_connection.cc \
    mock_certificate_file.cc \
    mock_connection.cc \
    mock_connection_health_checker.cc \
    mock_connection_info_reader.cc \
    mock_connectivity_trial.cc \
    mock_control.cc \
    mock_crypto_util_proxy.cc \
    mock_device.cc \
    mock_device_claimer.cc \
    mock_device_info.cc \
    mock_dhcp_properties.cc \
    mock_dns_client.cc \
    mock_dns_client_factory.cc \
    mock_dns_server_proxy.cc \
    mock_dns_server_proxy_factory.cc \
    mock_dns_server_tester.cc \
    mock_event_dispatcher.cc \
    mock_external_task.cc \
    mock_http_request.cc \
    mock_icmp.cc \
    mock_icmp_session.cc \
    mock_icmp_session_factory.cc \
    mock_ip_address_store.cc \
    mock_ipconfig.cc \
    mock_link_monitor.cc \
    mock_log.cc \
    mock_log_unittest.cc \
    mock_manager.cc \
    mock_metrics.cc \
    mock_passive_link_monitor.cc \
    mock_pending_activation_store.cc \
    mock_portal_detector.cc \
    mock_power_manager.cc \
    mock_power_manager_proxy.cc \
    mock_ppp_device.cc \
    mock_ppp_device_factory.cc \
    mock_process_manager.cc \
    mock_profile.cc \
    mock_property_store.cc \
    mock_resolver.cc \
    mock_routing_table.cc \
    mock_service.cc \
    mock_socket_info_reader.cc \
    mock_store.cc \
    mock_traffic_monitor.cc \
    mock_virtual_device.cc \
    net/attribute_list_unittest.cc \
    net/byte_string_unittest.cc \
    net/event_history_unittest.cc \
    net/ip_address_unittest.cc \
    net/netlink_attribute_unittest.cc \
    net/rtnl_handler_unittest.cc \
    net/rtnl_listener_unittest.cc \
    net/rtnl_message_unittest.cc \
    net/shill_time_unittest.cc \
    nice_mock_control.cc \
    passive_link_monitor_unittest.cc \
    pending_activation_store_unittest.cc \
    portal_detector_unittest.cc \
    power_manager_unittest.cc \
    ppp_daemon_unittest.cc \
    ppp_device_unittest.cc \
    pppoe/pppoe_service_unittest.cc \
    process_manager_unittest.cc \
    profile_unittest.cc \
    property_accessor_unittest.cc \
    property_observer_unittest.cc \
    property_store_unittest.cc \
    resolver_unittest.cc \
    result_aggregator_unittest.cc \
    routing_table_unittest.cc \
    rpc_task_unittest.cc \
    scope_logger_unittest.cc \
    service_property_change_test.cc \
    service_under_test.cc \
    service_unittest.cc \
    socket_info_reader_unittest.cc \
    socket_info_unittest.cc \
    static_ip_parameters_unittest.cc \
    technology_unittest.cc \
    testrunner.cc \
    traffic_monitor_unittest.cc \
    upstart/mock_upstart.cc \
    upstart/mock_upstart_proxy.cc \
    upstart/upstart_unittest.cc \
    virtual_device_unittest.cc \
    vpn/mock_vpn_provider.cc \
    json_store_unittest.cc
ifeq ($(SHILL_USE_BINDER), true)
LOCAL_SHARED_LIBRARIES += libbinder libbinderwrapper libutils libbrillo-binder
else
LOCAL_STATIC_LIBRARIES += libchrome_dbus_test_helpers
LOCAL_SRC_FILES += \
    dbus/chromeos_dbus_adaptor_unittest.cc \
    dbus/chromeos_manager_dbus_adaptor_unittest.cc
endif # SHILL_USE_BINDER
ifeq ($(SHILL_USE_WIFI), true)
LOCAL_SRC_FILES += \
    net/netlink_manager_unittest.cc \
    net/netlink_message_unittest.cc \
    net/netlink_packet_unittest.cc \
    net/netlink_socket_unittest.cc \
    net/nl80211_attribute_unittest.cc \
    supplicant/mock_supplicant_bss_proxy.cc \
    wifi/callback80211_metrics_unittest.cc \
    wifi/mac80211_monitor_unittest.cc \
    wifi/mock_mac80211_monitor.cc \
    wifi/mock_scan_session.cc \
    wifi/mock_tdls_manager.cc \
    wifi/mock_wake_on_wifi.cc \
    wifi/mock_wifi.cc \
    wifi/mock_wifi_provider.cc \
    wifi/mock_wifi_service.cc \
    wifi/scan_session_unittest.cc \
    wifi/tdls_manager_unittest.cc \
    wifi/wake_on_wifi_unittest.cc \
    wifi/wifi_endpoint_unittest.cc \
    wifi/wifi_provider_unittest.cc \
    wifi/wifi_service_unittest.cc \
    wifi/wifi_unittest.cc
endif
ifeq ($(SHILL_USE_DHCPV6), true)
LOCAL_SRC_FILES += dhcp/dhcpv6_config_unittest.cc
endif
ifeq ($(SHILL_USE_WIRED_8021X), true)
LOCAL_SRC_FILES += \
    ethernet/ethernet_eap_provider_unittest.cc \
    ethernet/ethernet_eap_service_unittest.cc \
    ethernet/mock_ethernet_eap_provider.cc
endif
ifneq (,$(filter true, $(SHILL_USE_WIRED_8021X) $(SHILL_USE_WIFI)))
LOCAL_SRC_FILES += \
    supplicant/mock_supplicant_eap_state_handler.cc \
    supplicant/mock_supplicant_interface_proxy.cc \
    supplicant/mock_supplicant_network_proxy.cc \
    supplicant/mock_supplicant_process_proxy.cc \
    supplicant/supplicant_eap_state_handler_unittest.cc \
    supplicant/wpa_supplicant_unittest.cc \
    eap_credentials_unittest.cc \
    eap_listener_unittest.cc \
    mock_eap_credentials.cc \
    mock_eap_listener.cc
endif
ifdef BRILLO
LOCAL_SHARED_LIBRARIES += libhardware
endif # BRILLO
$(eval $(shill_cpp_common))
include $(BUILD_NATIVE_TEST)

# helper scripts
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := eng tests
LOCAL_PREBUILT_EXECUTABLES := \
    bin/wpa_debug \
    bin/ff_debug
include $(BUILD_MULTI_PREBUILT)

# The following  two targets use the shill D-Bus API, which we do not expose
# if we are using Binder.
ifneq ($(SHILL_USE_BINDER), true)

# setup_wifi
# ========================================================
include $(CLEAR_VARS)
# The module name can't be the same of a directory in the source code.
LOCAL_MODULE := shill_setup_wifi
LOCAL_CPPFLAGS := $(shill_cpp_flags)
LOCAL_SHARED_LIBRARIES := \
    $(shill_shared_libraries) \
    libshill-client \
    libbrillo-dbus \
    libchrome-dbus
LOCAL_C_INCLUDES := $(shill_c_includes)
LOCAL_SRC_FILES := setup_wifi/main.cc
$(eval $(shill_cpp_common))
include $(BUILD_EXECUTABLE)

# test-rpc-proxy
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := shill-test-rpc-proxy
LOCAL_MODULE_TAGS := eng tests
LOCAL_CPPFLAGS := $(shill_cpp_flags)
LOCAL_SHARED_LIBRARIES := \
    $(shill_shared_libraries) \
    libshill-client \
    libbrillo-dbus \
    libchrome-dbus \
    libxmlrpc++
LOCAL_C_INCLUDES := \
    $(shill_c_includes) \
    external/cros/system_api/dbus \
    external/xmlrpcpp/src
$(eval $(shill_cpp_common))
LOCAL_SRC_FILES := $(call all-cpp-files-under,test-rpc-proxy)
include $(BUILD_EXECUTABLE)

endif # SHILL_USE_BINDER

