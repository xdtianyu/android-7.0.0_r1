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
*  ih264e_defs.h
*
* @brief
*  Definitions used in the encoder
*
* @author
*  ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_DEFS_H_
#define IH264E_DEFS_H_


#define PARSE_COEFF_DATA_BLOCK_4x4(pv_mb_coeff_data, ps_mb_coeff_data, u4_nnz, u4_sig_coeff_map, pi2_res_block)   \
{                                                                          \
    ps_mb_coeff_data = pv_mb_coeff_data;                                   \
    u4_nnz = ps_mb_coeff_data->i4_sig_map_nnz & 0xff;                      \
    if (u4_nnz)                                                            \
    {                                                                      \
        u4_sig_coeff_map = ps_mb_coeff_data->i4_sig_map_nnz >> 16;         \
        pi2_res_block = ps_mb_coeff_data->ai2_residue;                     \
        pv_mb_coeff_data = ps_mb_coeff_data->ai2_residue + ALIGN2(u4_nnz); \
    }                                                                      \
    else                                                                   \
    {                                                                      \
      pv_mb_coeff_data = ps_mb_coeff_data->ai2_residue;                    \
    }                                                                      \
}

/*****************************************************************************/
/* Width and height restrictions                                             */
/*****************************************************************************/
/**
 * Minimum width supported by codec
 */
#define MIN_WD   16

/**
 * Maximum width supported by codec
 */

#define MAX_WD   1920

/**
 * Minimum height supported by codec
 */
#define MIN_HT   16

/**
 * Maximum height supported by codec
 */

#define MAX_HT   1920

/*****************************************************************************/
/* Padding sizes                                                             */
/*****************************************************************************/
/**
 * Padding used for top of the frame
 */
#define PAD_TOP     32

/**
 * Padding used for bottom of the frame
 */
#define PAD_BOT     32

/**
 * Padding used at left of the frame
 */
#define PAD_LEFT    32

/**
 * Padding used at right of the frame
 */
#define PAD_RIGHT   32
/**
 * Padding for width
 */
#define PAD_WD      (PAD_LEFT + PAD_RIGHT)
/**
 * Padding for height
 */
#define PAD_HT      (PAD_TOP  + PAD_BOT)

/*
 * buffer width and height for half pel buffers
 */
#define HP_BUFF_WD  24
#define HP_BUFF_HT  18

/*****************************************************************************/
/* Number of frame restrictions                                              */
/*****************************************************************************/
/**
 *  Maximum number of reference pictures
 */
#define MAX_REF_PIC_CNT  2

/**
 *  Minimum number of reference pictures
 */
#define MIN_REF_PIC_CNT  1

/**
 *  Maximum number of B pictures between two I/P pictures
 */
#define MAX_NUM_BFRAMES     10

/**
 *  Maximum number of reference buffers in DPB manager
 */
#define MAX_REF_CNT  32

/*****************************************************************************/
/* Minimum size of inter prediction unit supported by encoder                */
/*****************************************************************************/
#define ENC_MIN_PU_SIZE     16

/*****************************************************************************/
/* Num cores releated defs                                                   */
/*****************************************************************************/
/**
 *  Maximum number of cores
 */
#define MAX_NUM_CORES       8

/**
 *  Maximum number of threads for pixel processing
 */
#define MAX_PROCESS_THREADS MAX_NUM_CORES

/**
 * Maximum process context sets
 * Used to stagger encoding of MAX_CTXT_SETS in parallel
 */
#define MAX_CTXT_SETS   1
/**
 * Maximum number of contexts
 * Kept as twice the number of threads, to make it easier to initialize the contexts
 * from master thread
 */
#define MAX_PROCESS_CTXT    MAX_NUM_CORES * MAX_CTXT_SETS

/*****************************************************************************/
/* Profile and level restrictions                                            */
/*****************************************************************************/
/**
 * Max level supported by the codec
 */
#define MAX_LEVEL  IH264_LEVEL_51

/**
 * Min level supported by the codec
 */
#define MIN_LEVEL  IH264_LEVEL_10

