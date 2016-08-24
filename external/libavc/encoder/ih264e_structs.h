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
*  ih264e_structs.h
*
* @brief
*  Structure definitions used in the encoder
*
* @author
*  Harish
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_STRUCTS_H_
#define IH264E_STRUCTS_H_

/*****************************************************************************/
/* Structure definitions                                                    */
/*****************************************************************************/

/* Early declaration of structs */
typedef struct _codec_t codec_t;
typedef struct _proc_t process_ctxt_t;


/*****************************************************************************/
/* Extern Function type definitions                                          */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief      intra prediction filters leaf level
******************************************************************************
 */
typedef void (*pf_intra_pred)(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                              WORD32 src_strd, WORD32 dst_strd,
                              WORD32 ui_neighboravailability);

/**
******************************************************************************
 *  @brief      inter prediction filters leaf level
******************************************************************************
 */

typedef void (*pf_inter_pred_luma_bilinear)(UWORD8 *pu1_src1, UWORD8 *pu1_src2, UWORD8 *pu1_dst,
                                            WORD32 src_strd1, WORD32 src_strd2, WORD32 dst_strd,
                                            WORD32 height, WORD32 width);

/**
******************************************************************************
 *  @brief      fwd transform leaf level
******************************************************************************
 */
typedef void (*pf_trans_quant)(UWORD8*pu1_src, UWORD8 *pu1_pred, WORD16 *pi2_out,
                               WORD32 i4_src_stride, UWORD32 u4_pred_stride, UWORD32 u4_dst_stride,
                               const UWORD16 *pu2_scale_mat, const UWORD16 *pu2_thresh_mat,
                               UWORD32 u4_qbit, UWORD32 u4_round_fact, UWORD8 *pu1_nnz);

typedef void (*pf_iquant_itrans)(WORD16 *pi2_src, UWORD8 *pu1_pred, UWORD8 *pu1_out,
                                 WORD32 i4_src_stride, UWORD32 u4_pred_stride, UWORD32 u4_out_stride,
                                 const UWORD16 *pu2_iscale_mat, const UWORD16 *pu2_weigh_mat,
                                 UWORD32 qp_div, WORD32 *pi4_tmp);

/**
******************************************************************************
 *  @brief      Padding leaf level
******************************************************************************
 */
typedef void (*pf_pad)(UWORD8 *pu1_src, WORD32 src_strd, WORD32 wd, WORD32 pad_size);

/**
******************************************************************************
 *  @brief      memory handling leaf level
******************************************************************************
 */
typedef void (*pf_memcpy)(UWORD8 *pu1_dst, UWORD8 *pu1_src, UWORD32 num_bytes);

typedef void (*pf_memset)(UWORD8 *pu1_dst, UWORD8 value, UWORD32 num_bytes);

typedef void (*pf_memcpy_mul8)(UWORD8 *pu1_dst, UWORD8 *pu1_src, UWORD32 num_bytes);

typedef void (*pf_memset_mul8)(UWORD8 *pu1_dst, UWORD8 value, UWORD32 num_bytes);

/**
******************************************************************************
 *  @brief      Sad computation
******************************************************************************
 */
typedef void (*pf_compute_sad)(UWORD8 *pu1_src, UWORD8 *pu1_est,
                               UWORD32 src_strd, UWORD32 est_strd,
                               WORD32 i4_max_sad, WORD32 *pi4_mb_distortion);

/**
******************************************************************************
 *  @brief     Intra mode eval:encoder level
******************************************************************************
 */
typedef void (*pf_evaluate_intra_modes)(UWORD8 *pu1_src, UWORD8 *pu1_ngbr_pels_i16, UWORD8 *pu1_dst,
                                        UWORD32 src_strd, UWORD32 dst_strd,
                                        WORD32 u4_n_avblty, UWORD32 *u4_intra_mode,
                                        WORD32 *pu4_sadmin,
                                        UWORD32 u4_valid_intra_modes);

typedef void (*pf_evaluate_intra_4x4_modes)(UWORD8 *pu1_src, UWORD8 *pu1_ngbr_pels, UWORD8 *pu1_dst,
                                            UWORD32 src_strd, UWORD32 dst_strd,
                                            WORD32 u4_n_avblty, UWORD32 *u4_intra_mode,
                                            WORD32 *pu4_sadmin,
                                            UWORD32 u4_valid_intra_modes, UWORD32 u4_lambda,
                                            UWORD32 u4_predictd_mode);

/**
******************************************************************************
 *  @brief     half_pel generation :encoder level
******************************************************************************
 */
typedef void (*pf_sixtapfilter_horz)(UWORD8 *pu1_src, UWORD8 *pu1_dst,
                                     WORD32 src_strd, WORD32 dst_strd);

typedef void (*pf_sixtap_filter_2dvh_vert)(UWORD8 *pu1_src, UWORD8 *pu1_dst1, UWORD8 *pu1_dst2,
                                           WORD32 src_strd, WORD32 dst_strd,
                                           WORD32 *pi16_pred1,
                                           WORD32 pi16_pred1_strd);
/**
******************************************************************************
 *  @brief     color space conversion
******************************************************************************
 */
typedef void (*pf_fmt_conv_420p_to_420sp)(UWORD8 *pu1_y_src, UWORD8 *pu1_u_src, UWORD8 *pu1_v_src,
                                          UWORD8 *pu1_y_dst, UWORD8 *pu1_uv_dst,
                                          UWORD16 u2_height, UWORD16 u2_width,
                                          UWORD16 src_y_strd, UWORD16 src_u_strd, UWORD16 src_v_strd,
                                          UWORD16 dst_y_strd, UWORD16 dst_uv_strd,
                                          UWORD32 convert_uv_only);

typedef void (*pf_fmt_conv_422ile_to_420sp)(UWORD8 *pu1_y_buf, UWORD8 *pu1_u_buf, UWORD8 *pu1_v_buf,
                                            UWORD8 *pu1_422i_buf,
                                            WORD32 u4_y_width, WORD32 u4_y_height, WORD32 u4_y_stride,
                                            WORD32 u4_u_stride, WORD32 u4_v_stride,
                                            WORD32 u4_422i_stride);



/**
******************************************************************************
 *  @brief     ME evaluation
******************************************************************************
 */
typedef void ih264e_compute_me_ft(process_ctxt_t *);

/**
******************************************************************************
 *  @brief     SKIP decision
******************************************************************************
 */
typedef WORD32 ih264e_skip_params_ft(process_ctxt_t *, WORD32);


/*****************************************************************************/
/* Enums                                                                     */
/*****************************************************************************/

/**
 ******************************************************************************
 *  @enum  CODEC_STATE_T
 *  @brief codec state
 ******************************************************************************
 */
typedef enum
{
    INIT_DONE,
    HEADER_DONE,
    FIRST_FRAME_DONE,
} CODEC_STATE_T;


/**
 ******************************************************************************
 *  @enum  JOBQ_CMD_T
 *  @brief list of job commands (used during job instantiation)
 ******************************************************************************
 */
typedef enum
{
    CMD_PROCESS,
    CMD_ENTROPY,
    CMD_FMTCONV,
    CMD_ME,
}JOBQ_CMD_T;


/*****************************************************************************/
/* Structures                                                                */
/*****************************************************************************/

/**
 * PU information
 */
typedef struct
{
    /**
     *  Motion Vector
     */
    mv_t s_mv;

    /**
     *  Ref index
     */
    WORD8   i1_ref_idx;

} enc_pu_mv_t;


/*
 * Total Pu info for an MB
 */
typedef struct
{

    /* Array with ME info for all lists */
    enc_pu_mv_t  s_me_info[2];

    /**
     *  PU X position in terms of min PU (4x4) units
     */
    UWORD32     b4_pos_x        : 4;

    /**
     *  PU Y position in terms of min PU (4x4) units
     */
    UWORD32     b4_pos_y        : 4;

    /**
     *  PU width in pixels = (b4_wd + 1) << 2
     */
    UWORD32     b4_wd           : 2;

    /**
     *  PU height in pixels = (b4_ht + 1) << 2
     */
    UWORD32     b4_ht           : 2;

    /**
     *  Intra or Inter flag for each partition - 0 or 1
     */
    UWORD32     b1_intra_flag   : 1;

    /**
     *  PRED_L0, PRED_L1, PRED_BI
     */
    UWORD32     b2_pred_mode    : 2;


} enc_pu_t;


