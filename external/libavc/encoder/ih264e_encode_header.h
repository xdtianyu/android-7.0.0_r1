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
*  ih264e_encode_header.h
*
* @brief
*  This file contains structures and interface prototypes for h264 bitstream
*  header encoding
*
* @author
*  ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_ENCODE_HEADER_H_
#define IH264E_ENCODE_HEADER_H_

/*****************************************************************************/
/* Function Macros                                                           */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief   Macro to put a code with specified number of bits into the
 *           bitstream
******************************************************************************
 */
#define PUT_BITS(ps_bitstrm, code_val, code_len, ret_val, syntax_string) \
         ENTROPY_TRACE(syntax_string, code_val);\
        ret_val |= ih264e_put_bits((ps_bitstrm), (code_val), (code_len))

/**
******************************************************************************
 *  @brief   Macro to put a code with specified number of bits into the
 *           bitstream using 0th order exponential Golomb encoding for
 *           signed numbers
******************************************************************************
 */
#define PUT_BITS_UEV(ps_bitstrm, code_val, ret_val, syntax_string) \
        ENTROPY_TRACE(syntax_string, code_val);\
        ret_val |= ih264e_put_uev((ps_bitstrm), (code_val))

/**
******************************************************************************
 *  @brief   Macro to put a code with specified number of bits into the
 *           bitstream using 0th order exponential Golomb encoding for
 *           signed numbers
******************************************************************************
 */
#define PUT_BITS_SEV(ps_bitstrm, code_val, ret_val, syntax_string) \
        ENTROPY_TRACE(syntax_string, code_val);\
        ret_val |= ih264e_put_sev((ps_bitstrm), (code_val))


/*****************************************************************************/
/* Extern Function Declarations                                              */
/*****************************************************************************/

/**
******************************************************************************
*
* @brief Generates SPS (Sequence Parameter Set)
*
* @par   Description
*  This function generates Sequence Parameter Set header as per the spec
*
* @param[in]   ps_bitstrm
*  pointer to bitstream context (handle)
*
* @param[in]   ps_sps
*  pointer to structure containing SPS data
*
* @return      success or failure error code
*
******************************************************************************
*/
WORD32      ih264e_generate_sps
    (
        bitstrm_t   *ps_bitstrm,
        sps_t       *ps_sps,
        vui_t       *ps_vui
    );

/**
******************************************************************************
*
* @brief Generates PPS (Picture Parameter Set)
*
* @par   Description
*  Generate Picture Parameter Set as per Section 7.3.2.2
*
* @param[in]   ps_bitstrm
*  pointer to bitstream context (handle)
*
* @param[in]   ps_pps
*  pointer to structure containing PPS data
*
* @return      success or failure error code
*
******************************************************************************
*/
WORD32      ih264e_generate_pps
    (
        bitstrm_t   *ps_bitstrm,
        pps_t       *ps_pps,
        sps_t       *ps_sps
    );

/**
******************************************************************************
*
* @brief Generates Slice Header
*
* @par   Description
*  Generate Slice Header as per Section 7.3.5.1
*
* @param[inout]   ps_bitstrm
*  pointer to bitstream context for generating slice header
*
* @param[in]   ps_slice_hdr
*  pointer to slice header params
*
* @param[in]   ps_pps
*  pointer to pps params referred by slice
*
* @param[in]   ps_sps
*  pointer to sps params referred by slice
*
* @param[out]   ps_dup_bit_strm_ent_offset
*  Bitstream struct to store bitstream state
*
* @param[out]   pu4_first_slice_start_offset
*  first slice offset is returned
*
* @return      success or failure error code
*
******************************************************************************
*/
WORD32      ih264e_generate_slice_header
    (
        bitstrm_t       *ps_bitstrm,
        slice_header_t  *ps_slice_hdr,
        pps_t           *ps_pps,
        sps_t           *ps_sps
    );

/**
******************************************************************************
*
* @brief Populates sps structure
*
* @par   Description
*  Populates sps structure for its use in header generation
*
* @param[in]   ps_codec
*  pointer to encoder context
*
* @param[out]  ps_sps
*  pointer to sps params that needs to be populated
*
* @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T      ih264e_populate_sps
        (
            codec_t *ps_codec,
            sps_t   *ps_sps
        );

/**
******************************************************************************
*
* @brief Populates pps structure
*
* @par   Description
*  Populates pps structure for its use in header generation
*
* @param[in]   ps_codec
*  pointer to encoder context
*
* @param[out]  ps_pps
*  pointer to pps params that needs to be populated
*
* @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_populate_pps
        (
            codec_t *ps_codec,
            pps_t *ps_pps
        );


/**
******************************************************************************
*
* @brief Populates slice header structure
*
* @par   Description
*  Populates slice header structure for its use in header generation
*
* @param[in]  ps_proc
*  pointer to proc context
*
* @param[out]  ps_slice_hdr
*  pointer to slice header structure that needs to be populated
*
* @param[in]  ps_pps
*  pointer to pps params structure referred by the slice
*
* @param[in]   ps_sps
*  pointer to sps params referred by the pps
*
* @return      success or failure error code
*
******************************************************************************
*/
WORD32 ih264e_populate_slice_header
        (
            process_ctxt_t *ps_proc,
            slice_header_t *ps_slice_hdr,
            pps_t *ps_pps,
            sps_t *ps_sps
        );


/**
******************************************************************************
*
* @brief inserts FILLER Nal Unit.
*
* @par   Description
*  In constant bit rate rc mode, when the bits generated by the codec is
*  underflowing the target bit rate, the encoder library inserts filler nal unit.
*
* @param[in]    ps_bitstrm
*  pointer to bitstream context (handle)
*
* @param[in]    insert_fill_bytes
*  Number of fill bytes to be inserted
*
* @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_add_filler_nal_unit
        (
            bitstrm_t   *ps_bitstrm,
            WORD32      insert_fill_bytes
        );


#endif //IH264E_ENCODE_HEADER_H_
