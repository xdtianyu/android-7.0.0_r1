# -*- mode: makefile -*-
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

LOCAL_PATH := $(call my-dir)

cacerts_google := $(call all-files-under,files)

cacerts_google_target_directory := $(TARGET_OUT)/etc/security/cacerts_google
$(foreach cacert, $(cacerts_google), $(eval $(call include-prebuilt-with-destination-directory,target-cacert-google-$(notdir $(cacert)),$(cacert),$(cacerts_google_target_directory))))
cacerts_google_target := $(addprefix $(cacerts_google_target_directory)/,$(foreach cacert,$(cacerts_google),$(notdir $(cacert))))
.PHONY: cacerts_google_target
cacerts_google: $(cacerts_google_target)

# This is so that build/target/product/core.mk can use cacerts_google in PRODUCT_PACKAGES
ALL_MODULES.cacerts_google.INSTALLED := $(cacerts_google_target)

cacerts_google_host_directory := $(HOST_OUT)/etc/security/cacerts_google
$(foreach cacert, $(cacerts_google), $(eval $(call include-prebuilt-with-destination-directory,host-cacert-google-$(notdir $(cacert)),$(cacert),$(cacerts_google_host_directory))))

cacerts_google_host := $(addprefix $(cacerts_google_host_directory)/,$(foreach cacert,$(cacerts_google),$(notdir $(cacert))))
.PHONY: cacerts_google-host
cacerts_google-host: $(cacerts_google_host)