typedef struct
{
    /** Descriptor of raw buffer                                     */
    iv_raw_buf_t                            s_raw_buf;

    /** Lower 32bits of time stamp corresponding to the above buffer */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to the above buffer */
    UWORD32                                 u4_timestamp_high;

    /** Flag to indicate if the current buffer is last buffer */
    UWORD32                                 u4_is_last;

    /** Flag to indicate if mb info is sent along with input buffer     */
    UWORD32                                 u4_mb_info_type;

    /** Flag to indicate the size of mb info structure                  */
    UWORD32                                 u4_mb_info_size;

    /** Buffer containing mb info if mb_info_type is non-zero           */
    void                                    *pv_mb_info;

    /** Flag to indicate if pic info is sent along with input buffer     */
    UWORD32                                 u4_pic_info_type;

    /** Buffer containing pic info if mb_info_type is non-zero           */
    void                                    *pv_pic_info;

}inp_buf_t;

typedef struct
{
    /** Descriptor of bitstream buffer                                     */
    iv_bits_buf_t                           s_bits_buf;

    /** Lower 32bits of time stamp corresponding to the above buffer */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to the above buffer */
    UWORD32                                 u4_timestamp_high;

    /** Flag to indicate if the current buffer is last buffer */
    UWORD32                                 u4_is_last;

}out_buf_t;

typedef struct
{
    /** Descriptor of picture buffer                                     */
    pic_buf_t                               s_pic_buf;

    /** Lower 32bits of time stamp corresponding to the above buffer */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to the above buffer */
    UWORD32                                 u4_timestamp_high;

    /** Flag to indicate if the current buffer is last buffer */
    UWORD32                                 u4_is_last;

    /** Picture count corresponding to current picture */
    WORD32                                  i4_pic_cnt;

}rec_buf_t;

typedef struct
{
    /** maximum width for which codec should request memory requirements    */
    UWORD32                                     u4_max_wd;

    /** maximum height for which codec should request memory requirements   */
    UWORD32                                     u4_max_ht;

    /** Maximum number of reference frames                                  */
    UWORD32                                     u4_max_ref_cnt;

    /** Maximum number of reorder frames                                    */
    UWORD32                                     u4_max_reorder_cnt;

    /** Maximum level supported                                             */
    UWORD32                                     u4_max_level;

    /** Input color format                                                  */
    IV_COLOR_FORMAT_T                           e_inp_color_fmt;

    /** Flag to enable/disable - To be used only for debugging/testing      */
    UWORD32                                     u4_enable_recon;

    /** Recon color format                                                  */
    IV_COLOR_FORMAT_T                           e_recon_color_fmt;

    /** Encoder Speed preset - Value between 0 (slowest) and 100 (fastest)  */
    IVE_SPEED_CONFIG                            u4_enc_speed_preset;

    /** Rate control mode                                                   */
    IVE_RC_MODE_T                               e_rc_mode;

    /** Maximum frame rate to be supported                                  */
    UWORD32                                     u4_max_framerate;

    /** Maximum bitrate to be supported                                     */
    UWORD32                                     u4_max_bitrate;

    /** Maximum number of consecutive  B frames                             */
    UWORD32                                     u4_num_bframes;

    /** Content type Interlaced/Progressive                                 */
    IV_CONTENT_TYPE_T                           e_content_type;

    /** Maximum search range to be used in X direction                      */
    UWORD32                                     u4_max_srch_rng_x;

    /** Maximum search range to be used in Y direction                      */
    UWORD32                                     u4_max_srch_rng_y;

    /** Slice Mode                                                          */
    IVE_SLICE_MODE_T                            e_slice_mode;

    /** Slice parameter                                                     */
    UWORD32                                     u4_slice_param;

    /** Processor architecture                                          */
    IV_ARCH_T                                   e_arch;

    /** SOC details                                                     */
    IV_SOC_T                                    e_soc;

    /** Input width to be sent in bitstream                                */
    UWORD32                                     u4_disp_wd;

    /** Input height to be sent in bitstream                               */
    UWORD32                                     u4_disp_ht;

    /** Input width                                                     */
    UWORD32                                     u4_wd;

    /** Input height                                                    */
    UWORD32                                     u4_ht;

    /** Input stride                                                    */
    UWORD32                                     u4_strd;

    /** Source frame rate                                               */
    UWORD32                                     u4_src_frame_rate;

    /** Target frame rate                                               */
    UWORD32                                     u4_tgt_frame_rate;

    /** Target bitrate in kilobits per second                           */
    UWORD32                                     u4_target_bitrate;

    /** Force current frame type                                        */
    IV_PICTURE_CODING_TYPE_T                    e_frame_type;

    /** Encoder mode                                                    */
    IVE_ENC_MODE_T                              e_enc_mode;

    /** Set initial Qp for I pictures                                   */
    UWORD32                                     u4_i_qp;

    /** Set initial Qp for P pictures                                   */
    UWORD32                                     u4_p_qp;

    /** Set initial Qp for B pictures                                   */
    UWORD32                                     u4_b_qp;

    /** Set minimum Qp for I pictures                                   */
    UWORD32                                     u4_i_qp_min;

    /** Set maximum Qp for I pictures                                   */
    UWORD32                                     u4_i_qp_max;

    /** Set minimum Qp for P pictures                                   */
    UWORD32                                     u4_p_qp_min;

    /** Set maximum Qp for P pictures                                   */
    UWORD32                                     u4_p_qp_max;

    /** Set minimum Qp for B pictures                                   */
    UWORD32                                     u4_b_qp_min;

    /** Set maximum Qp for B pictures                                   */
    UWORD32                                     u4_b_qp_max;

    /** Adaptive intra refresh mode                                     */
    IVE_AIR_MODE_T                              e_air_mode;

    /** Adaptive intra refresh period in frames                         */
    UWORD32                                     u4_air_refresh_period;

    /** VBV buffer delay                                                */
    UWORD32                                     u4_vbv_buffer_delay;

    /** VBV buffer size                                                 */
    UWORD32                                     u4_vbv_buf_size;

    /** Number of cores to be used                                      */
    UWORD32                                     u4_num_cores;

    /** ME speed preset - Value between 0 (slowest) and 100 (fastest)      */
    UWORD32                                     u4_me_speed_preset;

    /** Flag to enable/disable half pel motion estimation               */
    UWORD32                                     u4_enable_hpel;

    /** Flag to enable/disable quarter pel motion estimation            */
    UWORD32                                     u4_enable_qpel;

    /** Flag to enable/disable intra 4x4 analysis                       */
    UWORD32                                     u4_enable_intra_4x4;

    /** Flag to enable/disable intra 8x8 analysis                       */
    UWORD32                                     u4_enable_intra_8x8;

    /** Flag to enable/disable intra 16x16 analysis                     */
    UWORD32                                     u4_enable_intra_16x16;

    /** Flag to enable/disable fast SAD approximation                   */
    UWORD32                                     u4_enable_fast_sad;

    /*flag to enable/disable alternate reference frames                 */
    UWORD32                                     u4_enable_alt_ref;

    /*Flag to enable/disable computation of SATDQ in ME*/
    UWORD32                                     u4_enable_satqd;

    /*Minimum SAD to search for*/
    WORD32                                     i4_min_sad;

    /** Maximum search range in X direction for farthest reference      */
    UWORD32                                     u4_srch_rng_x;

    /** Maximum search range in Y direction for farthest reference      */
    UWORD32                                     u4_srch_rng_y;

    /** I frame interval                                                */
    UWORD32                                     u4_i_frm_interval;

    /** IDR frame interval                                              */
    UWORD32                                     u4_idr_frm_interval;

    /** Disable deblock level (0: Enable completely, 3: Disable completely */
    UWORD32                                     u4_disable_deblock_level;

    /** Profile                                                         */
    IV_PROFILE_T                                e_profile;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                     u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                     u4_timestamp_high;

    /** Flag to say if the current config parameter set is valid
     * Will be zero to start with and will be set to 1, when configured
     * Once encoder uses the parameter set, this will be set to zero */
    UWORD32                                     u4_is_valid;

    /** Command associated with this config param set */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_cmd;

    /** Input width in mbs                                                    */
    UWORD32                                     i4_wd_mbs;

    /** Input height in mbs                                                   */
    UWORD32                                     i4_ht_mbs;

    /** entropy coding mode flag                                              */
    UWORD32                                     u4_entropy_coding_mode;

    /** enable weighted prediction                                            */
    UWORD32                                     u4_weighted_prediction;

    /** enable constrained intra prediction                                   */
    UWORD32                                     u4_constrained_intra_pred;

    /** Pic info type */
    UWORD32                                     u4_pic_info_type;
    /**
     * MB info type
     */
    UWORD32                                     u4_mb_info_type;

}cfg_params_t;



