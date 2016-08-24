/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
*/
/**
*******************************************************************************
* @file
*  icv.h
*
* @brief
*  This header files contains all the common definitions
*
* @author
*  Ittiam
*
* @par List of Functions:
*
* @remarks
*  None
*
*******************************************************************************
*/
#ifndef __ICV_H__
#define __ICV_H__

/** Color formats */
typedef enum
{
    /** Dummy candidate */
    ICV_COLOR_NA     = 0x7FFFFFFF,

    /** YUV 420 Planar */
    ICV_YUV420P      = 0,

    /** YUV 420 Semi Planar UV*/
    ICV_YUV420SP_UV,

    /** YUV 420 Semi Planar VU*/
    ICV_YUV420SP_VU,

}ICV_COLOR_FMT_T;

/** Architecture Enumeration                               */
typedef enum
{
    ICV_ARCH_NA            =   0x7FFFFFFF,
    ICV_ARM_NONEON         =   0x0,
    ICV_ARM_NEONINTR,
    ICV_ARM_A9Q,
    ICV_ARM_A9A,
    ICV_ARM_A9,
    ICV_ARM_A7,
    ICV_ARM_A5,
    ICV_ARM_A15,
    ICV_ARMV8_GENERIC       = 0x100,
    ICV_ARM_A53,
    ICV_ARM_A57,
    ICV_X86_GENERIC         = 0x1000,
    ICV_X86_SSSE3,
    ICV_X86_SSE42,
    ICV_X86_AVX,
    ICV_X86_AVX2,
    ICV_MIPS_GENERIC        = 0x2000,
    ICV_MIPS_32,
}ICV_ARCH_T;

/** SOC Enumeration                               */
typedef enum
{
    ICV_SOC_NA              = 0x7FFFFFFF,
    ICV_SOC_GENERIC         = 0x0,
}ICV_SOC_T;


/** Max Color components */
#define MAX_COMPONENTS 4

/** Structure to define a picture */
typedef struct
{
    /** Buffer address */
    UWORD8 *apu1_buf[MAX_COMPONENTS];

    /** Width */
    WORD32 ai4_wd[MAX_COMPONENTS];

    /** Height */
    WORD32 ai4_ht[MAX_COMPONENTS];

    /** Stride */
    WORD32 ai4_strd[MAX_COMPONENTS];

    /** Color Format */
    ICV_COLOR_FMT_T e_color_fmt;

}icv_pic_t;


#endif  /* __ICV_H__ */
