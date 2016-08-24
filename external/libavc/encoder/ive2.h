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
*  ive2.h
*
* @brief
* This file contains all the necessary structure and  enumeration
* definitions needed for the Application  Program Interface(API) of the
* Ittiam Video Encoders  This is version 2
*
* @author
* Ittiam
*
* @par List of Functions:
*
* @remarks
* None
*
*******************************************************************************
*/

#ifndef _IVE2_H_
#define _IVE2_H_

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/

/** Maximum number of components in I/O Buffers                             */
#define IVE_MAX_IO_BUFFER_COMPONENTS   4

/** Maximum number of reference pictures                                    */
#define IVE_MAX_REF 16

/*****************************************************************************/
/* Enums                                                                     */
/*****************************************************************************/

/** Slice modes */
typedef enum
{
    IVE_SLICE_MODE_NA           = 0x7FFFFFFF,
    IVE_SLICE_MODE_NONE         = 0x0,

    IVE_SLICE_MODE_BYTES        = 0x1,
    IVE_SLICE_MODE_BLOCKS       = 0x2,
}IVE_SLICE_MODE_T;

/** Adaptive Intra refresh modes */
typedef enum
{
    IVE_AIR_MODE_NA             = 0x7FFFFFFF,
    IVE_AIR_MODE_NONE           = 0x0,
    IVE_AIR_MODE_CYCLIC         = 0x1,
    IVE_AIR_MODE_RANDOM         = 0x2,
    IVE_AIR_MODE_DISTORTION     = 0x3,
}IVE_AIR_MODE_T;

/** Rate control modes   */
typedef enum
{
  IVE_RC_NA                     = 0x7FFFFFFF,
  IVE_RC_NONE                   = 0x0,
  IVE_RC_STORAGE                = 0x1,
  IVE_RC_CBR_NON_LOW_DELAY      = 0x2,
  IVE_RC_CBR_LOW_DELAY          = 0x3,
  IVE_RC_TWOPASS                = 0x4,
  IVE_RC_RATECONTROLPRESET_DEFAULT = IVE_RC_STORAGE
}IVE_RC_MODE_T;

/** Encoder mode */
typedef enum
{
    IVE_ENC_MODE_NA                          = 0x7FFFFFFF,
    IVE_ENC_MODE_HEADER                      = 0x1,
    IVE_ENC_MODE_PICTURE                     = 0x0,
    IVE_ENC_MODE_DEFAULT                     = IVE_ENC_MODE_PICTURE,
}IVE_ENC_MODE_T;

/** Speed Config */
typedef enum IVE_SPEED_CONFIG
{
  IVE_QUALITY_DUMMY                         = 0x7FFFFFFF,
  IVE_CONFIG                                = 0,
  IVE_SLOWEST                               = 1,
  IVE_NORMAL                                = 2,
  IVE_FAST                                  = 3,
  IVE_HIGH_SPEED                            = 4,
  IVE_FASTEST                               = 5,
}IVE_SPEED_CONFIG;

/** API command type                                   */
typedef enum
{
    IVE_CMD_VIDEO_NA                          = 0x7FFFFFFF,
    IVE_CMD_VIDEO_CTL                         = IV_CMD_EXTENSIONS + 1,
    IVE_CMD_VIDEO_ENCODE,
    IVE_CMD_QUEUE_INPUT,
    IVE_CMD_DEQUEUE_INPUT,
    IVE_CMD_QUEUE_OUTPUT,
    IVE_CMD_DEQUEUE_OUTPUT,
    IVE_CMD_GET_RECON,
}IVE_API_COMMAND_TYPE_T;