/** Structure to hold format conversion context */
typedef struct
{
    /** Current row for which format conversion should be done */
    WORD32 i4_cur_row;

    /** Number of rows for which format conversion should be done */
    WORD32 i4_num_rows;

}fmt_conv_t;


/**
 * Structure to represent a processing job entry
 */
typedef struct
{
    /**
     * Command
     */
    WORD32 i4_cmd;

    /**
     * MB x of the starting MB
     */
    WORD16 i2_mb_x;

    /**
     * MB y of the starting MB
     */

    WORD16 i2_mb_y;

    /**
     * Number of MBs that need to be processed in this job
     */
    WORD16 i2_mb_cnt;

    /**
     * Process contexts base index
     * Will toggle between 0 and MAX_PROCESS_THREADS
     */
    WORD16 i2_proc_base_idx;

} job_t;


/**
 * Structure to represent a MV Bank buffer
 */
typedef struct
{
    /**
     *  Pointer to hold num PUs each MB in a picture
     */
    UWORD32 *pu4_mb_pu_cnt;

    /**
     * Pointer to hold enc_pu_t for each PU in a picture
     */
    enc_pu_t *ps_pic_pu;

    /**
     * Pointer to hold PU map for each MB in a picture
     */
    UWORD8 *pu1_pic_pu_map;

    /**
     * Pointer to hold the Slice map
     */
    UWORD16 *pu1_pic_slice_map;

    /**
     * Absolute POC for the current MV Bank
     */
    WORD32 i4_abs_poc;

    /**
     * Buffer Id
     */
    WORD32     i4_buf_id;

} mv_buf_t;


/**
 * Reference set containing pointers to MV buf and pic buf
 */
typedef struct
{
    /** Picture count */
    WORD32    i4_pic_cnt;

    /** POC */
    WORD32    i4_poc;

    /** picture buffer */
    pic_buf_t *ps_pic_buf;

    /** mv buffer */
    mv_buf_t  *ps_mv_buf;

}ref_set_t;

typedef struct
{

    /**
     * Pointer to current PPS
     */
    pps_t *ps_pps;

    /**
     * Pointer to current SPS
     */
    sps_t *ps_sps;

    /**
     * Pointer to current slice header structure
     */
    slice_header_t *ps_slice_hdr;

    /**
     * MB's x position within a picture in raster scan in MB units
     */
    WORD32 i4_mb_x;

    /**
     * MB's y position within a picture in raster scan in MB units
     */

    WORD32 i4_mb_y;

    /**
     * Current PU structure - set to MB enc_pu_t pointer at the start of MB processing and incremented
     * for every TU
     */
    enc_pu_t *ps_pu;

    /**
     * Pointer to frame level enc_pu_t for the current frame being parsed
     * where MVs and Intra pred modes will be updated
     */
    enc_pu_t *ps_pic_pu;

    /**
     *  Pointer to hold num PUs each MB in a picture
     */
    UWORD32 *pu4_mb_pu_cnt;

    /** PU Index map per MB. The indices in this map are w.r.t picture pu array and not
     * w.r.t MB pu array.
     * This will be used during mv prediction and since neighbors will have different MB pu map
     * it will be easier if they all have indices w.r.t picture level PU array rather than MB level
     * PU array.
     * pu1_pic_pu_map is map w.r.t MB's enc_pu_t array
     */
    UWORD32 *pu4_pic_pu_idx_map;

    /**
      * Pointer to pu_map for the current frame being parsed
      * where MVs and Intra pred modes will be updated
      */
     UWORD8 *pu1_pic_pu_map;

     /**
      *  PU count in current MB
      */
     WORD32 i4_mb_pu_cnt;

     /**
      *  PU count in current MB
      */
     WORD32 i4_mb_start_pu_idx;

     /**
      *  Top availability for current MB level
      */
     UWORD8 u1_top_mb_avail;

     /**
      *  Top right availability for current MB level
      */
     UWORD8 u1_top_rt_mb_avail;
     /**
      *  Top left availability for current MB level
      */
     UWORD8 u1_top_lt_mb_avail;
     /**
      *  left availability for current MB level
      */
     UWORD8 u1_left_mb_avail;

}mv_ctxt_t;

typedef struct
{
    /**
     * MB's x position within a picture in raster scan in MB units
     */
    WORD32 i4_mb_x;

    /**
     * MB's y position within a picture in raster scan in MB units
     */
    WORD32 i4_mb_y;

    /**
     * MB's x position within a Slice in raster scan in MB units
     */
    WORD32 i4_mb_slice_x;

    /**
     * MB's y position within a Slice in raster scan in MB units
     */
    WORD32 i4_mb_slice_y;

    /**
     * Vertical strength, Two bits per edge.
     * Stored in format. BS[15] | BS[14] | .. |BS[0]
     */
    UWORD32 *pu4_pic_vert_bs;

    /**
     * Boundary strength, Two bits per edge.
     * Stored in format. BS[15] | BS[14] | .. |BS[0]
     */
    UWORD32 *pu4_pic_horz_bs;

    /**
     *  Qp array stored for each mb
     */
    UWORD8  *pu1_pic_qp;

}bs_ctxt_t;

typedef struct
{
    /**
     * MB's x position within a picture in raster scan in MB units
     */
    WORD32 i4_mb_x;

    /**
     * MB's y position within a picture in raster scan in MB units
     */
    WORD32 i4_mb_y;

    /**
     * structure that contains BS and QP frame level arrays
     */
    bs_ctxt_t s_bs_ctxt;

    /**
     * Pointer to 0th luma pixel in current pic
     */
    UWORD8 *pu1_cur_pic_luma;

    /**
     * Pointer to 0th chroma pixel in current pic
     */
    UWORD8 *pu1_cur_pic_chroma;

    /**
     *  Points to the array of slice indices which is used to identify the slice
     *  to which each MB in a frame belongs.
     */
    UWORD8 *pu1_slice_idx;

}deblk_ctxt_t;


/**
 ******************************************************************************
 *  @brief      Structure to hold data and flags for 'n' mb processing for
 *                deblocking , padding and half pel generation.
 ******************************************************************************
 */
typedef struct
{
    /**
     * MB's x position last processed + 1
     */
    WORD32 i4_mb_x;

    /**
     * MB's y position ,current processing.
     */
    WORD32 i4_mb_y;

    /**
     * Number of MBs processed in a stretch
     */
    WORD32 i4_n_mbs;

}n_mb_process_ctxt_t;


/**
******************************************************************************
 *  @brief      Structure to hold coefficient info for a 4x4 subblock.
 *  The following can be used to type-cast coefficient data that is stored
 *  per subblock. Note that though i2_level is shown as an array that
 *  holds 16 coefficients, only the first few entries will be valid. Next
 *  subblocks data starts after the valid number of coefficients. Number
 *  of non-zero coefficients will be derived using number of non-zero bits
 *  in sig coeff map
******************************************************************************
 */
typedef struct
{
    /**
     * significant coefficient map and nnz are packed in
     * to msb (2 bytes) and lsb (2 bytes) respectively
     */
    WORD32  i4_sig_map_nnz;

    /**
     * array of non zero residue coefficients
     */
    WORD16  ai2_residue[16];

}tu_sblk_coeff_data_t;

/**
******************************************************************************
 *  @brief      Structure contains few common state variables such as MB indices,
 *  current SPS, PPS etc which are to be used in the entropy thread. By keeping
 *  it a different structure it is being explicitly signaled that these
 * variables are specific to entropy threads context and other threads should
 * not update these elements
******************************************************************************
 */
