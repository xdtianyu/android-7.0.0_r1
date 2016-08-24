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

#
# Build support for jsr330 within the Android Open Source Project
# See https://source.android.com/source/building.html for more information
#

LOCAL_PATH := $(call my-dir)

jsr330_src_files := $(call all-java-files-under, src)

# Target build
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(jsr330_src_files)
LOCAL_MODULE := jsr330
include $(BUILD_STATIC_JAVA_LIBRARY)

# Host-side Java build
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(jsr330_src_files)
LOCAL_MODULE := jsr330-host
include $(BUILD_HOST_JAVA_LIBRARY)

# Host-side Dalvik build
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(jsr330_src_files)
LOCAL_MODULE := jsr330-hostdex
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)

#
# TCK (Test Compatibility Kit)
# -- For DI frameworks that want to test compatibility with javax.
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under, tck)
LOCAL_MODULE := jsr330-tck-host
LOCAL_JAVA_LIBRARIES := jsr330-host junit
include $(BUILD_HOST_JAVA_LIBRARY)