/** Video Control API command type            */
typedef enum
{
    IVE_CMD_CT_NA                           = 0x7FFFFFFF,
    IVE_CMD_CTL_SETDEFAULT                  = 0x0,
    IVE_CMD_CTL_SET_DIMENSIONS              = 0x1,
    IVE_CMD_CTL_SET_FRAMERATE               = 0x2,
    IVE_CMD_CTL_SET_BITRATE                 = 0x3,
    IVE_CMD_CTL_SET_FRAMETYPE               = 0x4,
    IVE_CMD_CTL_SET_QP                      = 0x5,
    IVE_CMD_CTL_SET_ENC_MODE                = 0x6,
    IVE_CMD_CTL_SET_VBV_PARAMS              = 0x7,
    IVE_CMD_CTL_SET_AIR_PARAMS              = 0x8,
    IVE_CMD_CTL_SET_ME_PARAMS               = 0X9,
    IVE_CMD_CTL_SET_GOP_PARAMS              = 0XA,
    IVE_CMD_CTL_SET_PROFILE_PARAMS          = 0XB,
    IVE_CMD_CTL_SET_DEBLOCK_PARAMS          = 0XC,
    IVE_CMD_CTL_SET_IPE_PARAMS              = 0XD,
    IVE_CMD_CTL_SET_NUM_CORES               = 0x30,
    IVE_CMD_CTL_RESET                       = 0xA0,
    IVE_CMD_CTL_FLUSH                       = 0xB0,
    IVE_CMD_CTL_GETBUFINFO                  = 0xC0,
    IVE_CMD_CTL_GETVERSION                  = 0xC1,
    IVE_CMD_CTL_CODEC_SUBCMD_START          = 0x100,
}IVE_CONTROL_API_COMMAND_TYPE_T;

/* IVE_ERROR_BITS_T: A UWORD32 container will be used for reporting the error*/
/* code to the application. The first 8 bits starting from LSB have been     */
/* reserved for the codec to report internal error details. The rest of the  */
/* bits will be generic for all video encoders and each bit has an associated*/
/* meaning as mentioned below. The unused bit fields are reserved for future */
/* extenstions and will be zero in the current implementation                */
typedef enum {

    /* Bit 8 - Unsupported input parameter or configuration.                 */
    IVE_UNSUPPORTEDPARAM                        = 0x8,

    /* Bit 9 - Fatal error (stop the codec).If there is an                  */
    /* error and this bit is not set, the error is a recoverable one.       */
    IVE_FATALERROR                              = 0x9,

    IVE_ERROR_BITS_T_DUMMY_ELEMENT              = 0x7FFFFFFF
}IVE_ERROR_BITS_T;

