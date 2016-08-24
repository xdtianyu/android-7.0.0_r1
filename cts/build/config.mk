# Copyright (C) 2012 The Android Open Source Project
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

# Test XMLs, native executables, and packages will be placed in this
# directory before creating the final CTS distribution.
CTS_TESTCASES_OUT := $(HOST_OUT)/old-cts/old-android-cts/repository/testcases

COMPATIBILITY_TESTCASES_OUT_cts := $(HOST_OUT)/cts/android-cts/testcases

# Scanners of source files for tests which are then inputed into
# the XML generator to produce test XMLs.
CTS_NATIVE_TEST_SCANNER := $(HOST_OUT_EXECUTABLES)/cts-native-scanner
CTS_JAVA_TEST_SCANNER := $(HOST_OUT_EXECUTABLES)/cts-java-scanner
CTS_JAVA_TEST_SCANNER_DOCLET := $(HOST_OUT_JAVA_LIBRARIES)/cts-java-scanner-doclet.jar

# Generator of test XMLs from scanner output.
CTS_XML_GENERATOR := $(HOST_OUT_EXECUTABLES)/cts-xml-generator

# File indicating which tests should be blacklisted due to problems.
CTS_EXPECTATIONS := cts/tests/expectations/knownfailures.txt

# File indicating which tests should be blacklisted due to unsupported abi.
CTS_UNSUPPORTED_ABIS := cts/tests/expectations/unsupportedabis.txt

# Holds the target architecture to build for.
CTS_TARGET_ARCH := $(TARGET_ARCH)

# default module config filename
CTS_MODULE_TEST_CONFIG := AndroidTest.xml

# CTS build rules
BUILD_COMPATIBILITY_SUITE := cts/build/compatibility_test_suite.mk
BUILD_CTS_EXECUTABLE := cts/build/test_executable.mk
BUILD_CTS_PACKAGE := cts/build/test_package.mk
BUILD_CTS_GTEST_PACKAGE := cts/build/test_gtest_package.mk
BUILD_CTS_HOST_JAVA_LIBRARY := cts/build/test_host_java_library.mk
BUILD_CTS_TARGET_JAVA_LIBRARY := cts/build/test_target_java_library.mk
BUILD_CTS_DEQP_PACKAGE := cts/build/test_deqp_package.mk
BUILD_CTS_SUPPORT_PACKAGE := cts/build/support_package.mk
BUILD_CTS_MODULE_TEST_CONFIG := cts/build/module_test_config.mk
BUILD_CTS_DEVICE_INFO_PACKAGE := cts/build/device_info_package.mk
BUILD_CTS_TARGET_TESTNG_PACKAGE := cts/build/test_target_testng_package.mk
