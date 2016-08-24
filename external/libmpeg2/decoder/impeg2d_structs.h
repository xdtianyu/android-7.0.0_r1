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
#ifndef __IMPEG2D_STRUCTS_H__
#define __IMPEG2D_STRUCTS_H__

/* Decoder needs at least 4 reference buffers in order to support format conversion in a thread and
to support B pictures. Because of format conversion in a thread, codec delay is now 2 frames instead of 1.
To reduce this delay, format conversion has to wait for MB status before converting for B pictures.
To avoid this check the delay is increased to 2 and hence number of reference frames minimum is 4.
Because of temporal dependency in deinterlacer one additional buffer is also needed */
#define NUM_INT_FRAME_BUFFERS                     5


#define MAX_WIDTH               4096
#define MAX_HEIGHT              2160

#define MIN_WIDTH               16
#define MIN_HEIGHT              16


#define MAX_FRM_SIZE            (MAX_WIDTH * MAX_HEIGHT * 2)  /* Supports only 420P and 422ILE */

#define DEC_ORDER               0

#define MAX_BITSTREAM_BUFFER_SIZE       2000 * 1024

/* Flag to signal that buffer is held by deinterlacing */
#define MPEG2_BUF_MGR_DEINT (BUF_MGR_DISP << 1)

typedef enum
{
    CMD_PROCESS,
    CMD_FMTCONV,
}e_jobq_cmd_t;

/**
 * Structure to represent a processing job entry
 */
typedef struct
{
    /**
     * Command
     * Currently: PROCESS, FMTCONV are the only two jobs
     */
    WORD32 i4_cmd;

    /**
     * MB y of the starting MB
     */
    WORD16 i2_start_mb_y;

    /**
     * MB y of the last MB
     */

    WORD16 i2_end_mb_y;

    /**
     * Bitstream offset for the current job
     */
    WORD32 i4_bistream_ofst;

}job_t;

typedef struct
{
    /* Params of the reference buffer used as input to MC */
    UWORD32 u4_src_wd;
    UWORD32 u4_src_offset;

    /* Params of the buffer where MC output will be written */
    UWORD32 u4_dst_wd_res_buf;
    UWORD32 u4_dst_wd_cur_frm;
    UWORD32 u4_dst_offset_res_buf;
    UWORD32 u4_dst_offset_cur_frm;

    /* Operation Parameters */
    UWORD32 u4_rows;
    UWORD32 u4_cols;
    UWORD32 u4_mode;
}comp_mc_params_t;

typedef struct
{
    yuv_buf_t        s_ref;
    comp_mc_params_t s_luma;
    comp_mc_params_t s_chroma;
}mb_mc_params_t;

struct _dec_mb_params_t;

typedef UWORD8 pf_inv_quant_t (WORD16 *blk,
                                UWORD8 *weighting_matrix,
                                UWORD8 quant_scale,
                                WORD32 intra_flag,
                                WORD32 i4_num_coeffs,
                                WORD16 *pi2_coeffs,
                                UWORD8 *pu1_pos,
                                const UWORD8   *scan,
                                UWORD16 *u2_def_dc_pred,
                                UWORD16 u2_intra_dc_precision);

typedef IMPEG2D_ERROR_CODES_T  pf_vld_inv_quant_t  (void  *dec,
                             WORD16       *out_addr,
                             const UWORD8 *scan,
                             UWORD16      intra_flag,
                             UWORD16      colr_comp,
                             UWORD16      d_picture);

typedef void  pf_mc_t(void *, UWORD8 *, UWORD32 , UWORD8 *, UWORD32 ,
                 UWORD32 , UWORD32  );

