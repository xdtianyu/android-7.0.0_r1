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

#
# Builds a package which is needed by a test package and copies it to CTS
#
# Replace "include $(BUILD_PACKAGE)" with "include $(BUILD_CTS_SUPPORT_PACKAGE)"
#

# Disable by default so "m cts" will work in emulator builds
LOCAL_DEX_PREOPT := false
LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)

cts_support_apks :=
$(foreach fp, $(ALL_MODULES.$(LOCAL_PACKAGE_NAME).BUILT_INSTALLED),\
  $(eval pair := $(subst :,$(space),$(fp)))\
  $(eval built := $(word 1,$(pair)))\
  $(eval installed := $(CTS_TESTCASES_OUT)/$(notdir $(word 2,$(pair))))\
  $(eval $(call copy-one-file, $(built), $(installed)))\
  $(eval cts_support_apks += $(installed)))

# Have the module name depend on the cts files; so the cts files get generated when you run mm/mmm/mma/mmma.
$(my_register_name) : $(cts_support_apks)
