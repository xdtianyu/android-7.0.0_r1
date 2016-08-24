# Copyright (C) 2009 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

# include definition of core-junit-files
include $(LOCAL_PATH)/Common.mk

# note: ideally this should be junit-host, but leave as is for now to avoid
# changing all its dependencies
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_MODULE := junit
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_JAVA_LIBRARIES := hamcrest-host
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/Common.mk
include $(BUILD_HOST_JAVA_LIBRARY)

# ----------------------------------
# build a junit-targetdex jar

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under, src/junit/extensions)
LOCAL_SRC_FILES += $(core-junit-files)
LOCAL_SRC_FILES += $(junit-runner-files)
# TODO: lose the suffix here and rename "junit" to "junit-hostdex"
LOCAL_MODULE := junit-targetdex
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_LIBRARIES := core-oj core-libart
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/junit
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/Common.mk
include $(BUILD_JAVA_LIBRARY)

# ----------------------------------
# build a junit-hostdex jar

ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under, src/junit/extensions)
LOCAL_SRC_FILES += $(core-junit-files)
LOCAL_SRC_FILES += $(junit-runner-files)
LOCAL_MODULE := junit-hostdex
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/Common.mk
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)
endif # HOST_OS == linux

# ----------------------------------
# build a core-junit target jar that is built into Android system image

# TODO: remove extensions once core-tests is no longer dependent on it
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under, src/junit/extensions)
LOCAL_SRC_FILES += $(core-junit-files)
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_LIBRARIES := core-oj core-libart
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := core-junit
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/Common.mk
include $(BUILD_JAVA_LIBRARY)

# ----------------------------------
# build a core-junit-hostdex jar that contains exactly the same classes
# as core-junit.

ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
# TODO: remove extensions once apache-harmony/luni/ is no longer dependent
# on it
LOCAL_SRC_FILES := $(call all-java-files-under, src/junit/extensions)
LOCAL_SRC_FILES += $(core-junit-files)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := core-junit-hostdex
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/Common.mk
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)
endif # HOST_OS == linux

#-------------------------------------------------------
# build a junit-runner jar for the host JVM
# (like the junit classes in the frameworks/base android.test.runner.jar)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(junit-runner-files)
LOCAL_MODULE := junit-runner
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_LIBRARIES := core-oj core-libart core-junit
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/Common.mk
include $(BUILD_STATIC_JAVA_LIBRARY)

#-------------------------------------------------------
# build a junit-runner for the host dalvikvm
# (like the junit classes in the frameworks/base android.test.runner.jar)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(junit-runner-files)
LOCAL_MODULE := junit-runner-hostdex
LOCAL_MODULE_TAGS := optional
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_LIBRARIES := core-oj-hostdex core-libart-hostdex core-junit-hostdex
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/Common.mk
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)

#-------------------------------------------------------
# build a junit4-target jar representing the
# classes in external/junit that are not in the core public API 4
# Note: 'core' here means excluding the classes that are contained
# in the optional library android.test.runner. Developers who
# build against this jar shouldn't have to also include android.test.runner

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(junit4-target-src)
LOCAL_MODULE := junit4-target
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 4
LOCAL_STATIC_JAVA_LIBRARIES := hamcrest
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/Common.mk
include $(BUILD_STATIC_JAVA_LIBRARY)

#-------------------------------------------------------
# Same as above, but does not statically link in dependencies

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(junit4-target-src)
LOCAL_MODULE := junit4-target-nodeps
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := 4
LOCAL_JAVA_LIBRARIES := hamcrest
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/Common.mk
include $(BUILD_STATIC_JAVA_LIBRARY)

#-------------------------------------------------------
# Same as above, but for host dalvik. However, since we don't have
# the SDK to provide the junit.framework.* classes, we must add
# an extra library.

ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(junit4-target-src)
LOCAL_MODULE := junit4-target-hostdex
LOCAL_MODULE_TAGS := optional
LOCAL_JAVA_LIBRARIES := core-junit-hostdex
LOCAL_STATIC_JAVA_LIBRARIES := hamcrest-hostdex
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/Common.mk
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)
endif # HOST_OS == linux
