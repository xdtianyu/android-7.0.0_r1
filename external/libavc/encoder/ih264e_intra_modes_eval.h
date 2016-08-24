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
*  ih264e_intra_modes_eval.h
*
* @brief
*  This file contains declarations of routines that perform rate distortion
*  analysis on a macroblock if coded as intra.
*
* @author
*  ittiam
*
* @remarks
*  none
*
*******************************************************************************
*/

#ifndef IH264E_INTRA_MODES_EVAL_H_
#define IH264E_INTRA_MODES_EVAL_H_

/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/

/**
******************************************************************************
*
* @brief
*  derivation process for macroblock availability
*
* @par   Description
*  Calculates the availability of the left, top, topright and topleft macroblocks.
*
* @param[in] ps_proc_ctxt
*  pointer to proc context (handle)
*
* @remarks Based on section 6.4.5 in H264 spec
*
* @return  none
*
******************************************************************************
*/
void ih264e_derive_nghbr_avbl_of_mbs
        (
            process_ctxt_t *ps_proc_ctxt
        );

/**
******************************************************************************
*
* @brief
*  derivation process for subblock/partition availability
*
* @par   Description
*  Calculates the availability of the left, top, topright and topleft subblock
*  or partitions.
*
* @param[in]    ps_proc_ctxt
*  pointer to macroblock context (handle)
*
* @param[in]    i1_pel_pos_x
*  column position of the pel wrt the current block
*
* @param[in]    i1_pel_pos_y
*  row position of the pel in wrt current block
*
* @remarks     Assumptions: before calling this function it is assumed that
*   the neighbor availability of the current macroblock is already derived.
*   Based on table 6-3 of H264 specification
*
* @return      availability status (yes or no)
*
******************************************************************************
*/
UWORD8 ih264e_derive_ngbr_avbl_of_mb_partitions
        (
            block_neighbors_t *s_ngbr_avbl,
            WORD8 i1_pel_pos_x,
            WORD8 i1_pel_pos_y
        );

/**
******************************************************************************
*
* @brief
*  evaluate best intra 16x16 mode (rate distortion opt off)
*
* @par Description
*  This function evaluates all the possible intra 16x16 modes and finds the mode
*  that best represents the macro-block (least distortion) and occupies fewer
*  bits in the bit-stream.
*
* @param[in]   ps_proc_ctxt
*  pointer to process context (handle)
*
* @remarks
*  Ideally the cost of encoding a macroblock is calculated as
*  (distortion + lambda*rate). Where distortion is SAD/SATD,... between the
*  input block and the reconstructed block and rate is the number of bits taken
*  to place the macroblock in the bit-stream. In this routine the rate does not
*  exactly point to the total number of bits it takes, rather it points to header
*  bits necessary for encoding the macroblock. Assuming the deltaQP, cbp bits
*  and residual bits fall in to texture bits the number of bits taken to encoding
*  mbtype is considered as rate, we compute cost. Further we will approximate
*  the distortion as the deviation b/w input and the predicted block as opposed
*  to input and reconstructed block.
*
*  NOTE: As per the Document JVT-O079, for intra 16x16 macroblock,
*  the SAD and cost are one and the same.
*
* @return     none
*
******************************************************************************
*/
void ih264e_evaluate_intra16x16_modes_for_least_cost_rdoptoff
        (
            process_ctxt_t *ps_proc_ctxt
        );

/**
******************************************************************************
*
* @brief
*  evaluate best intra 8x8 mode (rate distortion opt on)
*
* @par Description
*  This function evaluates all the possible intra 8x8 modes and finds the mode
*  that best represents the macro-block (least distortion) and occupies fewer
*  bits in the bit-stream.
*
* @param[in]    ps_proc_ctxt
*  pointer to proc ctxt
*
* @remarks Ideally the cost of encoding a macroblock is calculated as
*  (distortion + lambda*rate). Where distortion is SAD/SATD,... between the
*  input block and the reconstructed block and rate is the number of bits taken
*  to place the macroblock in the bit-stream. In this routine the rate does not
*  exactly point to the total number of bits it takes, rather it points to header
*  bits necessary for encoding the macroblock. Assuming the deltaQP, cbp bits
*  and residual bits fall in to texture bits the number of bits taken to encoding
*  mbtype is considered as rate, we compute cost. Further we will approximate
*  the distortion as the deviation b/w input and the predicted block as opposed
*  to input and reconstructed block.
*
*  NOTE: TODO: This function needs to be tested
*
*  @return      none
*
******************************************************************************
*/
void ih264e_evaluate_intra8x8_modes_for_least_cost_rdoptoff
        (
            process_ctxt_t *ps_proc_ctxt
        );

