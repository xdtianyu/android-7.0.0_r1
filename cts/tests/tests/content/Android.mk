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

include $(CLEAR_VARS)

# don't include this package in any target
LOCAL_MODULE_TAGS := optional
# and when built explicitly put it in the data partition
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_APPS)

LOCAL_JAVA_LIBRARIES := android.test.runner

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v4 ctsdeviceutil ctstestrunner services.core

# Resource unit tests use a private locale and some densities
LOCAL_AAPT_FLAGS = -c xx_YY -c cs -c small -c normal -c large -c xlarge \
        -c 320dpi -c 240dpi -c 160dpi -c 32dpi \
        -c kok,kok_IN,kok_419,kok_419_VARIANT,kok_Knda_419,kok_Knda_419_VARIANT,kok_VARIANT,kok_Knda,tgl,tgl_PH

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := CtsContentTestCases

# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts

include $(BUILD_CTS_PACKAGE)
