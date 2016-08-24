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

GCC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

FLAGS += -mthumb -mcpu=cortex-m4 -march=armv7e-m -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -mno-thumb-interwork -ffast-math -fsingle-precision-constant -DARM -DUSE_NANOHUB_FLOAT_RUNTIME
APPFLAGS += -Wl,-T misc/cpu/$(CPU)/app.lkr -mno-pic-data-is-text-relative

#defines
FLAGS += -DCPU_NUM_PERSISTENT_RAM_BITS=32

#cpu runtime
SRCS_os += src/cpu/$(CPU)/atomicBitset.c src/cpu/$(CPU)/cpu.c src/cpu/$(CPU)/pendsv.c src/cpu/$(CPU)/atomic.c src/cpu/$(CPU)/appSupport.c src/cpu/$(CPU)/cpuMath.c

#cpu runtime for bootloader
SRCS_bl += src/cpu/$(CPU)/cpu.c

#floating point runtime (ARM)
SRCS_os += external/arm/arm_cos_f32.c external/arm/arm_sin_f32.c external/arm/arm_common_tables.c
FLAGS += -DARM_MATH_CM4 -D__FPU_PRESENT

#floating point runtime (FreeBSD)
SRCS_os += external/freebsd/lib/msun/src/e_atan2f.c external/freebsd/lib/msun/src/e_expf.c external/freebsd/lib/msun/src/s_atanf.c
FLAGS += -DFLT_EVAL_METHOD

#extra deps
DEPS += $(wildcard inc/cpu/$(CPU)/*.h)

#bad words for C-M4F
BADWORDS += "__floatundisf=When casting a uint64_t to float, use floatFromUint64"
BADWORDS += "__floatdisf=When casting a int64_t to float, use floatFromInt64"
BADWORDS += "__fixunssfdi=When casting a float to a uint64_t, use floatToUint64"
BADWORDS += "__fixsfdi=When casting a float to a int64_t, use floatToInt64"
BADWORDS += "__aeabi_uldivmod=Do not ever divide uint64_t by anything, see cpuMath.h"
BADWORDS += "__aeabi_ldivmod=Do not ever divide int64_t by anything, see cpuMath.h"
BADWORDS += "sinf=include nanhub_math.h before using sinf()"
BADWORDS += "cosf=include nanhub_math.h before using cosf()"
BADWORDS += "asinf=include nanhub_math.h before using asinf()"

#all softfloat double funcs are forbidden
BADWORDS += __muldf3 __divdf3 __subdf3 __adddf3 __truncdfsf2




