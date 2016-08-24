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
/*****************************************************************************/
/*                                                                           */
/*  File Name         : impeg2d.h                                        */
/*                                                                           */
/*  Description       : This file contains all the necessary structure and   */
/*                      enumeration definitions needed for the Application   */
/*                      Program Interface(API) of the Ittiam MPEG2 ASP       */
/*                      Decoder on Cortex A8 - Neon platform                 */
/*                                                                           */
/*  List of Functions : impeg2d_api_function                             */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         26 08 2010   100239(RCY)     Draft                                */
/*                                                                           */
/*****************************************************************************/

#ifndef __IMPEG2D_H__
#define __IMPEG2D_H__

#include "iv.h"
#include "ivd.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/
#define EXPORT_MPEG2DEC_FULLCODEC_MEM_RECORDS   22

/*****************************************************************************/
/* Function Macros                                                           */
/*****************************************************************************/
#define IS_IVD_CONCEALMENT_APPLIED(x)           (x & (1 << IVD_APPLIEDCONCEALMENT))
#define IS_IVD_INSUFFICIENTDATA_ERROR(x)        (x & (1 << IVD_INSUFFICIENTDATA))
#define IS_IVD_CORRUPTEDDATA_ERROR(x)           (x & (1 << IVD_CORRUPTEDDATA))
#define IS_IVD_CORRUPTEDHEADER_ERROR(x)         (x & (1 << IVD_CORRUPTEDHEADER))
#define IS_IVD_UNSUPPORTEDINPUT_ERROR(x)        (x & (1 << IVD_UNSUPPORTEDINPUT))
#define IS_IVD_UNSUPPORTEDPARAM_ERROR(x)        (x & (1 << IVD_UNSUPPORTEDPARAM))
#define IS_IVD_FATAL_ERROR(x)                   (x & (1 << IVD_FATALERROR))
#define IS_IVD_INVALID_BITSTREAM_ERROR(x)       (x & (1 << IVD_INVALID_BITSTREAM))
#define IS_IVD_INCOMPLETE_BITSTREAM_ERROR(x)    (x & (1 << IVD_INCOMPLETE_BITSTREAM))

#define SET_IVD_CONCEALMENT_APPLIED(x)          ((x) |= (x) | (1 << IVD_APPLIEDCONCEALMENT))
#define SET_IVD_INSUFFICIENTDATA_ERROR(x)       ((x) |= (x) | (1 << IVD_INSUFFICIENTDATA))
#define SET_IVD_CORRUPTEDDATA_ERROR(x)          ((x) |= (x) | (1 << IVD_CORRUPTEDDATA))
#define SET_IVD_CORRUPTEDHEADER_ERROR(x)        ((x) |= (x) | (1 << IVD_CORRUPTEDHEADER))
#define SET_IVD_UNSUPPORTEDINPUT_ERROR(x)       ((x) |= (x) | (1 << IVD_UNSUPPORTEDINPUT))
#define SET_IVD_UNSUPPORTEDPARAM_ERROR(x)       ((x) |= (x) | (1 << IVD_UNSUPPORTEDPARAM))
#define SET_IVD_FATAL_ERROR(x)                  ((x) |= (x) | (1 << IVD_FATALERROR))
#define SET_IVD_INVALID_BITSTREAM_ERROR(x)      ((x) |= (x) | (1 << IVD_INVALID_BITSTREAM))
#define SET_IVD_INCOMPLETE_BITSTREAM_ERROR(x)   ((x) |= (x) | (1 << IVD_INCOMPLETE_BITSTREAM))

/*****************************************************************************/
/* API Function Prototype                                                    */
/*****************************************************************************/
IV_API_CALL_STATUS_T impeg2d_api_function(iv_obj_t *ps_handle,
                                          void *pv_api_ip,
                                          void *pv_api_op);

/*****************************************************************************/
/* Enums                                                                     */
/*****************************************************************************/
/* Codec Error codes for MPEG2 ASP Decoder                                   */

