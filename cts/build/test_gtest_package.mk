# Copyright 2012 The Android Open Source Project
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
# Builds a package and defines a rule to generate the associated test
# package XML needed by CTS.
#
# Replace "include $(BUILD_PACKAGE)" with "include $(BUILD_CTS_GTEST_PACKAGE)"
#

# Disable by default so "m cts" will work in emulator builds
LOCAL_DEX_PREOPT := false
LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_CTS_SUPPORT_PACKAGE)
include $(BUILD_CTS_MODULE_TEST_CONFIG)

cts_package_xml := $(CTS_TESTCASES_OUT)/$(LOCAL_PACKAGE_NAME).xml
$(cts_package_xml): PRIVATE_PATH := $(LOCAL_PATH)
$(cts_package_xml): PRIVATE_TEST_PACKAGE := android.$(notdir $(LOCAL_PATH))
$(cts_package_xml): PRIVATE_EXECUTABLE := $(LOCAL_MODULE)
$(cts_package_xml): PRIVATE_MANIFEST := $(LOCAL_PATH)/AndroidManifest.xml
$(cts_package_xml): PRIVATE_TEST_LIST := $(LOCAL_PATH)/$(LOCAL_MODULE)_list.txt
$(cts_package_xml): $(LOCAL_PATH)/$(LOCAL_MODULE)_list.txt
$(cts_package_xml): $(cts_support_apks)
$(cts_package_xml): $(cts_module_test_config)
$(cts_package_xml): $(addprefix $(LOCAL_PATH)/,$(LOCAL_SRC_FILES))  $(CTS_NATIVE_TEST_SCANNER) $(CTS_XML_GENERATOR)
	$(hide) echo Generating test description for wrapped native package $(PRIVATE_EXECUTABLE)
	$(hide) mkdir -p $(CTS_TESTCASES_OUT)
	$(hide) cat $(PRIVATE_TEST_LIST) | \
			$(CTS_NATIVE_TEST_SCANNER) -t $(PRIVATE_TEST_PACKAGE) | \
			$(CTS_XML_GENERATOR) -t wrappednative \
						-m $(PRIVATE_MANIFEST) \
						-n $(PRIVATE_EXECUTABLE) \
						-p $(PRIVATE_TEST_PACKAGE) \
						-e $(CTS_EXPECTATIONS) \
						-b $(CTS_UNSUPPORTED_ABIS) \
						-a $(CTS_TARGET_ARCH) \
						-o $@

# Have the module name depend on the cts files; so the cts files get generated when you run mm/mmm/mma/mmma.
$(my_register_name) : $(cts_package_xml) $(cts_module_test_config)
