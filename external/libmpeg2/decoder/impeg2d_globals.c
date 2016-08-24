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
#include <stdio.h>
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

#include "impeg2d.h"
#include "impeg2d_bitstream.h"
#include "impeg2d_structs.h"
#include "impeg2d_globals.h"
#include "impeg2d_mc.h"

/*****************************************************************************/
/* MC params table                                                           */
/*****************************************************************************/
const mc_type_consts_t gas_impeg2d_mc_params_luma[][2] =
{
    /* frame prediction in P frame picture */
    {{1,0,1,1,MB_SIZE,MB_SIZE,0},
     {1,0,1,1,MB_SIZE,MB_SIZE,0}},
    /* field prediction in P frame picture */
    {{2,0,1,2,MB_SIZE/2,MB_SIZE*2,0},
     {2,0,1,2,MB_SIZE/2,MB_SIZE*2,1}},
    /* frame prediction in B frame picture */
    {{1,0,1,1,MB_SIZE,MB_SIZE,0},
     {1,0,1,1,MB_SIZE,MB_SIZE,0}},
    /* field prediction in B frame picture */
    {{2,0,1,2,MB_SIZE/2,MB_SIZE*2,0},
     {2,0,1,2,MB_SIZE/2,MB_SIZE*2,1}},
    /* dual prime prediction in P frame picture */
    {{2,0,1,2,MB_SIZE/2,MB_SIZE*2,0},
     {2,0,1,2,MB_SIZE/2,MB_SIZE*2,1}},

    /* field prediction in P field picture */
    {{1,0,2,2,MB_SIZE,MB_SIZE,0},{1,0,2,2,MB_SIZE,MB_SIZE,0}},
    /* 16x8 prediction in P field picture */
    {{1,0,2,2,MB_SIZE/2,MB_SIZE,0},{1,8,2,2,MB_SIZE/2,MB_SIZE,(1*MB_SIZE/2)}},
    /* field prediction in B field picture */
    {{1,0,2,2,MB_SIZE,MB_SIZE,0},{1,0,2,2,MB_SIZE,MB_SIZE,0}},
    /* 16x8 prediction in B field picture */
    {{1,0,2,2,MB_SIZE/2,MB_SIZE,0},{1,8,2,2,MB_SIZE/2,MB_SIZE,(1*MB_SIZE/2)}},
    /* dual prime prediction in P field picture */
    {{1,0,2,2,MB_SIZE,MB_SIZE,0},{1,0,2,2,MB_SIZE,MB_SIZE,0}}

};

const mc_type_consts_t gas_impeg2d_mc_params_chroma[10][2] =
{
    /* frame prediction in P frame picture */
    {{1,0,1,1,MB_CHROMA_SIZE,MB_CHROMA_SIZE,0},{1,0,1,1,MB_CHROMA_SIZE,MB_CHROMA_SIZE,0}},
    /* field prediction in P frame picture */
    {{2,0,1,2,MB_CHROMA_SIZE/2,MB_CHROMA_SIZE*2,0},{2,0,1,2,MB_CHROMA_SIZE/2,
    MB_CHROMA_SIZE*2,1}},
    /* frame prediction in B frame picture */
    {{1,0,1,1,MB_CHROMA_SIZE,MB_CHROMA_SIZE,0},{1,0,1,1,MB_CHROMA_SIZE,
    MB_CHROMA_SIZE,0}},
    /* field prediction in B frame picture */
    {{2,0,1,2,MB_CHROMA_SIZE/2,MB_CHROMA_SIZE*2,0},{2,0,1,2,MB_CHROMA_SIZE/2,
    MB_CHROMA_SIZE*2,1}},
    /* dual prime prediction in P frame picture */
    {{2,0,1,2,MB_CHROMA_SIZE/2,MB_CHROMA_SIZE*2,0},{2,0,1,2,MB_CHROMA_SIZE/2,
    MB_CHROMA_SIZE*2,1}},

    /* field prediction in P field picture */
    {{1,0,2,2,MB_CHROMA_SIZE,MB_CHROMA_SIZE,0},{1,0,2,2,MB_CHROMA_SIZE,
    MB_CHROMA_SIZE,0}},
    /* 16x8 prediction in P field picture */
    {{1,0,2,2,MB_CHROMA_SIZE/2,MB_CHROMA_SIZE,0},{1,4,2,2,MB_CHROMA_SIZE/2,
    MB_CHROMA_SIZE,(1*MB_CHROMA_SIZE/2)}},
    /* field prediction in B field picture */
    {{1,0,2,2,MB_CHROMA_SIZE,MB_CHROMA_SIZE,0},{1,0,2,2,MB_CHROMA_SIZE,
    MB_CHROMA_SIZE,0}},
    /* 16x8 prediction in B field picture */
    {{1,0,2,2,MB_CHROMA_SIZE/2,MB_CHROMA_SIZE,0},{1,4,2,2,MB_CHROMA_SIZE/2,
    MB_CHROMA_SIZE,(1*MB_CHROMA_SIZE/2)}},
    /* dual prime prediction in P field picture */
    {{1,0,2,2,MB_CHROMA_SIZE,MB_CHROMA_SIZE,0},{1,0,2,2,MB_CHROMA_SIZE,
    MB_CHROMA_SIZE,0}}

};