typedef enum
{

    IMPEG2D_UNKNOWN_ERROR = IVD_DUMMY_ELEMENT_FOR_CODEC_EXTENSIONS + 1,
    /* API calls without init call */
    IMPEG2D_INIT_NOT_DONE,
    /* Query number of Memory Records API */
    IMPEG2D_QUERY_NUM_MEM_REC_FAIL,

    /* Fill Memory Records API */
    IMPEG2D_FILL_NUM_MEM_REC_NOT_SUFFICIENT,

    /* Initialize Decoder API */
    IMPEG2D_INIT_DEC_SCR_MEM_INSUFFICIENT,
    IMPEG2D_INIT_DEC_PER_MEM_INSUFFICIENT,
    IMPEG2D_INIT_NUM_MEM_REC_NOT_SUFFICIENT,
    IMPEG2D_INIT_CHROMA_FORMAT_HEIGHT_ERROR,

    /* Decode Sequence Header API */
    IMPEG2D_FRM_HDR_START_CODE_NOT_FOUND,
    IMPEG2D_FRM_HDR_MARKER_BIT_NOT_FOUND,
    IMPEG2D_PROF_LEVEL_NOT_SUPPORTED,
    IMPEG2D_FMT_NOT_SUPPORTED,
    IMPEG2D_SCALABILITIY_NOT_SUPPORTED,
    IMPEG2D_PIC_SIZE_NOT_SUPPORTED,

    /* Search for start code API */
    //IMPEG2D_SEARCH_START_CODE_FAIL         ,
    /* Decode Video Frame API    */
    IMPEG2D_START_CODE_NOT_FOUND,
    IMPEG2D_MARKER_BIT_NOT_FOUND,
    IMPEG2D_INVALID_STUFFING,
    IMPEG2D_PROFILE_LEVEL_NOT_SUP,
    IMPEG2D_CHROMA_FMT_NOT_SUP,
    IMPEG2D_SCALABLITY_NOT_SUP,
    IMPEG2D_FRM_HDR_DECODE_ERR,
    IMPEG2D_MB_HDR_DECODE_ERR,
    IMPEG2D_MB_TEX_DECODE_ERR,
    IMPEG2D_INCORRECT_QUANT_MATRIX,
    IMPEG2D_INVALID_SKIP_MB,
    IMPEG2D_NOT_SUPPORTED_ERR,
    IMPEG2D_BITSTREAM_BUFF_EXCEEDED_ERR,
    IMPEG2D_INVALID_PIC_TYPE,
    IMPEG2D_INVALID_HUFFMAN_CODE,
    IMPEG2D_NO_FREE_BUF_ERR,

    /* slice header errors */
    IMPEG2D_INVALID_VERT_SIZE,
    IMPEG2D_MB_DATA_DECODE_ERR,

    /* Get Display Frame API */
    IMPEG2D_GET_DISP_FRM_FAIL,

    /* Sample Version limitation */
    IMPEG2D_SAMPLE_VERSION_LIMIT_ERR,
    /**
     * Width/height greater than max width and max height
     */
    IMPEG2D_UNSUPPORTED_DIMENSIONS,

    /* Unknown API Command */
    IMPEG2D_UNKNOWN_API_COMMAND

} IMPEG2D_ERROR_CODES_T;

/*****************************************************************************/
/* Extended Structures                                                       */
/*****************************************************************************/
typedef enum
{
    /** Set number of cores/threads to be used */
    IMPEG2D_CMD_CTL_SET_NUM_CORES = IVD_CMD_CTL_CODEC_SUBCMD_START,

    /** Set processor details */
    IMPEG2D_CMD_CTL_SET_PROCESSOR = IVD_CMD_CTL_CODEC_SUBCMD_START + 0x001,

    /** Get display buffer dimensions */
    IMPEG2D_CMD_CTL_GET_BUFFER_DIMENSIONS = IVD_CMD_CTL_CODEC_SUBCMD_START
                    + 0x100,

} IMPEG2D_CMD_CTL_SUB_CMDS;

/*****************************************************************************/
/*  Get Number of Memory Records                                             */
/*****************************************************************************/

typedef struct
{
    iv_num_mem_rec_ip_t s_ivd_num_mem_rec_ip_t;
} impeg2d_num_mem_rec_ip_t;

typedef struct
{
    iv_num_mem_rec_op_t s_ivd_num_mem_rec_op_t;
} impeg2d_num_mem_rec_op_t;

/*****************************************************************************/
/*  Fill Memory Records                                                      */
/*****************************************************************************/