typedef struct
{
    /**
     * Pointer to the cabac context
     */
    cabac_ctxt_t *ps_cabac;

    /**
     * start of frame / start of slice flag
     */
    WORD32 i4_sof;

    /**
     * end of frame / end of slice flag
     */
    WORD32 i4_eof;

    /**
     * generate header upon request
     */
    WORD32 i4_gen_header;

    /**
     *  seq_parameter_set_id
     */
    UWORD32 u4_sps_id;

    /**
     * Pointer to base of sequence parameter set structure array
     */
    sps_t *ps_sps_base;

    /**
     *  pic_parameter_set_id
     */
    UWORD32 u4_pps_id;

    /**
     * Pointer to base of Picture parameter set structure array
     */
    pps_t *ps_pps_base;

    /**
     * Current slice idx
     */
    WORD32 i4_cur_slice_idx;

    /**
     * Points to the array of slice indices which is used to identify the independent slice
     * to which each MB in a frame belongs.
     */
    UWORD8 *pu1_slice_idx;

    /**
     * Pointer to base of slice header structure array
     */
    slice_header_t *ps_slice_hdr_base;

    /**
     * entropy status
     */
    UWORD8  *pu1_entropy_map;

    /**
     * MB's x position within a picture in raster scan in MB units
     */
    WORD32 i4_mb_x;

    /**
     * MB's y position within a picture in raster scan in MB units
     */
    WORD32 i4_mb_y;

    /**
     * MB start address
     */
    WORD32 i4_mb_cnt;

    /**
     * MB start address
     */
    WORD32 i4_mb_start_add;

    /**
     * MB end address
     */
    WORD32 i4_mb_end_add;

    /**
     * Input width in mbs
     */
    WORD32 i4_wd_mbs;

    /**
     * Input height in mbs
     */
    WORD32 i4_ht_mbs;

    /**
     * Bitstream structure
     */
    bitstrm_t *ps_bitstrm;

    /**
     *  transform_8x8_mode_flag
     */
    WORD8 i1_transform_8x8_mode_flag;

    /**
     *  entropy_coding_mode_flag
     */
    WORD8 u1_entropy_coding_mode_flag;

    /**
     * Pointer to the top row nnz for luma
     */
    UWORD8 (*pu1_top_nnz_luma)[4];

    /**
     * left nnz for luma
     */
    UWORD32 u4_left_nnz_luma;

    /**
     * Pointer to zero runs before for the mb
     */
    UWORD8  au1_zero_run[16];

    /**
     * Pointer to the top row nnz for chroma
     */
    UWORD8 (*pu1_top_nnz_cbcr)[4];

    /**
     * left nnz for chroma
     */
    UWORD8 u4_left_nnz_cbcr;

    /**
     * Pointer frame level mb subblock coeff data
     */
    void *pv_pic_mb_coeff_data;

    /**
     * Pointer to mb subblock coeff data and number of subblocks and scan idx
     * Incremented each time a coded subblock is processed
     */
    void *pv_mb_coeff_data;

    /**
     * Pointer frame level mb header data
     */
    void *pv_pic_mb_header_data;

    /**
     * Pointer to mb header data and
     * incremented each time a coded mb is encoded
     */
    void *pv_mb_header_data;

    /**
     * Error code during parse stage
     */
    IH264E_ERROR_T i4_error_code;

    /**
     * Void pointer to job context
     */
    void *pv_proc_jobq, *pv_entropy_jobq;

    /**
     * Flag to signal end of frame
     */
    WORD32 i4_end_of_frame;

    /**
     * Abs POC count of the frame
     */
     WORD32 i4_abs_pic_order_cnt;

     /**
      * mb skip run
      */
     WORD32 *pi4_mb_skip_run;

     /**
      * Flag to signal end of sequence
      */
     UWORD32 u4_is_last;

     /**
      * Lower 32bits of time-stamp corresponding to the buffer being encoded
      */
     UWORD32 u4_timestamp_low;

     /**
      * Upper 32bits of time-stamp corresponding to the buffer being encoded
      */
     UWORD32 u4_timestamp_high;

     /**
      * Current Picture count - used for synchronization
      */
     WORD32  i4_pic_cnt;

     /**
      * Number of bits consumed by header for I and P mb types
      */
     UWORD32 u4_header_bits[MAX_MB_TYPE];

     /**
      * Number of bits consumed by residue for I and P mb types
      */
     UWORD32 u4_residue_bits[MAX_MB_TYPE];

} entropy_ctxt_t;

/**
******************************************************************************
*  @brief      macro block info.
******************************************************************************
*/
typedef struct
{
    /**
     * mb type
     */
    UWORD16 u2_is_intra;

    /**
     * mb type
     */
    UWORD16 u2_mb_type;

    /**
     * csbp
     */
    UWORD32 u4_csbp;

    /**
     * mb distortion
     */
    WORD32 i4_mb_distortion;

}mb_info_t;

/**
******************************************************************************
*  @brief      structure presenting the neighbor availability of a mb
*  or subblk or any other partition
******************************************************************************
*/
typedef struct
{
    /**
     * left blk/subblk/partition
     */
    UWORD8 u1_mb_a;

    /**
     * top blk/subblk/partition
     */
    UWORD8 u1_mb_b;

    /**
     * topright blk/subblk/partition
     */
    UWORD8 u1_mb_c;

    /**
     * topleft blk/subblk/partition
     */
    UWORD8 u1_mb_d;

}block_neighbors_t;

/**
 ******************************************************************************
 *  @brief      MB info  related variables used during NMB processing
 ******************************************************************************
 */
typedef struct
{
    UWORD32 u4_mb_type;
    UWORD32 u4_min_sad;
    UWORD32 u4_min_sad_reached;
    WORD32  i4_mb_cost;
    WORD32  i4_mb_distortion;

    enc_pu_mv_t as_skip_mv[4];

    enc_pu_mv_t as_pred_mv[2];

    block_neighbors_t s_ngbr_avbl;

    /*
     * Buffer to hold best subpel buffer in each MB of NMB
     */
    UWORD8 *pu1_best_sub_pel_buf;

    /*
     * Stride for subpel buffer
     */
    UWORD32 u4_bst_spel_buf_strd;

}mb_info_nmb_t;

/**
 ******************************************************************************
 *  @brief      Pixel processing thread context
 ******************************************************************************
 */
struct _proc_t
{
    /**
     * entropy context
     */
    entropy_ctxt_t s_entropy;

    /**
     * me context
     */
    me_ctxt_t s_me_ctxt;

    /**
     * Pointer to codec context
     */
    codec_t *ps_codec;

    /**
     * N mb process contest
     */
    n_mb_process_ctxt_t s_n_mb_ctxt;

    /**
     * Source pointer to current MB luma
     */
    UWORD8 *pu1_src_buf_luma;

    /**
     * Source pointer to current MB chroma
     */
    UWORD8 *pu1_src_buf_chroma;

    /**
     * Recon pointer to current MB luma
     */
    UWORD8 *pu1_rec_buf_luma;

    /**
     * Recon pointer to current MB chroma
     */
    UWORD8 *pu1_rec_buf_chroma;

    /**
     * Ref pointer to current MB luma
     */
    UWORD8 *apu1_ref_buf_luma[MAX_REF_PIC_CNT];

    /**
     * Ref pointer to current MB chroma
     */
    UWORD8 *apu1_ref_buf_chroma[MAX_REF_PIC_CNT];

    /**
     * pointer to luma plane of input buffer (base :: mb (0,0))
     */
    UWORD8 *pu1_src_buf_luma_base;

    /**
     * pointer to luma plane of reconstructed buffer (base :: mb (0,0))
     */
    UWORD8 *pu1_rec_buf_luma_base;

    /**
     * pointer to luma plane of ref buffer (base :: mb (0,0))
     */
    UWORD8 *apu1_ref_buf_luma_base[MAX_REF_PIC_CNT];

    /**
     * pointer to  chroma plane of input buffer (base :: mb (0,0))
     */
    UWORD8 *pu1_src_buf_chroma_base;

    /*
     * Buffer for color space conversion of luma
     */
    UWORD8 *pu1_y_csc_buf;

    /*
     * Buffer for color space conversion of luma
     */

    UWORD8 *pu1_uv_csc_buf;

    /**
     * pointer to  chroma plane of reconstructed buffer (base :: mb (0,0))
     */
    UWORD8 *pu1_rec_buf_chroma_base;

    /**
     * pointer to  chroma plane of reconstructed buffer (base :: mb (0,0))
     */
    UWORD8 *apu1_ref_buf_chroma_base[MAX_REF_PIC_CNT];

