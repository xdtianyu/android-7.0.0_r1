# Copyright (C) 2013 The Android Open Source Project
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

###################################################################
# Host build
###################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    $(call all-java-files-under, src) \
    $(call all-java-files-under, cglib-and-asm/src)

LOCAL_JAVA_LIBRARIES := junit objenesis-host ant
LOCAL_MODULE := mockito-host
LOCAL_MODULE_TAGS := optional
LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)


###################################################################
# Target build
###################################################################

# Builds the Mockito source code, but does not include any run-time
# dependencies. Most projects should use mockito-target instead, which includes
# everything needed to run Mockito tests.
include $(CLEAR_VARS)

# Exclude source used to dynamically create classes since target builds use 
# dexmaker instead and including it causes conflicts.
explicit_target_excludes := \
    src/org/mockito/internal/creation/AbstractMockitoMethodProxy.java \
    src/org/mockito/internal/creation/AcrossJVMSerializationFeature.java \
    src/org/mockito/internal/creation/CglibMockMaker.java \
    src/org/mockito/internal/creation/DelegatingMockitoMethodProxy.java \
    src/org/mockito/internal/creation/MethodInterceptorFilter.java \
    src/org/mockito/internal/creation/MockitoMethodProxy.java \
    src/org/mockito/internal/creation/SerializableMockitoMethodProxy.java \
    src/org/mockito/internal/invocation/realmethod/FilteredCGLIBProxyRealMethod.java \
    src/org/mockito/internal/invocation/realmethod/CGLIBProxyRealMethod.java \
    src/org/mockito/internal/invocation/realmethod/HasCGLIBMethodProxy.java

target_src_files := \
    $(call all-java-files-under, src)
target_src_files := \
    $(filter-out src/org/mockito/internal/creation/cglib/%, $(target_src_files))
target_src_files := \
    $(filter-out src/org/mockito/internal/creation/jmock/%, $(target_src_files))
target_src_files := \
    $(filter-out $(explicit_target_excludes), $(target_src_files))

LOCAL_SRC_FILES := $(target_src_files)
LOCAL_JAVA_LIBRARIES := junit4-target objenesis-target
LOCAL_MODULE := mockito-api
LOCAL_SDK_VERSION := 10
LOCAL_MODULE_TAGS := optional
include $(BUILD_STATIC_JAVA_LIBRARY)

# Main target for dependent projects. Bundles all the run-time dependencies
# needed to run Mockito tests on the device.
include $(CLEAR_VARS)

LOCAL_MODULE := mockito-target
LOCAL_STATIC_JAVA_LIBRARIES := mockito-target-minus-junit4 junit4-target
LOCAL_SDK_VERSION := 10
LOCAL_MODULE_TAGS := optional
include $(BUILD_STATIC_JAVA_LIBRARY)

# A mockito target that doesn't pull in junit4-target. This is used to work around
# issues caused by multiple copies of junit4 in the classpath, usually when a test
# using mockito is run using android.test.runner.
include $(CLEAR_VARS)
LOCAL_MODULE := mockito-target-minus-junit4
LOCAL_STATIC_JAVA_LIBRARIES := mockito-api dexmaker dexmaker-mockmaker objenesis-target
LOCAL_JAVA_LIBRARIES := junit4-target
LOCAL_SDK_VERSION := 10
LOCAL_MODULE_TAGS := optional
include $(BUILD_STATIC_JAVA_LIBRARY)


###################################################################
# Host build
###################################################################

# Builds the Mockito source code, but does not include any run-time
# dependencies. Since host modules are not compiled against the SDK,
# an explicit inclusion of core-junit-hostdex is needed in contrast
# with the target module above.
ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(target_src_files)
LOCAL_JAVA_LIBRARIES := core-junit-hostdex junit4-target-hostdex \
    objenesis-hostdex
LOCAL_MODULE := mockito-api-hostdex
LOCAL_MODULE_TAGS := optional
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)
endif # HOST_OS == linux


###################################################
# Clean up temp vars
###################################################
explicit_target_excludes :=
target_src_files :=
