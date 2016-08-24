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

#include <plat/inc/cmsis.h>
#include <plat/inc/plat.h>
#include <syscall.h>
#include <string.h>
#include <seos.h>
#include <heap.h>
#include <cpu.h>


#define HARD_FAULT_DROPBOX_MAGIC_MASK       0xFFFFC000
#define HARD_FAULT_DROPBOX_MAGIC_VAL        0x31414000
#define HARD_FAULT_DROPBOX_MAGIC_HAVE_DROP  0x00002000
#define HARD_FAULT_DROPBOX_MAGIC_DATA_MASK  0x00001FFF

struct RamPersistedDataAndDropbox {
    uint32_t magic; // and part of dropbox
    uint32_t r[16];
    uint32_t sr_hfsr_cfsr_lo;
    uint32_t bits;
    uint32_t RFU;
};

/* //if your device persists ram, you can use this instead:
 * static struct RamPersistedDataAndDropbox* getPersistedData(void)
 * {
 *     static struct RamPersistedDataAndDropbox __attribute__((section(".neverinit"))) dbx;
 *     return &dbx;
 * }
 */

static struct RamPersistedDataAndDropbox* getPersistedData(void)
{
    uint32_t bytes = 0;
    void *loc = platGetPersistentRamStore(&bytes);

    return bytes >= sizeof(struct RamPersistedDataAndDropbox) ? (struct RamPersistedDataAndDropbox*)loc : NULL;
}

static struct RamPersistedDataAndDropbox* getInitedPersistedData(void)
{
    struct RamPersistedDataAndDropbox* dbx = getPersistedData();

    if ((dbx->magic & HARD_FAULT_DROPBOX_MAGIC_MASK) != HARD_FAULT_DROPBOX_MAGIC_VAL) {
        dbx->bits = 0;
        dbx->magic = HARD_FAULT_DROPBOX_MAGIC_VAL;
    }

    return dbx;
}

void cpuInit(void)
{
    /* set SVC to be highest possible priority */
    NVIC_SetPriority(SVCall_IRQn, 0xff);

    /* FPU on */
    SCB->CPACR |= 0x00F00000;
}

//pack all our SR regs into 45 bits
static void cpuPackSrBits(uint32_t *dstLo, uint32_t *dstHi, uint32_t sr, uint32_t hfsr, uint32_t cfsr)
{
    //mask of useful bits:
    // SR:   11111111 00000000 11111101 11111111 (total of 23 bits)
    // HFSR: 01000000 00000000 00000000 00000010 (total of  2 bits)
    // CFSR: 00000011 00001111 10111111 10111111 (total of 20 bits)
    // so our total is 45 bits. we pack this into 2 longs (for now)

    sr   &= 0xFF00FDFF;
    hfsr &= 0x40000002;
    cfsr &= 0x030FBFBF;

    *dstLo = sr | ((cfsr << 4) & 0x00FF0000) | (hfsr >> 12) | (hfsr << 8);
    *dstHi = ((cfsr & 0x01000000) >> 18) | ((cfsr & 0x02000000) >> 13) | (cfsr & 0x00000fff);
}

//unpack the SR bits
static void cpuUnpackSrBits(uint32_t srcLo, uint32_t srcHi, uint32_t *srP, uint32_t *hfsrP, uint32_t *cfsrP)
{
    *srP = srcLo & 0xFF00FDFF;
    *hfsrP = ((srcLo << 12) & 0x40000000) | ((srcLo >> 8) & 0x00000002);
    *cfsrP = ((srcLo & 0x00FB0000) >> 4) | (srcHi & 0x0FBF) | ((srcHi << 13) & 0x02000000) | ((srcHi << 18) & 0x01000000);
}

void cpuInitLate(void)
{
    struct RamPersistedDataAndDropbox *dbx = getInitedPersistedData();

    /* print and clear dropbox */
    if (dbx->magic & HARD_FAULT_DROPBOX_MAGIC_HAVE_DROP) {
        uint32_t i, hfsr, cfsr, sr;

        cpuUnpackSrBits(dbx->sr_hfsr_cfsr_lo, dbx->magic & HARD_FAULT_DROPBOX_MAGIC_DATA_MASK, &sr, &hfsr, &cfsr);

        osLog(LOG_INFO, "Hard Fault Dropbox not empty. Contents:\n");
        for (i = 0; i < 16; i++)
            osLog(LOG_INFO, "  R%02lu  = 0x%08lX\n", i, dbx->r[i]);
        osLog(LOG_INFO, "  SR   = %08lX\n", sr);
        osLog(LOG_INFO, "  HFSR = %08lX\n", hfsr);
        osLog(LOG_INFO, "  CFSR = %08lX\n", cfsr);
    }
    dbx->magic &=~ HARD_FAULT_DROPBOX_MAGIC_HAVE_DROP;
}

bool cpuRamPersistentBitGet(uint32_t which)
{
    struct RamPersistedDataAndDropbox *dbx = getInitedPersistedData();

    return (which < CPU_NUM_PERSISTENT_RAM_BITS) && ((dbx->bits >> which) & 1);
}

void cpuRamPersistentBitSet(uint32_t which, bool on)
{
    struct RamPersistedDataAndDropbox *dbx = getInitedPersistedData();

    if (which < CPU_NUM_PERSISTENT_RAM_BITS) {
        if (on)
            dbx->bits |= (1ULL << which);
        else
            dbx->bits &=~ (1ULL << which);
    }
}

