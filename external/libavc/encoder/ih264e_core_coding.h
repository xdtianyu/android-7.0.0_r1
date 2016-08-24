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
******************************************************************************
* @file
*  ih264e_core_coding.h
*
* @brief
*  This file contains extern declarations of core coding routines
*
* @author
*  ittiam
*
* @remarks
*  none
******************************************************************************
*/

#ifndef IH264E_CORE_CODING_H_
#define IH264E_CORE_CODING_H_

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief      Enable/Disable Hadamard transform of DC Coeff's
******************************************************************************
 */
#define DISABLE_DC_TRANSFORM 0
#define ENABLE_DC_TRANSFORM 1

/**
*******************************************************************************
 *  @brief bit masks for DC and AC control flags
*******************************************************************************
 */

#define DC_COEFF_CNT_LUMA_MB        16
#define NUM_4X4_BLKS_LUMA_MB_ROW    4
#define NUM_LUMA4x4_BLOCKS_IN_MB    16
#define NUM_CHROMA4x4_BLOCKS_IN_MB  8

#define SIZE_4X4_BLK_HRZ            TRANS_SIZE_4
#define SIZE_4X4_BLK_VERT           TRANS_SIZE_4

#define CNTRL_FLAG_DC_MASK_LUMA     0x0000FFFF
#define CNTRL_FLAG_AC_MASK_LUMA     0xFFFF0000

#define CNTRL_FLAG_AC_MASK_CHROMA_U 0xF0000000
#define CNTRL_FLAG_DC_MASK_CHROMA_U 0x0000F000

#define CNTRL_FLAG_AC_MASK_CHROMA_V 0x0F000000
#define CNTRL_FLAG_DC_MASK_CHROMA_V 0x00000F00

#define CNTRL_FLAG_AC_MASK_CHROMA   ( CNTRL_FLAG_AC_MASK_CHROMA_U | CNTRL_FLAG_AC_MASK_CHROMA_V )
#define CNTRL_FLAG_DC_MASK_CHROMA   ( CNTRL_FLAG_DC_MASK_CHROMA_U | CNTRL_FLAG_DC_MASK_CHROMA_V )

#define CNTRL_FLAG_DCBLK_MASK_CHROMA 0x0000C000

/**
*******************************************************************************
 *  @brief macros for transforms
*******************************************************************************
 */
#define DEQUEUE_BLKID_FROM_CONTROL( u4_cntrl,  blk_lin_id)                     \
{                                                                              \
  blk_lin_id = CLZ(u4_cntrl);                                                  \
  u4_cntrl &= (0x7FFFFFFF >> blk_lin_id);                                      \
};

#define IND2SUB_LUMA_MB(u4_blk_id,i4_offset_x,i4_offset_y)                      \
{                                                                               \
     i4_offset_x = (u4_blk_id % 4) << 2;                                        \
     i4_offset_y = (u4_blk_id / 4) << 2;                                        \
}

#define IND2SUB_CHROMA_MB(u4_blk_id,i4_offset_x,i4_offset_y)                   \
{                                                                              \
     i4_offset_x = ((u4_blk_id & 0x1 ) << 3) + (u4_blk_id > 3);                \
     i4_offset_y = (u4_blk_id & 0x2) << 1;                                     \
}


