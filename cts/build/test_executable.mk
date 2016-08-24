# Copyright (C) 2011 The Android Open Source Project
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
# Builds an executable and defines a rule to generate the associated test
# package XML needed by CTS.
#
# 1. Replace "include $(BUILD_EXECUTABLE)"
#    with "include $(BUILD_CTS_EXECUTABLE)"
#
# 2. Define LOCAL_CTS_TEST_PACKAGE to group the tests under a package
#    as needed by CTS.
#

LOCAL_CXX_STL := libc++
include $(BUILD_EXECUTABLE)
include $(BUILD_CTS_MODULE_TEST_CONFIG)

cts_executable_bin :=
$(foreach fp, $(ALL_MODULES.$(LOCAL_MODULE).BUILT) $(ALL_MODULES.$(LOCAL_MODULE)$(TARGET_2ND_ARCH_MODULE_SUFFIX).BUILT),\
  $(eval installed := $(CTS_TESTCASES_OUT)/$(notdir $(fp)))\
  $(eval $(call copy-one-file, $(fp), $(installed)))\
  $(eval cts_executable_bin += $(installed)))

cts_executable_xml := $(CTS_TESTCASES_OUT)/$(LOCAL_MODULE).xml
$(cts_executable_xml): PRIVATE_TEST_PACKAGE := $(LOCAL_CTS_TEST_PACKAGE)
$(cts_executable_xml): PRIVATE_EXECUTABLE := $(LOCAL_MODULE)
$(cts_executable_xml): PRIVATE_LIST_EXECUTABLE := $(HOST_OUT_EXECUTABLES)/$(LOCAL_MODULE)_list
$(cts_executable_xml): $(HOST_OUT_EXECUTABLES)/$(LOCAL_MODULE)_list
$(cts_executable_xml): $(cts_executable_bin)
$(cts_executable_xml): $(cts_module_test_config)
$(cts_executable_xml): $(addprefix $(LOCAL_PATH)/,$(LOCAL_SRC_FILES)) $(CTS_EXPECTATIONS) $(CTS_UNSUPPORTED_ABIS) $(CTS_NATIVE_TEST_SCANNER) $(CTS_XML_GENERATOR)
	$(hide) echo Generating test description for native package $(PRIVATE_TEST_PACKAGE)
	$(hide) mkdir -p $(CTS_TESTCASES_OUT)
	$(hide) $(PRIVATE_LIST_EXECUTABLE) --gtest_list_tests | \
			$(CTS_NATIVE_TEST_SCANNER) -t $(PRIVATE_TEST_PACKAGE) | \
			$(CTS_XML_GENERATOR) -t native \
						-n $(PRIVATE_EXECUTABLE) \
						-p $(PRIVATE_TEST_PACKAGE) \
						-e $(CTS_EXPECTATIONS) \
						-b $(CTS_UNSUPPORTED_ABIS) \
						-a $(CTS_TARGET_ARCH) \
						-o $@

# Have the module name depend on the cts files; so the cts files get generated when you run mm/mmm/mma/mmma.
$(my_register_name) : $(cts_executable_bin) $(cts_executable_xml) $(cts_module_test_config)
