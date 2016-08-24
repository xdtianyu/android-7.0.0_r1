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

LOCAL_PATH:= $(call my-dir)

# for Android JUnit runner and rules
include $(CLEAR_VARS)
LOCAL_MODULE := android-support-test
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := rules/rules-0.5-release.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := android-support-test-nodep
LOCAL_SDK_VERSION := 23
LOCAL_STATIC_JAVA_LIBRARIES := android-support-test-rules-nodep android-support-test-runner-nodep
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := android-support-test-rules-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := rules/rules-0.5-release-no-dep.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := android-support-test-runner-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := runner/runner-0.5-release-no-dep.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

# for espresso-core
## Note: the following jar already contains android-support-test
include $(CLEAR_VARS)
LOCAL_MODULE := espresso-core
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso-core-2.2.2-release.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := espresso-core-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso-core-2.2.2-release-no-dep.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

# for espresso-contrib
## Note: the following jar already contains espresso-core
include $(CLEAR_VARS)
LOCAL_MODULE := espresso-contrib
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso-contrib-2.2.2-release.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := espresso-contrib-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso-contrib-2.2.2-release-no-dep.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

# for espresso-idling-resource
include $(CLEAR_VARS)
LOCAL_MODULE := espresso-idling-resource-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso-idling-resource-2.2.2-release-no-dep.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

# for espresso-intents
## Note: the following jar already contains espresso-core
include $(CLEAR_VARS)
LOCAL_MODULE := espresso-intents
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso-intents-2.2.2-release.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := espresso-intents-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso-intents-2.2.2-release-no-dep.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

# for espresso-web
## Note: the following jar already contains espresso-core
include $(CLEAR_VARS)
LOCAL_MODULE := espresso-web
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso-web-2.2.2-release.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := espresso-web-nodep
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := espresso/espresso-web-2.2.2-release-no-dep.jar
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_SUFFIX := $(COMMON_JAVA_PACKAGE_SUFFIX)
# Uninstallable static Java libraries.
LOCAL_UNINSTALLABLE_MODULE := true
include $(BUILD_PREBUILT)
