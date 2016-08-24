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

#variant makefile for nucleo


ifneq ($(PLATFORM),stm32f4xx)
        $(error "NUCLEO variant cannot be build on a platform that is not stm32f411")
endif

FLAGS += -DPLATFORM_HW_TYPE=0x4E75   #'Nu' ->nucleo
FLAGS += -DDEBUG_SWD

#board configuration shims
SRCS_os += src/variant/$(VARIANT)/i2c.c
SRCS_os += src/variant/$(VARIANT)/spi.c

#keys

#drivers
# AMS ALS/prox
SRCS_os += src/drivers/ams_tmd2772/ams_tmd2772.c

# ROHM ALS/prox
SRCS_os += src/drivers/rohm_rpr0521/rohm_rpr0521.c

# Fusion algorithm
SRCS_os += src/algos/fusion.c src/algos/mat.c src/algos/quat.c src/algos/vec.c

# BMI160 accel and gyro, BMM150 mag drivers
FLAGS += -DUSE_BMM150
SRCS_os += src/drivers/bosch_bmi160/bosch_bmi160.c \
	src/drivers/bosch_bmi160/bosch_bmm150_slave.c \
	src/algos/mag_cal.c \
	src/algos/time_sync.c

# Orientation sensor driver
SRCS_os += src/drivers/orientation/orientation.c

# Window orientation sensor driver
SRCS_os += src/drivers/window_orientation/window_orientation.c

# Bosch BMP280 Barometer/Temperature
SRCS_os += src/drivers/bosch_bmp280/bosch_bmp280.c

# Hall effect sensor driver
SRCS_os += src/drivers/hall/hall.c

# Camera Vsync driver
SRCS_os += src/drivers/vsync/vsync.c

# Tilt detection
SRCS_os += src/drivers/tilt_detection/tilt_detection.c

#linker script
LKR_os = misc/variant/common/stm32f411.os.lkr
LKR_bl = misc/variant/common/stm32f411.bl.lkr
OSFLAGS_os += -Wl,-T $(LKR_os)
OSFLAGS_bl += -Wl,-T $(LKR_bl)
DEPS += $(LKR_os) $(LKR_bl)
DEPS += $(LKR)
