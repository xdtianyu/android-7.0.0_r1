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

#variant makefile for lunchbox

ifneq ($(PLATFORM),stm32f4xx)
        $(error "LUNCHBOX variant cannot be build on a platform that is not stm32f411")
endif

FLAGS += -DPLATFORM_HW_TYPE=0x4C75   #'Lu' -> lunchbox
FLAGS += -DDEBUG_UART_PIN=16         #GPIOB0 is debug uart at 2MBps


#board configuration shims
SRCS_os += links/variant/src/i2c.c
SRCS_os += links/variant/src/spi.c

#keys

#drivers
# ROHM ALS/prox
SRCS_os += src/drivers/rohm_rpr0521/rohm_rpr0521.c

# Fusion algorithms
SRCS_os += src/algos/fusion.c src/algos/mat.c src/algos/quat.c src/algos/vec.c

# BMI160 accel and gyro, BMM150 mag drivers
FLAGS += -DUSE_BMM150 -DMAG_SLAVE_PRESENT
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

DEPS += $(wildcard links/variant/inc/*.h)

#linker script
LKR_os = misc/variant/common/stm32f411.os.lkr
LKR_bl = misc/variant/common/stm32f411.bl.lkr
OSFLAGS_os += -Wl,-T $(LKR_os)
OSFLAGS_bl += -Wl,-T $(LKR_bl)
DEPS += $(LKR_os) $(LKR_bl)
