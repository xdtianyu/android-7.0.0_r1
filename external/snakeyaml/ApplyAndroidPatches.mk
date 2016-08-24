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
# Apply the android patches in src/patches/android
# - Required in order to build correctly for AOSP.
#

#
# Input variables:
# (Constant)
# -- snakeyaml_need_patch_src_files: List of .java files that will need to be patched.
#
# This mk file will automatically look up the corresponding patch in src/patches/android
# and apply it to every file in $(snakeyaml_need_patch_src_files).
#

LOCAL_MODULE_CLASS := JAVA_LIBRARIES

# Apply all of the Android patches in src/patches/android by running patch-android-src script on them.
intermediates:= $(local-generated-sources-dir)
GEN := $(addprefix $(intermediates)/, $(snakeyaml_need_patch_src_files)) # List of all files that need to be patched.
$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = $(PRIVATE_PATH)/patch-android-src $(PRIVATE_PATH)/ $< $@
$(GEN): $(intermediates)/%.java : $(LOCAL_PATH)/%.java $(LOCAL_PATH)/patch-android-src
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)
