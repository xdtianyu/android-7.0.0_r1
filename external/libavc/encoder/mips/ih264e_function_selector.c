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
*  ih264e_function_selector.c
*
* @brief
*  Contains functions to initialize function pointers used in h264
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


/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* System Include Files */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* User Include Files */
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264_defs.h"
#include "ih264_size_defs.h"
#include "ih264e_defs.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ime_distortion_metrics.h"
#include "ime_defs.h"
#include "ime_structs.h"
#include "ih264_error.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ih264_macros.h"
#include "ih264_platform_macros.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"
#include "ih264e_rate_control.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_cabac.h"
#include "ih264e_platform_macros.h"

/**
*******************************************************************************
*
* @brief Initialize the intra/inter/transform/deblk function pointers of
* codec context
*
* @par Description: the current routine initializes the function pointers of
* codec context basing on the architecture in use
*
* @param[in] ps_codec
*  Codec context pointer
*
* @returns  none
*
* @remarks none
*
*******************************************************************************
*/

void ih264e_init_function_ptr(void *pv_codec)
{
    codec_t *ps_codec = (codec_t *)pv_codec;
    ih264e_init_function_ptr_generic(ps_codec);
}

IV_ARCH_T ih264e_default_arch(void)
{
    return ARCH_NA;
}