/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief
*  This function performs does the DCT transform then Hadamard transform
*  and quantization for a macroblock when the mb mode is intra 16x16 mode
*
* @par Description:
*  First  cf4 is done on all 16 4x4 blocks of the 16x16 input block.
*  Then hadamard transform is done on the DC coefficients
*  Quantization is then performed on the 16x16 block, 4x4 wise
*
* @param[in] pu1_src
*  Pointer to source sub-block
*
* @param[in] pu1_pred
*  Pointer to prediction sub-block
*
* @param[in] pi2_out
*  Pointer to residual sub-block
*  The output will be in linear format
*  The first 16 continuous locations will contain the values of Dc block
*  After DC block and a stride 1st AC block will follow
*  After one more stride next AC block will follow
*  The blocks will be in raster scan order
*
* @param[in] src_strd
*  Source stride
*
* @param[in] pred_strd
*  Prediction stride
*
* @param[in] dst_strd
*  Destination stride
*
* @param[in] pu2_scale_matrix
*  The quantization matrix for 4x4 transform
*
* @param[in] pu2_threshold_matrix
*  Threshold matrix
*
* @param[in] u4_qbits
*  15+QP/6
*
* @param[in] u4_round_factor
*  Round factor for quant
*
* @param[out] pu1_nnz
*  Memory to store the non-zeros after transform
*  The first byte will be the nnz of DC block
*  From the next byte the AC nnzs will be stored in raster scan order
*
* @param u4_dc_flag
*  Signals if Dc transform is to be done or not
*   1 -> Dc transform will be done
*   0 -> Dc transform will not be done
*
* @remarks
*
*******************************************************************************
*/
void ih264e_luma_16x16_resi_trans_dctrans_quant(
                codec_t *ps_codec, UWORD8 *pu1_src, UWORD8 *pu1_pred,
                WORD16 *pi2_out, WORD32 src_strd, WORD32 pred_strd,
                WORD32 dst_strd, const UWORD16 *pu2_scale_matrix,
                const UWORD16 *pu2_threshold_matrix, UWORD32 u4_qbits,
                UWORD32 u4_round_factor, UWORD8 *pu1_nnz, UWORD32 u4_dc_flag);

/**
*******************************************************************************
*
* @brief
*  This function performs the intra 16x16 inverse transform process for H264
*  it includes inverse Dc transform, inverse quant and then inverse transform
*
* @par Description:
*
* @param[in] pi2_src
*  Input data, 16x16 size
*  First 16 mem locations will have the Dc coffs in rater scan order in linear fashion
*  after a stride 1st AC clock will be present again in raster can order
*  Then each AC block of the 16x16 block will follow in raster scan order
*
* @param[in] pu1_pred
*  The predicted data, 16x16 size
*  Block by block form
*
* @param[in] pu1_out
*  Output 16x16
*  In block by block form
*
* @param[in] src_strd
*  Source stride
*
* @param[in] pred_strd
*  input stride for prediction buffer
*
* @param[in] out_strd
*  input stride for output buffer
*
* @param[in] pu2_iscale_mat
*  Inverse quantization matrix for 4x4 transform
*
* @param[in] pu2_weigh_mat
*  weight matrix of 4x4 transform
*
* @param[in] qp_div
*  QP/6
*
* @param[in] pi4_tmp
*  Input temporary buffer
*  needs to be at least 20 in size
*
* @param[in] pu4_cntrl
*  Controls the transform path
*  total Last 17 bits are used
*  the 16th th bit will correspond to DC block
*  and 32-17 will correspond to the ac blocks in raster scan order
*  bit equaling zero indicates that the entire 4x4 block is zero for DC
*  For AC blocks a bit equaling zero will mean that all 15 AC coffs of the block is nonzero
*
* @param[in] pi4_tmp
*  Input temporary buffer
*  needs to be at least COFF_CNT_SUB_BLK_4x4+COFF_CNT_SUB_BLK_4x4 size
*
* @returns
*  none
*
* @remarks
*  The all zero case must be taken care outside
*
*******************************************************************************
*/
void ih264e_luma_16x16_idctrans_iquant_itrans_recon(
                codec_t *ps_codec, WORD16 *pi2_src, UWORD8 *pu1_pred,
                UWORD8 *pu1_out, WORD32 src_strd, WORD32 pred_strd,
                WORD32 out_strd, const UWORD16 *pu2_iscale_mat,
                const UWORD16 *pu2_weigh_mat, UWORD32 qp_div, UWORD32 u4_cntrl,
                UWORD32 u4_dc_trans_flag, WORD32 *pi4_tmp);

