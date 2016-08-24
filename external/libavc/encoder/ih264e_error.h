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
*  ih264e_error.h
*
* @brief
*  Definitions related to error handling
*
* @author
*  ittiam
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_ERROR_H_
#define IH264E_ERROR_H_

/**
******************************************************************************
*  @brief   Error start codes for various classes of errors in H264 encoder
******************************************************************************
*/
#define SET_ERROR_ON_RETURN(error, severity, out_status, ret_code) \
    if (error != IH264E_SUCCESS) \
    {\
        out_status = ((1 << severity) | error);\
        return (ret_code);\
    }


/**
******************************************************************************
 *  @brief   Extended error code for each error in  H264 encoder
******************************************************************************
 */
typedef enum
{
    /* NOTE: the ive error codes ends at 0x80 */
    IVE_ERR_CODEC_EXTENSIONS                                        = 0x80,

    /* bit stream error start */
    IH264E_BITSTREAM_ERROR_START                                    = IVE_ERR_CODEC_EXTENSIONS,

    /* codec error start */
    IH264E_CODEC_ERROR_START                                        = IH264E_BITSTREAM_ERROR_START + 0x10,

    /** no error */
    IH264E_SUCCESS                                                  = 0,

    /** bitstream init failure, buffer ptr not aligned to WORD (32bits)     */
    IH264E_BITSTREAM_BUFPTR_ALIGN_FAIL                              = IH264E_BITSTREAM_ERROR_START + 0x01,

    /** bitstream init failure, buf size not multiple of WORD size (32bits) */
    IH264E_BITSTREAM_BUFSIZE_ALIGN_FAIL                             = IH264E_BITSTREAM_ERROR_START + 0x02,

    /** bitstream runtime failure, buf size limit exceeded during encode    */
    IH264E_BITSTREAM_BUFFER_OVERFLOW                                = IH264E_BITSTREAM_ERROR_START + 0x03,

    /**width not set within supported limit */
    IH264E_WIDTH_NOT_SUPPORTED                                      = IH264E_CODEC_ERROR_START + 0x01,

    /**height not set within supported limit */
    IH264E_HEIGHT_NOT_SUPPORTED                                     = IH264E_CODEC_ERROR_START + 0x02,

    /**Unsupported number of reference pictures passed as an argument */
    IH264E_NUM_REF_UNSUPPORTED                                      = IH264E_CODEC_ERROR_START + 0x03,

    /**Unsupported number of reference pictures passed as an argument */
    IH264E_NUM_REORDER_UNSUPPORTED                                  = IH264E_CODEC_ERROR_START + 0x04,

    /**codec level not supported */
    IH264E_CODEC_LEVEL_NOT_SUPPORTED                                = IH264E_CODEC_ERROR_START + 0x05,

    /**input chroma format not supported */
    IH264E_INPUT_CHROMA_FORMAT_NOT_SUPPORTED                        = IH264E_CODEC_ERROR_START + 0x06,

    /**recon chroma format not supported */
    IH264E_RECON_CHROMA_FORMAT_NOT_SUPPORTED                        = IH264E_CODEC_ERROR_START + 0x07,

    /**rate control option configured is not supported */
    IH264E_RATE_CONTROL_MODE_NOT_SUPPORTED                          = IH264E_CODEC_ERROR_START + 0x08,

    /**frame rate configured is not supported */
    IH264E_FRAME_RATE_NOT_SUPPORTED                                 = IH264E_CODEC_ERROR_START + 0x09,

    /**bit rate configured is not supported */
    IH264E_BITRATE_NOT_SUPPORTED                                    = IH264E_CODEC_ERROR_START + 0x0A,

    /**frame rate not supported */
    IH264E_BFRAMES_NOT_SUPPORTED                                    = IH264E_CODEC_ERROR_START + 0x0B,

    /**content type not supported */
    IH264E_CONTENT_TYPE_NOT_SUPPORTED                               = IH264E_CODEC_ERROR_START + 0x0C,

    /**unsupported horizontal search range */
    IH264E_HORIZONTAL_SEARCH_RANGE_NOT_SUPPORTED                    = IH264E_CODEC_ERROR_START + 0x0D,

    /**unsupported vertical search range */
    IH264E_VERTICAL_SEARCH_RANGE_NOT_SUPPORTED                      = IH264E_CODEC_ERROR_START + 0x0E,

    /**Unsupported slice type input */
    IH264E_SLICE_TYPE_INPUT_INVALID                                 = IH264E_CODEC_ERROR_START + 0x0F,

    /**unsupported architecture type */
    IH264E_ARCH_TYPE_NOT_SUPPORTED                                  = IH264E_CODEC_ERROR_START + 0x10,

    /**unsupported soc type */
    IH264E_SOC_TYPE_NOT_SUPPORTED                                   = IH264E_CODEC_ERROR_START + 0x11,

    /**target frame rate exceeds source frame rate */
    IH264E_TGT_FRAME_RATE_EXCEEDS_SRC_FRAME_RATE                    = IH264E_CODEC_ERROR_START + 0x12,

    /**invalid force frame input */
    IH264E_INVALID_FORCE_FRAME_INPUT                                = IH264E_CODEC_ERROR_START + 0x13,

    /**invalid me speed preset */
    IH264E_INVALID_ME_SPEED_PRESET                                  = IH264E_CODEC_ERROR_START + 0x14,

    /**invalid encoder speed preset */
    IH264E_INVALID_ENC_SPEED_PRESET                                 = IH264E_CODEC_ERROR_START + 0x15,

    /**invalid deblocking param */
    IH264E_INVALID_DEBLOCKING_TYPE_INPUT                            = IH264E_CODEC_ERROR_START + 0x16,

    /**invalid max qp */
    IH264E_INVALID_MAX_FRAME_QP                                     = IH264E_CODEC_ERROR_START + 0x17,

    /**invalid min qp */
    IH264E_INVALID_MIN_FRAME_QP                                     = IH264E_CODEC_ERROR_START + 0x18,

    /**invalid init qp */
    IH264E_INVALID_INIT_QP                                          = IH264E_CODEC_ERROR_START + 0x19,

    /**version buffer size is insufficient */
    IH264E_CXA_VERS_BUF_INSUFFICIENT                                = IH264E_CODEC_ERROR_START + 0x1A,

    /**init not done */
    IH264E_INIT_NOT_DONE                                            = IH264E_CODEC_ERROR_START + 0x1B,

    /**invalid refresh type input */
    IH264E_INVALID_AIR_MODE                                         = IH264E_CODEC_ERROR_START + 0x1C,

    /** Unsupported air mode */
    IH264E_INVALID_AIR_REFRESH_PERIOD                               = IH264E_CODEC_ERROR_START + 0x1D,

    /**In sufficient memory allocated for MV Bank */
    IH264E_INSUFFICIENT_MEM_MVBANK                                  = IH264E_CODEC_ERROR_START + 0x1E,

    /**In sufficient memory allocated for MV Bank */
    IH264E_INSUFFICIENT_MEM_PICBUF                                  = IH264E_CODEC_ERROR_START + 0x1F,

    /**Buffer manager error */
    IH264E_BUF_MGR_ERROR                                            = IH264E_CODEC_ERROR_START + 0x20,

    /**No free MV Bank buffer available to store current pic */
    IH264E_NO_FREE_MVBANK                                           = IH264E_CODEC_ERROR_START + 0x21,

    /**No free picture buffer available to store current pic */
    IH264E_NO_FREE_PICBUF                                           = IH264E_CODEC_ERROR_START + 0x22,

    /**Invalid encoder operation mode */
    IH264E_INVALID_ENC_OPERATION_MODE                               = IH264E_CODEC_ERROR_START + 0x23,

    /**Invalid half pel option */
    IH264E_INVALID_HALFPEL_OPTION                                   = IH264E_CODEC_ERROR_START + 0x24,

    /**Invalid quarter pel option */
    IH264E_INVALID_QPEL_OPTION                                      = IH264E_CODEC_ERROR_START + 0x25,

    /**Invalid fast sad option */
    IH264E_INVALID_FAST_SAD_OPTION                                  = IH264E_CODEC_ERROR_START + 0x26,

    /**Invalid intra 4x4 option */
    IH264E_INVALID_INTRA4x4_OPTION                                  = IH264E_CODEC_ERROR_START + 0x27,

    /**Invalid intra frame interval */
    IH264E_INVALID_INTRA_FRAME_INTERVAL                             = IH264E_CODEC_ERROR_START + 0x28,

    /**Invalid idr frame interval */
    IH264E_INVALID_IDR_FRAME_INTERVAL                               = IH264E_CODEC_ERROR_START + 0x29,

    /**Invalid buffer delay */
    IH264E_INVALID_BUFFER_DELAY                                     = IH264E_CODEC_ERROR_START + 0x2A,

    /**Invalid num cores */
    IH264E_INVALID_NUM_CORES                                        = IH264E_CODEC_ERROR_START + 0x2B,

    /**profile not supported */
    IH264E_PROFILE_NOT_SUPPORTED                                    = IH264E_CODEC_ERROR_START + 0x2C,

    /**Unsupported slice type input */
    IH264E_SLICE_PARAM_INPUT_INVALID                                = IH264E_CODEC_ERROR_START + 0x2D,

    /**Invalid alt ref option */
    IH264E_INVALID_ALT_REF_OPTION                                   = IH264E_CODEC_ERROR_START + 0x2E,

    /**No free picture buffer available to store recon pic */
    IH264E_NO_FREE_RECONBUF                                         = IH264E_CODEC_ERROR_START + 0x2F,

    /**Not enough memory allocated as output buffer */
    IH264E_INSUFFICIENT_OUTPUT_BUFFER                               = IH264E_CODEC_ERROR_START + 0x30,

    /**Invalid entropy coding mode */
    IH264E_INVALID_ENTROPY_CODING_MODE                              = IH264E_CODEC_ERROR_START + 0x31,

    /**Invalid Constrained Intra prediction mode */
    IH264E_INVALID_CONSTRAINED_INTRA_PREDICTION_MODE                = IH264E_CODEC_ERROR_START + 0x32,

    /**max failure error code to ensure enum is 32 bits wide */
    IH264E_FAIL                                                     = -1,

}IH264E_ERROR_T;


#endif /* IH264E_ERROR_H_ */