    /**
     * Pointer to ME NMB info
     */
    mb_info_nmb_t *ps_nmb_info;

    mb_info_nmb_t *ps_cur_mb;

    /**
     * source luma stride
     */
    WORD32 i4_src_strd;

    /**
     * source chroma stride
     */
    WORD32 i4_src_chroma_strd;

    /**
     * recon stride & ref stride
     * (strides for luma and chroma are the same)
     */
    WORD32 i4_rec_strd;

    /**
     * Offset for half pel x plane from the pic buf
     */
    UWORD32 u4_half_x_offset;

    /**
     * Offset for half pel y plane from half x plane
     */
    UWORD32 u4_half_y_offset;

    /**
     * Offset for half pel xy plane from half y plane
     */
    UWORD32 u4_half_xy_offset;

    /**
     * pred buffer pointer (temp buffer 1)
     */
    UWORD8 *pu1_pred_mb;

    /**
     * pred buffer pointer (prediction buffer for intra 16x16
     */
    UWORD8 *pu1_pred_mb_intra_16x16;

    /**
     * pred buffer pointer (prediction buffer for intra 16x16_plane
     */
    UWORD8 *pu1_pred_mb_intra_16x16_plane;

    /**
     * pred buffer pointer (prediction buffer for intra chroma
     */
    UWORD8 *pu1_pred_mb_intra_chroma;

    /**
     * pred buffer pointer (prediction buffer for intra chroma plane
     */
    UWORD8 *pu1_pred_mb_intra_chroma_plane;

    /**
     * temp. reference buffer ptr for intra 4x4 when rdopt is on
     */
    UWORD8 *pu1_ref_mb_intra_4x4;

    /**
     * prediction buffer stride
     */
    WORD32 i4_pred_strd;

    /**
     * transform buffer pointer (temp buffer 2)
     */
    WORD16 *pi2_res_buf;

    /**
     * temp. transform buffer ptr for intra 4x4 when rdopt is on
     */
    WORD16 *pi2_res_buf_intra_4x4;

    /**
     * transform buffer stride
     */
    WORD32 i4_res_strd;

    /**
     * scratch buffer for inverse transform (temp buffer 3)
     */
    void *pv_scratch_buff;

    /**
     * frame num
     */
    WORD32 i4_frame_num;

    /**
     * start address of frame / sub-frame
     */
    WORD32 i4_frame_strt_add;

    /**
     *  IDR pic
     */
    UWORD32 u4_is_idr;

    /**
     *  idr_pic_id
     */
    UWORD32 u4_idr_pic_id;

    /**
     * Input width in mbs
     */
    WORD32 i4_wd_mbs;

    /**
     * Input height in mbs
     */
    WORD32 i4_ht_mbs;

    /**
     *  slice_type
     */
    WORD32  i4_slice_type;

    /**
     * Current slice idx
     */
    WORD32 i4_cur_slice_idx;

    /**
     * MB's x position within a picture in raster scan in MB units
     */
    WORD32 i4_mb_x;

    /**
     * MB's y position within a picture in raster scan in MB units
     */
    WORD32 i4_mb_y;

    /**
     * MB's x position within a Slice in raster scan in MB units
     */
    WORD32 i4_mb_slice_x;

    /**
     * MB's y position within a Slice in raster scan in MB units
     */
    WORD32 i4_mb_slice_y;

    /**
     *  mb type
     */
    UWORD32 u4_mb_type;

    /**
     *  is intra
     */
    UWORD32 u4_is_intra;

    /**
     * mb neighbor availability pointer
     */
    block_neighbors_t *ps_ngbr_avbl;

    /**
     * lambda (lagrange multiplier for cost computation)
     */
    UWORD32 u4_lambda;

    /**
     * mb distortion
     */
    WORD32 i4_mb_distortion;

    /**
     * mb cost
     */
    WORD32 i4_mb_cost;

    /********************************************************************/
    /* i4_ngbr_avbl_mb_16 - ngbr avbl of curr mb                        */
    /* i4_ngbr_avbl_sb_8 - ngbr avbl of all 8x8 sub blocks of curr mb   */
    /* i4_ngbr_avbl_sb_4 - ngbr avbl of all 4x4 sub blocks of curr mb   */
    /* i4_ngbr_avbl_mb_c - chroma ngbr avbl of curr mb                  */
    /********************************************************************/
    WORD32  i4_ngbr_avbl_16x16_mb;
    WORD32  ai4_neighbor_avail_8x8_subblks[4];
    UWORD8  au1_ngbr_avbl_4x4_subblks[16];
    WORD32  i4_chroma_neighbor_avail_8x8_mb;

    /**
     * array to store the mode of mb sub blocks
     */
    UWORD8  au1_intra_luma_mb_4x4_modes[16];

    /**
     * array to store the predicted mode of mb sub blks
     */
    UWORD8  au1_predicted_intra_luma_mb_4x4_modes[16];

    /**
     * macro block intra 16x16 mode
     */
    UWORD8  u1_l_i16_mode;

    /**
     * array to store the mode of the macro block intra 8x8 4 modes
     */
    UWORD8  au1_intra_luma_mb_8x8_modes[4];

    /**
     * intra chroma mb mode
     */
    UWORD8  u1_c_i8_mode;

    /********************************************************************/
    /* array to store pixels from the neighborhood for intra prediction */
    /* i16 - 16 left pels + 1 top left pel + 16 top pels = 33 pels      */
    /* i8 - 8 lpels + 1 tlpels + 8 tpels + 8 tr pels = 25 pels          */
    /* i4 - 4 lpels + 1 tlpels + 4 tpels + 4 tr pels = 13 pels          */
    /* ic - 8 left pels + 1 top left pel + 8 top pels )*2               */
    /********************************************************************/
    UWORD8 au1_ngbr_pels[34];

    /**
     * array for 8x8 intra pels filtering (temp buff 4)
     */
    UWORD8 au1_neighbor_pels_i8x8_unfiltered[25];

    /**
     * Number of sub partitons in the inter pred MB
     */
    UWORD32 u4_num_sub_partitions;

    /**
     *  Pointer to hold num PUs each MB in a picture
     */
    UWORD32 *pu4_mb_pu_cnt;

    /**
     * Pointer to the array of structures having motion vectors, size
     *  and position of sub partitions
     */
    enc_pu_t *ps_pu;

    /**
     * Pointer to the pu of current co-located MB in list 1
     */
    enc_pu_t *ps_colpu;

    /**
     * predicted motion vector
     */
    enc_pu_mv_t *ps_skip_mv;

    /**
     * predicted motion vector
     */
    enc_pu_mv_t *ps_pred_mv;

    /**
     * top row mb syntax information base
     * In normal working scenarios, for a given context set,
     * the mb syntax info pointer is identical across all process threads.
     * But when the hard bound on slices are enabled, in multi core, frame
     * is partitioned in to sections equal to set number of cores and each
     * partition is run independently. In this scenario, a ctxt set will alone
     * appear to run multiple frames at a time. For this to occur, the common
     * pointers across the proc ctxt should disappear.
     *
     * This is done by allocating MAX_PROCESS_THREADS memory and distributing
     * across individual ctxts when byte bnd per slice is enabled.
     */
    mb_info_t *ps_top_row_mb_syntax_ele_base;

    /**
     * top row mb syntax information
     */
    mb_info_t *ps_top_row_mb_syntax_ele;

    /**
     * left mb syntax information
     */
    mb_info_t s_left_mb_syntax_ele;

    /**
     * top left mb syntax information
     */
    mb_info_t s_top_left_mb_syntax_ele;

    /**
     * top left mb syntax information
     */

    mb_info_t s_top_left_mb_syntax_ME;

    /**
     * left mb motion vector
     */
    enc_pu_t s_left_mb_pu_ME;

    /**
     * top left mb motion vector
     */
    enc_pu_t s_top_left_mb_pu_ME;

    /**
     * mb neighbor availability pointer
     */
    block_neighbors_t s_ngbr_avbl;

    /**
     * In case the macroblock type is intra, the intra modes of all
     * partitions for the left mb are stored in the array below
     */
    UWORD8 au1_left_mb_intra_modes[16];

