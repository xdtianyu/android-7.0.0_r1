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

LOCAL_PATH := $(call my-dir)

#
# Subprojects with separate makefiles
#

subdirs := currysrc
subdir_makefiles := $(call all-named-subdir-makefiles,$(subdirs))

# build the android_icu4j srcgen jar
# ============================================================

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE := android_icu4j_srcgen
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_JAVA_LIBRARIES := currysrc
LOCAL_JAVA_RESOURCE_FILES := $(LOCAL_PATH)/resources/replacements.txt
LOCAL_SRC_FILES := $(call all-java-files-under, src/)
include $(BUILD_HOST_JAVA_LIBRARY)

include $(subdir_makefiles)
