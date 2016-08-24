# Copyright (C) 2014 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

###
### netd service AIDL interface.
###
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall -Werror
LOCAL_CLANG := true
LOCAL_MODULE := libnetdaidl
LOCAL_SHARED_LIBRARIES := \
        libbinder \
        libutils
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/binder
LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/binder
LOCAL_C_INCLUDES := $(LOCAL_PATH)/binder
LOCAL_SRC_FILES := \
        binder/android/net/INetd.aidl \
        binder/android/net/UidRange.cpp

include $(BUILD_SHARED_LIBRARY)

###
### netd daemon.
###
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
        $(call include-path-for, libhardware_legacy)/hardware_legacy \
        bionic/libc/dns/include \
        external/mdnsresponder/mDNSShared \
        system/netd/include \

LOCAL_CLANG := true
LOCAL_CPPFLAGS := -std=c++11 -Wall -Werror
LOCAL_MODULE := netd

LOCAL_INIT_RC := netd.rc

LOCAL_SHARED_LIBRARIES := \
        libbinder \
        libcrypto \
        libcutils \
        libdl \
        libhardware_legacy \
        liblog \
        liblogwrap \
        libmdnssd \
        libnetdaidl \
        libnetutils \
        libnl \
        libsysutils \
        libbase \
        libutils \

LOCAL_STATIC_LIBRARIES := \
        libpcap \

LOCAL_SRC_FILES := \
        BandwidthController.cpp \
        ClatdController.cpp \
        CommandListener.cpp \
        Controllers.cpp \
        DnsProxyListener.cpp \
        DummyNetwork.cpp \
        DumpWriter.cpp \
        FirewallController.cpp \
        FwmarkServer.cpp \
        IdletimerController.cpp \
        InterfaceController.cpp \
        LocalNetwork.cpp \
        MDnsSdListener.cpp \
        NatController.cpp \
        NetdCommand.cpp \
        NetdConstants.cpp \
        NetdNativeService.cpp \
        NetlinkHandler.cpp \
        NetlinkManager.cpp \
        Network.cpp \
        NetworkController.cpp \
        PhysicalNetwork.cpp \
        PppController.cpp \
        ResolverController.cpp \
        RouteController.cpp \
        SockDiag.cpp \
        SoftapController.cpp \
        StrictController.cpp \
        TetherController.cpp \
        UidRanges.cpp \
        VirtualNetwork.cpp \
        main.cpp \
        oem_iptables_hook.cpp \
        binder/android/net/metrics/IDnsEventListener.aidl \

LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/binder

include $(BUILD_EXECUTABLE)


###
### ndc binary.
###
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall -Werror
LOCAL_CLANG := true
LOCAL_MODULE := ndc
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_SRC_FILES := ndc.c

include $(BUILD_EXECUTABLE)

###
### netd unit tests.
###
include $(CLEAR_VARS)
LOCAL_MODULE := netd_unit_test
LOCAL_CFLAGS := -Wall -Werror -Wunused-parameter
LOCAL_C_INCLUDES := system/netd/server system/netd/server/binder system/core/logwrapper/include
LOCAL_SRC_FILES := \
        NetdConstants.cpp IptablesBaseTest.cpp \
        BandwidthController.cpp BandwidthControllerTest.cpp \
        FirewallControllerTest.cpp FirewallController.cpp \
        SockDiagTest.cpp SockDiag.cpp \
        StrictController.cpp StrictControllerTest.cpp \
        UidRanges.cpp \

LOCAL_MODULE_TAGS := tests
LOCAL_SHARED_LIBRARIES := liblog libbase libcutils liblogwrap
include $(BUILD_NATIVE_TEST)

