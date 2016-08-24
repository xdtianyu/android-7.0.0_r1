/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <plat/inc/bl.h>
#include <platform.h>
#include <stdbool.h>
#include <stdint.h>
#include <mpu.h>
#include <cpu.h>

struct CortexMpu {
    volatile uint32_t CTRL;
    volatile uint32_t RNR;
    volatile uint32_t RBAR;
    volatile uint32_t RASR;
};

#define MPU ((struct CortexMpu*)0xE000ED94UL)

#define MPU_REG_ROM          0
#define MPU_REG_RAM          1
#define MPU_REG_NULL_PAGE    2


/* region type */
#define MPU_TYPE_DEVICE (0x10UL << 16)
#define MPU_TYPE_MEMORY (0x0FUL << 16)

/* region execute priviledges */
#define MPU_BIT_XN      (1UL << 28) /* no execute */

/* region access priviledges */
#define MPU_NA          (0UL << 24) /* S: no access   U: no access */
#define MPU_U_NA_S_RW   (1UL << 24) /* S: RW          U: no access */
#define MPU_U_RO_S_RW   (2UL << 24) /* S: RW          U: RO        */
#define MPU_RW          (3UL << 24) /* S: RW          U: RW        */
#define MPU_U_NA_S_RO   (5UL << 24) /* S: RO          U: no access */
#define MPU_U_RO_S_RO   (6UL << 24) /* S: RO          U: RO        */

/* subregion mask (not used so all ones) */
#define MPU_SRD_BITS    0xFF00UL
#define MPU_BIT_ENABLE  1UL

/* these define rom */
extern uint8_t __shared_end[];
extern uint8_t __ram_start[];
extern uint8_t __ram_end[];

static void mpuRegionCfg(uint32_t regionNo, uint32_t start, uint32_t len, uint32_t attrs) /* region will be rounded to acceptable boundaries (32B minimum, self-aligned) by GROWTH */
{
    uint32_t proposedStart, proposedLen, lenVal = 1;
    uint64_t intState;

    /* expand until it works */
    do {
        /* special case 4GB region */
        if (lenVal == 32) {
            proposedStart = 0;
            break;
        }

        proposedStart = start &~ ((1ULL << lenVal) - 1);
        proposedLen = start + len - proposedStart;
        if (proposedLen < 32)
            proposedLen = 32;
        lenVal = (proposedLen & (proposedLen - 1)) ? 32 - __builtin_clz(proposedLen) : 31 - __builtin_clz(proposedLen);

    } while (proposedStart & ((1ULL << lenVal) - 1));

    intState = cpuIntsOff();
    asm volatile("dsb\nisb");

    MPU->RNR = regionNo;
    MPU->RASR = 0; /* disable region before changing it */
    MPU->RBAR = proposedStart;
    MPU->RASR = MPU_SRD_BITS | MPU_BIT_ENABLE | attrs | (lenVal << 1);

    asm volatile("dsb\nisb");
    cpuIntsRestore(intState);
}

static void mpuCfgRom(bool allowSvcWrite)
{
    mpuRegionCfg(MPU_REG_ROM, (uint32_t)&BL, __shared_end - (uint8_t*)&BL, MPU_TYPE_MEMORY | (allowSvcWrite ? MPU_U_RO_S_RW : MPU_U_RO_S_RO));
}

static void mpuCfgRam(bool allowSvcExecute)
{
    mpuRegionCfg(MPU_REG_RAM, (uint32_t)&__ram_start, __ram_end - __ram_start, MPU_TYPE_MEMORY | MPU_RW | (allowSvcExecute ? 0 : MPU_BIT_XN));
}


void mpuStart(void)
{
    MPU->CTRL = 0x07; //MPU on, even during faults, supervisor default: allow, user default: default deny

    mpuCfgRom(false);
    mpuCfgRam(false);
    mpuRegionCfg(MPU_REG_NULL_PAGE, 0, 4096, MPU_TYPE_MEMORY | MPU_NA | MPU_BIT_XN);
}

void mpuAllowRamExecution(bool allowSvcExecute)
{
    mpuCfgRam(allowSvcExecute);
}

void mpuAllowRomWrite(bool allowSvcWrite)
{
    mpuCfgRom(allowSvcWrite);
}