/**
*******************************************************************************
*
* @brief
*  This function performs does the DCT transform then Hadamard transform
*  and quantization for a chroma macroblock
*
* @par Description:
*  First  cf4 is done on all 16 4x4 blocks of the 8x8input block
*  Then hadamard transform is done on the DC coefficients
*  Quantization is then performed on the 8x8 block, 4x4 wise
*
* @param[in] pu1_src
*  Pointer to source sub-block
*  The input is in interleaved format for two chroma planes
*
* @param[in] pu1_pred
*  Pointer to prediction sub-block
*  Prediction is in inter leaved format
*
* @param[in] pi2_out
*  Pointer to residual sub-block
*  The output will be in linear format
*  The first 4 continuous locations will contain the values of DC block for U
*  and then next 4 will contain for V.
*  After DC block and a stride 1st AC block of U plane will follow
*  After one more stride next AC block of V plane will follow
*  The blocks will be in raster scan order
*
*  After all the AC blocks of U plane AC blocks of V plane will follow in exact
*  same way
*
* @param[in] src_strd
*  Source stride
*
* @param[in] pred_strd
*  Prediction stride
*
* @param[in] dst_strd
*  Destination stride
*
* @param[in] pu2_scale_matrix
*  The quantization matrix for 4x4 transform
*
* @param[in] pu2_threshold_matrix
*  Threshold matrix
*
* @param[in] u4_qbits
*  15+QP/6
*
* @param[in] u4_round_factor
*  Round factor for quant
*
* @param[out] pu1_nnz
*  Memory to store the non-zeros after transform
*  The first byte will be the nnz od DC block for U plane
*  From the next byte the AC nnzs will be storerd in raster scan order
*  The fifth byte will be nnz of Dc block of V plane
*  Then Ac blocks will follow
*
* @param u4_dc_flag
*  Signals if Dc transform is to be done or not
*   1 -> Dc transform will be done
*   0 -> Dc transform will not be done
*
* @remarks
*
*******************************************************************************
*/
void ih264e_chroma_8x8_resi_trans_dctrans_quant(
                codec_t *ps_codec, UWORD8 *pu1_src, UWORD8 *pu1_pred,
                WORD16 *pi2_out, WORD32 src_strd, WORD32 pred_strd,
                WORD32 out_strd, const UWORD16 *pu2_scale_matrix,
                const UWORD16 *pu2_threshold_matrix, UWORD32 u4_qbits,
                UWORD32 u4_round_factor, UWORD8 *pu1_nnz_c);

/**
*******************************************************************************
* @brief
*  This function performs the inverse transform with process for chroma MB of H264
*
* @par Description:
*  Does inverse DC transform ,inverse quantization inverse transform
*
* @param[in] pi2_src
*  Input data, 16x16 size
*  The input is in the form of, first 4 locations will contain DC coeffs of
*  U plane, next 4 will contain DC coeffs of V plane, then AC blocks of U plane
*  in raster scan order will follow, each block as linear array in raster scan order.
*  After a stride next AC block will follow. After all AC blocks of U plane
*  V plane AC blocks will follow in exact same order.
*
* @param[in] pu1_pred
*  The predicted data, 8x16 size, U and V interleaved
*
* @param[in] pu1_out
*  Output 8x16, U and V interleaved
*
* @param[in] src_strd
*  Source stride
*
* @param[in] pred_strd
*  input stride for prediction buffer
*
* @param[in] out_strd
*  input stride for output buffer
*
* @param[in] pu2_iscale_mat
*  Inverse quantization martix for 4x4 transform
*
* @param[in] pu2_weigh_mat
*  weight matrix of 4x4 transform
*
* @param[in] qp_div
*  QP/6
*
* @param[in] pi4_tmp
*  Input temporary buffer
*  needs to be at least COFF_CNT_SUB_BLK_4x4 + Number of Dc cofss for chroma * number of planes
*  in size
*
* @param[in] pu4_cntrl
*  Controls the transform path
*  the 15 th bit will correspond to DC block of U plane , 14th will indicate the V plane Dc block
*  32-28 bits will indicate AC blocks of U plane in raster scan order
*  27-23 bits will indicate AC blocks of V plane in rater scan order
*  The bit 1 implies that there is at least one non zero coff in a block
*
* @returns
*  none
*
* @remarks
*******************************************************************************
*/
void ih264e_chroma_8x8_idctrans_iquant_itrans_recon(
                codec_t *ps_codec, WORD16 *pi2_src, UWORD8 *pu1_pred,
                UWORD8 *pu1_out, WORD32 src_strd, WORD32 pred_strd,
                WORD32 out_strd, const UWORD16 *pu2_iscale_mat,
                const UWORD16 *pu2_weigh_mat, UWORD32 qp_div, UWORD32 u4_cntrl,
                WORD32 *pi4_tmp);

