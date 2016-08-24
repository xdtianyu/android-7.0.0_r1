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
# Build support for jcommander within the Android Open Source Project
# See https://source.android.com/source/building.html for more information
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Host JAR builds have every single file because all the standard APIs are available.
jcommander_all_src_files := $(call all-java-files-under, src/main)
# Filter out PathConverter since android is missing java.nio.file APIs.
jcommander_android_src_files := $(filter-out %/PathConverter.java,$(jcommander_all_src_files))

# Target Java build
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(jcommander_android_src_files)
LOCAL_MODULE := jcommander
LOCAL_MODULE_TAGS := optional
include $(BUILD_STATIC_JAVA_LIBRARY)

# Host Java build
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(jcommander_all_src_files)
LOCAL_MODULE := jcommander-host
LOCAL_MODULE_TAGS := optional
include $(BUILD_HOST_JAVA_LIBRARY)

# Host dalvik build
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(jcommander_android_src_files)
LOCAL_MODULE := jcommander-hostdex
LOCAL_MODULE_TAGS := optional
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)

# TODO: also add the tests once we have testng working.
