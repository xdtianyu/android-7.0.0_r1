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

LOCAL_PATH:= $(call my-dir)

# Make mock HAL library
# ============================================================

include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES :=

LOCAL_CFLAGS += -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-function \
                -Wunused-variable -Winit-self -Wwrite-strings -Wshadow

LOCAL_C_INCLUDES += \
	$(JNI_H_INCLUDE) \
	$(LOCAL_PATH)/../../service/jni \
	$(call include-path-for, libhardware)/hardware \
	$(call include-path-for, libhardware_legacy)/hardware_legacy \
	packages/apps/Test/connectivity/sl4n/rapidjson/include \
	libcore/include

LOCAL_SRC_FILES := \
	jni/wifi_hal_mock.cpp

ifdef INCLUDE_NAN_FEATURE
LOCAL_SRC_FILES += \
	jni/wifi_nan_hal_mock.cpp
endif

LOCAL_MODULE := libwifi-hal-mock

LOCAL_STATIC_LIBRARIES += libwifi-hal
LOCAL_SHARED_LIBRARIES += \
	libnativehelper \
	libcutils \
	libutils \
	libhardware \
	libhardware_legacy \
	libnl \
	libdl \
	libwifi-service

include $(BUILD_SHARED_LIBRARY)

# Make test APK
# ============================================================
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

RESOURCE_FILES := $(call all-named-files-under, R.java, $(intermediates.COMMON))

LOCAL_SRC_FILES := $(call all-subdir-java-files) \
	$RESOURCE_FILES

ifndef INCLUDE_NAN_FEATURE
LOCAL_SRC_FILES := $(filter-out $(call all-java-files-under, \
          src/com/android/server/wifi/nan),$(LOCAL_SRC_FILES))
endif

# Provide jack a list of classes to exclude form code coverage
# This list is generated from the java source files in this module
# The list is a comma separated list of class names with * matching zero or more characters.
# Example:
#   Input files: src/com/android/server/wifi/Test.java src/com/android/server/wifi/AnotherTest.java
#   Generated exclude list: com.android.server.wifi.Test*,com.android.server.wifi.AnotherTest*

# Filter all src files to just java files
local_java_files := $(filter %.java,$(LOCAL_SRC_FILES))
# Transform java file names into full class names.
# This only works if the class name matches the file name and the directory structure
# matches the package.
local_classes := $(subst /,.,$(patsubst src/%.java,%,$(local_java_files)))
# Utility variables to allow replacing a space with a comma
comma:= ,
empty:=
space:= $(empty) $(empty)
# Convert class name list to jacoco exclude list
# This appends a * to all classes and replace the space separators with commas.
# These patterns will match all classes in this module and their inner classes.
jacoco_exclude := $(subst $(space),$(comma),$(patsubst %,%*,$(local_classes)))

jacoco_include := com.android.server.wifi.*,android.net.wifi.*

LOCAL_JACK_COVERAGE_INCLUDE_FILTER := $(jacoco_include)
LOCAL_JACK_COVERAGE_EXCLUDE_FILTER := $(jacoco_exclude)

# wifi-service and services must be included here so that the latest changes
# will be used when tests. Otherwise the tests would run against the installed
# system.
# TODO figure out if this is the correct thing to do, this seems to not be right
# since neither is declared a static java library.
LOCAL_STATIC_JAVA_LIBRARIES := \
	android-support-test \
	mockito-target \
	services \
	wifi-service \

LOCAL_JAVA_LIBRARIES := \
	android.test.runner \
	wifi-service \
	services \

# These must be explicitly included because they are not normally accessible
# from apps.
LOCAL_JNI_SHARED_LIBRARIES := \
	libwifi-service \
	libc++ \
	libLLVM \
	libutils \
	libunwind \
	libhardware_legacy \
	libbase \
	libhardware \
	libnl \
	libcutils \
	libnetutils \
	libbacktrace \
	libnativehelper \
	liblzma \

ifdef WPA_SUPPLICANT_VERSION
LOCAL_JNI_SHARED_LIBRARIES += libwpa_client
endif

LOCAL_PACKAGE_NAME := FrameworksWifiTests
LOCAL_JNI_SHARED_LIBRARIES += libwifi-hal-mock

include $(BUILD_PACKAGE)
