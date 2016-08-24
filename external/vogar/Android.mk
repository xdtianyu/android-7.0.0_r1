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

# build vogar jar
# ============================================================

include $(CLEAR_VARS)

LOCAL_MODULE := vogar
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_SRC_FILES := $(call all-java-files-under, src/)
LOCAL_JAVA_RESOURCE_DIRS := resources

LOCAL_STATIC_JAVA_LIBRARIES := \
  caliper-host \
  caliper-gson-host \
  guavalib \
  mockito-host \
  vogar-jsr305 \
  vogar-kxml-libcore-20110123

# Vogar uses android.jar.
LOCAL_CLASSPATH := prebuilts/sdk/9/android.jar
LOCAL_JAVA_LANGUAGE_VERSION := 1.7

include $(BUILD_HOST_JAVA_LIBRARY)

# Build dependencies.
# ============================================================
include $(CLEAR_VARS)

LOCAL_PREBUILT_JAVA_LIBRARIES := \
    vogar-jsr305:lib/jsr305$(COMMON_JAVA_PACKAGE_SUFFIX) \
    vogar-kxml-libcore-20110123:lib/kxml-libcore-20110123$(COMMON_JAVA_PACKAGE_SUFFIX)

include $(BUILD_HOST_PREBUILT)

# copy vogar script
# ============================================================
include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE := vogar
LOCAL_SRC_FILES := bin/vogar-android
include $(BUILD_PREBUILT)

