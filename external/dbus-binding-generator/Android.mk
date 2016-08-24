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

libdbusBindingGenCFlags := -Wall -Werror

include $(CLEAR_VARS)
LOCAL_MODULE := libdbus-binding-gen-host
LOCAL_CFLAGS := $(libdbusBindingGenCFlags)
LOCAL_CPP_EXTENSION := .cc
# Hack these includes, because we're not actually linking the libraries.
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(TOP)/external/dbus \
    $(TOP)/external/gtest/include
LOCAL_SHARED_LIBRARIES := libbrillo libchrome
LOCAL_STATIC_LIBRARIES := libexpat
LOCAL_SRC_FILES := \
    chromeos-dbus-bindings/adaptor_generator.cc \
    chromeos-dbus-bindings/dbus_signature.cc \
    chromeos-dbus-bindings/header_generator.cc \
    chromeos-dbus-bindings/indented_text.cc \
    chromeos-dbus-bindings/method_name_generator.cc \
    chromeos-dbus-bindings/name_parser.cc \
    chromeos-dbus-bindings/proxy_generator.cc \
    chromeos-dbus-bindings/xml_interface_parser.cc
include $(BUILD_HOST_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := dbus-binding-generator
LOCAL_CFLAGS := $(libdbusBindingGenCFlags)
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := $(LOCAL_PATH) $(TOP)/external/gtest/include
LOCAL_SHARED_LIBRARIES := libbrillo libchrome
LOCAL_SRC_FILES := chromeos-dbus-bindings/generate_chromeos_dbus_bindings.cc
LOCAL_STATIC_LIBRARIES := libdbus-binding-gen-host libexpat
include $(BUILD_HOST_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE := dbus-binding-generator-tests
LOCAL_CFLAGS := $(libdbusBindingGenCFlags)
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(TOP)/external/dbus
LOCAL_SHARED_LIBRARIES := libbrillo libchrome
LOCAL_STATIC_LIBRARIES := libdbus-binding-gen-host libgmock_host libexpat
LOCAL_SRC_FILES := \
    chromeos-dbus-bindings/adaptor_generator_unittest.cc \
    chromeos-dbus-bindings/dbus_signature_unittest.cc \
    chromeos-dbus-bindings/indented_text_unittest.cc \
    chromeos-dbus-bindings/method_name_generator_unittest.cc \
    chromeos-dbus-bindings/name_parser_unittest.cc \
    chromeos-dbus-bindings/proxy_generator_mock_unittest.cc \
    chromeos-dbus-bindings/proxy_generator_unittest.cc \
    chromeos-dbus-bindings/test_utils.cc \
    chromeos-dbus-bindings/testrunner.cc \
    chromeos-dbus-bindings/xml_interface_parser_unittest.cc
include $(BUILD_HOST_NATIVE_TEST)