/**
******************************************************************************
*
* @brief
*  evaluate best intra 4x4 mode (rate distortion opt on)
*
* @par Description
*  This function evaluates all the possible intra 4x4 modes and finds the mode
*  that best represents the macro-block (least distortion) and occupies fewer
*  bits in the bit-stream.
*
* @param[in]    ps_proc_ctxt
*  pointer to proc ctxt
*
* @remarks
*  Ideally the cost of encoding a macroblock is calculated as
*  (distortion + lambda*rate). Where distortion is SAD/SATD,... between the
*  input block and the reconstructed block and rate is the number of bits taken
*  to place the macroblock in the bit-stream. In this routine the rate does not
*  exactly point to the total number of bits it takes, rather it points to header
*  bits necessary for encoding the macroblock. Assuming the deltaQP, cbp bits
*  and residual bits fall in to texture bits the number of bits taken to encoding
*  mbtype is considered as rate, we compute cost. Further we will approximate
*  the distortion as the deviation b/w input and the predicted block as opposed
*  to input and reconstructed block.
*
*  NOTE: As per the Document JVT-O079, for the whole intra 4x4 macroblock,
*  24*lambda is added to the SAD before comparison with the best SAD for
*  inter prediction. This is an empirical value to prevent using too many intra
*  blocks.
*
* @return      none
*
******************************************************************************
*/
void ih264e_evaluate_intra4x4_modes_for_least_cost_rdopton
        (
            process_ctxt_t *ps_proc_ctxt
        );

/**
******************************************************************************
*
* @brief
*  evaluate best intra 4x4 mode (rate distortion opt off)
*
* @par Description
*  This function evaluates all the possible intra 4x4 modes and finds the mode
*  that best represents the macro-block (least distortion) and occupies fewer
*  bits in the bit-stream.
*
* @param[in]    ps_proc_ctxt
*  pointer to proc ctxt
*
* @remarks
*  Ideally the cost of encoding a macroblock is calculated as
*  (distortion + lambda*rate). Where distortion is SAD/SATD,... between the
*  input block and the reconstructed block and rate is the number of bits taken
*  to place the macroblock in the bit-stream. In this routine the rate does not
*  exactly point to the total number of bits it takes, rather it points to header
*  bits necessary for encoding the macroblock. Assuming the deltaQP, cbp bits
*  and residual bits fall in to texture bits the number of bits taken to encoding
*  mbtype is considered as rate, we compute cost. Further we will approximate
*  the distortion as the deviation b/w input and the predicted block as opposed
*  to input and reconstructed block.
*
*  NOTE: As per the Document JVT-O079, for the whole intra 4x4 macroblock,
*  24*lambda is added to the SAD before comparison with the best SAD for
*  inter prediction. This is an empirical value to prevent using too many intra
*  blocks.
*
* @return      none
*
******************************************************************************
*/
void ih264e_evaluate_intra4x4_modes_for_least_cost_rdoptoff
        (
            process_ctxt_t *ps_proc_ctxt
        );

/**
******************************************************************************
*
* @brief
*  evaluate best chroma intra 8x8 mode (rate distortion opt off)
*
* @par Description
*  This function evaluates all the possible chroma intra 8x8 modes and finds
*  the mode that best represents the macroblock (least distortion) and occupies
*  fewer bits in the bitstream.
*
* @param[in] ps_proc_ctxt
*  pointer to macroblock context (handle)
*
* @remarks
*  For chroma best intra pred mode is calculated based only on SAD
*
* @returns none
*
******************************************************************************
*/
void ih264e_evaluate_chroma_intra8x8_modes_for_least_cost_rdoptoff
        (
            process_ctxt_t *ps_proc_ctxt
        );