/*****************************************************************************/
/* MC function pointer table                                                 */
/*****************************************************************************/
const dec_mb_params_t gas_impeg2d_func_frm_fw_or_bk[4] =
{
    /*0MV*/
    {impeg2d_dec_1mv_mb,MC_FRM_FW_OR_BK_1MV,impeg2d_mc_1mv},
    /* motion_type Field based */
    {impeg2d_dec_2mv_fw_or_bk_mb,MC_FRM_FW_OR_BK_2MV,impeg2d_mc_fw_or_bk_mb},
    /* motion_type Frame based */
    {impeg2d_dec_1mv_mb,MC_FRM_FW_OR_BK_1MV,impeg2d_mc_1mv},
    /* motion_type Dual prime based */
    {impeg2d_dec_frm_dual_prime,MC_FRM_FW_DUAL_PRIME_1MV,impeg2d_mc_frm_dual_prime},
};

const dec_mb_params_t gas_impeg2d_func_fld_fw_or_bk[4] =
{
    /*0MV*/
    {impeg2d_dec_1mv_mb,MC_FRM_FW_OR_BK_1MV,impeg2d_mc_1mv},
    /* motion_type Field based */
    {impeg2d_dec_1mv_mb,MC_FLD_FW_OR_BK_1MV,impeg2d_mc_1mv},
    /* motion_type 16x8 MC */
    {impeg2d_dec_2mv_fw_or_bk_mb,MC_FLD_FW_OR_BK_2MV,impeg2d_mc_fw_or_bk_mb},
    /* motion_type Dual prime based */
    {impeg2d_dec_fld_dual_prime,MC_FLD_FW_DUAL_PRIME_1MV,impeg2d_mc_fld_dual_prime},
};


const dec_mb_params_t gas_impeg2d_func_frm_bi_direct[4] =
{
    {NULL,MC_FRM_FW_OR_BK_1MV,NULL},
    /* motion_type Field based */
    {impeg2d_dec_4mv_mb,MC_FRM_FW_AND_BK_4MV,impeg2d_mc_4mv},
    /* motion_type Frame based */
    {impeg2d_dec_2mv_interp_mb,MC_FRM_FW_AND_BK_2MV,impeg2d_mc_2mv},
    /* Reserved not applicable */
    {NULL,MC_FRM_FW_OR_BK_1MV,NULL},
};

const dec_mb_params_t gas_impeg2d_func_fld_bi_direct[4] =
{
    {NULL,MC_FRM_FW_OR_BK_1MV,NULL},
    /* motion_type Field based */
    {impeg2d_dec_2mv_interp_mb,MC_FLD_FW_AND_BK_2MV,impeg2d_mc_2mv},
    /* motion_type 16x8 MC */
    {impeg2d_dec_4mv_mb,MC_FLD_FW_AND_BK_4MV,impeg2d_mc_4mv},
    /* Reserved not applicable */
    {NULL,MC_FRM_FW_OR_BK_1MV,NULL},
};