/**
******************************************************************************
*
* @brief  This function packs residue of an i16x16 luma mb for entropy coding
*
* @par   Description
*  An i16 macro block contains two classes of units, dc 4x4 block and
*  4x4 ac blocks. while packing the mb, the dc block is sent first, and
*  the 16 ac blocks are sent next in scan order. Each and every block is
*  represented by 3 parameters (nnz, significant coefficient map and the
*  residue coefficients itself). If a 4x4 unit does not have any coefficients
*  then only nnz is sent. Inside a 4x4 block the individual coefficients are
*  sent in scan order.
*
*  The first byte of each block will be nnz of the block, if it is non zero,
*  a 2 byte significance map is sent. This is followed by nonzero coefficients.
*  This is repeated for 1 dc + 16 ac blocks.
*
* @param[in]  pi2_res_mb
*  pointer to residue mb
*
* @param[in, out]  pv_mb_coeff_data
*  buffer pointing to packed residue coefficients
*
* @param[in]  u4_res_strd
*  residual block stride
*
* @param[out]  u1_cbp_l
*  coded block pattern luma
*
* @param[in]   pu1_nnz
*  number of non zero coefficients in each 4x4 unit
*
* @param[out]
*  Control signal for inverse transform of 16x16 blocks
*
* @return none
*
* @ remarks
*
******************************************************************************
*/
void ih264e_pack_l_mb_i16(WORD16 *pi2_res_mb, void **pv_mb_coeff_data,
                          WORD32 i4_res_strd, UWORD8 *u1_cbp_l, UWORD8 *pu1_nnz,
                          UWORD32 *pu4_cntrl);

/**
******************************************************************************
*
* @brief  This function packs residue of an i8x8 chroma mb for entropy coding
*
* @par   Description
*  An i8 chroma macro block contains two classes of units, dc 2x2 block and
*  4x4 ac blocks. while packing the mb, the dc block is sent first, and
*  the 4 ac blocks are sent next in scan order. Each and every block is
*  represented by 3 parameters (nnz, significant coefficient map and the
*  residue coefficients itself). If a 4x4 unit does not have any coefficients
*  then only nnz is sent. Inside a 4x4 block the individual coefficients are
*  sent in scan order.
*
*  The first byte of each block will be nnz of the block, if it is non zero,
*  a 2 byte significance map is sent. This is followed by nonzero coefficients.
*  This is repeated for 1 dc + 4 ac blocks.
*
* @param[in]  pi2_res_mb
*  pointer to residue mb
*
* @param[in, out]  pv_mb_coeff_data
*  buffer pointing to packed residue coefficients
*
* @param[in]  u4_res_strd
*  residual block stride
*
* @param[out]  u1_cbp_c
*  coded block pattern chroma
*
* @param[in]   pu1_nnz
*  number of non zero coefficients in each 4x4 unit
*
* @param[out]   pu1_nnz
*  Control signal for inverse transform
*
* @param[in]   u4_swap_uv
*  Swaps the order of U and V planes in entropy bitstream
*
* @return none
*
* @ remarks
*
******************************************************************************
*/
void ih264e_pack_c_mb(WORD16 *pi2_res_mb, void **pv_mb_coeff_data,
                      WORD32 i4_res_strd, UWORD8 *u1_cbp_c, UWORD8 *pu1_nnz,
                      UWORD32 u4_kill_coffs_flag, UWORD32 *pu4_cntrl,
                      UWORD32 u4_swap_uv);

/**
*******************************************************************************
*
* @brief performs luma core coding when intra mode is i16x16
*
* @par Description:
*  If the current mb is to be coded as intra of mb type i16x16, the mb is first
*  predicted using one of i16x16 prediction filters, basing on the intra mode
*  chosen. Then, error is computed between the input blk and the estimated blk.
*  This error is transformed (hierarchical transform i.e., dct followed by hada-
*  -mard), quantized. The quantized coefficients are packed in scan order for
*  entropy coding.
*
* @param[in] ps_proc_ctxt
*  pointer to the current macro block context
*
* @returns u1_cbp_l
*  coded block pattern luma
*
* @remarks none
*
*******************************************************************************
*/
UWORD8 ih264e_code_luma_intra_macroblock_16x16
        (
            process_ctxt_t *ps_proc
        );