    /**
     * In case the macroblock type is intra, the intra modes of all
     * partitions for the top mb are stored in the array below
     *
     * In normal working scenarios, for a given context set,
     * the mb syntax info pointer is identical across all process threads.
     * But when the hard bound on slices are enabled, in multi core, frame
     * is partitioned in to sections equal to set number of cores and each
     * partition is run independently. In this scenario, a ctxt set will alone
     * appear to run multiple frames at a time. For this to occur, the common
     * pointers across the proc ctxt should disappear.
     *
     * This is done by allocating MAX_PROCESS_THREADS memory and distributing
     * across individual ctxts when byte bnd per slice is enabled.
     */
    UWORD8 *pu1_top_mb_intra_modes_base;

    /**
     * In case the macroblock type is intra, the intra modes of all
     * partitions for the top mb are stored in the array below
     */
    UWORD8 *pu1_top_mb_intra_modes;

    /**
     * left mb motion vector
     */
    enc_pu_t s_left_mb_pu;

    /**
     * top left mb motion vector
     */
    enc_pu_t s_top_left_mb_pu;

    /**
     * top row motion vector info
     *
     * In normal working scenarios, for a given context set,
     * the top row pu pointer is identical across all process threads.
     * But when the hard bound on slices are enabled, in multi core, frame
     * is partitioned in to sections equal to set number of cores and each
     * partition is run independently. In this scenario, a ctxt set will alone
     * appear to run multiple frames at a time. For this to occur, the common
     * pointers across the proc ctxt should disappear.
     *
     * This is done by allocating MAX_PROCESS_THREADS memory and distributing
     * across individual ctxts when byte bnd per slice is enabled.
     */
    enc_pu_t *ps_top_row_pu_base;

    /**
     * top row motion vector info
     */
    enc_pu_t *ps_top_row_pu;

    enc_pu_t *ps_top_row_pu_ME;

    /**
     * coded block pattern
     */
    UWORD32 u4_cbp;

    /**
     * csbp
     */
    UWORD32 u4_csbp;

    /**
     *  number of non zero coeffs
     */
    UWORD32 au4_nnz[5];

    /**
     *  number of non zero coeffs for intra 4x4 when rdopt is on
     */
    UWORD32 au4_nnz_intra_4x4[4];

    /**
     * frame qp & mb qp
     */
    UWORD32 u4_frame_qp, u4_mb_qp;

    /**
     * mb qp previous
     */
    UWORD32 u4_mb_qp_prev;

    /**
     * quantization parameters for luma & chroma planes
     */
    quant_params_t *ps_qp_params[3];

    /**
     * Pointer frame level mb subblock coeff data
     */
    void *pv_pic_mb_coeff_data;

    /**
     * Pointer to mb subblock coeff data and number of subblocks and scan idx
     * Incremented each time a coded subblock is processed
     */
    void *pv_mb_coeff_data;

    /**
     * Pointer frame level mb header data
     */
    void *pv_pic_mb_header_data;

    /**
     * Pointer to mb header data and
     * incremented each time a coded mb is encoded
     */
    void *pv_mb_header_data;

    /**
     * Signal that pic_init is called first time
     */
    WORD32 i4_first_pic_init;

    /**
     * Current MV Bank's buffer ID
     */
    WORD32 i4_cur_mv_bank_buf_id;

    /**
     * Void pointer to job context
     */
    void *pv_proc_jobq, *pv_entropy_jobq;

    /**
     * Number of MBs to be processed in the current Job
     */
    WORD32 i4_mb_cnt;

    /**
     * ID for the current context - Used for debugging
     */
    WORD32 i4_id;

    /**
     * Pointer to current picture buffer structure
     */
    pic_buf_t *ps_cur_pic;

    /**
     * Pointer to current picture's mv buffer structure
     */
    mv_buf_t *ps_cur_mv_buf;

    /**
     * Flag to indicate if ps_proc was initialized at least once in a frame.
     * This is needed to handle cases where a core starts to handle format
     * conversion jobs directly
     */
    WORD32 i4_init_done;

    /**
     * Process status: one byte per MB
     */
    UWORD8 *pu1_proc_map;

    /**
     * Deblk status: one byte per MB
     */
    UWORD8 *pu1_deblk_map;

    /**
     * Process status: one byte per MB
     */
    UWORD8 *pu1_me_map;

    /*
     * Intra refresh mask.
     * Indicates if an Mb is coded in intra mode within the current AIR interval
     * NOTE Refreshes after each AIR period
     * NOTE The map is shared between process
     */
    UWORD8 *pu1_is_intra_coded;

    /**
     * Disable deblock level (0: Enable completely, 3: Disable completely
     */
    UWORD32 u4_disable_deblock_level;

    /**
     * Pointer to the structure that contains deblock context
     */
    deblk_ctxt_t s_deblk_ctxt;

    /**
     * Points to the array of slice indices which is used to identify the independent
     * slice to which each MB in a frame belongs.
     */
    UWORD8 *pu1_slice_idx;

    /**
     * Pointer to base of slice header structure array
     */
    slice_header_t *ps_slice_hdr_base;

    /**
     * Number of mb's to process in one loop
     */
    WORD32 i4_nmb_ntrpy;

    /**
     * Number of mb's to process in one loop
     */
    UWORD32 u4_nmb_me;

    /**
     * Structure for current input buffer
     */
    inp_buf_t s_inp_buf;

    /**
     * api call cnt
     */
    WORD32 i4_encode_api_call_cnt;

    /**
     * Current Picture count - used for synchronization
     */
    WORD32 i4_pic_cnt;

    /**
      * Intermediate buffer for interpred leaf level functions
      */
    WORD32 ai16_pred1[HP_BUFF_WD * HP_BUFF_HT];

    /**
     * Reference picture for the current picture
     * TODO: Only 2 reference assumed currently
     */
    pic_buf_t *aps_ref_pic[MAX_REF_PIC_CNT];

    /**
     * Reference MV buff for the current picture
     */
    mv_buf_t *aps_mv_buf[MAX_REF_PIC_CNT];

    /**
     * frame info used by RC
     */
    frame_info_t s_frame_info;

    /*
     * NOTE NOT PERSISTANT INSIDE FUNCTIONS
     * Min sad for current MB
     * will be populated initially
     * Once a sad less than eq to u4_min_sad is reached, the value will be copied to the cariable
     */
    UWORD32  u4_min_sad;

    /*
     * indicates weather we have rached minimum sa or not
     */
    UWORD32 u4_min_sad_reached;

    /**
     * Current error code
     */
    WORD32 i4_error_code;

    /*
     * Enables or disables computation of recon
     */
    UWORD32 u4_compute_recon;

    /*
     * Temporary buffers to be used for subpel computation
     */
    UWORD8 *apu1_subpel_buffs[SUBPEL_BUFF_CNT];

    /*
     * Buffer holding best sub pel values
     */
    UWORD8 *pu1_best_subpel_buf;

    /*
     * Stride for buffer holding best sub pel
     */
    UWORD32 u4_bst_spel_buf_strd;

};

/**
 ******************************************************************************
 *  @brief      Rate control related variables
 ******************************************************************************
 */
typedef struct
{
    void *pps_rate_control_api;

    void *pps_frame_time;

    void *pps_time_stamp;

    void *pps_pd_frm_rate;

    /**
     * frame rate pull down
     */
    WORD32 pre_encode_skip[MAX_CTXT_SETS];

    /**
     * skip frame (cbr)
     */
    WORD32 post_encode_skip[MAX_CTXT_SETS];

    /**
     * rate control type
     */
    rc_type_e e_rc_type;

    /**
     * pic type
     */
    picture_type_e e_pic_type;

    /**
     * intra cnt in previous frame
     */
    WORD32 num_intra_in_prev_frame;

    /**
     * avg activity of prev frame
     */
    WORD32 i4_avg_activity;

}rate_control_ctxt_t;

/**
 * Codec context
 */
struct _codec_t
{
    /**
     * Id of current pic (input order)
     */
    WORD32 i4_poc;

    /**
     * Number of encode frame API calls made
     * This variable must only be used for context selection [Read only]
     */
    WORD32 i4_encode_api_call_cnt;

    /**
     * Number of pictures encoded
     */
    WORD32 i4_pic_cnt;

    /**
     * Number of threads created
     */
    WORD32 i4_proc_thread_cnt;

    /**
     * Mutex used to keep the control calls thread-safe
     */
    void *pv_ctl_mutex;

