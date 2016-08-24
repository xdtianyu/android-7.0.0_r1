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
LOCAL_PATH:= $(call my-dir)

jack_server_version := 4.8.ALPHA
jack_server_jar := $(LOCAL_PATH)/jack-server-$(jack_server_version).jar


ifneq ($(ANDROID_JACK_VM_ARGS),)
jack_vm_args := $(ANDROID_JACK_VM_ARGS)
else
jack_vm_args := -Dfile.encoding=UTF-8 -XX:+TieredCompilation
endif
available_jack_jars := $(wildcard $(LOCAL_PATH)/jacks/jack-*.jar)

ifdef JACK_SERVER
ifneq ($(JACK_SERVER),true)
jack_server_disabled=true
endif
endif

.PHONY: setup-jack-server
setup-jack-server : PRIVATE_JACK_ADMIN := $(LOCAL_PATH)/jack-admin
setup-jack-server : PRIVATE_PATH := $(LOCAL_PATH)
setup-jack-server : PRIVATE_SERVER_VERSION := $(jack_server_version)
setup-jack-server : PRIVATE_SERVER_JAR := $(jack_server_jar)
setup-jack-server: $(JACK) $(LOCAL_PATH)/jack-launcher.jar $(jack_server_jar) $(available_jack_jars)
ifndef jack_server_disabled
	@echo Ensure Jack server is installed and started
ifneq ($(dist_goal),)
	$(hide) $(PRIVATE_JACK_ADMIN) stop-server 2>&1 || (exit 0)
	$(hide) $(PRIVATE_JACK_ADMIN) kill-server 2>&1 || (exit 0)
	$(hide) $(PRIVATE_JACK_ADMIN) uninstall-server 2>&1 || (exit 0)
endif
	$(hide) $(PRIVATE_JACK_ADMIN) install-server $(PRIVATE_PATH)/jack-launcher.jar $(PRIVATE_SERVER_JAR)  2>&1 || (exit 0)
ifneq ($(dist_goal),)
	$(hide) mkdir -p "$(DIST_DIR)/logs/jack/"
	$(hide) JACK_SERVER_VM_ARGUMENTS="$(jack_vm_args) -Dcom.android.jack.server.log.file=$(abspath $(DIST_DIR))/logs/jack/jack-server-%u-%g.log" $(PRIVATE_JACK_ADMIN) start-server 2>&1 || exit 0
else
	$(hide) JACK_SERVER_VM_ARGUMENTS="$(jack_vm_args)" $(PRIVATE_JACK_ADMIN) start-server 2>&1 || exit 0
endif
	$(hide) $(PRIVATE_JACK_ADMIN) update server $(PRIVATE_SERVER_JAR) $(PRIVATE_SERVER_VERSION) 2>&1 || exit 0
	$(hide) $(foreach jack_jar,$(available_jack_jars),$(PRIVATE_JACK_ADMIN) update jack $(jack_jar) $(patsubst $(PRIVATE_PATH)/jacks/jack-%.jar,%,$(jack_jar)) || exit 47;)
endif
