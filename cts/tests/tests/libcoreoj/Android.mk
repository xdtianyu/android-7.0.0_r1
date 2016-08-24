# Copyright (C) 2016 The Android Open Source Project
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

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_jdwp_test_src_files)
LOCAL_STATIC_JAVA_LIBRARIES := core-ojtests-public
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := CtsLibcoreOj
LOCAL_NO_EMMA_INSTRUMENT := true
LOCAL_NO_EMMA_COMPILE := true
LOCAL_CTS_TEST_PACKAGE := android.libcore.oj
LOCAL_CTS_TARGET_RUNTIME_ARGS := cts_jdwp_test_runtime_target := dalvikvm|\#ABI\#|
LOCAL_CTS_TESTCASE_XML_INPUT := $(LOCAL_PATH)/CtsTestPackage.xml
LOCAL_JAVA_LANGUAGE_VERSION := 1.8
# Also include the source code as part of the jar. DO NOT REMOVE.
# FIXME: build/core/java_library.mk:14: *** cts/tests/tests/libcoreoj: Target java libraries may not set LOCAL_ASSET_DIR.
#LOCAL_ASSET_DIR := libcore/ojluni/src/test
include $(BUILD_CTS_TARGET_TESTNG_PACKAGE)
