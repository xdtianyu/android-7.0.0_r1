# Copyright (C) 2008 The Android Open Source Project
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

# Reusable Sensor test classes and helpers

include $(CLEAR_VARS)

LOCAL_MODULE := cts-sensors-tests

LOCAL_MODULE_TAGS := tests

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_STATIC_JAVA_LIBRARIES := ctsdeviceutil

LOCAL_JAVA_LIBRARIES := platform-test-annotations

LOCAL_SDK_VERSION := current

# TODO: sensors need to be refactored out into their own namespace: android.hardware.sensors.cts
LOCAL_SRC_FILES := $(call all-java-files-under, src/android/hardware/cts/helpers)
LOCAL_SRC_FILES += \
    src/android/hardware/cts/SensorTestCase.java \
    src/android/hardware/cts/SingleSensorTests.java \
    src/android/hardware/cts/SensorIntegrationTests.java \
    src/android/hardware/cts/SensorBatchingTests.java \
    src/android/hardware/cts/SensorTest.java \
    src/android/hardware/cts/SensorManagerStaticTest.java \
    src/android/hardware/cts/SensorAdditionalInfoTest.java

include $(BUILD_STATIC_JAVA_LIBRARY)


# CtsHardwareTestCases package

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts

LOCAL_STATIC_JAVA_LIBRARIES := \
    ctsdeviceutil \
    compatibility-device-util \
    ctstestrunner \
    mockito-target \
    android-ex-camera2

LOCAL_SRC_FILES := $(call all-java-files-under, src) $(call all-renderscript-files-under, src)

LOCAL_PACKAGE_NAME := CtsHardwareTestCases

LOCAL_CTS_MODULE_CONFIG := $(LOCAL_PATH)/Old$(CTS_MODULE_TEST_CONFIG)

LOCAL_SDK_VERSION := current

LOCAL_JAVA_LIBRARIES := android.test.runner

include $(BUILD_CTS_PACKAGE)
