#
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
#

LOCAL_PATH := $(call my-dir)

#==========================================================
# build repackaged ICU for target
#
# This is done in the libcore/JavaLibraries.mk file as there are circular
# dependencies between ICU and libcore
#==========================================================

#==========================================================
# build repackaged ICU tests for target
#
# Builds against core-libart and core-oj so that it can access all the
# repackaged android.icu classes and methods and not just the ones available
# through the Android API.
#==========================================================
include $(CLEAR_VARS)

# Don't include this package in any target
LOCAL_MODULE_TAGS := tests
LOCAL_SRC_FILES := \
    $(call all-java-files-under,src/main/tests) \
    $(call all-java-files-under,cts-coverage/src/main/tests) \
    $(call all-java-files-under,runner/src/main/java)
LOCAL_JAVA_RESOURCE_DIRS := src/main/tests runner/src/main/java
LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-test
LOCAL_JAVA_LIBRARIES := \
        core-oj \
        core-libart
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_MODULE := android-icu4j-tests
include $(BUILD_STATIC_JAVA_LIBRARY)

#==========================================================
# build repackaged ICU for host for testing purposes
#
# Uses the repackaged versions of the data jars
#==========================================================
include $(CLEAR_VARS)

# Don't include this package in any target
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under,src/main/java)
LOCAL_STATIC_JAVA_LIBRARIES := \
    icu4j-icudata-host-jarjar \
    icu4j-icutzdata-host-jarjar
LOCAL_JAVA_RESOURCE_DIRS := resources
LOCAL_MODULE := android-icu4j-host
include $(BUILD_HOST_JAVA_LIBRARY)

#==========================================================
# build repackaged ICU tests for host for testing purposes
#
# Run the tests using junit with the following command:
#   java -cp ${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/android-icu4j-tests-host.jar org.junit.runner.JUnitCore android.icu.dev.test.TestAll
#
# Run the tests using the ICU4J test framework with the following command:
#   java -cp ${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/android-icu4j-tests-host.jar android.icu.dev.test.TestAll
#==========================================================
include $(CLEAR_VARS)

# Don't include this package in any target
LOCAL_MODULE_TAGS := tests
LOCAL_SRC_FILES := \
    $(call all-java-files-under,src/main/tests) \
    $(call all-java-files-under,runner/src/main/java) \
    $(call all-java-files-under,cts-coverage/src/main/tests) \
    $(call all-java-files-under,runner/src/host/java)
LOCAL_JAVA_RESOURCE_DIRS := src/main/tests
LOCAL_STATIC_JAVA_LIBRARIES := \
    android-icu4j-host \
    junit
LOCAL_MODULE := android-icu4j-tests-host
include $(BUILD_HOST_JAVA_LIBRARY)
