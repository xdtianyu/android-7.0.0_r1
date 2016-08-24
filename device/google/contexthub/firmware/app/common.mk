#
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

SELF_DIR := $(SELF_MKFILE:Makefile=)
SELF_FILES := $(wildcard $(SELF_DIR)*.c)
APP_NM := $(SELF_DIR)app
CLEANFILES := $(CLEANFILES) $(APP_NM).elf  $(APP_NM).bin
DELIVERABLES := $(DELIVERABLES) $(APP_NM).napp
APP_ELF := $(APP_NM).elf
APP_BIN := $(APP_NM).bin
APP_APP := $(APP_NM).napp
APPFLAGS += $(EXTRA_FLAGS) -Wall -Werror

define APPRULE
$(APP_APP): $(APP_BIN)
	nanoapp_postprocess -v -a $(APP_ID) $(APP_BIN) $(APP_APP)

$(APP_BIN): $(APP_ELF)
	$(OBJCOPY) -j.relocs -j.flash -j.data -j.dynsym -O binary $(APP_ELF) $(APP_BIN)

$(APP_ELF): $(SELF_FILES) symlinks
	$(GCC) -o $(APP_ELF) $(FLAGS) $(APPFLAGS) -fvisibility=hidden $(SELF_FILES)
endef

$(eval $(APPRULE))