/**
 * Maximum number of slice headers that are held in memory simultaneously
 * For single core implementation only 1 slice header is enough.
 * But for multi-core parsing thread needs to ensure that slice headers are
 * stored till the last CB in a slice is decoded.
 * Parsing thread has to wait till last CB of a slice is consumed before reusing
 * overwriting the slice header
 * MAX_SLICE_HDR_CNT is assumed to be a power of 2
 */

#define LOG2_MAX_SLICE_HDR_CNT 8
#define MAX_SLICE_HDR_CNT (1 << LOG2_MAX_SLICE_HDR_CNT)

/* Generic declarations */
#define DEFAULT_MAX_LEVEL               40
#define DEFAULT_RECON_ENABLE            0
#define DEFAULT_RC                      IVE_RC_STORAGE
#define DEFAULT_MAX_FRAMERATE           120000
#define DEFAULT_MAX_BITRATE             20000000
#define DEFAULT_MAX_NUM_BFRAMES         0
#define DEFAULT_MAX_SRCH_RANGE_X        256
#define DEFAULT_MAX_SRCH_RANGE_Y        256
#define DEFAULT_SLICE_PARAM             256
#define DEFAULT_SRC_FRAME_RATE          30000
#define DEFAULT_TGT_FRAME_RATE          30000
#define DEFAULT_BITRATE                 6000000
#define DEFAULT_QP_MIN                  10
#define DEFAULT_QP_MAX                  51
#define DEFAULT_I_QP                    25
#define DEFAULT_P_QP                    28
#define DEFAULT_B_QP                    28
#define DEFAULT_AIR_MODE                IVE_AIR_MODE_NONE
#define DEFAULT_AIR_REFRESH_PERIOD      30
#define DEFAULT_VBV_DELAY               1000
#define DEFAULT_VBV_SIZE                16800000 /* level 3.1 */
#define DEFAULT_NUM_CORES               1
#define DEFAULT_ME_SPEED_PRESET         100
#define DEFAULT_HPEL                    1
#define DEFAULT_QPEL                    1
#define DEFAULT_I4                      1
#define DEFAULT_I8                      0
#define DEFAULT_I16                     1
#define DEFAULT_ENABLE_FAST_SAD         0
#define DEFAULT_ENABLE_SATQD            1
#define DEFAULT_MIN_SAD_ENABLE          0
#define DEFAULT_MIN_SAD_DISABLE         -1
#define DEFAULT_SRCH_RNG_X              64
#define DEFAULT_SRCH_RNG_Y              48
#define DEFAULT_I_INTERVAL              30
#define DEFAULT_IDR_INTERVAL            1000
#define DEFAULT_B_FRAMES                0
#define DEFAULT_DISABLE_DEBLK_LEVEL     0
#define DEFAULT_PROFILE                 IV_PROFILE_BASE
#define DEFAULT_MIN_INTRA_FRAME_RATE    1
#define DEFAULT_MAX_INTRA_FRAME_RATE    2147483647
#define DEFAULT_MIN_BUFFER_DELAY        30
#define DEFAULT_MAX_BUFFER_DELAY        20000
#define DEFAULT_STRIDE                  0
#define DEFAULT_ENC_SPEED_PRESET        IVE_USER_DEFINED
#define DEFAULT_PRE_ENC_ME              0
#define DEFAULT_PRE_ENC_IPE             0
#define DEFAULT_ENTROPY_CODING_MODE     0

/** Maximum number of entries in input buffer list */
#define MAX_INP_BUF_LIST_ENTRIES         32

/** Maximum number of entries in output buffer list */
#define MAX_OUT_BUF_LIST_ENTRIES         32

/** Maximum number of entries in recon buffer list used within the encoder */
#define MAX_REC_LIST_ENTRIES             16

/** Number of buffers created to hold half-pel planes for every reference buffer */
#define HPEL_PLANES_CNT                 1

/** Number of buffers Needed for SUBPEL and BIPRED computation */
#define SUBPEL_BUFF_CNT                 4

/**
 *****************************************************************************
 * Macro to compute total size required to hold on set of scaling matrices
 *****************************************************************************
 */