/* IVE_ERROR_CODES_T: The list of error codes depicting the possible error  */
/* scenarios that can be encountered while encoding                         */
typedef enum
{

    IVE_ERR_NA                                                  = 0x7FFFFFFF,
    IVE_ERR_NONE                                                = 0x00,
    IVE_ERR_INVALID_API_CMD                                     = 0x01,
    IVE_ERR_INVALID_API_SUB_CMD                                 = 0x02,
    IVE_ERR_IP_GET_MEM_REC_API_STRUCT_SIZE_INCORRECT            = 0x03,
    IVE_ERR_OP_GET_MEM_REC_API_STRUCT_SIZE_INCORRECT            = 0x04,
    IVE_ERR_IP_FILL_MEM_REC_API_STRUCT_SIZE_INCORRECT           = 0x05,
    IVE_ERR_OP_FILL_MEM_REC_API_STRUCT_SIZE_INCORRECT           = 0x06,
    IVE_ERR_IP_INIT_API_STRUCT_SIZE_INCORRECT                   = 0x07,
    IVE_ERR_OP_INIT_API_STRUCT_SIZE_INCORRECT                   = 0x08,
    IVE_ERR_IP_RETRIEVE_MEM_REC_API_STRUCT_SIZE_INCORRECT       = 0x09,
    IVE_ERR_OP_RETRIEVE_MEM_REC_API_STRUCT_SIZE_INCORRECT       = 0x0A,
    IVE_ERR_IP_ENCODE_API_STRUCT_SIZE_INCORRECT                 = 0x0B,
    IVE_ERR_OP_ENCODE_API_STRUCT_SIZE_INCORRECT                 = 0x0C,
    IVE_ERR_IP_CTL_SETDEF_API_STRUCT_SIZE_INCORRECT             = 0x0D,
    IVE_ERR_OP_CTL_SETDEF_API_STRUCT_SIZE_INCORRECT             = 0x0E,
    IVE_ERR_IP_CTL_GETBUFINFO_API_STRUCT_SIZE_INCORRECT         = 0x0F,
    IVE_ERR_OP_CTL_GETBUFINFO_API_STRUCT_SIZE_INCORRECT         = 0x10,
    IVE_ERR_IP_CTL_GETVERSION_API_STRUCT_SIZE_INCORRECT         = 0x11,
    IVE_ERR_OP_CTL_GETVERSION_API_STRUCT_SIZE_INCORRECT         = 0x12,
    IVE_ERR_IP_CTL_FLUSH_API_STRUCT_SIZE_INCORRECT              = 0x13,
    IVE_ERR_OP_CTL_FLUSH_API_STRUCT_SIZE_INCORRECT              = 0x14,
    IVE_ERR_IP_CTL_RESET_API_STRUCT_SIZE_INCORRECT              = 0x15,
    IVE_ERR_OP_CTL_RESET_API_STRUCT_SIZE_INCORRECT              = 0x16,
    IVE_ERR_IP_CTL_SETCORES_API_STRUCT_SIZE_INCORRECT           = 0x17,
    IVE_ERR_OP_CTL_SETCORES_API_STRUCT_SIZE_INCORRECT           = 0x18,
    IVE_ERR_IP_CTL_SETDIM_API_STRUCT_SIZE_INCORRECT             = 0x19,
    IVE_ERR_OP_CTL_SETDIM_API_STRUCT_SIZE_INCORRECT             = 0x1A,
    IVE_ERR_IP_CTL_SETFRAMERATE_API_STRUCT_SIZE_INCORRECT       = 0x1B,
    IVE_ERR_OP_CTL_SETFRAMERATE_API_STRUCT_SIZE_INCORRECT       = 0x1C,
    IVE_ERR_IP_CTL_SETBITRATE_API_STRUCT_SIZE_INCORRECT         = 0x1D,
    IVE_ERR_OP_CTL_SETBITRATE_API_STRUCT_SIZE_INCORRECT         = 0x1E,
    IVE_ERR_IP_CTL_SETFRAMETYPE_API_STRUCT_SIZE_INCORRECT       = 0x1F,
    IVE_ERR_OP_CTL_SETFRAMETYPE_API_STRUCT_SIZE_INCORRECT       = 0x20,
    IVE_ERR_IP_CTL_SETMEPARAMS_API_STRUCT_SIZE_INCORRECT        = 0x21,
    IVE_ERR_OP_CTL_SETMEPARAMS_API_STRUCT_SIZE_INCORRECT        = 0x22,
    IVE_ERR_IP_CTL_SETIPEPARAMS_API_STRUCT_SIZE_INCORRECT       = 0x23,
    IVE_ERR_OP_CTL_SETIPEPARAMS_API_STRUCT_SIZE_INCORRECT       = 0x24,
    IVE_ERR_IP_CTL_SETGOPPARAMS_API_STRUCT_SIZE_INCORRECT       = 0x25,
    IVE_ERR_OP_CTL_SETGOPPARAMS_API_STRUCT_SIZE_INCORRECT       = 0x26,
    IVE_ERR_IP_CTL_SETDEBLKPARAMS_API_STRUCT_SIZE_INCORRECT     = 0x27,
    IVE_ERR_OP_CTL_SETDEBLKPARAMS_API_STRUCT_SIZE_INCORRECT     = 0x28,
    IVE_ERR_IP_CTL_SETQPPARAMS_API_STRUCT_SIZE_INCORRECT        = 0x29,
    IVE_ERR_OP_CTL_SETQPPARAMS_API_STRUCT_SIZE_INCORRECT        = 0x2A,
    IVE_ERR_FILL_NUM_MEM_RECS_POINTER_NULL                      = 0x2B,
    IVE_ERR_NUM_MEM_REC_NOT_SUFFICIENT                          = 0x2C,
    IVE_ERR_MEM_REC_STRUCT_SIZE_INCORRECT                       = 0x2D,
    IVE_ERR_MEM_REC_BASE_POINTER_NULL                           = 0x2E,
    IVE_ERR_MEM_REC_OVERLAP_ERR                                 = 0x2F,
    IVE_ERR_MEM_REC_INSUFFICIENT_SIZE                           = 0x30,
    IVE_ERR_MEM_REC_ALIGNMENT_ERR                               = 0x31,
    IVE_ERR_MEM_REC_INCORRECT_TYPE                              = 0x32,
    IVE_ERR_HANDLE_NULL                                         = 0x33,
    IVE_ERR_HANDLE_STRUCT_SIZE_INCORRECT                        = 0x34,
    IVE_ERR_API_FUNCTION_PTR_NULL                               = 0x35,
    IVE_ERR_INVALID_CODEC_HANDLE                                = 0x36,
    IVE_ERR_CTL_GET_VERSION_BUFFER_IS_NULL                      = 0x37,
    IVE_ERR_IP_CTL_SETAIRPARAMS_API_STRUCT_SIZE_INCORRECT       = 0x38,
    IVE_ERR_OP_CTL_SETAIRPARAMS_API_STRUCT_SIZE_INCORRECT       = 0x39,
    IVE_ERR_IP_CTL_SETENCMODE_API_STRUCT_SIZE_INCORRECT         = 0x3A,
    IVE_ERR_OP_CTL_SETENCMODE_API_STRUCT_SIZE_INCORRECT         = 0x3B,
    IVE_ERR_IP_CTL_SETVBVPARAMS_API_STRUCT_SIZE_INCORRECT       = 0x3C,
    IVE_ERR_OP_CTL_SETVBVPARAMS_API_STRUCT_SIZE_INCORRECT       = 0x3D,
    IVE_ERR_IP_CTL_SETPROFILE_API_STRUCT_SIZE_INCORRECT         = 0x3E,
    IVE_ERR_OP_CTL_SETPROFILE_API_STRUCT_SIZE_INCORRECT         = 0x3F,

}IVE_ERROR_CODES_T;


