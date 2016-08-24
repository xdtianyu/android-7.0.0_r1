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
# Build support for testng within the Android Open Source Project
# See https://source.android.com/source/building.html for more information
#
#

# This file generates Version.java.
# Factored out as a separate file for reuse between multiple Android build rules.

LOCAL_MODULE_CLASS := JAVA_LIBRARIES

# Apply all of the Android patches in src/patches/android by running patch-android-src script on them.
intermediates:= $(local-generated-sources-dir)
GEN := $(addprefix $(intermediates)/, $(testng_src_files_need_gen)) # List of all files that need to be generated.
$(GEN) : PRIVATE_PATH := $(testng_path)
## ./generate-version-file ./src/main/resources/org/testng/internal/VersionTemplateJava "@version@" kobalt/src/Build.kt "VERSION" > Version.java
$(GEN) : PRIVATE_CUSTOM_TOOL = \
  $(PRIVATE_PATH)/generate-version-file "$(PRIVATE_PATH)/src/main/resources/org/testng/internal/VersionTemplateJava" \
  "@version@" \
  "$(PRIVATE_PATH)/kobalt/src/Build.kt" \
  "VERSION" > $@
$(GEN): $(intermediates)/%.java : $(LOCAL_PATH)/src/main/resources/org/testng/internal/VersionTemplateJava \
  $(LOCAL_PATH)/kobalt/src/Build.kt \
  $(LOCAL_PATH)/generate-version-file
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