#define SCALING_MAT_SIZE(m_scaling_mat_size)                                 \
{                                                                            \
    m_scaling_mat_size = 6 * TRANS_SIZE_4 * TRANS_SIZE_4;                    \
    m_scaling_mat_size += 6 * TRANS_SIZE_8 * TRANS_SIZE_8;                   \
    m_scaling_mat_size += 6 * TRANS_SIZE_16 * TRANS_SIZE_16;                 \
    m_scaling_mat_size += 2 * TRANS_SIZE_32 * TRANS_SIZE_32;                 \
}

/**
 ******************************************************************************
 *  @brief Macros to get raster scan position of a block[8x8] / sub block[4x4]
 ******************************************************************************
 */
#define GET_BLK_RASTER_POS_X(x)     ((x & 0x01))
#define GET_BLK_RASTER_POS_Y(y)     ((y >> 1))
#define GET_SUB_BLK_RASTER_POS_X(x) ((x & 0x01))
#define GET_SUB_BLK_RASTER_POS_Y(y) ((y >> 1))

#define NUM_RC_MEMTABS 17

/**
 ***************************************************************************
 * Enum to hold various mem records being request
 ****************************************************************************
 */
enum
{
    /**
     * Codec Object at API level
     */
    MEM_REC_IV_OBJ,

    /**
     * Codec context
     */
    MEM_REC_CODEC,

    /**
     * Cabac context
     */
    MEM_REC_CABAC,

    /**
     * Cabac context_mb_info
     */
    MEM_REC_CABAC_MB_INFO,

    /**
     * entropy context
     */
    MEM_REC_ENTROPY,

    /**
     * Buffer to hold coeff data
     */
    MEM_REC_MB_COEFF_DATA,

    /**
     * Buffer to hold coeff data
     */
    MEM_REC_MB_HEADER_DATA,

    /**
     * Motion vector bank
     */
    MEM_REC_MVBANK,

    /**
     * Motion vector bits
     */
    MEM_REC_MVBITS,

    /**
     * Holds mem records passed to the codec.
     */
    MEM_REC_BACKUP,

    /**
     * Holds SPS
     */
    MEM_REC_SPS,

    /**
     * Holds PPS
     */
    MEM_REC_PPS,

    /**
     * Holds Slice Headers
     */
    MEM_REC_SLICE_HDR,

    /**
     * Contains map indicating slice index per MB basis
     */
    MEM_REC_SLICE_MAP,

    /**
     * Holds thread handles
     */
    MEM_REC_THREAD_HANDLE,

    /**
     * Holds control call mutex
     */
    MEM_REC_CTL_MUTEX,

    /**
     * Holds entropy call mutex
     */
    MEM_REC_ENTROPY_MUTEX,

    /**
     * Holds memory for Process JOB Queue
     */
    MEM_REC_PROC_JOBQ,

    /**
     * Holds memory for Entropy JOB Queue
     */
    MEM_REC_ENTROPY_JOBQ,

    /**
     * Contains status map indicating processing status per MB basis
     */
    MEM_REC_PROC_MAP,

    /**
     * Contains status map indicating deblocking status per MB basis
     */
    MEM_REC_DBLK_MAP,

    /*
     * Contains AIR map and mask
     */
    MEM_REC_AIR_MAP,

    /**
     * Contains status map indicating ME status per MB basis
     */
    MEM_REC_ME_MAP,

    /**
     * Holds dpb manager context
     */
    MEM_REC_DPB_MGR,

    /**
     * Holds intermediate buffers needed during processing stage
     * Memory for process contexts is allocated in this memtab
     */
    MEM_REC_PROC_SCRATCH,

    /**
     * Holds buffers for vert_bs, horz_bs and QP (all frame level)
     */
    MEM_REC_QUANT_PARAM,

    /**
     * Holds top row syntax information
     */
    MEM_REC_TOP_ROW_SYN_INFO,

    /**
     * Holds buffers for vert_bs, horz_bs and QP (all frame level)
     */
    MEM_REC_BS_QP,

    /**
     * Holds input buffer manager context
     */
    MEM_REC_INP_PIC,

    /**
     * Holds output buffer manager context
     */
    MEM_REC_OUT,

