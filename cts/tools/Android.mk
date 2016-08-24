# Copyright (C) 2008 The Android Open Source Project
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

# Build the CTS harness

JUNIT_HOST_JAR := $(HOST_OUT_JAVA_LIBRARIES)/junit.jar
HOSTTESTLIB_JAR := $(HOST_OUT_JAVA_LIBRARIES)/hosttestlib.jar
TF_JAR := $(HOST_OUT_JAVA_LIBRARIES)/tradefed-prebuilt.jar
CTS_TF_JAR := $(HOST_OUT_JAVA_LIBRARIES)/old-cts-tradefed.jar
CTS_TF_EXEC_PATH ?= $(HOST_OUT_EXECUTABLES)/old-cts-tradefed

cts_prebuilt_jar := $(HOST_OUT)/old-cts/old-android-cts/tools/cts-prebuilt.jar
$(cts_prebuilt_jar): PRIVATE_TESTS_DIR := $(HOST_OUT)/old-cts/old-android-cts/repository/testcases
$(cts_prebuilt_jar): PRIVATE_PLANS_DIR := $(HOST_OUT)/old-cts/old-android-cts/repository/plans
$(cts_prebuilt_jar): PRIVATE_TOOLS_DIR := $(HOST_OUT)/old-cts/old-android-cts/tools
$(cts_prebuilt_jar): $(JUNIT_HOST_JAR) $(HOSTTESTLIB_JAR) $(TF_JAR) $(CTS_TF_JAR) $(CTS_TF_EXEC_PATH) $(ADDITIONAL_TF_JARS) | $(ACP) $(HOST_OUT_EXECUTABLES)/adb
	mkdir -p $(PRIVATE_TESTS_DIR)
	mkdir -p $(PRIVATE_PLANS_DIR)
	mkdir -p $(PRIVATE_TOOLS_DIR)
	$(ACP) -fp $(JUNIT_HOST_JAR) $(HOSTTESTLIB_JAR) $(TF_JAR) $(CTS_TF_JAR) $(CTS_TF_EXEC_PATH) $(ADDITIONAL_TF_JARS) $(PRIVATE_TOOLS_DIR)

.PHONY: cts-harness
cts-harness : $(cts_prebuilt_jar)

# Put the test coverage report in the dist dir if "old-cts" is among the build goals.
ifneq ($(filter old-cts, $(MAKECMDGOALS)),)
  $(call dist-for-goals,old-cts,$(CTS_TF_JAR))
  $(call dist-for-goals,old-cts,$(HOSTTESTLIB_JAR))
endif

include $(call all-subdir-makefiles)
