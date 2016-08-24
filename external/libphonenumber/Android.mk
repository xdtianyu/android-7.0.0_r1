#
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

# For the platform, compile everything except the carrier to phone number
# which isn't used.
libphonenumber_platform_resource_dirs := \
    libphonenumber/src \
    geocoder/src \
    internal/prefixmapper/src

libphonenumber_platform_src_files := \
    $(call all-java-files-under, libphonenumber/src) \
    $(call all-java-files-under, geocoder/src) \
    $(call all-java-files-under, internal/prefixmapper/src) \

libphonenumber_src_files := \
    $(libphonenumber_platform_src_files) \
    $(call all-java-files-under, carrier/src)

libphonenumber_resource_dirs := \
    $(libphonenumber_platform_resource_dirs) \
    carrier/src

libphonenumber_test_files := \
    $(call all-java-files-under, carrier/test) \
    $(call all-java-files-under, geocoder/test) \
    $(call all-java-files-under, internal/prefixmapper/test) \
    $(call all-java-files-under, libphonenumber/test)

libphonenumber_test_resource_dirs := \
    carrier/test \
    geocoder/test \
    libphonenumber/test

# For platform use, builds directly against core-libart to avoid circular
# dependencies. *NOT* for unbundled use.
include $(CLEAR_VARS)
LOCAL_MODULE := libphonenumber-platform
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(libphonenumber_platform_src_files)
LOCAL_JAVA_RESOURCE_DIRS := $(libphonenumber_platform_resource_dirs)
LOCAL_JARJAR_RULES := $(LOCAL_PATH)/jarjar-rules.txt
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_LIBRARIES := core-oj core-libart
LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_STATIC_JAVA_LIBRARY)

# For unbundled use, supports gingerbread and up.
include $(CLEAR_VARS)
LOCAL_MODULE := libphonenumber
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(libphonenumber_src_files)
LOCAL_JAVA_RESOURCE_DIRS := $(libphonenumber_resource_dirs)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_SDK_VERSION := 9
LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_STATIC_JAVA_LIBRARY)

# Tests for unbundled use.
# vogar --timeout 0 --classpath out/target/common/obj/JAVA_LIBRARIES/libphonenumber_intermediates/classes.jar \
#   --classpath out/target/common/obj/JAVA_LIBRARIES/libphonenumber-test_intermediates/classes.jar \
#   com.google.i18n.phonenumbers
include $(CLEAR_VARS)
LOCAL_MODULE := libphonenumber-test
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(libphonenumber_test_files)
LOCAL_JAVA_RESOURCE_DIRS := $(libphonenumber_test_resource_dirs)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_SDK_VERSION := current
LOCAL_STATIC_JAVA_LIBRARIES := libphonenumber
LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_STATIC_JAVA_LIBRARY)