/**
*******************************************************************************
*
* @brief performs luma core coding when intra mode is i4x4
*
* @par Description:
*  If the current mb is to be coded as intra of mb type i4x4, the mb is first
*  predicted using one of i4x4 prediction filters, basing on the intra mode
*  chosen. Then, error is computed between the input blk and the estimated blk.
*  This error is dct transformed and quantized. The quantized coefficients are
*  packed in scan order for entropy coding.
*
* @param[in] ps_proc_ctxt
*  pointer to the current macro block context
*
* @returns u1_cbp_l
*  coded block pattern luma
*
* @remarks
*  The traversal of 4x4 subblocks in the 16x16 macroblock is as per the scan order
*  mentioned in h.264 specification
*
*******************************************************************************
*/
UWORD8 ih264e_code_luma_intra_macroblock_4x4
        (
            process_ctxt_t *ps_proc
        );

/**
*******************************************************************************
*
* @brief performs luma core coding when intra mode is i4x4
*
* @par Description:
*  If the current mb is to be coded as intra of mb type i4x4, the mb is first
*  predicted using one of i4x4 prediction filters, basing on the intra mode
*  chosen. Then, error is computed between the input blk and the estimated blk.
*  This error is dct transformed and quantized. The quantized coefficients are
*  packed in scan order for entropy coding.
*
* @param[in] ps_proc_ctxt
*  pointer to the current macro block context
*
* @returns u1_cbp_l
*  coded block pattern luma
*
* @remarks
*  The traversal of 4x4 subblocks in the 16x16 macroblock is as per the scan order
*  mentioned in h.264 specification
*
*******************************************************************************
*/
UWORD8 ih264e_code_luma_intra_macroblock_4x4_rdopt_on
        (
            process_ctxt_t *ps_proc
        );

/**
*******************************************************************************
*
* @brief performs chroma core coding for intra macro blocks
*
* @par Description:
*  If the current MB is to be intra coded with mb type chroma I8x8, the MB is
*  first predicted using intra 8x8 prediction filters. The predicted data is
*  compared with the input for error and the error is transformed. The DC
*  coefficients of each transformed sub blocks are further transformed using
*  Hadamard transform. The resulting coefficients are quantized, packed and sent
*  for entropy coding.
*
* @param[in] ps_proc_ctxt
*  pointer to the current macro block context
*
* @returns u1_cbp_c
*  coded block pattern chroma
*
* @remarks
*  The traversal of 4x4 subblocks in the 8x8 macroblock is as per the scan order
*  mentioned in h.264 specification
*
*******************************************************************************
*/
UWORD8 ih264e_code_chroma_intra_macroblock_8x8
        (
            process_ctxt_t *ps_proc
        );

/**
*******************************************************************************
* @brief performs luma core coding when  mode is inter
*
* @par Description:
*  If the current mb is to be coded as inter predicted mb,based on the sub mb
*  partitions and corresponding motion vectors generated by ME, prediction is done.
*  Then, error is computed between the input blk and the estimated blk.
*  This error is transformed ( dct and with out hadamard), quantized. The
*  quantized coefficients are packed in scan order for entropy coding.
*
* @param[in] ps_proc_ctxt
*  pointer to the current macro block context
*
* @returns u1_cbp_l
*  coded block pattern luma
*
* @remarks none
*
*******************************************************************************
*/
UWORD8 ih264e_code_luma_inter_macroblock_16x16
        (
            process_ctxt_t *ps_proc
        );

/**
*******************************************************************************
* @brief performs chroma core coding for inter macro blocks
*
* @par Description:
*  If the current mb is to be coded as inter predicted mb, based on the sub mb
*  partitions and corresponding motion vectors generated by ME, prediction is done.
*  Then, error is computed between the input blk and the estimated blk.
*  This error is transformed, quantized. The quantized coefficients
*  are packed in scan order for entropy coding.
*
* @param[in] ps_proc_ctxt
*  pointer to the current macro block context
*
* @returns u1_cbp_l
*  coded block pattern luma
*
* @remarks none
*
*******************************************************************************
*/
UWORD8 ih264e_code_chroma_inter_macroblock_8x8
        (
            process_ctxt_t *ps_proc
        );

#endif /* IH264E_CORE_CODING_H_ */
