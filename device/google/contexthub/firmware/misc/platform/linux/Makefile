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

DELIVERABLES = $(APP).bin
LKR = misc/platform/$(PLATFORM)/linux.extra.lkr

FLAGS += -I. -fno-unwind-tables -fstack-reuse=all -ffunction-sections -fdata-sections -m32
FLAGS += -Wl,-T $(LKR) -Wl,--gc-sections


#platform drivers
SRCS += src/platform/$(PLATFORM)/platform.c \
	src/platform/$(PLATFORM)/i2c.c \
	src/platform/$(PLATFORM)/spi.c \
	src/platform/$(PLATFORM)/rtc.c \
	src/platform/$(PLATFORM)/hostIntf.c

#extra deps
DEPS += $(wildcard inc/platform/$(PLATFORM)/*.h)
DEPS += $(LKR)

#platform flags
FLAGS += -DPLATFORM_HOST_INTF_I2C_BUS=0
FLAGS += -DPLATFORM_HW_TYPE=0x8086 -DPLATFORM_HW_VER=0
FLAGS += -DPLAT_HAS_NO_U_TYPES_H

#platform-specific rules

%.bin : %.elf
	mv $< $@

