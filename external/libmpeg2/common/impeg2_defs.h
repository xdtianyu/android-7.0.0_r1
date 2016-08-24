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

#ifndef __IMPEG2_DEFS_H__
#define __IMPEG2_DEFS_H__

#include <assert.h>

/******************************************************************************
* MPEG2 Start code and other code definitions
*******************************************************************************/
#define START_CODE_PREFIX               0x000001
#define SEQUENCE_HEADER_CODE            0x000001B3
#define EXTENSION_START_CODE            0x000001B5
#define USER_DATA_START_CODE            0x000001B2
#define GOP_START_CODE                  0x000001B8
#define PICTURE_START_CODE              0x00000100
#define SEQUENCE_END_CODE               0x000001B7
#define RESERVED_START_CODE             0x000001B0
#define MB_ESCAPE_CODE                  0x008

/******************************************************************************
* MPEG2 Length of various codes definitions
*******************************************************************************/
#define START_CODE_LEN                  32
#define START_CODE_PREFIX_LEN           24
#define MB_ESCAPE_CODE_LEN              11
#define EXT_ID_LEN                      4
#define MB_QUANT_SCALE_CODE_LEN         5
#define MB_DCT_TYPE_LEN                 1
#define MB_MOTION_TYPE_LEN              2
#define BYTE_LEN                        8

/******************************************************************************
* MPEG1 code definitions
*******************************************************************************/
#define MB_STUFFING_CODE                0x00F

/******************************************************************************
* MPEG1 Length of various codes definitions
*******************************************************************************/
#define MB_STUFFING_CODE_LEN             11

/******************************************************************************
* MPEG2 MB definitions
*******************************************************************************/
#define MPEG2_INTRA_MB                  0x04
#define MPEG2_INTRAQ_MB                 0x44
#define MPEG2_INTER_MB                  0x28
#define MB_MOTION_BIDIRECT              0x30
#define MB_INTRA_OR_PATTERN             0x0C

/******************************************************************************
* Tools definitions
*******************************************************************************/
#define SPATIAL_SCALABILITY             0x01
#define TEMPORAL_SCALABILITY            0x03

/******************************************************************************
* Extension IDs definitions
*******************************************************************************/
#define SEQ_DISPLAY_EXT_ID              0x02
#define SEQ_SCALABLE_EXT_ID             0x05
#define QUANT_MATRIX_EXT_ID             0x03
#define COPYRIGHT_EXT_ID                0x04
#define PIC_DISPLAY_EXT_ID              0x07
#define PIC_SPATIAL_SCALABLE_EXT_ID     0x09
#define PIC_TEMPORAL_SCALABLE_EXT_ID    0x0A
#define CAMERA_PARAM_EXT_ID             0x0B
#define ITU_T_EXT_ID                    0x0C
/******************************************************************************
* Extension IDs Length definitions
*******************************************************************************/
#define CAMERA_PARAMETER_EXTENSION_LEN  377
#define COPYRIGHT_EXTENSION_LEN          88
#define GROUP_OF_PICTURE_LEN             59


/******************************************************************************
* MPEG2 Picture structure definitions
*******************************************************************************/
#define TOP_FIELD                       1
#define BOTTOM_FIELD                    2
#define FRAME_PICTURE                   3

/******************************************************************************
* MPEG2 Profile definitions
*******************************************************************************/
#define MPEG2_SIMPLE_PROFILE            0x05
#define MPEG2_MAIN_PROFILE              0x04

/******************************************************************************
* MPEG2 Level definitions
*******************************************************************************/
#define MPEG2_LOW_LEVEL                 0x0a
#define MPEG2_MAIN_LEVEL                0x08

/******************************************************************************
* MPEG2 Prediction types
*******************************************************************************/
#define FIELD_PRED                      0
#define FRAME_PRED                      1
#define DUAL_PRED                       2
#define RESERVED                        -1
#define MC_16X8_PRED                    3

/*****************************************************************************
* MPEG2 Motion vector format
******************************************************************************/
#define FIELD_MV                        0
#define FRAME_MV                        1

/******************************************************************************/
/* General Video related definitions                                          */
/******************************************************************************/

#define BLK_SIZE 8
#define NUM_COEFFS ((BLK_SIZE)*(BLK_SIZE))
#define LUMA_BLK_SIZE (2 * (BLK_SIZE))
#define CHROMA_BLK_SIZE (BLK_SIZE)
#define  BLOCKS_IN_MB            6
#define  MB_SIZE                16
#define  MB_CHROMA_SIZE          8
#define  NUM_PELS_IN_BLOCK      64
#define  NUM_LUMA_BLKS           4
#define  NUM_CHROMA_BLKS         2
#define  MAX_COLR_COMPS          3
#define  Y_LUMA                  0
#define  U_CHROMA                1
#define  V_CHROMA                2
#define  MB_LUMA_MEM_SIZE           ((MB_SIZE) * (MB_SIZE))
#define  MB_CHROMA_MEM_SIZE         ((MB_SIZE/2) * (MB_SIZE/2))