typedef struct dec_state_struct_t
{
    WORD16          ai2_vld_buf[NUM_PELS_IN_BLOCK];
    WORD16          ai2_idct_stg1[NUM_PELS_IN_BLOCK];


    UWORD8          au1_intra_quant_matrix[NUM_PELS_IN_BLOCK];
    UWORD8          au1_inter_quant_matrix[NUM_PELS_IN_BLOCK];

    IMPEG2D_ERROR_CODES_T (*pf_decode_slice)(struct dec_state_struct_t *);

    pf_vld_inv_quant_t *pf_vld_inv_quant;

    pf_idct_recon_t *pf_idct_recon[4];

    pf_mc_t         *pf_mc[4];
    pf_interpred_t  *pf_fullx_halfy_8x8;
    pf_interpred_t  *pf_halfx_fully_8x8;
    pf_interpred_t  *pf_halfx_halfy_8x8;
    pf_interpred_t  *pf_fullx_fully_8x8;


    pf_interpolate_t *pf_interpolate;
    pf_copy_mb_t     *pf_copy_mb;

    pf_memset0_one_16bit_buf_t *pf_memset_16bit_8x8_linear_block;
    pf_memset_8bit_t    *pf_memset_8bit_8x8_block;
    pf_copy_yuv420p_buf_t *pf_copy_yuv420p_buf;
    pf_fmt_conv_yuv420p_to_yuv422ile_t *pf_fmt_conv_yuv420p_to_yuv422ile;
    pf_fmt_conv_yuv420p_to_yuv420sp_t  *pf_fmt_conv_yuv420p_to_yuv420sp_uv;
    pf_fmt_conv_yuv420p_to_yuv420sp_t  *pf_fmt_conv_yuv420p_to_yuv420sp_vu;

    stream_t         s_bit_stream;
/* @ */

    UWORD16         u2_is_mpeg2; /* 0 if stream is MPEG1 1 otherwise */
    UWORD16         u2_frame_width;  /* Width of the frame */
    UWORD16         u2_frame_height; /* Height of the frame */
    UWORD16         u2_picture_width;
    UWORD16         u2_horizontal_size;
    UWORD16         u2_vertical_size;
    UWORD16         u2_create_max_width;
    UWORD16         u2_create_max_height;
    UWORD16         u2_reinit_max_width;
    UWORD16         u2_reinit_max_height;
    UWORD16         u2_header_done;
    UWORD16         u2_decode_header;

    UWORD16         u2_mb_x;
    UWORD16         u2_mb_y;
    UWORD16         u2_num_horiz_mb;
    UWORD16         u2_num_vert_mb;
    UWORD16         u2_num_flds_decoded;
    void            *pv_pic_buf_mg;

    UWORD32         u4_frm_buf_stride; /* for display Buffer */

    UWORD16         u2_field_dct;
    UWORD16         u2_read_dct_type;

    UWORD16         u2_read_motion_type;
    UWORD16         u2_motion_type;

    const UWORD16   *pu2_mb_type;
    UWORD16         u2_fld_pic;
    UWORD16         u2_frm_pic;

    yuv_buf_t       s_cur_frm_buf;

    UWORD16         u2_fld_parity;
    UWORD16         u2_def_dc_pred[MAX_COLR_COMPS];

    /* Variables related to Motion Vector predictors */

    WORD16          ai2_pred_mv[2][2][2];
    e_pred_direction_t   e_mb_pred;
    UWORD16         au2_fcode_data[2];

    /* Variables related to reference pictures */
    yuv_buf_t       as_recent_fld[2][2];

    UWORD8          u1_quant_scale;
    UWORD16         u2_num_mbs_left;
    UWORD16         u2_first_mb;
    UWORD16         u2_num_skipped_mbs;

    UWORD8          *pu1_inv_scan_matrix;

    UWORD16         u2_progressive_sequence;
    e_pic_type_t         e_pic_type;

    UWORD16         u2_full_pel_forw_vector;
    UWORD16         u2_forw_f_code;
    UWORD16         u2_full_pel_back_vector;
    UWORD16         u2_back_f_code;

    WORD16          ai2_mv[2][2][2]; /* Motion vectors */

    /* Bitstream code present in Picture coding extension */
    UWORD16         au2_f_code[2][2];
    UWORD16         u2_intra_dc_precision;
    UWORD16         u2_picture_structure;
    UWORD16         u2_top_field_first;
    UWORD16         u2_frame_pred_frame_dct;
    UWORD16         u2_concealment_motion_vectors;
    UWORD16         u2_q_scale_type;
    UWORD16         u2_intra_vlc_format;
    UWORD16         u2_alternate_scan;
    UWORD16         u2_repeat_first_field;
    UWORD16         u2_progressive_frame;


    /* Bitstream code related to frame rate of the bitstream */
    UWORD16         u2_frame_rate_code;
    UWORD16         u2_frame_rate_extension_n;
    UWORD16         u2_frame_rate_extension_d;
    UWORD16         u2_framePeriod;   /* Frame period in milli seconds */

    /* Members related to display dimensions of bitstream */
    /* The size values may not be returned right now. But they are read */
    /* and can be returned if there is a requirement.                   */
    UWORD16         u2_display_horizontal_size;
    UWORD16         u2_display_vertical_size;
    UWORD16         u2_aspect_ratio_info;

    /* Members related to motion compensation */
    yuv_buf_t       s_mc_fw_buf;
    yuv_buf_t       s_mc_bk_buf;
    yuv_buf_t       s_mc_buf;
    mb_mc_params_t  as_mb_mc_params[2][2];
    yuv_buf_t       as_ref_buf[2][2];
    e_mb_type_t       s_mb_type;

    yuv_buf_t       s_dest_buf;

    /* Variable to handle intra MB */
    UWORD16         u2_prev_intra_mb;
    UWORD16         u2_coded_mb;

    /* Bidirect function pointers */
    const struct _dec_mb_params_t *ps_func_bi_direct;

    /* Forw or Back function pointers */
    const struct _dec_mb_params_t *ps_func_forw_or_back;


    /* CBP of the current MB        */
    UWORD16         u2_cbp;
    void            *pv_video_scratch;


    /* For global error handling */
    void            *pv_stack_cntxt;

/* @ */
    WORD32          i4_chromaFormat;
    UWORD32         u4_xdmBufID;
    UWORD32         u4_num_mem_records;
    /* For holding memRecords */
    void            *pv_memTab;

    UWORD8          u1_flushfrm;
    UWORD8          u1_flushcnt;
    iv_yuv_buf_t    as_frame_buf[MAX_FRAME_BUFFER];
    iv_yuv_buf_t    ps_yuv_buf;

    ivd_get_display_frame_op_t  s_disp_op;


    UWORD32         u4_non_zero_cols;
    UWORD32         u4_non_zero_rows;

    UWORD32         u4_num_frames_decoded;

    /* Adding error code variable to signal benign errors. */
    UWORD32         u4_error_code;

    WORD32          i4_num_cores;

    UWORD8          u1_first_frame_done;

    void            *pv_codec_thread_handle;
    void            *ps_dec_state_multi_core;
    UWORD32         u4_inp_ts;
    pic_buf_t       *ps_cur_pic;
    pic_buf_t       *ps_disp_pic;
    pic_buf_t       *aps_ref_pics[2];

    WORD32          i4_disp_buf_id;
    WORD32          i4_cur_buf_id;
    iv_yuv_buf_t    *ps_disp_frm_buf;

    UWORD32         u4_share_disp_buf;
    void            *pv_pic_buf_base;

    disp_mgr_t      s_disp_mgr;
    UWORD8          *pu1_chroma_ref_buf[BUF_MGR_MAX_CNT];
    ivd_out_bufdesc_t as_disp_buffers[BUF_MGR_MAX_CNT];

    /* Flag to signal last coeff in a 8x8 block is one
    after mismatch contol */
    WORD32          i4_last_value_one;

    WORD32          i4_start_mb_y;
    WORD32          i4_end_mb_y;

    /**
     * Job queue buffer base
     */
    void            *pv_jobq_buf;

    /**
     * Job Queue mem tab size
     */
    WORD32          i4_jobq_buf_size;

    /**
     * Job Queue context
     */
    void            *pv_jobq;

    /* Pointer to input bitstream */
    UWORD8          *pu1_inp_bits_buf;

    /* Number of bytes in the input bitstream */
    UWORD32         u4_num_inp_bytes;

    /* Bytes consumed */
    WORD32          i4_bytes_consumed;

    IVD_ARCH_T      e_processor_arch;

    IVD_SOC_T       e_processor_soc;

    WORD32          i4_frame_decoded;

    /** Flag to enable deinterlace */
    UWORD32          u4_deinterlace;

    /** Deinterlacer context */
    void            *pv_deinterlacer_ctxt;

    /** Picture buffer held by deinterlacer */
    pic_buf_t       *ps_deint_pic;

    /** Buffer used after deinterlacer for format conversion */
    UWORD8          *pu1_deint_fmt_buf;

}dec_state_t;




typedef void (*func_decmb_params)(dec_state_t *);
typedef void  (*mc_funcs)(dec_state_t *);
typedef struct _dec_mb_params_t
{
    func_decmb_params    pf_func_mb_params;
    e_mb_type_t            s_mb_type;
    mc_funcs             pf_mc;
}dec_mb_params_t;



#define MAX_THREADS     4


#define MAX_MB_ROWS     (MAX_HEIGHT / 16) // number of rows for 1080p

typedef struct _dec_state_multi_core
{
    // contains the decoder state of decoder for each thread
    dec_state_t *ps_dec_state[MAX_THREADS];
    UWORD32     au4_thread_launched[MAX_THREADS];
    // number of rows: first thread will populate the row offsets and update
    // row_offset_cnt. Other threads should pick up offset from this thread
    // and start decoding
    UWORD32     au4_row_offset[MAX_MB_ROWS];
    volatile    UWORD32 u4_row_offset_cnt;
}dec_state_multi_core_t;



#endif /* #ifndef __IMPEG2D_STRUCTS_H__ */