/*****************************************************************************/
/*   Initialize encoder                                                      */
/*****************************************************************************/

/** Input structure : Initialize the encoder                                */
typedef struct
{
    /** size of the structure                                               */
    UWORD32                                 u4_size;

    /** Command type                                                        */
    IV_API_COMMAND_TYPE_T                   e_cmd;

    /** Number of memory records                                            */
    UWORD32                                 u4_num_mem_rec;

    /** pointer to array of memrecords structures should be filled by codec
    with details of memory resource requirements                            */
    iv_mem_rec_t                            *ps_mem_rec;

    /** maximum width for which codec should request memory requirements    */
    UWORD32                                 u4_max_wd;

    /** maximum height for which codec should request memory requirements   */
    UWORD32                                 u4_max_ht;

    /** Maximum number of reference frames                                  */
    UWORD32                                 u4_max_ref_cnt;

    /** Maximum number of reorder frames                                    */
    UWORD32                                 u4_max_reorder_cnt;

    /** Maximum level supported                                             */
    UWORD32                                 u4_max_level;

    /** Input color format                                                  */
    IV_COLOR_FORMAT_T                       e_inp_color_fmt;

    /** Flag to enable/disable - To be used only for debugging/testing      */
    UWORD32                                 u4_enable_recon;

    /** Recon color format                                                  */
    IV_COLOR_FORMAT_T                       e_recon_color_fmt;

    /** Rate control mode                                                   */
    IVE_RC_MODE_T                           e_rc_mode;

    /** Maximum frame rate to be supported                                  */
    UWORD32                                 u4_max_framerate;

    /** Maximum bitrate to be supported                                     */
    UWORD32                                 u4_max_bitrate;

    /** Maximum number of consecutive  B frames                             */
    UWORD32                                 u4_num_bframes;

    /** Content type Interlaced/Progressive                                 */
    IV_CONTENT_TYPE_T                       e_content_type;

    /** Maximum search range to be used in X direction                      */
    UWORD32                                 u4_max_srch_rng_x;

    /** Maximum search range to be used in Y direction                      */
    UWORD32                                 u4_max_srch_rng_y;

    /** Slice Mode                                                          */
    IVE_SLICE_MODE_T                        e_slice_mode;

    /** Slice parameter                                                     */
    UWORD32                                 u4_slice_param;

    /** Processor architecture                                          */
    IV_ARCH_T                                   e_arch;

    /** SOC details                                                     */
    IV_SOC_T                                    e_soc;


}ive_init_ip_t;

/** Output structure : Initialize the encoder                           */
typedef struct
{
    /** Size of the structure                                           */
    UWORD32                                 u4_size;

    /** Return error code                                               */
    UWORD32                                 u4_error_code;
}ive_init_op_t;


/*****************************************************************************/
/*   Video Encode - Deprecated                                               */
/*****************************************************************************/

typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    IVE_API_COMMAND_TYPE_T                  e_cmd;

    /** Descriptor for input raw buffer                                 */
    iv_raw_buf_t                            s_inp_buf;

    /** Buffer containing pic info if mb_info_type is non-zero           */
    void                                    *pv_bufs;

    /** Flag to indicate if mb info is sent along with input buffer     */
    UWORD32                                 u4_mb_info_type;

    /** Buffer containing mb info if mb_info_type is non-zero           */
    void                                    *pv_mb_info;

    /** Flag to indicate if pic info is sent along with input buffer     */
    UWORD32                                 u4_pic_info_type;

    /** Buffer containing pic info if mb_info_type is non-zero           */
    void                                    *pv_pic_info;

    /** Lower 32bits of input time stamp                                */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of input time stamp                                */
    UWORD32                                 u4_timestamp_high;

    /** Flag to indicate if this is the last input in the stream       */
    UWORD32                                 u4_is_last;

    /** Descriptor for output bit-stream buffer                         */
    iv_bits_buf_t                           s_out_buf;

    /** Descriptor for recon buffer                                     */
    iv_raw_buf_t                            s_recon_buf;

}ive_video_encode_ip_t;


typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** error code                                                      */
    UWORD32                                 u4_error_code;

    /* Output present                                                   */
    WORD32                                  output_present;

    /* dump recon                                                       */
    WORD32                                  dump_recon;

    /* encoded frame type                                               */
    UWORD32                                 u4_encoded_frame_type;

    /** Flag to indicate if this is the last output from the encoder    */
    UWORD32                                 u4_is_last;

    /** Lower 32bits of input time stamp                                */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of input time stamp                                */
    UWORD32                                 u4_timestamp_high;

    /** Descriptor for input raw buffer freed from codec                */
    iv_raw_buf_t                            s_inp_buf;

    /** Descriptor for output bit-stream buffer                         */
    iv_bits_buf_t                           s_out_buf;

    /** Descriptor for recon buffer                                     */
    iv_raw_buf_t                            s_recon_buf;

}ive_video_encode_op_t;

/*****************************************************************************/
/*   Queue Input raw buffer - Send the YUV buffer to be encoded              */
/*****************************************************************************/
/** Input structure : Queue input buffer to the encoder                 */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Command : IVE_CMD_QUEUE_INPUT                                   */
    IVE_API_COMMAND_TYPE_T                  e_cmd;

    /** Descriptor for input raw buffer                                 */
    iv_raw_buf_t                            s_inp_buf;

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

    /** Lower 32bits of input time stamp                                */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of input time stamp                                */
    UWORD32                                 u4_timestamp_high;


    /** Flag to enable/disable blocking the current API call            */
    UWORD32                                 u4_is_blocking;

    /** Flag to indicate if this is the last input in the stream       */
    UWORD32                                 u4_is_last;

}ive_queue_inp_ip_t;

/** Input structure : Queue output buffer to the encoder                */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Return error code                                               */
    UWORD32                                 u4_error_code;
}ive_queue_inp_op_t;

/*****************************************************************************/
/*   Dequeue Input raw buffer - Get free YUV buffer from the encoder         */
/*****************************************************************************/
/** Input structure : Dequeue input buffer from the encoder             */

typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Command: IVE_CMD_DEQUEUE_INPUT                                  */
    IVE_API_COMMAND_TYPE_T                  e_cmd;

    /** Flag to enable/disable blocking the current API call            */
    UWORD32                                 u4_is_blocking;

}ive_dequeue_inp_ip_t;

/** Output structure : Dequeue input buffer from the encoder            */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Return error code                                               */
    UWORD32                                 u4_error_code;

    /** Buffer descriptor of the buffer returned from encoder           */
    iv_raw_buf_t                            s_inp_buf;

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

    /** Lower 32bits of input time stamp                                */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of input time stamp                                */
    UWORD32                                 u4_timestamp_high;

    /** Flag to indicate if this is the last input in the stream       */
    UWORD32                                 u4_is_last;


}ive_dequeue_inp_op_t;

/*****************************************************************************/
/*   Queue Output bitstream buffer - Send the bistream buffer to be filled   */
/*****************************************************************************/
/** Input structure : Queue output buffer to the encoder                 */

typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Command : IVE_CMD_QUEUE_OUTPUT                                  */
    IVE_API_COMMAND_TYPE_T                  e_cmd;

    /** Descriptor for output bit-stream buffer                         */
    iv_bits_buf_t                           s_out_buf;

    /** Flag to enable/disable blocking the current API call            */
    UWORD32                                 u4_is_blocking;

    /** Flag to indicate if this is the last output in the stream       */
    UWORD32                                 u4_is_last;

}ive_queue_out_ip_t;

/** Output structure : Queue output buffer to the encoder               */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Return error code                                               */
    UWORD32                                 u4_error_code;

}ive_queue_out_op_t;


/*****************************************************************************/
/* Dequeue Output bitstream buffer - Get the bistream buffer filled          */
/*****************************************************************************/
/** Input structure : Dequeue output buffer from the encoder            */

typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Command : IVE_CMD_DEQUEUE_OUTPUT                                */
    IVE_API_COMMAND_TYPE_T                  e_cmd;

    /** Flag to enable/disable blocking the current API call            */
    UWORD32                                 u4_is_blocking;
}ive_dequeue_out_ip_t;

/** Output structure : Dequeue output buffer from the encoder           */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Return error code                                               */
    UWORD32                                 u4_error_code;

    /** Descriptor for output bit-stream buffer                         */
    iv_bits_buf_t                           s_out_buf;

    /** Lower 32bits of timestamp corresponding to this buffer           */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of timestamp corresponding to this buffer           */
    UWORD32                                 u4_timestamp_high;

    /** Flag to indicate if this is the last output in the stream       */
    UWORD32                                 u4_is_last;

}ive_dequeue_out_op_t;

/*****************************************************************************/
/* Get Recon data - Get the reconstructed data from encoder                  */
/*****************************************************************************/
/** Input structure : Get recon data from the encoder                   */

typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Command : IVE_CMD_GET_RECON                                     */
    IVE_API_COMMAND_TYPE_T                  e_cmd;

    /** Flag to enable/disable blocking the current API call            */
    UWORD32                                 u4_is_blocking;

    /** Descriptor for recon buffer                                     */
    iv_raw_buf_t                            s_recon_buf;

    /** Flag to indicate if this is the last recon in the stream       */
    UWORD32                                 u4_is_last;

}ive_get_recon_ip_t;

/** Output structure : Get recon data from the encoder                  */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Return error code                                               */
    UWORD32                                 u4_error_code;

    /** Lower 32bits of time stamp corresponding to this buffer          */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to this buffer          */
    UWORD32                                 u4_timestamp_high;

    /** Flag to indicate if this is the last recon in the stream       */
    UWORD32                                 u4_is_last;

}ive_get_recon_op_t;

/*****************************************************************************/
/*   Video control  Flush                                                    */
/*****************************************************************************/

/** Input structure : Flush all the buffers from the encoder            */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                  e_cmd;

    /** Sub command type : IVE_CMD_CTL_FLUSH                            */
    IVE_CONTROL_API_COMMAND_TYPE_T          e_sub_cmd;
}ive_ctl_flush_ip_t;

/** Output structure : Flush all the buffers from the encoder           */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Return error code                                               */
    UWORD32                                 u4_error_code;
}ive_ctl_flush_op_t;

/*****************************************************************************/
/*   Video control reset                                                     */
/*****************************************************************************/
/** Input structure : Reset the encoder                                 */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                  e_cmd;

    /** Sub command type : IVE_CMD_CTL_RESET                            */
    IVE_CONTROL_API_COMMAND_TYPE_T          e_sub_cmd;
}ive_ctl_reset_ip_t;

/** Output structure : Reset the encoder                                */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                 u4_size;

    /** Return error code                                               */
    UWORD32                                 u4_error_code;
}ive_ctl_reset_op_t;

/*****************************************************************************/
/*   Video control:Get Buf Info                                              */
/*****************************************************************************/

/** Input structure : Get encoder buffer requirements                   */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_GETBUFINFO                       */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** maximum width for which codec should request memory requirements    */
    UWORD32                                     u4_max_wd;

    /** maximum height for which codec should request memory requirements   */
    UWORD32                                     u4_max_ht;

    /** Input color format                                                  */
    IV_COLOR_FORMAT_T                           e_inp_color_fmt;

}ive_ctl_getbufinfo_ip_t;

/** Output structure : Get encoder buffer requirements                  */
typedef struct
{
    /** size of the structure                                           */
    UWORD32 u4_size;

    /** Return error code                                               */
    UWORD32 u4_error_code;

    /** Minimum number of input buffers required for codec              */
    UWORD32 u4_min_inp_bufs;

    /** Minimum number of output buffers required for codec             */
    UWORD32 u4_min_out_bufs;

    /** Number of components in input buffers required for codec        */
    UWORD32 u4_inp_comp_cnt;

    /** Number of components in output buffers required for codec       */
    UWORD32 u4_out_comp_cnt;

    /** Minimum sizes of each component in input buffer required        */
    UWORD32 au4_min_in_buf_size[IVE_MAX_IO_BUFFER_COMPONENTS];

    /** Minimum sizes of each component in output buffer  required      */
    UWORD32 au4_min_out_buf_size[IVE_MAX_IO_BUFFER_COMPONENTS];

}ive_ctl_getbufinfo_op_t;