    /**
     * Current active config parameters
     */
    cfg_params_t s_cfg;

    /**
     * Array containing the config parameter sets
     */
    cfg_params_t as_cfg[MAX_ACTIVE_CONFIG_PARAMS];

    /**
     * Color format used by encoder internally
     */
    IV_COLOR_FORMAT_T e_codec_color_format;

    /**
     * recon stride
     * (strides for luma and chroma are the same)
     */
    WORD32 i4_rec_strd;

    /**
     * Flag to enable/disable deblocking of a frame
     */
    WORD32 i4_disable_deblk_pic;

    /**
     * Number of continuous frames where deblocking was disabled
     */
    WORD32 i4_disable_deblk_pic_cnt;

    /**
     * frame type
     */
    PIC_TYPE_T pic_type;

    /**
     * frame qp
     */
    UWORD32 u4_frame_qp;

    /**
     * frame num
     */
    WORD32 i4_frame_num;

    /**
     *  slice_type
     */
    WORD32  i4_slice_type;

    /*
     * Force current frame to specific type
     */
    IV_PICTURE_CODING_TYPE_T force_curr_frame_type;

    /**
     *  IDR pic
     */
    UWORD32 u4_is_idr;

    /**
     *  idr_pic_id
     */
    WORD32 i4_idr_pic_id;

    /**
     * Flush mode
     */
    WORD32 i4_flush_mode;

    /**
     * Encode header mode
     */
    WORD32 i4_header_mode;

    /**
     * Flag to indicate if header has already
     * been generated when i4_api_call_cnt 0
     */
    UWORD32 u4_header_generated;

    /**
     * Encode generate header
     */
    WORD32 i4_gen_header;

    /**
     * To signal successful completion of init
     */
    WORD32 i4_init_done;

    /**
     * To signal that at least one picture was decoded
     */
    WORD32 i4_first_pic_done;

    /**
     * Reset flag - Codec is reset if this flag is set
     */
    WORD32 i4_reset_flag;

    /**
     * Current error code
     */
    WORD32 i4_error_code;

    /**
     * threshold residue
     */
    WORD32 u4_thres_resi;

    /**
     * disable intra inter gating
     */
    UWORD32 u4_inter_gate;

    /**
     * Holds mem records passed during init.
     * This will be used to return the mem records during retrieve call
     */
    iv_mem_rec_t *ps_mem_rec_backup;

    /**
     * Flag to determine if the entropy thread is active
     */
    volatile UWORD32 au4_entropy_thread_active[MAX_CTXT_SETS];

    /**
     * Mutex used to keep the entropy calls thread-safe
     */
    void *pv_entropy_mutex;

    /**
     * Job queue buffer base
     */
    void *pv_proc_jobq_buf, *pv_entropy_jobq_buf;

    /**
     * Job Queue mem tab size
     */
    WORD32 i4_proc_jobq_buf_size, i4_entropy_jobq_buf_size;

    /**
     * Memory for MV Bank buffer manager
     */
    void *pv_mv_buf_mgr_base;

    /**
     * MV Bank buffer manager
     */
    void *pv_mv_buf_mgr;

    /**
     * Pointer to MV Buf structure array
     */
    void *ps_mv_buf;

    /**
     * Base address for Motion Vector bank buffer
     */
    void *pv_mv_bank_buf_base;

    /**
     * MV Bank size allocated
     */
    WORD32 i4_total_mv_bank_size;

    /**
     * Memory for Picture buffer manager for reference pictures
     */
    void *pv_ref_buf_mgr_base;

    /**
     * Picture buffer manager for reference pictures
     */
    void *pv_ref_buf_mgr;

    /**
     * Number of reference buffers added to the buffer manager
     */
    WORD32 i4_ref_buf_cnt;

    /**
     * Pointer to Pic Buf structure array
     */
    void *ps_pic_buf;

    /**
     * Base address for Picture buffer
     */
    void *pv_pic_buf_base;

    /**
     * Total pic buffer size allocated
     */
    WORD32 i4_total_pic_buf_size;

    /**
     * Memory for Buffer manager for output buffers
     */
     void *pv_out_buf_mgr_base;

    /**
     * Buffer manager for output buffers
     */
     void *pv_out_buf_mgr;

    /**
     * Current output buffer's buffer ID
     */
    WORD32 i4_out_buf_id;

    /**
     * Number of output buffers added to the buffer manager
     */
    WORD32 i4_out_buf_cnt;

    /**
     * Memory for Picture buffer manager for input buffers
     */
     void *pv_inp_buf_mgr_base;

    /**
     * Picture buffer manager for input buffers
     */
     void *pv_inp_buf_mgr;

    /**
     * Current input buffer's buffer ID
     */
    WORD32 i4_inp_buf_id;

    /**
     * Number of input buffers added to the buffer manager
     */
    WORD32 i4_inp_buf_cnt;

    /**
     * Current input buffer
     */
    pic_buf_t *ps_inp_buf;

    /**
     * Pointer to dpb manager structure
     */
    void *pv_dpb_mgr;

    /**
     * Pointer to base of Sequence parameter set structure array
     */
    sps_t *ps_sps_base;

    /**
     * Pointer to base of Picture parameter set structure array
     */
    pps_t *ps_pps_base;

    /**
     *  seq_parameter_set_id
     */
    WORD32 i4_sps_id;

    /**
     *  pic_parameter_set_id
     */
    WORD32 i4_pps_id;

    /**
     * Pointer to base of slice header structure array
     */
    slice_header_t *ps_slice_hdr_base;

    /**
     * packed residue coeff data size for 1 row of mbs
     */
    UWORD32 u4_size_coeff_data;

    /**
     * packed header data size for 1 row of mbs
     */
    UWORD32 u4_size_header_data;

    /**
     * Processing context - One for each processing thread
     * Create two sets, each set used for alternate frames
     */
    process_ctxt_t as_process[MAX_PROCESS_CTXT];

    /**
     * Thread handle for each of the processing threads
     */
    void *apv_proc_thread_handle[MAX_PROCESS_THREADS];

    /**
     * Thread created flag for each of the processing threads
     */
    WORD32 ai4_process_thread_created[MAX_PROCESS_THREADS];

    /**
     * Void pointer to process job context
     */
    void *pv_proc_jobq, *pv_entropy_jobq;

    /**
     * Number of MBs processed together for better instruction cache handling
     */
    WORD32 i4_proc_nmb;

    /**
     * Previous POC lsb
     */
    WORD32 i4_prev_poc_lsb;

    /**
     * Previous POC msb
     */
    WORD32 i4_prev_poc_msb;

    /**
     * Max POC lsb that has arrived till now
     */
    WORD32 i4_max_prev_poc_lsb;

    /**
     * Context for format conversion
     */
    fmt_conv_t s_fmt_conv;

    /**
     * Absolute pic order count
     */
    WORD32 i4_abs_pic_order_cnt;

    /**
     *  Pic order count of lsb
     */
    WORD32 i4_pic_order_cnt_lsb;

    /**
     * Array giving current picture being processed in each context set
     */
    WORD32 ai4_pic_cnt[MAX_CTXT_SETS];

    /*
     * Min sad to search for
     */
    UWORD32 u4_min_sad;

    /**
     * Reference picture set
     */
    ref_set_t as_ref_set[MAX_DPB_SIZE + MAX_CTXT_SETS];


    /*
     * Air pic cnt
     * Contains the number of pictures that have been encoded with air
     * This value is moudulo air refresh period
     */
    WORD32 i4_air_pic_cnt;

    /*
     * Intra refresh map
     * Stores the frames at which intra refresh should occur for a MB
     */
    UWORD16 *pu2_intr_rfrsh_map;

    /*
     * Indicates if the current frame is used as a reference frame
     */
    UWORD32 u4_is_curr_frm_ref;

    /*
     * Indicates if there can be non reference frames in the stream
     */
    WORD32 i4_non_ref_frames_in_stream;

    /*
     * Memory for color space conversion for luma plane
     */
    UWORD8 *pu1_y_csc_buf_base;

    /*
     * Memory for color space conversion foe chroma plane
     */
    UWORD8 *pu1_uv_csc_buf_base;

    /**
     * Function pointers for intra pred leaf level functions luma
     */
    pf_intra_pred apf_intra_pred_16_l[MAX_I16x16];
    pf_intra_pred apf_intra_pred_8_l[MAX_I8x8];
    pf_intra_pred apf_intra_pred_4_l[MAX_I4x4];

