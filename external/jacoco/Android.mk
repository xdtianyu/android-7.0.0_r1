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

# Build jacoco from sources for the platform
#
# Note: this is only intended to be used for the platform development. This is *not* intended
# to be used in the SDK where apps can use the official jacoco release.
include $(CLEAR_VARS)

jacoco_src_files := $(call all-java-files-under,org.jacoco.core/src)
jacoco_src_files += $(call all-java-files-under,org.jacoco.agent/src)
jacoco_src_files += $(call all-java-files-under,org.jacoco.agent.rt/src)

# Some Jacoco source files depend on classes that do not exist in Android. While these classes are
# not executed at runtime (because we use offline instrumentation), they will cause issues when
# compiling them with ART during dex pre-opting. Therefore, it would prevent from applying code
# coverage on classes in the bootclasspath (frameworks, services, ...) or system apps.
# Note: we still may need to update the source code to cut dependencies in mandatory jacoco classes.
jacoco_android_exclude_list := \
  %org.jacoco.core/src/org/jacoco/core/runtime/ModifiedSystemClassRuntime.java \
  %org.jacoco.agent.rt/src/org/jacoco/agent/rt/internal/PreMain.java \
  %org.jacoco.agent.rt/src/org/jacoco/agent/rt/internal/CoverageTransformer.java \
  %org.jacoco.agent.rt/src/org/jacoco/agent/rt/internal/JmxRegistration.java

LOCAL_SRC_FILES := $(filter-out $(jacoco_android_exclude_list),$(jacoco_src_files))

# In order to include Jacoco in core libraries, we cannot depend on anything in the
# bootclasspath (or we would create dependency cycle). Therefore we compile against
# the SDK android.jar which gives the same APIs Jacoco depends on.
LOCAL_SDK_VERSION := 9

LOCAL_MODULE := jacocoagent
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_JAVA_LIBRARIES := jacoco-asm
include $(BUILD_STATIC_JAVA_LIBRARY)

#
# Build asm-5.0.1 as a static library.
#
include $(CLEAR_VARS)

LOCAL_MODULE := jacoco-asm
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_SRC_FILES := asm-debug-all-5.0.1$(COMMON_JAVA_PACKAGE_SUFFIX)
# Workaround for b/27319022
LOCAL_JACK_FLAGS := -D jack.import.jar.debug-info=false
LOCAL_UNINSTALLABLE_MODULE := true

include $(BUILD_PREBUILT)
