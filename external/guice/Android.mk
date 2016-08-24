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
# Build support for guice within the Android Open Source Project
# See https://source.android.com/source/building.html for more information
#

###################################
#           Guice                 #
###################################

#
# Builds the 'no_aop' flavor for Android.
# -- see core/pom.xml NO_AOP rule.
#
guice_exclude_src_files := \
  core/src/com/google/inject/spi/InterceptorBinding.java \
  core/src/com/google/inject/internal/InterceptorBindingProcessor.java \
  core/src/com/google/inject/internal/InterceptorStackCallback.java \
  core/src/com/google/inject/internal/InterceptorStackCallback.java \
  core/src/com/google/inject/internal/util/LineNumbers.java \
  core/src/com/google/inject/internal/MethodAspect.java \
  core/src/com/google/inject/internal/ProxyFactory.java

guice_exclude_test_files := \
  core/test/com/googlecode/guice/BytecodeGenTest.java \
  core/test/com/google/inject/IntegrationTest.java \
  core/test/com/google/inject/MethodInterceptionTest.java \
  core/test/com/google/inject/internal/ProxyFactoryTest.java

guice_munge_flags := \
  -DNO_AOP
#
#
#

LOCAL_PATH := $(call my-dir)

guice_src_files_raw := $(call all-java-files-under,core/src)
guice_test_files_raw := $(call all-java-files-under,core/test)
guice_src_files := $(filter-out $(guice_exclude_src_files),$(guice_src_files_raw))
guice_test_files := $(filter-out $(guice_exclude_test_files),$(guice_test_files_raw))
munge_host_jar := $(HOST_OUT)/framework/munge-host.jar
munge_zip_location := lib/build/munge.jar

#
# Target-side Dalvik build
include $(CLEAR_VARS)
LOCAL_SRC_FILES := # None. Everything is post-processed by munge. See below.
LOCAL_MODULE := guice
LOCAL_STATIC_JAVA_LIBRARIES := guava jsr330
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
munge_src_arguments := $(guice_src_files)
include $(LOCAL_PATH)/AndroidCallMunge.mk
include $(BUILD_STATIC_JAVA_LIBRARY)


#
# Host-side Java build
include $(CLEAR_VARS)
LOCAL_SRC_FILES := # None. Everything is post-processed by munge. See below.
LOCAL_MODULE := guice-host
LOCAL_STATIC_JAVA_LIBRARIES := guavalib jsr330-host

munge_src_arguments := $(guice_src_files)
include $(LOCAL_PATH)/AndroidCallMunge.mk
include $(BUILD_HOST_JAVA_LIBRARY)


#
# Host-side Dalvik build
include $(CLEAR_VARS)
LOCAL_SRC_FILES := # None. Everything is post-processed by munge. See below.
LOCAL_MODULE := guice-hostdex
LOCAL_STATIC_JAVA_LIBRARIES := guava-hostdex jsr330-hostdex
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
munge_src_arguments := $(guice_src_files)
include $(LOCAL_PATH)/AndroidCallMunge.mk
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)

###################################
#           Munge                 #
###################################

# This is required to post-process the guice source to strip out the AOP-specific code.
# We build it from source (conveniently zipped inside of lib/build/munge.jar) instead
# of relying on a prebuilt.

munge_zipped_src_files_raw := $(filter %.java,$(shell unzip -Z1 "$(LOCAL_PATH)/$(munge_zip_location)"))
munge_zipped_unsupported_files := MungeTask.java # Missing ant dependencies in Android.
munge_zipped_src_files := $(filter-out $(munge_zipped_unsupported_files),$(munge_zipped_src_files_raw))

#
# We build munge from lib/build/munge.jar source code.
#

# (Munge) Host-side Java build
include $(CLEAR_VARS)
LOCAL_SRC_FILES := # None because we get everything by unzipping the munge jar first.
LOCAL_MODULE := munge-host
LOCAL_JAVA_LIBRARIES := junit
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
# Unzip munge and build it
intermediates:= $(local-generated-sources-dir)
GEN := $(addprefix $(intermediates)/, $(munge_zipped_src_files)) # List of all files that need to be patched.
$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_INPUT_FILE := $(munge_zipped_src_files)
$(GEN) : PRIVATE_ZIP_LOCATION := $(munge_zip_location)
$(GEN) : PRIVATE_CUSTOM_TOOL = unzip -p "$(PRIVATE_PATH)/$(PRIVATE_ZIP_LOCATION)" $(shell echo $@ | awk -F / "{if (NF>1) {print \$$NF}}")  >$@ ## unzip -p munge.jar Filename.java > intermediates/Filename.java
$(GEN): $(intermediates)/%.java : $(LOCAL_PATH)/$(PRIVATE_ZIP_LOCATION)
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

include $(BUILD_HOST_JAVA_LIBRARY)


# Rules for target, hostdex, etc., are omitted since munge is only used during the build.

# TODO: Consider adding tests.
