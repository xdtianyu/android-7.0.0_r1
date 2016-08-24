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
# The following optional support has been disabled:
# - ant
# - bsh
#
# JUnit support is enabled, but needs to be explicitly added in with LOCAL_STATIC_JAVA_LIBRARIES
# by whichever app/library is also including testng.

LOCAL_PATH := $(call my-dir)

##
## Common variables, don't repeat yourself.
##

# Memorize path so we can use it later.
testng_path := $(LOCAL_PATH)

# These files don't build on Android, either due to missing java.* APIs or due to missing dependencies (see above).
testng_android_unsupported_src_files := \
  src/main/java/com/beust/testng/TestNGAntTask.java \
  src/main/java/org/testng/TestNGAntTask.java \
  src/main/java/org/testng/internal/Bsh.java \
  src/main/java/org/testng/internal/PropertyUtils.java \
  src/main/java/org/testng/internal/PathUtils.java

# These files don't exist in the source tree, they need to be generated during the build.
testng_src_files_need_gen := src/generated/java/org/testng/internal/Version.java

# Android-specific replacements of some of the above files.
testng_src_files_android_specific := $(call all-java-files-under,android-src)
# Everything under src/main, before we remove android-unsupported files.
testng_src_files_unfiltered := $(call all-java-files-under,src/main)
# The nominal files we use to build for Android is everything in src/main that's supported, plus everything in android-src.
testng_src_files := $(filter-out $(testng_android_unsupported_src_files),$(testng_src_files_unfiltered)) $(testng_src_files_android_specific)

##
## Build rules follow.
##

# target jar (static)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(testng_src_files)
LOCAL_MODULE := testng
LOCAL_STATIC_JAVA_LIBRARIES := jcommander snakeyaml guice
LOCAL_JAVA_LIBRARIES := junit-targetdex junit4-target
include $(LOCAL_PATH)/GenerateTemplates.mk # Generate Version.java
include $(BUILD_STATIC_JAVA_LIBRARY)

# target jar (standalone, e.g. add it to classpath manually)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(testng_src_files)
LOCAL_MODULE := testng-lib
LOCAL_STATIC_JAVA_LIBRARIES := jcommander snakeyaml guice
LOCAL_JAVA_LIBRARIES := junit-targetdex junit4-target
include $(LOCAL_PATH)/GenerateTemplates.mk # Generate Version.java
include $(BUILD_JAVA_LIBRARY)

# host jar
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(testng_src_files)
LOCAL_MODULE := testng-host
LOCAL_STATIC_JAVA_LIBRARIES := jcommander-host snakeyaml-host guice-host
LOCAL_JAVA_LIBRARIES := junit
LOCAL_IS_HOST_MODULE := true
include $(LOCAL_PATH)/GenerateTemplates.mk # Generate Version.java
include $(BUILD_HOST_JAVA_LIBRARY)

# host dex
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(testng_src_files)
LOCAL_MODULE := testng-hostdex
LOCAL_STATIC_JAVA_LIBRARIES := jcommander-hostdex snakeyaml-hostdex guice-hostdex
LOCAL_JAVA_LIBRARIES := junit-hostdex junit4-target-hostdex
include $(LOCAL_PATH)/GenerateTemplates.mk # Generate Version.java
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)

# TODO: also add the tests once we have testng working.