/**
******************************************************************************
*
* @brief
*  Evaluate best intra 16x16 mode (among VERT, HORZ and DC) and do the
*  prediction.
*
* @par Description
*  This function evaluates first three 16x16 modes and compute corresponding sad
*  and return the buffer predicted with best mode.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[in] pu1_ngbr_pels_i16
*  UWORD8 pointer to neighbouring pels
*
* @param[out] pu1_dst
*  UWORD8 pointer to the destination
*
* @param[in] src_strd
*  integer source stride
*
* @param[in] dst_strd
*  integer destination stride
*
* @param[in] u4_n_avblty
*  availability of neighbouring pixels
*
* @param[in] u4_intra_mode
*  Pointer to the variable in which best mode is returned
*
* @param[in] pu4_sadmin
*  Pointer to the variable in which minimum sad is returned
*
* @param[in] u4_valid_intra_modes
*  Says what all modes are valid
*
* @returns      none
*
******************************************************************************
*/
typedef void ih264e_evaluate_intra_modes_ft(UWORD8 *pu1_src,
                                            UWORD8 *pu1_ngbr_pels_i16,
                                            UWORD8 *pu1_dst,
                                            UWORD32 src_strd,
                                            UWORD32 dst_strd,
                                            WORD32 u4_n_avblty,
                                            UWORD32 *u4_intra_mode,
                                            WORD32 *pu4_sadmin,
                                            UWORD32 u4_valid_intra_modes);

ih264e_evaluate_intra_modes_ft ih264e_evaluate_intra16x16_modes;
ih264e_evaluate_intra_modes_ft ih264e_evaluate_intra_chroma_modes;

/* assembly */
ih264e_evaluate_intra_modes_ft ih264e_evaluate_intra16x16_modes_a9q;
ih264e_evaluate_intra_modes_ft ih264e_evaluate_intra_chroma_modes_a9q;

ih264e_evaluate_intra_modes_ft ih264e_evaluate_intra16x16_modes_av8;
ih264e_evaluate_intra_modes_ft ih264e_evaluate_intra_chroma_modes_av8;

/* x86 intrinsics */
ih264e_evaluate_intra_modes_ft ih264e_evaluate_intra16x16_modes_ssse3;
ih264e_evaluate_intra_modes_ft ih264e_evaluate_intra_chroma_modes_ssse3;

/**
******************************************************************************
*
* @brief
*  Evaluate best intra 4x4 mode and perform prediction.
*
* @par Description
*  This function evaluates  4x4 modes and compute corresponding sad
*  and return the buffer predicted with best mode.
*
* @param[in] pu1_src
*  UWORD8 pointer to the source
*
* @param[in] pu1_ngbr_pels
*  UWORD8 pointer to neighbouring pels
*
* @param[out] pu1_dst
*  UWORD8 pointer to the destination
*
* @param[in] src_strd
*  integer source stride
*
* @param[in] dst_strd
*  integer destination stride
*
* @param[in] u4_n_avblty
*  availability of neighbouring pixels
*
* @param[in] u4_intra_mode
*  Pointer to the variable in which best mode is returned
*
* @param[in] pu4_sadmin
*  Pointer to the variable in which minimum cost is returned
*
* @param[in] u4_valid_intra_modes
*  Says what all modes are valid
*
* @param[in] u4_lambda
*  Lamda value for computing cost from SAD
*
* @param[in] u4_predictd_mode
*  Predicted mode for cost computation
*
* @returns      none
*
******************************************************************************
*/
typedef void ih264e_evaluate_intra_4x4_modes_ft(UWORD8 *pu1_src,
                                                UWORD8 *pu1_ngbr_pels,
                                                UWORD8 *pu1_dst,
                                                UWORD32 src_strd,
                                                UWORD32 dst_strd,
                                                WORD32 u4_n_avblty,
                                                UWORD32 *u4_intra_mode,
                                                WORD32 *pu4_sadmin,
                                                UWORD32 u4_valid_intra_modes,
                                                UWORD32  u4_lambda,
                                                UWORD32 u4_predictd_mode);

ih264e_evaluate_intra_4x4_modes_ft ih264e_evaluate_intra_4x4_modes;

/* x86 intrinsics */
ih264e_evaluate_intra_4x4_modes_ft ih264e_evaluate_intra_4x4_modes_ssse3;

/* assembly */
ih264e_evaluate_intra_4x4_modes_ft ih264e_evaluate_intra_4x4_modes_a9q;
ih264e_evaluate_intra_4x4_modes_ft ih264e_evaluate_intra_4x4_modes_av8;

#endif /* IH264E_INTRA_MODES_EVAL_H_ */
