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

LOCAL_PATH := $(call my-dir)

# Common variables
# ========================================================

buffetCommonCppExtension := .cc
buffetCommonCFlags := -DBUFFET_USE_WIFI_BOOTSTRAPPING -Wall -Werror \
	-Wno-char-subscripts -Wno-missing-field-initializers \
	-Wno-unused-function -Wno-unused-parameter \

buffetCommonCppFlags := \
	-Wno-deprecated-register \
	-Wno-sign-compare \
	-Wno-sign-promo \
	-Wno-non-virtual-dtor \

buffetCommonCIncludes := \
	$(LOCAL_PATH)/.. \
	external/cros/system_api \
	external/gtest/include \

buffetSharedLibraries := \
	libapmanager-client \
	libavahi-common \
	libavahi-client \
	libbinder \
	libbinderwrapper \
	libbrillo \
	libbrillo-binder \
	libbrillo-dbus \
	libbrillo-http \
	libbrillo-stream \
	libchrome \
	libchrome-dbus \
	libcutils \
	libdbus \
	libnativepower \
	libshill-client \
	libutils \
	libweave \
	libwebserv \

ifdef BRILLO

buffetSharedLibraries += \
	libkeymaster_messages \
	libkeystore_binder \

endif

# weave-common
# Code shared between weaved daemon and libweaved client library
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := weave-common
LOCAL_CPP_EXTENSION := $(buffetCommonCppExtension)
LOCAL_CFLAGS := $(buffetCommonCFlags)
LOCAL_CPPFLAGS := $(buffetCommonCppFlags)
LOCAL_C_INCLUDES := $(buffetCommonCIncludes)
LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/brillo
LOCAL_SHARED_LIBRARIES := $(buffetSharedLibraries)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_CLANG := true

LOCAL_SRC_FILES := \
	brillo/android/weave/IWeaveClient.aidl \
	brillo/android/weave/IWeaveCommand.aidl \
	brillo/android/weave/IWeaveService.aidl \
	brillo/android/weave/IWeaveServiceManager.aidl \
	brillo/android/weave/IWeaveServiceManagerNotificationListener.aidl \
	common/binder_constants.cc \
	common/binder_utils.cc \

include $(BUILD_STATIC_LIBRARY)

# weave-daemon-common
# Code shared between weaved daemon and unit test runner.
# This is essentially the implementation of weaved in a static library format.
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := weave-daemon-common
LOCAL_CPP_EXTENSION := $(buffetCommonCppExtension)
LOCAL_CFLAGS := $(buffetCommonCFlags)
# TODO(avakulenko): Remove -Wno-deprecated-declarations when legacy libweave
# APIs are removed (see: b/25917708).
LOCAL_CPPFLAGS := $(buffetCommonCppFlags) -Wno-deprecated-declarations
LOCAL_C_INCLUDES := $(buffetCommonCIncludes)
LOCAL_SHARED_LIBRARIES := $(buffetSharedLibraries)
LOCAL_STATIC_LIBRARIES := weave-common
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
	brillo/weaved_system_properties.cc \
	buffet/ap_manager_client.cc \
	buffet/avahi_mdns_client.cc \
	buffet/binder_command_proxy.cc \
	buffet/binder_weave_service.cc \
	buffet/buffet_config.cc \
	buffet/dbus_constants.cc \
	buffet/flouride_socket_bluetooth_client.cc \
	buffet/http_transport_client.cc \
	buffet/manager.cc \
	buffet/shill_client.cc \
	buffet/socket_stream.cc \
	buffet/webserv_client.cc \

ifdef BRILLO
LOCAL_SRC_FILES += buffet/keystore_encryptor.cc
else
LOCAL_SRC_FILES += buffet/fake_encryptor.cc
endif

include $(BUILD_STATIC_LIBRARY)

# weaved
# The main binary of the weave daemon.
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := weaved
LOCAL_REQUIRED_MODULES := \
	avahi-daemon \
	libweaved \
	webservd \

LOCAL_CPP_EXTENSION := $(buffetCommonCppExtension)
LOCAL_CFLAGS := $(buffetCommonCFlags)
LOCAL_CPPFLAGS := $(buffetCommonCppFlags)
LOCAL_C_INCLUDES := $(buffetCommonCIncludes)
LOCAL_INIT_RC := weaved.rc
LOCAL_SHARED_LIBRARIES := $(buffetSharedLibraries)
LOCAL_STATIC_LIBRARIES := weave-common \

LOCAL_WHOLE_STATIC_LIBRARIES := weave-daemon-common
LOCAL_CLANG := true

LOCAL_SRC_FILES := \
	buffet/main.cc

include $(BUILD_EXECUTABLE)

# libweaved
# The client library for the weave daemon. You should link to libweaved,
# if you need to communicate with weaved.
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweaved
LOCAL_CPP_EXTENSION := $(buffetCommonCppExtension)
LOCAL_CFLAGS := $(buffetCommonCFlags)
LOCAL_CPPFLAGS := $(buffetCommonCppFlags)
LOCAL_C_INCLUDES := external/gtest/include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := \
	libbinder \
	libbinderwrapper \
	libbrillo \
	libchrome \
	libutils \

LOCAL_STATIC_LIBRARIES := weave-common

LOCAL_CLANG := true

LOCAL_SRC_FILES := \
	libweaved/command.cc \
	libweaved/service.cc \

include $(BUILD_SHARED_LIBRARY)

# weaved_test
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := weaved_test
LOCAL_MODULE_TAGS := eng
LOCAL_CPP_EXTENSION := $(buffetCommonCppExtension)
LOCAL_CFLAGS := $(buffetCommonCFlags)
LOCAL_CPPFLAGS := $(buffetCommonCppFlags)
LOCAL_C_INCLUDES := \
	$(buffetCommonCIncludes) \
	external/gmock/include \

LOCAL_SHARED_LIBRARIES := \
	$(buffetSharedLibraries) \

LOCAL_STATIC_LIBRARIES := \
	libbrillo-test-helpers \
	libchrome_test_helpers \
	libgtest \
	libgmock \
	libweave-test \
	weave-daemon-common \
	weave-common \

LOCAL_CLANG := true

LOCAL_SRC_FILES := \
	buffet/binder_command_proxy_unittest.cc \
	buffet/buffet_config_unittest.cc \
	buffet/buffet_testrunner.cc \

include $(BUILD_NATIVE_TEST)
