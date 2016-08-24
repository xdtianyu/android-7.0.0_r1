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
# Builds a package and defines a rule to generate the associated test
# package XML needed by CTS.
#
# Replace "include $(BUILD_PACKAGE)" with "include $(BUILD_CTS_PACKAGE)"
#

# Disable by default so "m cts" will work in emulator builds
LOCAL_DEX_PREOPT := false
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_STATIC_JAVA_LIBRARIES += platform-test-annotations

include $(BUILD_CTS_SUPPORT_PACKAGE)
include $(BUILD_CTS_MODULE_TEST_CONFIG)

cts_src_dirs := $(LOCAL_PATH)
cts_src_dirs += $(sort $(dir $(LOCAL_GENERATED_SOURCES)))
cts_src_dirs := $(addprefix -s , $(cts_src_dirs))

cts_package_xml := $(CTS_TESTCASES_OUT)/$(LOCAL_PACKAGE_NAME).xml
$(cts_package_xml): PRIVATE_SRC_DIRS := $(cts_src_dirs)
$(cts_package_xml): PRIVATE_INSTRUMENTATION := $(LOCAL_INSTRUMENTATION_FOR)
$(cts_package_xml): PRIVATE_PACKAGE := $(LOCAL_PACKAGE_NAME)
ifneq ($(filter cts/suite/cts/%, $(LOCAL_PATH)),)
PRIVATE_CTS_TEST_PACKAGE_NAME_ := com.android.cts.$(notdir $(LOCAL_PATH))
else
PRIVATE_CTS_TEST_PACKAGE_NAME_ := android.$(notdir $(LOCAL_PATH))
endif
$(cts_package_xml): PRIVATE_TEST_PACKAGE := $(PRIVATE_CTS_TEST_PACKAGE_NAME_)
$(cts_package_xml): PRIVATE_MANIFEST := $(LOCAL_PATH)/AndroidManifest.xml
$(cts_package_xml): PRIVATE_TEST_TYPE := $(if $(LOCAL_CTS_TEST_RUNNER),$(LOCAL_CTS_TEST_RUNNER),'')
$(cts_package_xml): $(cts_support_apks)
$(cts_package_xml): $(cts_module_test_config)
$(cts_package_xml): $(CTS_EXPECTATIONS) $(CTS_UNSUPPORTED_ABIS) $(CTS_JAVA_TEST_SCANNER_DOCLET) $(CTS_JAVA_TEST_SCANNER) $(CTS_XML_GENERATOR)
	$(hide) echo Generating test description for java package $(PRIVATE_PACKAGE)
	$(hide) mkdir -p $(CTS_TESTCASES_OUT)
	$(hide) $(CTS_JAVA_TEST_SCANNER) \
						$(PRIVATE_SRC_DIRS) \
						-d $(CTS_JAVA_TEST_SCANNER_DOCLET) | \
			$(CTS_XML_GENERATOR) \
						-t $(PRIVATE_TEST_TYPE) \
						-m $(PRIVATE_MANIFEST) \
						-i "$(PRIVATE_INSTRUMENTATION)" \
						-n $(PRIVATE_PACKAGE) \
						-p $(PRIVATE_TEST_PACKAGE) \
						-e $(CTS_EXPECTATIONS) \
						-b $(CTS_UNSUPPORTED_ABIS) \
						-a $(CTS_TARGET_ARCH) \
						-o $@
# Have the module name depend on the cts files; so the cts files get generated when you run mm/mmm/mma/mmma.
$(my_register_name) : $(cts_package_xml) $(cts_module_test_config)
