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

###########################################################
# Package w/ tests

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests
LOCAL_SDK_VERSION := current
LOCAL_STATIC_JAVA_LIBRARIES := android-support-test ctsdeviceutil ctstestrunner
# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_DEX_PREOPT := false
LOCAL_PACKAGE_NAME := CtsPrivilegedUpdateTests

LOCAL_SRC_FILES := $(call all-java-files-under, src)

include $(BUILD_CTS_SUPPORT_PACKAGE)


###########################################################
# Variant: Privileged app upgrade

include $(CLEAR_VARS)

LOCAL_MODULE := CtsShimPrivUpgradePrebuilt
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE_CLASS := APPS
LOCAL_BUILT_MODULE_STEM := package.apk
# Make sure the build system doesn't try to resign the APK
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_COMPATIBILITY_SUITE := cts

LOCAL_SRC_FILES := CtsShimPrivUpgrade.apk

include $(BUILD_PREBUILT)

# Add package to the set of APKs available to CTS
# Unceremoneously ripped from cts/build/support_package.mk
cts_support_apks :=
$(foreach fp, $(ALL_MODULES.$(LOCAL_MODULE).BUILT_INSTALLED),\
  $(eval pair := $(subst :,$(space),$(fp)))\
  $(eval built := $(word 1,$(pair)))\
  $(eval installed := $(CTS_TESTCASES_OUT)/$(notdir $(word 2,$(pair))))\
  $(eval $(call copy-one-file, $(built), $(installed)))\
  $(eval cts_support_apks += $(installed)))

# Have the module name depend on the cts files; so the cts files get generated when you run mm/mmm/mma/mmma.
$(my_register_name) : $(cts_support_apks)


###########################################################
# Variant: Privileged app upgrade (wrong SHA)

include $(CLEAR_VARS)

LOCAL_MODULE := CtsShimPrivUpgradeWrongSHAPrebuilt
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE_CLASS := APPS
LOCAL_BUILT_MODULE_STEM := package.apk
# Make sure the build system doesn't try to resign the APK
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_COMPATIBILITY_SUITE := cts

LOCAL_SRC_FILES := CtsShimPrivUpgradeWrongSHA.apk

include $(BUILD_PREBUILT)

# Add package to the set of APKs available to CTS
# Unceremoneously ripped from cts/build/support_package.mk
cts_support_apks :=
$(foreach fp, $(ALL_MODULES.$(LOCAL_MODULE).BUILT_INSTALLED),\
  $(eval pair := $(subst :,$(space),$(fp)))\
  $(eval built := $(word 1,$(pair)))\
  $(eval installed := $(CTS_TESTCASES_OUT)/$(notdir $(word 2,$(pair))))\
  $(eval $(call copy-one-file, $(built), $(installed)))\
  $(eval cts_support_apks += $(installed)))

# Have the module name depend on the cts files; so the cts files get generated when you run mm/mmm/mma/mmma.
$(my_register_name) : $(cts_support_apks)
