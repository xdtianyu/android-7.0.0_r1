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

# Include definitions of DAGGER2_PROCESSOR_CLASSES/LIBRARIES
include external/dagger2/dagger2_annotation_processor.mk

# build caliper host jar
# ============================================================

include $(CLEAR_VARS)

LOCAL_MODULE := caliper-host
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_SRC_FILES := $(call all-java-files-under, caliper/src/main/java/)
LOCAL_JAVA_RESOURCE_DIRS := caliper/src/main/resources
LOCAL_IS_HOST_MODULE := true

LOCAL_STATIC_JAVA_LIBRARIES := \
  apache-commons-math-host \
  caliper-gson-host \
  caliper-java-allocation-instrumenter-host \
  caliper-jersey-client-host \
  caliper-jersey-core-host \
  caliper-joda-time-host \
  caliper-jsr311-api-host \
  dagger2-host \
  dagger2-inject-host \
  guavalib

# Use Dagger2 annotation processor
PROCESSOR_LIBRARIES := $(DAGGER2_PROCESSOR_LIBRARIES)
PROCESSOR_CLASSES := $(DAGGER2_PROCESSOR_CLASSES)
include external/dagger2/java_annotation_processors.mk

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)

# Remember the location of the generated files, this is needed for when
# building for target
caliper_host_generated_sources_dir := $(local-generated-sources-dir)/annotation_processor_output

# build caliper target api jar
# ============================================================
# This contains just those classes needed for benchmarks to compile.

include $(CLEAR_VARS)

LOCAL_MODULE := caliper-api-target
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_SRC_FILES := \
  caliper/src/main/java/com/google/caliper/AfterExperiment.java \
  caliper/src/main/java/com/google/caliper/BeforeExperiment.java \
  caliper/src/main/java/com/google/caliper/Param.java \
  caliper/src/main/java/com/google/caliper/All.java \
  caliper/src/main/java/com/google/caliper/Benchmark.java

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_JAVA_LIBRARY)

# build caliper tests
# ============================================================
# vogar --expectations $ANDROID_BUILD_TOP/external/caliper/expectations/knownfailures.txt \
        --test-only \
        --classpath $ANDROID_BUILD_TOP/out/host/common/obj/JAVA_LIBRARIES/caliper-tests_intermediates/javalib.jar \
        com.google.caliper

include $(CLEAR_VARS)

LOCAL_MODULE := caliper-tests
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_SRC_FILES := $(call all-java-files-under, caliper/src/test/java/)
LOCAL_JAVA_RESOURCE_DIRS := caliper/src/test/resources
LOCAL_IS_HOST_MODULE := true

LOCAL_STATIC_JAVA_LIBRARIES := \
  caliper-host \
  junit \
  mockito-host

# Use Dagger2 annotation processor
PROCESSOR_LIBRARIES := $(DAGGER2_PROCESSOR_LIBRARIES)
PROCESSOR_CLASSES := $(DAGGER2_PROCESSOR_CLASSES)
include external/dagger2/java_annotation_processors.mk

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)

# build caliper examples
# ============================================================

include $(CLEAR_VARS)

LOCAL_MODULE := caliper-examples
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_SRC_FILES := $(call all-java-files-under, examples/src/main/java/)
LOCAL_IS_HOST_MODULE := true

LOCAL_STATIC_JAVA_LIBRARIES := \
  caliper-host \
  junit \
  mockito-host

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)

# Build host dependencies.
# ============================================================
include $(CLEAR_VARS)

LOCAL_PREBUILT_JAVA_LIBRARIES := \
    caliper-gson-host:lib/gson-2.2.2$(COMMON_JAVA_PACKAGE_SUFFIX) \
    caliper-java-allocation-instrumenter-host:lib/java-allocation-instrumenter-2.0$(COMMON_JAVA_PACKAGE_SUFFIX) \
    caliper-jersey-client-host:lib/jersey-client-1.11$(COMMON_JAVA_PACKAGE_SUFFIX) \
    caliper-jersey-core-host:lib/jersey-core-1.11$(COMMON_JAVA_PACKAGE_SUFFIX) \
    caliper-joda-time-host:lib/joda-time-2.1$(COMMON_JAVA_PACKAGE_SUFFIX) \
    caliper-jsr311-api-host:lib/jsr311-api-1.1.1$(COMMON_JAVA_PACKAGE_SUFFIX)

include $(BUILD_HOST_PREBUILT)
