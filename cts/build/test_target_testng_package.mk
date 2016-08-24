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
# Builds a host library and defines a rule to generate the associated test
# package XML needed by CTS.
#
# Disable by default so "m cts" will work in emulator builds
LOCAL_DEX_PREOPT := false
LOCAL_STATIC_JAVA_LIBRARIES += platform-test-annotations
include $(BUILD_JAVA_LIBRARY)
include $(BUILD_CTS_MODULE_TEST_CONFIG)

cts_library_jar := $(CTS_TESTCASES_OUT)/$(LOCAL_MODULE).jar
$(cts_library_jar): $(LOCAL_BUILT_MODULE)
	$(copy-file-to-target)

CTS_DEQP_CONFIG_PATH := $(call my-dir)

cts_library_xml := $(CTS_TESTCASES_OUT)/$(LOCAL_MODULE).xml
$(cts_library_xml): MUSTPASS_XML_FILE := $(LOCAL_CTS_TESTCASE_XML_INPUT)
$(cts_library_xml): PRIVATE_DUMMY_CASELIST := $(CTS_DEQP_CONFIG_PATH)/deqp_dummy_test_list
$(cts_library_xml): $(cts_library_jar)
$(cts_library_xml): $(cts_module_test_config)
$(cts_library_xml): $(CTS_EXPECTATIONS) $(CTS_UNSUPPORTED_ABIS) $(CTS_XML_GENERATOR) $(CTS_TESTCASE_XML_INPUT)
	$(hide) echo Generating test description for target testng package $(PRIVATE_LIBRARY)
	$(hide) mkdir -p $(CTS_TESTCASES_OUT)

# Query build ABIs by routing a dummy test list through xml generator and parse result. Use sed to insert the ABI string into the XML files.
	$(hide) SUPPORTED_ABI_ATTR=`$(CTS_XML_GENERATOR) -t dummyTest \
									-n dummyName \
									-p invalid.dummy \
									-e $(CTS_EXPECTATIONS) \
									-b $(CTS_UNSUPPORTED_ABIS) \
									-a $(CTS_TARGET_ARCH) \
									< $(PRIVATE_DUMMY_CASELIST) \
									| grep --only-matching -e " abis=\"[^\"]*\""` && \
			$(SED_EXTENDED) -e "s:^(\s*)<Test ((.[^/]|[^/])*)(/?)>$$:\1<Test \2 $${SUPPORTED_ABI_ATTR}\4>:" \
				< $(MUSTPASS_XML_FILE) \
				> $@

# Have the module name depend on the cts files; so the cts files get generated when you run mm/mmm/mma/mmma.
$(my_register_name) : $(cts_library_jar) $(cts_library_xml) $(cts_module_test_config)
