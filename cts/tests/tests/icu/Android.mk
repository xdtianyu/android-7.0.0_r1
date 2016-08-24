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

include $(CLEAR_VARS)

# Don't include this package in any target
LOCAL_MODULE_TAGS := tests
# When built explicitly put it in the data partition
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_DEX_PREOPT := false

LOCAL_PROGUARD_ENABLED := disabled

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_JAVA_RESOURCE_DIRS := resources

# The aim of this package is to run tests against the implementation in use by
# the current android system.
LOCAL_STATIC_JAVA_LIBRARIES := \
	cts-core-test-runner \
	android-icu4j-tests

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts

LOCAL_PACKAGE_NAME := CtsIcuTestCases

LOCAL_SDK_VERSION := current

include $(BUILD_CTS_SUPPORT_PACKAGE)

# Version 1 of the CTS framework has it's own logic for generating XML files based on scanning the
# source for test methods and classes written using JUnit 3 (doesn't work for JUnit 4 @RunWith
# tests). Since the ICU tests are not written using JUnit (although they are run with a custom JUnit
# RunnerBuilder) this provides an alternative. This generates an XML representation based off a
# list of the tests that are run by version 2 of the CTS framework (which doesn't require the list
# in advance). The tools/update-test-list.sh script will take a host_log_[0-9]+.zip created by
# CTSv1 and extract the list of tests run and update the test-list.txt file.

CTS_ICU_TEST_LIST_PATH := $(LOCAL_PATH)/test-list.txt
cts_package_xml := $(CTS_TESTCASES_OUT)/CtsIcuTestCases.xml
$(cts_package_xml): $(HOST_OUT_JAVA_LIBRARIES)/cts-icu-tools.jar $(CTS_ICU_TEST_LIST_PATH) \
	$(call intermediates-dir-for,APPS,$(LOCAL_PACKAGE_NAME))/package.apk
	java -Xmx256M -classpath $(HOST_OUT_JAVA_LIBRARIES)/cts-icu-tools.jar \
		android.icu.cts.tools.GenerateTestCaseXML \
		$(CTS_ICU_TEST_LIST_PATH) \
		$(TARGET_ARCH) \
		$@

# build cts-icu-tools tool
# ============================================================
include $(CLEAR_VARS)

# Don't include this package in any target
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, tools)
LOCAL_JAVA_RESOURCE_DIRS := resources

LOCAL_STATIC_JAVA_LIBRARIES := \
	descGen \
	jsr305lib

LOCAL_MODULE := cts-icu-tools

include $(BUILD_HOST_JAVA_LIBRARY)
