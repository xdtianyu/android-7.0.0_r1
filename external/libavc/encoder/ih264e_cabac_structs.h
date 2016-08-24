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
 *  ih264e_cabac_structs.h
 *
 * @brief
 *  This file contains cabac related structure definitions.
 *
 * @author
 *  Doney Alex
 *
 * @remarks
 *  none
 *
 *******************************************************************************
 */

#ifndef IH264E_CABAC_STRUCTS_H_
#define IH264E_CABAC_STRUCTS_H_



#define CABAC_INIT_IDC 2


/**
 ******************************************************************************
 *  @brief     typedef for  context model
 ******************************************************************************
 */

/* bits 0 to 5 :state
   bit 6       :mps */
typedef UWORD8 bin_ctxt_model;

/**
 ******************************************************************************
 *  @brief      MB info for cabac
 ******************************************************************************
 */
typedef struct
{
    /* Neighbour availability Variables needed to get CtxtInc, for CABAC */
    UWORD8 u1_mb_type; /* !< macroblock type: I/P/B/SI/SP */

    UWORD8 u1_cbp; /* !< Coded Block Pattern */
    UWORD8 u1_intrapred_chroma_mode;

    /*************************************************************************/
    /*               Arrangnment of AC CSBP                                  */
    /*        bits:  b7 b6 b5 b4 b3 b2 b1 b0                                 */
    /*        CSBP:  V1 V0 U1 U0 Y3 Y2 Y1 Y0                                 */
    /*************************************************************************/
    UWORD8 u1_yuv_ac_csbp;
    /*************************************************************************/
    /*               Arrangnment of DC CSBP                                  */
    /*        bits:  b7  b6  b5  b4  b3  b2  b1  b0                          */
    /*        CSBP:   x   x   x   x   x  Vdc Udc Ydc                         */
    /*************************************************************************/
    UWORD8 u1_yuv_dc_csbp;

    WORD8 i1_ref_idx[4];
    UWORD8 u1_mv[4][4];
} mb_info_ctxt_t;


/**
 ******************************************************************************
 *  @brief      CSBP info for CABAC
 ******************************************************************************
 */
typedef struct
{
    /*************************************************************************/
    /*               Arrangnment of Luma AC CSBP for leftMb                  */
    /*        bits:  b7 b6 b5 b4 b3 b2 b1 b0                                 */
    /*        CSBP:   X  X  X  X Y3 Y2 Y1 Y0                                 */
    /*************************************************************************/
    /*************************************************************************/
    /*  Points either to u1_y_ac_csbp_top_mb or  u1_y_ac_csbp_bot_mb         */
    /*************************************************************************/
    UWORD8 u1_y_ac_csbp_top_mb;
    UWORD8 u1_y_ac_csbp_bot_mb;

    /*************************************************************************/
    /*               Arrangnment of Chroma AC CSBP for leftMb                */
    /*        bits:  b7 b6 b5 b4 b3 b2 b1 b0                                 */
    /*        CSBP:   X  X  X  X V1 V0 U1 U0                                 */
    /*************************************************************************/
    /*************************************************************************/
    /*  Points either to u1_uv_ac_csbp_top_mb or  u1_uv_ac_csbp_bot_mb       */
    /*************************************************************************/
    UWORD8 u1_uv_ac_csbp_top_mb;
    UWORD8 u1_uv_ac_csbp_bot_mb;

    /*************************************************************************/
    /*               Arrangnment of DC CSBP                                  */
    /*        bits:  b7  b6  b5  b4  b3  b2  b1  b0                          */
    /*        CSBP:   x   x   x   x   x  Vdc Udc Ydc                         */
    /*************************************************************************/
    /*************************************************************************/
    /*  Points either to u1_yuv_dc_csbp_top_mb or  u1_yuv_dc_csbp_bot_mb     */
    /*************************************************************************/
    UWORD8 u1_yuv_dc_csbp_top_mb;
    UWORD8 u1_yuv_dc_csbp_bot_mb;
} cab_csbp_t;

/**
 ******************************************************************************
 *  @brief      CABAC Encoding Environment
 ******************************************************************************
 */

typedef struct
{
    /** cabac interval start L  */
    UWORD32 u4_code_int_low;

    /** cabac interval range R  */
    UWORD32 u4_code_int_range;

    /** bytes_outsanding; number of 0xFF bits that occur during renorm
    *  These  will be accumulated till the carry bit is knwon
    */
    UWORD32  u4_out_standing_bytes;

    /** bits generated during renormalization
    *   A byte is put to stream/u4_out_standing_bytes from u4_low(L) when
    *   u4_bits_gen exceeds 8
    */
    UWORD32  u4_bits_gen;
} encoding_envirnoment_t;


/**
 ******************************************************************************
 *  @brief      CABAC Context structure : Variables to handle Cabac
 ******************************************************************************
 */
typedef struct
{

    /*  Base pointer to all the cabac contexts  */
    bin_ctxt_model au1_cabac_ctxt_table[NUM_CABAC_CTXTS];


    cab_csbp_t s_lft_csbp;

    /**
     * pointer to Bitstream structure
     */
    bitstrm_t *ps_bitstrm;

    /* Pointer to mb_info_ctxt_t map_base */
    mb_info_ctxt_t *ps_mb_map_ctxt_inc_base;

    /* Pointer to encoding_envirnoment_t */
    encoding_envirnoment_t s_cab_enc_env;

    /* These things need to be updated at each MbLevel */

    /* Prev ps_mb_qp_delta_ctxt */
    WORD8 i1_prevps_mb_qp_delta_ctxt;

    /* Pointer to mb_info_ctxt_t map */
    mb_info_ctxt_t *ps_mb_map_ctxt_inc;

    /* Pointer to default mb_info_ctxt_t */
    mb_info_ctxt_t *ps_def_ctxt_mb_info;

    /* Pointer to current mb_info_ctxt_t */
    mb_info_ctxt_t *ps_curr_ctxt_mb_info;

    /* Pointer to left mb_info_ctxt_t */
    mb_info_ctxt_t *ps_left_ctxt_mb_info;

    /* Pointer to top mb_info_ctxt_t  */
    mb_info_ctxt_t *ps_top_ctxt_mb_info;

    /* Poniter to left csbp structure */
    cab_csbp_t *ps_lft_csbp;
    UWORD8 *pu1_left_y_ac_csbp;
    UWORD8 *pu1_left_uv_ac_csbp;
    UWORD8 *pu1_left_yuv_dc_csbp;

    /***************************************************************************/
    /*       Ref_idx contexts  are stored in the following way                 */
    /*  Array Idx 0,1 for reference indices in Forward direction               */
    /*  Array Idx 2,3 for reference indices in backward direction              */
    /***************************************************************************/
    /* Dimensions for u1_left_ref_ctxt_inc_arr is [2][4] for Mbaff:Top and Bot */
    WORD8 i1_left_ref_idx_ctx_inc_arr[2][4];
    WORD8 *pi1_left_ref_idx_ctxt_inc;

    /* Dimensions for u1_left_mv_ctxt_inc_arr is [2][4][4] for Mbaff case */
    UWORD8 u1_left_mv_ctxt_inc_arr[2][4][4];
    UWORD8 (*pu1_left_mv_ctxt_inc)[4];

} cabac_ctxt_t;

#endif /* IH264E_CABAC_STRUCTS_H_ */
