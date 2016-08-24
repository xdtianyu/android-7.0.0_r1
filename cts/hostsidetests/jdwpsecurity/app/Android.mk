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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := CtsJdwpApp
LOCAL_SRC_FILES := $(call all-subdir-java-files)
# Tag this module as a cts test artifact
LOCAL_COMPATIBILITY_SUITE := cts
include $(BUILD_JAVA_LIBRARY)

# Copy the built module to the cts dir
cts_library_jar := $(CTS_TESTCASES_OUT)/$(LOCAL_MODULE).jar
$(cts_library_jar): $(LOCAL_BUILT_MODULE)
	$(copy-file-to-target)

# Have the module name depend on the cts files; so the cts files get generated when you run mm/mmm/mma/mmma.
$(my_register_name) : $(cts_library_jar)