/*****************************************************************************/
/*   Video control:Get Version Info                                          */
/*****************************************************************************/

/** Input structure : Get encoder version information                   */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;
    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_GETVERSION                       */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Buffer where version info will be returned                      */
    UWORD8                                      *pu1_version;

    /** Size of the buffer allocated for version info                   */
    UWORD32                                     u4_version_bufsize;
}ive_ctl_getversioninfo_ip_t;

/** Output structure : Get encoder version information                  */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_getversioninfo_op_t;


/*****************************************************************************/
/*   Video control:set  default params                                       */
/*****************************************************************************/
/** Input structure : Set default encoder parameters                    */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SETDEFAULT                       */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;

}ive_ctl_setdefault_ip_t;

/** Output structure : Set default encoder parameters                   */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_setdefault_op_t;

/*****************************************************************************/
/*   Video control  Set Frame dimensions                                     */
/*****************************************************************************/

/** Input structure : Set frame dimensions                              */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_DIMENSIONS                   */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Input width                                                     */
    UWORD32                                     u4_wd;

    /** Input height                                                    */
    UWORD32                                     u4_ht;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;

}ive_ctl_set_dimensions_ip_t;

/** Output structure : Set frame dimensions                             */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_dimensions_op_t;


/*****************************************************************************/
/*   Video control  Set Frame rates                                          */
/*****************************************************************************/

/** Input structure : Set frame rate                                    */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_FRAMERATE                   */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Source frame rate                                               */
    UWORD32                                     u4_src_frame_rate;

    /** Target frame rate                                               */
    UWORD32                                     u4_tgt_frame_rate;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;

}ive_ctl_set_frame_rate_ip_t;

/** Output structure : Set frame rate                                    */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_frame_rate_op_t;

/*****************************************************************************/
/*   Video control  Set Bitrate                                              */
/*****************************************************************************/

/** Input structure : Set bitrate                                       */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_BITRATE                      */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Target bitrate in kilobits per second                           */
    UWORD32                                     u4_target_bitrate;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;

}ive_ctl_set_bitrate_ip_t;

/** Output structure : Set bitrate                                      */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_bitrate_op_t;

/*****************************************************************************/
/*   Video control  Set Frame type                                           */
/*****************************************************************************/

/** Input structure : Set frametype                                     */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_FRAMETYPE                    */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Force current frame type                                        */
    IV_PICTURE_CODING_TYPE_T                    e_frame_type;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;

}ive_ctl_set_frame_type_ip_t;

/** Output structure : Set frametype                                     */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_frame_type_op_t;

/*****************************************************************************/
/*   Video control  Set Encode mode                                          */
/*****************************************************************************/

/** Input structure : Set encode mode                                   */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_ENC_MODE                    */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Encoder mode                                                    */
    IVE_ENC_MODE_T                              e_enc_mode;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;

}ive_ctl_set_enc_mode_ip_t;

/** Output structure : Set encode mode                                  */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;

}ive_ctl_set_enc_mode_op_t;

/*****************************************************************************/
/*   Video control  Set QP                                                   */
/*****************************************************************************/

/** Input structure : Set QP                                            */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_QP                           */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

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

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;


}ive_ctl_set_qp_ip_t;

/** Output structure : Set QP                                           */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_qp_op_t;

/*****************************************************************************/
/*   Video control  Set AIR params                                           */
/*****************************************************************************/

/** Input structure : Set AIR params                                    */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;
    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_AIR_PARAMS                   */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Adaptive intra refresh mode                                     */
    IVE_AIR_MODE_T                              e_air_mode;

    /** Adaptive intra refresh period in frames                         */
    UWORD32                                     u4_air_refresh_period;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;


}ive_ctl_set_air_params_ip_t;

/** Output structure : Set AIR params                                   */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_air_params_op_t;

/*****************************************************************************/
/*   Video control  Set VBV params                                           */
/*****************************************************************************/

/** Input structure : Set VBV params                                    */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_VBV_PARAMS                   */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** VBV buffer delay                                                */
    UWORD32                                     u4_vbv_buffer_delay;

    /** VBV buffer size                                                 */
    UWORD32                                     u4_vbv_buf_size;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;


}ive_ctl_set_vbv_params_ip_t;

/** Output structure : Set VBV params                                   */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_vbv_params_op_t;


/*****************************************************************************/
/*   Video control  Set Processor Details                                    */
/*****************************************************************************/