uint64_t cpuIntsOff(void)
{
    uint32_t state;

    asm volatile (
        "mrs %0, PRIMASK    \n"
        "cpsid i            \n"
        :"=r"(state)
    );

    return state;
}

uint64_t cpuIntsOn(void)
{
    uint32_t state;

    asm volatile (
        "mrs %0, PRIMASK    \n"
        "cpsie i            \n"
        :"=r"(state)
    );

    return state;
}

void cpuIntsRestore(uint64_t state)
{

    asm volatile(
        "msr PRIMASK, %0   \n"
        ::"r"((uint32_t)state)
    );
}

static void __attribute__((used)) syscallHandler(uintptr_t *excRegs)
{
    uint16_t *svcPC = ((uint16_t *)(excRegs[6])) - 1;
    uint32_t svcNo = (*svcPC) & 0xFF;
    uint32_t syscallNr = excRegs[0];
    SyscallFunc handler;
    va_list args_long = *(va_list*)(excRegs + 1);
    uintptr_t *fastParams = excRegs + 1;
    va_list args_fast = *(va_list*)(&fastParams);

    if (svcNo > 1)
        osLog(LOG_WARN, "Unknown SVC 0x%02lX called at 0x%08lX\n", svcNo, (unsigned long)svcPC);
    else if (!(handler = syscallGetHandler(syscallNr)))
        osLog(LOG_WARN, "Unknown syscall 0x%08lX called at 0x%08lX\n", (unsigned long)syscallNr, (unsigned long)svcPC);
    else
        handler(excRegs, svcNo ? args_fast : args_long);
}

void SVC_Handler(void);
void __attribute__((naked)) SVC_Handler(void)
{
    asm volatile(
        "tst lr, #4         \n"
        "ite eq             \n"
        "mrseq r0, msp      \n"
        "mrsne r0, psp      \n"
        "b syscallHandler   \n"
    );
}

static void __attribute__((used)) logHardFault(uintptr_t *excRegs, uintptr_t* otherRegs, bool tinyStack)
{
    struct RamPersistedDataAndDropbox *dbx = getInitedPersistedData();
    uint32_t i, hi;

    for (i = 0; i < 4; i++)
        dbx->r[i] = excRegs[i];
    for (i = 0; i < 8; i++)
        dbx->r[i + 4] = otherRegs[i];
    dbx->r[12] = excRegs[4];
    dbx->r[13] = (uint32_t)(excRegs + 8);
    dbx->r[14] = excRegs[5];
    dbx->r[15] = excRegs[6];

    cpuPackSrBits(&dbx->sr_hfsr_cfsr_lo, &hi, excRegs[7], SCB->HFSR, SCB->CFSR);
    dbx->magic |= HARD_FAULT_DROPBOX_MAGIC_HAVE_DROP | (hi & HARD_FAULT_DROPBOX_MAGIC_DATA_MASK);

    if (!tinyStack) {
        osLog(LOG_ERROR, "*HARD FAULT* SR  = %08lX\n", (unsigned long)excRegs[7]);
        osLog(LOG_ERROR, "R0  = %08lX   R8  = %08lX\n", (unsigned long)excRegs[0], (unsigned long)otherRegs[4]);
        osLog(LOG_ERROR, "R1  = %08lX   R9  = %08lX\n", (unsigned long)excRegs[1], (unsigned long)otherRegs[5]);
        osLog(LOG_ERROR, "R2  = %08lX   R10 = %08lX\n", (unsigned long)excRegs[2], (unsigned long)otherRegs[6]);
        osLog(LOG_ERROR, "R3  = %08lX   R11 = %08lX\n", (unsigned long)excRegs[3], (unsigned long)otherRegs[7]);
        osLog(LOG_ERROR, "R4  = %08lX   R12 = %08lX\n", (unsigned long)otherRegs[0], (unsigned long)excRegs[4]);
        osLog(LOG_ERROR, "R5  = %08lX   SP  = %08lX\n", (unsigned long)otherRegs[1], (unsigned long)(excRegs + 8));
        osLog(LOG_ERROR, "R6  = %08lX   LR  = %08lX\n", (unsigned long)otherRegs[2], (unsigned long)excRegs[5]);
        osLog(LOG_ERROR, "R7  = %08lX   PC  = %08lX\n", (unsigned long)otherRegs[3], (unsigned long)excRegs[6]);
        osLog(LOG_ERROR, "HFSR= %08lX   CFSR= %08lX\n", (unsigned long)SCB->HFSR, (unsigned long)SCB->CFSR);
    }

    //reset
    SCB->AIRCR = 0x05FA0004;

    //and in case somehow we do not, loop
    while(1);
}

void HardFault_Handler(void);
static uint32_t  __attribute__((used)) hfStack[16];

void __attribute__((naked)) HardFault_Handler(void)
{
    asm volatile(
        "ldr r3, =__stack_bottom   \n"
        "cmp sp, r3                \n"
        "itte le                   \n"
        "ldrle sp, =hfStack + 64   \n"
        "movle r2, #1              \n"
        "movgt r2, #0              \n"
        "tst lr, #4                \n"
        "ite eq                    \n"
        "mrseq r0, msp             \n"
        "mrsne r0, psp             \n"
        "push  {r4-r11}            \n"
        "mov   r1, sp              \n"
        "b     logHardFault        \n"
    );
}


