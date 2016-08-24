#
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
#

JACK_STABLE_VERSION := 3.36.CANDIDATE
JACK_DOGFOOD_VERSION := 3.36.CANDIDATE
JACK_SDKTOOL_VERSION := 4.7.BETA
JACK_LANG_DEV_VERSION := 3.36.CANDIDATE

ifneq ($(ANDROID_JACK_DEFAULT_VERSION),)
JACK_DEFAULT_VERSION := $(JACK_$(ANDROID_JACK_DEFAULT_VERSION)_VERSION)
ifeq ($(JACK_DEFAULT_VERSION),)
$(error "$(ANDROID_JACK_DEFAULT_VERSION)" is an invalid value for ANDROID_JACK_DEFAULT_VERSION)
endif
JACK_APPS_VERSION := $(ANDROID_JACK_DEFAULT_VERSION)
else
ifneq (,$(TARGET_BUILD_APPS))
# Unbundled branches
JACK_DEFAULT_VERSION := $(JACK_DOGFOOD_VERSION)
else
# Complete android tree
JACK_DEFAULT_VERSION := $(JACK_DOGFOOD_VERSION)
endif
JACK_APPS_VERSION := DOGFOOD
endif
