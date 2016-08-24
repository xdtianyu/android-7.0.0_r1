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
#ifndef __IMPEG2D_VLD_TABLES_H__
#define __IMPEG2D_VLD_TABLES_H__


#define MB_ADDR_INCR_OFFSET       34
#define MB_ADDR_INCR_LEN          11
#define MB_TYPE_LEN               6
#define MV_CODE_LEN               11
#define MB_CBP_LEN                9



#define MB_BIDRECT          0x20
#define MB_QUANT            0x10
#define MB_MV_FORW          0x8
#define MB_MV_BACK          0x4
#define MB_PATTERN          0x2
#define MB_TYPE_INTRA       0x1
#define MB_FORW_OR_BACK     (MB_MV_FORW    | MB_MV_BACK)
#define MB_CODED            (MB_TYPE_INTRA | MB_PATTERN)


#define MPEG2_MB_ADDR_INCR_OFFSET       34
#define MPEG2_INTRA_MBTYPE_OFFSET       69
#define MPEG2_INTER_MBTYPE_OFFSET       105
#define MPEG2_BVOP_MBTYPE_OFFSET        125
#define MPEG2_DCT_DC_SIZE_OFFSET        12
#define MPEG2_CBP_OFFSET                64
#define MPEG2_MOTION_CODE_OFFSET        17
#define MPEG2_DMV_OFFSET                2

#define MPEG2_AC_COEFF_MAX_LEN          16
#define MB_ADDR_INCR_LEN                11
#define MPEG2_INTRA_MBTYPE_LEN          2
#define MPEG2_INTER_MBTYPE_LEN          6

#define MPEG2_DCT_DC_SIZE_LEN           9
#define MPEG2_DCT_DC_LUMA_SIZE_LEN      9
#define MPEG2_DCT_DC_CHROMA_SIZE_LEN    10
#define MPEG2_CBP_LEN                   9
#define MPEG2_MOTION_CODE_LEN           11
#define MPEG2_DMV_LEN                   2

#define END_OF_BLOCK                    0x01
#define ESCAPE_CODE                     0x06

/* Table to be used for decoding the MB increment value */
extern const WORD16  gai2_impeg2d_mb_addr_incr[][2];
extern const WORD16  gai2_impeg2d_dct_dc_size[][11][2];

extern const UWORD16 gau2_impeg2d_dct_coeff_zero[];
extern const UWORD16 gau2_impeg2d_dct_coeff_one[];
extern const UWORD16 gau2_impeg2d_offset_zero[];
extern const UWORD16 gau2_impeg2d_offset_one[];

extern const UWORD16 gau2_impeg2d_tab_zero_1_9[];
extern const UWORD16 gau2_impeg2d_tab_one_1_9[];
extern const UWORD16 gau2_impeg2d_tab_zero_10_16[];
extern const UWORD16 gau2_impeg2d_tab_one_10_16[];

extern const UWORD16 gau2_impeg2d_p_mb_type[];
extern const UWORD16 gau2_impeg2d_b_mb_type[];
extern const UWORD16 gau2_impeg2d_mv_code[];
extern const WORD16  gai2_impeg2d_dec_mv[4];
extern const UWORD16 gau2_impeg2d_cbp_code[];


#endif /* __IMPEG2D_VLD_TABLES_H__ */

