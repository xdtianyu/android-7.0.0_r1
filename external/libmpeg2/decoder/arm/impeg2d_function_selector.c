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
*  impeg2d_function_selector.c
*
* @brief
*  Contains functions to initialize function pointers used in mpeg2
*
* @author
*  Naveen
*
* @par List of Functions:
* @remarks
*  None
*
*******************************************************************************
*/
/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "iv_datatypedef.h"
#include "iv.h"

#include "impeg2_buf_mgr.h"
#include "impeg2_disp_mgr.h"
#include "impeg2_defs.h"
#include "impeg2_platform_macros.h"
#include "impeg2_inter_pred.h"
#include "impeg2_idct.h"
#include "impeg2_globals.h"
#include "impeg2_mem_func.h"
#include "impeg2_format_conv.h"
#include "impeg2_macros.h"

#include "ivd.h"
#include "impeg2d.h"
#include "impeg2d_bitstream.h"
#include "impeg2d_structs.h"
#include "impeg2d_vld_tables.h"
#include "impeg2d_vld.h"
#include "impeg2d_pic_proc.h"
#include "impeg2d_debug.h"
#include "impeg2d_mc.h"
#include "impeg2d_function_selector.h"

void impeg2d_init_function_ptr(void *pv_codec)
{
    dec_state_t *ps_codec   = (dec_state_t *)pv_codec;
    IVD_ARCH_T e_proc_arch  = ps_codec->e_processor_arch;

    switch(e_proc_arch)
    {
#if defined(ARMV8)
        case ARCH_ARMV8_GENERIC:
        default:
            impeg2d_init_function_ptr_av8(ps_codec);
            break;
#elif !defined(DISABLE_NEON)
        case ARCH_ARM_A5:
        case ARCH_ARM_A7:
        case ARCH_ARM_A9:
        case ARCH_ARM_A15:
        case ARCH_ARM_A9Q:
        default:
            impeg2d_init_function_ptr_a9q(ps_codec);
            break;
#else
        default:
#endif
        case ARCH_ARM_NONEON:
            impeg2d_init_function_ptr_generic(ps_codec);
            break;
    }
}

void impeg2d_init_arch(void *pv_codec)
{
    dec_state_t *ps_codec = (dec_state_t *)pv_codec;
#ifdef DEFAULT_ARCH
#if DEFAULT_ARCH == D_ARCH_ARM_NONEON
    ps_codec->e_processor_arch = ARCH_ARM_NONEON;
#elif DEFAULT_ARCH == D_ARCH_ARMV8_GENERIC
    ps_codec->e_processor_arch = ARCH_ARMV8_GENERIC;
#elif DEFAULT_ARCH == D_ARCH_ARM_NEONINTR
    ps_codec->e_processor_arch = ARCH_ARM_NEONINTR;
#else
    ps_codec->e_processor_arch = ARCH_ARM_A9Q;
#endif
#else
    ps_codec->e_processor_arch = ARCH_ARM_A9Q;
#endif
}