#define BITS_IN_INT     32
/******************************************************************************/
/* MPEG2 Motion compensation related definitions                              */
/******************************************************************************/
#define REF_FRM_MB_WIDTH        18
#define REF_FRM_MB_HEIGHT       18
#define REF_FLD_MB_HEIGHT       10
#define REF_FLD_MB_WIDTH        18

/******************************************************************************/
/* Maximum number of bits per MB                                              */
/******************************************************************************/
#define I_MB_BIT_SIZE 90
#define P_MB_BIT_SIZE 90
#define B_MB_BIT_SIZE 150

/******************************************************************************/
/* Aspect ratio related definitions                                           */
/******************************************************************************/
#define MPG1_NTSC_4_3       0x8
#define MPG1_PAL_4_3        0xc
#define MPG1_NTSC_16_9      0x6
#define MPG1_PAL_16_9       0x3
#define MPG1_1_1            0x1

#define MPG2_4_3            0x2
#define MPG2_16_9           0x3
#define MPG2_1_1            0x1

/******************************************************************************/
/* Inverse Quantizer Output range                                             */
/******************************************************************************/
#define IQ_OUTPUT_MAX 2047
#define IQ_OUTPUT_MIN -2048

/******************************************************************************/
/* IDCT Output range                                                          */
/******************************************************************************/
#define IDCT_OUTPUT_MAX  255
#define IDCT_OUTPUT_MIN -256

/******************************************************************************/
/* Output pixel range                                                         */
/******************************************************************************/
#define PEL_VALUE_MAX 255
#define PEL_VALUE_MIN 0

/******************************************************************************/
/* inv scan types                                                             */
/******************************************************************************/
#define ZIG_ZAG_SCAN        0
#define VERTICAL_SCAN       1

/******************************************************************************/
/* Related VLD codes                                                          */
/******************************************************************************/
#define ESC_CODE_VALUE 0x0058
#define EOB_CODE_VALUE 0x07d0

#define END_OF_BLOCK                    0x01
#define ESCAPE_CODE                     0x06

#define END_OF_BLOCK_ZERO               0x01ff
#define END_OF_BLOCK_ONE                0x01ff

/******************** Idct Specific ***************/
#define TRANS_SIZE_8            8
#define IDCT_STG1_SHIFT        12
#define IDCT_STG2_SHIFT        16

#define IDCT_STG1_ROUND        ((1 << IDCT_STG1_SHIFT) >> 1)
#define IDCT_STG2_ROUND        ((1 << IDCT_STG2_SHIFT) >> 1)


/******************************************************************************
* Sample Version Definitions
*******************************************************************************/
#define SAMPLE_VERS_MAX_FRAMES_DECODE   999

#define MAX_FRAME_BUFFER                     7

/* vop coding type */
typedef enum
{
    I_PIC = 1,
    P_PIC,
    B_PIC,
    D_PIC
} e_pic_type_t;

typedef enum
{
    MPEG_2_VIDEO,
    MPEG_1_VIDEO
} e_video_type_t;

typedef enum
{
    FORW,
    BACK,
    BIDIRECT
} e_pred_direction_t;

typedef enum
{
    TOP,
    BOTTOM
} e_field_t;

/* Motion vectors (first/second) */
enum
{
    FIRST,
    SECOND,
    THIRD,
    FOURTH
};

enum
{
    MV_X,
    MV_Y
};

/* Enumeration defining the various kinds of interpolation possible in
motion compensation */
typedef enum
{
  FULL_XFULL_Y,
    FULL_XHALF_Y,
    HALF_XFULL_Y,
    HALF_XHALF_Y
} e_sample_type_t;
typedef enum
{
    /* Params of the reference buffer used as input to MC */
    /* frame prediction in P frame picture */
    MC_FRM_FW_OR_BK_1MV,
    /* field prediction in P frame picture */
    MC_FRM_FW_OR_BK_2MV,
    /* frame prediction in B frame picture */
    MC_FRM_FW_AND_BK_2MV,
    /* field prediction in B frame picture */
    MC_FRM_FW_AND_BK_4MV,
    /* dual prime prediction in P frame picture */
    MC_FRM_FW_DUAL_PRIME_1MV,
    /* frame prediction in P field picture */
    MC_FLD_FW_OR_BK_1MV,
    /* 16x8 prediction in P field picture */
    MC_FLD_FW_OR_BK_2MV,
    /* field prediction in B field picture */
    MC_FLD_FW_AND_BK_2MV,
    /* 16x8 prediction in B field picture */
    MC_FLD_FW_AND_BK_4MV,
    /* dual prime prediction in P field picture */
    MC_FLD_FW_DUAL_PRIME_1MV,
} e_mb_type_t;

#endif /* __IMPEG2_DEFS_H__ */