/** Input structure : Set processor details                             */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_NUM_CORES                    */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Total number of cores to be used                                */
    UWORD32                                     u4_num_cores;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;

}ive_ctl_set_num_cores_ip_t;

/** Output structure : Set processor details                            */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_num_cores_op_t;

/*****************************************************************************/
/*   Video control  Set Intra Prediction estimation params                   */
/*****************************************************************************/

/** Input structure : Set IPE params                                    */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_IPE_PARAMS                   */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Flag to enable/disbale intra 4x4 analysis                       */
    UWORD32                                     u4_enable_intra_4x4;

    /** Flag to enable/disable pre-enc stage of Intra Pred estimation   */
    UWORD32                                     u4_pre_enc_ipe;

    /** Speed preset - Value between 0 (slowest) and 100 (fastest)      */
    IVE_SPEED_CONFIG                            u4_enc_speed_preset;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                     u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                     u4_timestamp_high;

    /** Constrained intra pred flag                                     */
    UWORD32                                     u4_constrained_intra_pred;

}ive_ctl_set_ipe_params_ip_t;

/** Output structure : Set IPE Params                                   */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_ipe_params_op_t;

/*****************************************************************************/
/*   Video control  Set Motion estimation params                             */
/*****************************************************************************/

/** Input structure : Set ME Params                                     */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_ME_PARAMS                    */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Flag to enable/disable pre-enc stage of Motion estimation       */
    UWORD32                                     u4_pre_enc_me;

    /** Speed preset - Value between 0 (slowest) and 100 (fastest)      */
    UWORD32                                     u4_me_speed_preset;

    /** Flag to enable/disable half pel motion estimation               */
    UWORD32                                     u4_enable_hpel;

    /** Flag to enable/disable quarter pel motion estimation            */
    UWORD32                                     u4_enable_qpel;

    /** Flag to enable/disable fast SAD approximation                   */
    UWORD32                                     u4_enable_fast_sad;

    /** Flag to enable/disable alternate reference frames               */
    UWORD32                                     u4_enable_alt_ref;

    /** Maximum search range in X direction for farthest reference      */
    UWORD32                                     u4_srch_rng_x;

    /** Maximum search range in Y direction for farthest reference      */
    UWORD32                                     u4_srch_rng_y;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                     u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                     u4_timestamp_high;

}ive_ctl_set_me_params_ip_t;

/** Output structure : Set ME Params                                    */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_me_params_op_t;

/*****************************************************************************/
/*   Video control  Set GOP params                                           */
/*****************************************************************************/

/** Input structure : Set GOP Params                                    */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_GOP_PARAMS                   */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** I frame interval                                                */
    UWORD32                                     u4_i_frm_interval;

    /** IDR frame interval                                              */
    UWORD32                                     u4_idr_frm_interval;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;

}ive_ctl_set_gop_params_ip_t;

/** Output structure : Set GOP params                                   */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_gop_params_op_t;

/*****************************************************************************/
/*   Video control  Set Deblock params                                       */
/*****************************************************************************/

/** Input structure : Set Deblock Params                                */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_GOP_PARAMS                   */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Disable deblock level (0: Enable completely, 3: Disable completely */
    UWORD32                                     u4_disable_deblock_level;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;

}ive_ctl_set_deblock_params_ip_t;

/** Output structure : Set Deblock Params                               */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_deblock_params_op_t;

/*****************************************************************************/
/*   Video control  Set Profile params                                       */
/*****************************************************************************/

/** Input structure : Set Profile Params                                */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Command type : IVE_CMD_VIDEO_CTL                                */
    IVE_API_COMMAND_TYPE_T                      e_cmd;

    /** Sub command type : IVE_CMD_CTL_SET_PROFILE_PARAMS               */
    IVE_CONTROL_API_COMMAND_TYPE_T              e_sub_cmd;

    /** Profile                                                         */
    IV_PROFILE_T                               e_profile;

    /** Lower 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_low;

    /** Upper 32bits of time stamp corresponding to input buffer,
     * from which this command takes effect                             */
    UWORD32                                 u4_timestamp_high;

    /** Entropy coding mode flag: 0-CAVLC, 1-CABAC                       */
    UWORD32                                 u4_entropy_coding_mode;

}ive_ctl_set_profile_params_ip_t;

/** Output structure : Set Profile Params                               */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Return error code                                               */
    UWORD32                                     u4_error_code;
}ive_ctl_set_profile_params_op_t;


#endif /* _IVE2_H_ */