typedef struct
{
    iv_fill_mem_rec_ip_t s_ivd_fill_mem_rec_ip_t;
    /* Flag to enable sharing of reference buffers between decoder
     and application */

    UWORD32 u4_share_disp_buf;

    /* format in which codec has to give out frame data for display */
    IV_COLOR_FORMAT_T e_output_format;

    /**
     * Flag to enable/disable deinterlacing
     */
    UWORD32 u4_deinterlace;

} impeg2d_fill_mem_rec_ip_t;

typedef struct
{
    iv_fill_mem_rec_op_t s_ivd_fill_mem_rec_op_t;
} impeg2d_fill_mem_rec_op_t;

/*****************************************************************************/
/*  Retrieve Memory Records                                                  */
/*****************************************************************************/

typedef struct
{
    iv_retrieve_mem_rec_ip_t s_ivd_retrieve_mem_rec_ip_t;
} impeg2d_retrieve_mem_rec_ip_t;

typedef struct
{
    iv_retrieve_mem_rec_op_t s_ivd_retrieve_mem_rec_op_t;
} impeg2d_retrieve_mem_rec_op_t;

/*****************************************************************************/
/*   Initialize decoder                                                      */
/*****************************************************************************/

typedef struct
{
    ivd_init_ip_t s_ivd_init_ip_t;
    /* Flag to enable sharing of reference buffers between decoder
     and application */
    UWORD32 u4_share_disp_buf;

    /**
     * Flag to enable/disable deinterlacing
     */
    UWORD32 u4_deinterlace;

} impeg2d_init_ip_t;

typedef struct
{
    ivd_init_op_t s_ivd_init_op_t;
} impeg2d_init_op_t;

/*****************************************************************************/
/*   Video Decode                                                            */
/*****************************************************************************/

typedef struct
{
    ivd_video_decode_ip_t s_ivd_video_decode_ip_t;
} impeg2d_video_decode_ip_t;

typedef struct
{
    ivd_video_decode_op_t s_ivd_video_decode_op_t;
} impeg2d_video_decode_op_t;

/*****************************************************************************/
/*   Get Display Frame                                                       */
/*****************************************************************************/

typedef struct
{
    ivd_get_display_frame_ip_t s_ivd_get_display_frame_ip_t;
} impeg2d_get_display_frame_ip_t;

typedef struct
{
    ivd_get_display_frame_op_t s_ivd_get_display_frame_op_t;
} impeg2d_get_display_frame_op_t;

/*****************************************************************************/
/*   Set Display Frame                                                       */
/*****************************************************************************/
typedef struct
{
    ivd_set_display_frame_ip_t s_ivd_set_display_frame_ip_t;
} impeg2d_set_display_frame_ip_t;

typedef struct
{
    ivd_set_display_frame_op_t s_ivd_set_display_frame_op_t;
} impeg2d_set_display_frame_op_t;

/*****************************************************************************/
/*   Release Display Buffers                                                 */
/*****************************************************************************/

typedef struct
{
    ivd_rel_display_frame_ip_t s_ivd_rel_display_frame_ip_t;
} impeg2d_rel_display_frame_ip_t;

typedef struct
{
    ivd_rel_display_frame_op_t s_ivd_rel_display_frame_op_t;
} impeg2d_rel_display_frame_op_t;

/*****************************************************************************/
/*   Video control  Flush                                                    */
/*****************************************************************************/

typedef struct
{
    ivd_ctl_flush_ip_t s_ivd_ctl_flush_ip_t;
} impeg2d_ctl_flush_ip_t;

typedef struct
{
    ivd_ctl_flush_op_t s_ivd_ctl_flush_op_t;
} impeg2d_ctl_flush_op_t;

/*****************************************************************************/
/*   Video control reset                                                     */
/*****************************************************************************/

typedef struct
{
    ivd_ctl_reset_ip_t s_ivd_ctl_reset_ip_t;
} impeg2d_ctl_reset_ip_t;

typedef struct
{
    ivd_ctl_reset_op_t s_ivd_ctl_reset_op_t;
} impeg2d_ctl_reset_op_t;

/*****************************************************************************/
/*   Video control  Set Params                                               */
/*****************************************************************************/

