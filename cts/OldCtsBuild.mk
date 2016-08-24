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

LOCAL_PATH := $(call my-dir)

# Functions to get the paths of the build outputs.

define cts-get-lib-paths
	$(foreach lib,$(1),$(CTS_TESTCASES_OUT)/$(lib).jar)
endef

define cts-get-ui-lib-paths
	$(foreach lib,$(1),$(CTS_TESTCASES_OUT)/$(lib).jar)
endef

define cts-get-native-paths
	$(foreach exe,$(1),$(CTS_TESTCASES_OUT)/$(exe)$(2))
endef

define cts-get-package-paths
	$(foreach pkg,$(1),$(CTS_TESTCASES_OUT)/$(pkg).apk)
endef

define cts-get-test-xmls
	$(foreach name,$(1),$(CTS_TESTCASES_OUT)/$(name).xml)
endef

define cts-get-executable-paths
	$(foreach executable,$(1),$(CTS_TESTCASES_OUT)/$(executable))
endef

define cts-get-deqp-test-xmls
	$(foreach api,$(1),$(CTS_TESTCASES_OUT)/com.drawelements.deqp.$(api).xml)
endef
