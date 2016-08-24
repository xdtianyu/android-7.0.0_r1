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

/* User include files */
#include "iv_datatypedef.h"
#include "iv.h"
#include "ivd.h"
#include "ithread.h"

#include "impeg2_macros.h"
#include "impeg2_buf_mgr.h"
#include "impeg2_disp_mgr.h"
#include "impeg2_defs.h"
#include "impeg2_platform_macros.h"
#include "impeg2_inter_pred.h"
#include "impeg2_idct.h"
#include "impeg2_format_conv.h"
#include "impeg2_mem_func.h"

#include "impeg2d.h"
#include "impeg2d_bitstream.h"
#include "impeg2d_debug.h"
#include "impeg2d_structs.h"
#include "impeg2d_function_selector.h"

void impeg2d_init_function_ptr(void *pv_codec)
{
    dec_state_t *ps_codec = (dec_state_t *)pv_codec;

    impeg2d_init_function_ptr_generic(pv_codec);
    switch(ps_codec->e_processor_arch)
    {
        case ARCH_X86_GENERIC:
            impeg2d_init_function_ptr_generic(pv_codec);
        break;
        case ARCH_X86_SSSE3:
            impeg2d_init_function_ptr_ssse3(pv_codec);
            break;
        case ARCH_X86_SSE42:
            impeg2d_init_function_ptr_sse42(pv_codec);
        break;
        case ARCH_X86_AVX2:
#ifndef DISABLE_AVX2
            impeg2d_init_function_ptr_avx2(pv_codec);
#else
            impeg2d_init_function_ptr_sse42(pv_codec);
#endif
        break;
        default:
            impeg2d_init_function_ptr_sse42(pv_codec);
        break;
    }
}
void impeg2d_init_arch(void *pv_codec)
{
    dec_state_t *ps_codec = (dec_state_t*) pv_codec;

#ifdef DEFAULT_ARCH
#if DEFAULT_ARCH == D_ARCH_X86_SSE42
    ps_codec->e_processor_arch = ARCH_X86_SSE42;
#elif DEFAULT_ARCH == D_ARCH_X86_SSSE3
    ps_codec->e_processor_arch = ARCH_X86_SSSE3;
#elif DEFAULT_ARCH == D_ARCH_X86_AVX2
    ps_codec->e_processor_arch = D_ARCH_X86_AVX2;
#else
    ps_codec->e_processor_arch = ARCH_X86_GENERIC;
#endif
#else
    ps_codec->e_processor_arch = ARCH_X86_SSE42;
#endif

}