typedef struct
{
    ivd_ctl_set_config_ip_t s_ivd_ctl_set_config_ip_t;
} impeg2d_ctl_set_config_ip_t;

typedef struct
{
    ivd_ctl_set_config_op_t s_ivd_ctl_set_config_op_t;
} impeg2d_ctl_set_config_op_t;

/*****************************************************************************/
/*   Video control:Get Buf Info                                              */
/*****************************************************************************/

typedef struct
{
    ivd_ctl_getbufinfo_ip_t s_ivd_ctl_getbufinfo_ip_t;
} impeg2d_ctl_getbufinfo_ip_t;

typedef struct
{
    ivd_ctl_getbufinfo_op_t s_ivd_ctl_getbufinfo_op_t;
} impeg2d_ctl_getbufinfo_op_t;

/*****************************************************************************/
/*   Video control:Getstatus Call                                            */
/*****************************************************************************/

typedef struct
{
    ivd_ctl_getstatus_ip_t s_ivd_ctl_getstatus_ip_t;
} impeg2d_ctl_getstatus_ip_t;

typedef struct
{
    ivd_ctl_getstatus_op_t s_ivd_ctl_getstatus_op_t;
} impeg2d_ctl_getstatus_op_t;

/*****************************************************************************/
/*   Video control:Get Version Info                                          */
/*****************************************************************************/

typedef struct
{
    ivd_ctl_getversioninfo_ip_t s_ivd_ctl_getversioninfo_ip_t;
} impeg2d_ctl_getversioninfo_ip_t;

typedef struct
{
    ivd_ctl_getversioninfo_op_t s_ivd_ctl_getversioninfo_op_t;
} impeg2d_ctl_getversioninfo_op_t;

/*****************************************************************************/
/*   Video control:Disable Qpel                                              */
/*****************************************************************************/

typedef struct
{
    UWORD32 u4_size;
    IVD_API_COMMAND_TYPE_T e_cmd;
    IVD_CONTROL_API_COMMAND_TYPE_T e_sub_cmd;
    UWORD32 u4_num_cores;
} impeg2d_ctl_set_num_cores_ip_t;

typedef struct
{
    UWORD32 u4_size;
    UWORD32 u4_error_code;
} impeg2d_ctl_set_num_cores_op_t;

typedef struct
{
    /**
     * size
     */
    UWORD32 u4_size;
    /**
     * cmd
     */
    IVD_API_COMMAND_TYPE_T e_cmd;
    /**
     * sub cmd
     */
    IVD_CONTROL_API_COMMAND_TYPE_T e_sub_cmd;
    /**
     * Processor type
     */
    UWORD32 u4_arch;
    /**
     * SOC type
     */
    UWORD32 u4_soc;

    /**
     * num_cores
     */
    UWORD32 u4_num_cores;

} impeg2d_ctl_set_processor_ip_t;

typedef struct
{
    /**
     * size
     */
    UWORD32 u4_size;
    /**
     * error_code
     */
    UWORD32 u4_error_code;
} impeg2d_ctl_set_processor_op_t;

typedef struct
{

    /**
     * size
     */
    UWORD32 u4_size;

    /**
     * cmd
     */
    IVD_API_COMMAND_TYPE_T e_cmd;

    /**
     * sub cmd
     */
    IVD_CONTROL_API_COMMAND_TYPE_T e_sub_cmd;
} impeg2d_ctl_get_frame_dimensions_ip_t;

typedef struct
{

    /**
     * size
     */
    UWORD32 u4_size;

    /**
     * error_code
     */
    UWORD32 u4_error_code;

    /**
     * x_offset[3]
     */
    UWORD32 u4_x_offset[3];

    /**
     * y_offset[3]
     */
    UWORD32 u4_y_offset[3];

    /**
     * disp_wd[3]
     */
    UWORD32 u4_disp_wd[3];

    /**
     * disp_ht[3]
     */
    UWORD32 u4_disp_ht[3];

    /**
     * buffer_wd[3]
     */
    UWORD32 u4_buffer_wd[3];

    /**
     * buffer_ht[3]
     */
    UWORD32 u4_buffer_ht[3];
} impeg2d_ctl_get_frame_dimensions_op_t;

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* __IMPEG2D_H__ */
