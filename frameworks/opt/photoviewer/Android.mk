# Copyright (C) 2012 The Android Open Source Project
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


# Build libphotoviewer linking non-statically against the libraries it needs.
# This is to allow the library to be loaded dynamically in a context where
# the required libraries already exist. You should only use this library
# if you're certain that you need it; see go/extradex-design for more context.
appcompat_res_dirs := appcompat/res res ../../../$(SUPPORT_LIBRARY_ROOT)/v7/appcompat/res

include $(CLEAR_VARS)
LOCAL_MODULE := libphotoviewer_appcompat_dynamic

LOCAL_JAVA_LIBRARIES := android-support-v4 \
    android-support-v7-appcompat

LOCAL_SDK_VERSION := current
LOCAL_SRC_FILES := \
     $(call all-java-files-under, src) \
     $(call all-java-files-under, appcompat/src) \
     $(call all-logtags-files-under, src)

LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(appcompat_res_dirs))
LOCAL_AAPT_FLAGS := --auto-add-overlay

include $(BUILD_STATIC_JAVA_LIBRARY)

# Dynamic version of non-appcompat library
# You should only use this library if you're certain that you need it; see
# go/extradex-design for more context.
include $(CLEAR_VARS)

activity_res_dirs := activity/res res
LOCAL_MODULE := libphotoviewer_dynamic

LOCAL_JAVA_LIBRARIES := android-support-v4

LOCAL_SDK_VERSION := current
LOCAL_SRC_FILES := \
     $(call all-java-files-under, src) \
     $(call all-java-files-under, activity/src) \
     $(call all-logtags-files-under, src)

LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(activity_res_dirs))
LOCAL_AAPT_FLAGS := --auto-add-overlay

include $(BUILD_STATIC_JAVA_LIBRARY)


# Build the regular static libraries based on the above.
include $(CLEAR_VARS)

activity_res_dirs := activity/res res
LOCAL_MODULE := libphotoviewer_appcompat

LOCAL_STATIC_JAVA_LIBRARIES := libphotoviewer_appcompat_dynamic \
    android-support-v4 android-support-v7-appcompat

LOCAL_SDK_VERSION := current
LOCAL_SOURCE_FILES_ALL_GENERATED := true

LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(appcompat_res_dirs))
LOCAL_AAPT_FLAGS := --auto-add-overlay

include $(BUILD_STATIC_JAVA_LIBRARY)


include $(CLEAR_VARS)

activity_res_dirs := activity/res res
LOCAL_MODULE := libphotoviewer

LOCAL_STATIC_JAVA_LIBRARIES := libphotoviewer_dynamic android-support-v4

LOCAL_SDK_VERSION := current
LOCAL_SOURCE_FILES_ALL_GENERATED := true

LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(activity_res_dirs))
LOCAL_AAPT_FLAGS := --auto-add-overlay

include $(BUILD_STATIC_JAVA_LIBRARY)



##################################################
# Build all sub-directories

include $(call all-makefiles-under,$(LOCAL_PATH))