    /**
     * Function pointers for intra pred leaf level functions chroma
     */
    pf_intra_pred apf_intra_pred_c[MAX_CH_I8x8];

    /**
     * luma core coding function pointer
     */
    UWORD8 (*luma_energy_compaction[4])(process_ctxt_t *ps_proc);

    /**
     * chroma core coding function pointer
     */
    UWORD8 (*chroma_energy_compaction[2])(process_ctxt_t *ps_proc);

    /**
     * forward transform for intra blk of mb type 16x16
     */
    ih264_luma_16x16_resi_trans_dctrans_quant_ft *pf_resi_trans_dctrans_quant_16x16;

    /**
     * inverse transform for intra blk of mb type 16x16
     */
    ih264_luma_16x16_idctrans_iquant_itrans_recon_ft *pf_idctrans_iquant_itrans_recon_16x16;

    /**
     * forward transform for 4x4 blk luma
     */
    ih264_resi_trans_quant_ft *pf_resi_trans_quant_4x4;

    /**
     * forward transform for 4x4 blk luma
     */
    ih264_resi_trans_quant_ft *pf_resi_trans_quant_chroma_4x4;

    /*
     * hadamard transform and quant for a 4x4 block
     */
    ih264_hadamard_quant_ft *pf_hadamard_quant_4x4;

    /*
     *  hadamard transform and quant for a 4x4 block
     */
    ih264_hadamard_quant_ft *pf_hadamard_quant_2x2_uv;

    /**
     * inverse transform for 4x4 blk
     */
    ih264_iquant_itrans_recon_ft *pf_iquant_itrans_recon_4x4;

    /**
     * inverse transform for chroma 4x4 blk
     */
    ih264_iquant_itrans_recon_chroma_ft *pf_iquant_itrans_recon_chroma_4x4;

    /**
     * inverse transform for 4x4 blk with only single dc coeff
     */
    ih264_iquant_itrans_recon_ft *pf_iquant_itrans_recon_4x4_dc;

    /**
     * inverse transform for chroma 4x4 blk with only single dc coeff
     */
    ih264_iquant_itrans_recon_chroma_ft *pf_iquant_itrans_recon_chroma_4x4_dc;

    /*
     * Inverse hadamard transform and iquant for a 4x4 block
     */
    ih264_ihadamard_scaling_ft *pf_ihadamard_scaling_4x4;

    /*
     * Inverse hadamard transform and iquant for a 4x4 block
     */
    ih264_ihadamard_scaling_ft *pf_ihadamard_scaling_2x2_uv;

    /*
     * Function for interleave copy*
     */
    ih264_interleave_copy_ft *pf_interleave_copy;

    /**
     * forward transform for 8x8 blk
     */
    ih264_resi_trans_quant_ft *pf_resi_trans_quant_8x8;

    /**
     * inverse transform for 8x8 blk
     */
    /**
     * inverse transform for 4x4 blk
     */
    ih264_iquant_itrans_recon_ft *pf_iquant_itrans_recon_8x8;

    /**
     * forward transform for chroma MB
     */
    ih264_chroma_8x8_resi_trans_dctrans_quant_ft *pf_resi_trans_dctrans_quant_8x8_chroma;

    /**
     * inverse transform for chroma MB
     */
    ih264_idctrans_iquant_itrans_recon_ft *pf_idctrans_iquant_itrans_recon_8x8_chroma;

    /**
     * deblock vertical luma edge with blocking strength 4
     */
    ih264_deblk_edge_bs4_ft *pf_deblk_luma_vert_bs4;

    /**
     * deblock vertical chroma edge with blocking strength 4
     */
    ih264_deblk_chroma_edge_bs4_ft *pf_deblk_chroma_vert_bs4;

    /**
     * deblock vertical luma edge with blocking strength less than 4
     */
    ih264_deblk_edge_bslt4_ft *pf_deblk_luma_vert_bslt4;

    /**
     * deblock vertical chroma edge with blocking strength less than 4
     */
    ih264_deblk_chroma_edge_bslt4_ft *pf_deblk_chroma_vert_bslt4;

    /**
     * deblock horizontal luma edge with blocking strength 4
     */
    ih264_deblk_edge_bs4_ft *pf_deblk_luma_horz_bs4;

    /**
     * deblock horizontal chroma edge with blocking strength 4
     */
    ih264_deblk_chroma_edge_bs4_ft *pf_deblk_chroma_horz_bs4;

    /**
     * deblock horizontal luma edge with blocking strength less than 4
     */
    ih264_deblk_edge_bslt4_ft *pf_deblk_luma_horz_bslt4;

    /**
     * deblock horizontal chroma edge with blocking strength less than 4
     */
    ih264_deblk_chroma_edge_bslt4_ft *pf_deblk_chroma_horz_bslt4;


    /**
     * functions for padding
     */
    pf_pad pf_pad_top;
    pf_pad pf_pad_bottom;
    pf_pad pf_pad_left_luma;
    pf_pad pf_pad_left_chroma;
    pf_pad pf_pad_right_luma;
    pf_pad pf_pad_right_chroma;

    /**
     * Inter pred leaf level functions
     */
    ih264_inter_pred_luma_ft    *pf_inter_pred_luma_copy;
    ih264_inter_pred_luma_ft    *pf_inter_pred_luma_horz;
    ih264_inter_pred_luma_ft    *pf_inter_pred_luma_vert;
    pf_inter_pred_luma_bilinear  pf_inter_pred_luma_bilinear;
    ih264_inter_pred_chroma_ft  *pf_inter_pred_chroma;

    /**
     * fn ptrs for compute sad routines
     */
    ime_compute_sad_ft *apf_compute_sad_16x16[2];
    ime_compute_sad_ft *pf_compute_sad_16x8;


    /**
     * Function pointer for computing ME
     * 1 for PSLICE and 1 for BSLICE
     */
    ih264e_compute_me_ft *apf_compute_me[2];

    /**
     * Function pointers for computing SKIP parameters
     */
    ih264e_skip_params_ft *apf_find_skip_params_me[2];

    /**
     * fn ptrs for memory handling operations
     */
    pf_memcpy pf_mem_cpy;
    pf_memset pf_mem_set;
    pf_memcpy_mul8 pf_mem_cpy_mul8;
    pf_memset_mul8 pf_mem_set_mul8;

    /**
     * intra mode eval -encoder level function
     */
    pf_evaluate_intra_modes pf_ih264e_evaluate_intra16x16_modes;
    pf_evaluate_intra_modes pf_ih264e_evaluate_intra_chroma_modes;
    pf_evaluate_intra_4x4_modes pf_ih264e_evaluate_intra_4x4_modes;

    /* Half pel generation function - encoder level
     *
     */
    pf_sixtapfilter_horz pf_ih264e_sixtapfilter_horz;
    pf_sixtap_filter_2dvh_vert pf_ih264e_sixtap_filter_2dvh_vert;

    /**
     * color space conversion form YUV 420P to YUV 420Sp
     */
    pf_fmt_conv_420p_to_420sp pf_ih264e_conv_420p_to_420sp;


    /**
     * color space conversion form YUV 420P to YUV 420Sp
     */
    pf_fmt_conv_422ile_to_420sp pf_ih264e_fmt_conv_422i_to_420sp;

    /**
     * write mb layer for a given slice I, P, B
     */
    IH264E_ERROR_T (*pf_write_mb_syntax_layer[2][3]) ( entropy_ctxt_t *ps_ent_ctxt );

    /**
     * Output buffer
     */
    out_buf_t as_out_buf[MAX_CTXT_SETS];

    /**
     * recon buffer
     */
    rec_buf_t as_rec_buf[MAX_CTXT_SETS];

    /**
     * rate control context
     */
    rate_control_ctxt_t s_rate_control;

    /**
     * VUI structure
     */
    vui_t s_vui;

    /**
     * input buffer queue
     */
    inp_buf_t as_inp_list[MAX_NUM_BFRAMES];

    /**
     * Flag to indicate if any IDR requests are pending
     */
    WORD32 i4_pending_idr_flag;

    /*
    *Flag to indicate if we have recived the last input frame
    */
    WORD32 i4_last_inp_buff_received;

};

#endif /* IH264E_STRUCTS_H_ */