    /**
     * Holds picture buffer manager context and array of pic_buf_ts
     * Also holds reference picture buffers in non-shared mode
     */
    MEM_REC_REF_PIC,

    /*
     * Mem record for color space conversion
     */
    MEM_REC_CSC,

    /**
     * NMB info struct
     */
    MEM_REC_MB_INFO_NMB,

    /**
     * Rate control of memory records.
     */
    MEM_REC_RC,

    /**
     * Place holder to compute number of memory records.
     */
    MEM_REC_CNT = MEM_REC_RC + NUM_RC_MEMTABS,

    /*
     * Do not add anything below
     */
};

#define DISABLE_DEBLOCK_INTERVAL 8

/**
 ****************************************************************************
 * Disable deblock levels
 * Level 0 enables deblocking completely and level 4 disables completely
 * Other levels are intermediate values to control deblocking level
 ****************************************************************************
 */
enum
{
    /**
     * Enable deblocking completely
     */
    DISABLE_DEBLK_LEVEL_0,

    /**
     * Disable only within MB edges - Not supported currently
     */
    DISABLE_DEBLK_LEVEL_1,

    /**
     * Enable deblocking once in DEBLOCK_INTERVAL number of pictures
     * and for I slices
     */
    DISABLE_DEBLK_LEVEL_2,

    /**
     * Enable deblocking only for I slices
     */
    DISABLE_DEBLK_LEVEL_3,

    /**
     * Disable deblocking completely
     */
    DISABLE_DEBLK_LEVEL_4
};

/**
 ****************************************************************************
 * Number of buffers for I/O based on format
 ****************************************************************************
 */

/** Minimum number of input buffers */
#define MIN_INP_BUFS                 2

/** Minimum number of output buffers */
#define MIN_OUT_BUFS                1

/** Minimum number of components in bitstream buffer */
#define MIN_BITS_BUFS_COMP           1

/** Minimum number of components in raw buffer */
#define MIN_RAW_BUFS_420_COMP        3
#define MIN_RAW_BUFS_422ILE_COMP     1
#define MIN_RAW_BUFS_RGB565_COMP     1
#define MIN_RAW_BUFS_RGBA8888_COMP   1
#define MIN_RAW_BUFS_420SP_COMP      2

/** Maximum number of active config paramter sets */
#define MAX_ACTIVE_CONFIG_PARAMS 32

/**
******************************************************************************
 *  @brief Thresholds for luma & chroma to determine if the 8x8 subblock needs
 *  to be encoded or skipped
******************************************************************************
*/
#define LUMA_SUB_BLOCK_SKIP_THRESHOLD 4
#define LUMA_BLOCK_SKIP_THRESHOLD 5
#define CHROMA_BLOCK_SKIP_THRESHOLD 4

/**
******************************************************************************
 *  @brief      defines the first byte of a NAL unit
 *  forbidden zero bit - nal_ref_idc - nal_unit_type
******************************************************************************
*/
/* [0 - 11 - 00111] */
#define NAL_SPS_FIRST_BYTE 0x67

/* [0 - 11 - 01000] */
#define NAL_PPS_FIRST_BYTE 0x68

/* [0 - 11 - 00001] */
#define NAL_SLICE_FIRST_BYTE 0x61

/* [0 - 00 - 00001] */
#define NAL_NON_REF_SLICE_FIRST_BYTE 0x01

/* [0 - 11 - 00101] */
#define NAL_IDR_SLICE_FIRST_BYTE 0x65

/* [0 - 00 - 01100] */
#define NAL_FILLER_FIRST_BYTE 0x0C

/* [0 - 00 - 00110] */
#define NAL_SEI_FIRST_BYTE 0x06

#define H264_ALLOC_INTER_FRM_INTV        2

#define H264_MPEG_QP_MAP    255

#define MPEG2_QP_ELEM       (H264_MPEG_QP_MAP + 1)
#define H264_QP_ELEM        (MAX_H264_QP + 1)

#define H264_INIT_QUANT_I                26
#define H264_INIT_QUANT_P                34

#endif /*IH264E_DEFS_H_*/
