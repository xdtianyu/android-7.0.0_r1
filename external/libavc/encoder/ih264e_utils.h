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
*  ih264e_utils.h
*
* @brief
*  Contains declarations of miscellaneous utility functions used by the encoder
*
* @author
*  Harish
*
* @par List of Functions:
*  -ih264e_input_queue_update()
*  -ih264e_get_min_level()
*  -ih264e_get_lvl_idx()
*  -ih264e_get_dpb_size()
*  -ih264e_get_total_pic_buf_size()
*  -ih264e_get_pic_mv_bank_size()
*  -ih264e_pic_buf_mgr_add_bufs()
*  -ih264e_mv_buf_mgr_add_bufs()
*  -ih264e_init_quant_params()
*  -ih264e_init_air_map()
*  -ih264e_codec_init()
*  -ih264e_pic_init()
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef IH264E_UTILS_H_
#define IH264E_UTILS_H_

/**
 *******************************************************************************
 *
 * @brief
 *  Queues the current buffer, gets back a another buffer for encoding with corrent
 *  picture type
 *
 * @par Description:
 *
 * @param[in] ps_codec
 *   Pointer to codec descriptor
 *
 * @param[in] ps_ive_ip
 *   Current input buffer to the encoder
 *
 * @param[out] ps_inp
 *   Buffer to be encoded in the current pass
 *
 * @returns
 *   Flag indicating if we have a pre-enc skip or not
 *
 * @remarks
 *
 *******************************************************************************
 */
WORD32 ih264e_input_queue_update(codec_t *ps_codec,
                                 ive_video_encode_ip_t *ps_ive_ip,
                                 inp_buf_t *ps_enc_buff);

/**
*******************************************************************************
*
* @brief
*  Used to get minimum level index for a given picture size
*
* @par Description:
*  Gets the minimum level index and then gets corresponding level.
*  Also used to ignore invalid levels like 2.3, 3.3 etc
*
* @param[in] wd
*  Width
*
* @param[in] ht
*  Height
*
* @returns  Level index for a given level
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_get_min_level(WORD32 wd, WORD32 ht);

/**
*******************************************************************************
*
* @brief
*  Used to get level index for a given level
*
* @par Description:
*  Converts from level_idc (which is multiplied by 30) to an index that can be
*  used as a lookup. Also used to ignore invalid levels like 2.2 , 3.2 etc
*
* @param[in] level
*  Level of the stream
*
* @returns  Level index for a given level
*
* @remarks
*
*******************************************************************************
*/
WORD32 ih264e_get_lvl_idx(WORD32 level);

/**
*******************************************************************************
*
* @brief returns maximum number of pictures allowed in dpb for a given level
*
* @par Description:
*  For given width, height and level, number of pictures allowed in decoder
*  picture buffer is computed as per Annex A.3.1
*
* @param[in] level
*  level of the bit-stream
*
* @param[in] pic_size
*  width * height
*
* @returns  Number of buffers in DPB
*
* @remarks
*  From annexure A.3.1 of H264 specification,
*  max_dec_frame_buffering <= MaxDpbSize, where MaxDpbSize is equal to
*  Min( 1024 * MaxDPB / ( PicWidthInMbs * FrameHeightInMbs * 384 ), 16 ) and
*  MaxDPB is given in Table A-1 in units of 1024 bytes. However the MaxDPB size
*  presented in the look up table gas_ih264_lvl_tbl is in units of 512
*  bytes. Hence the expression is modified accordingly.
*
*******************************************************************************
*/
WORD32 ih264e_get_dpb_size(WORD32 level, WORD32 pic_size);

/**
*******************************************************************************
*
* @brief
*  Used to get reference picture buffer size for a given level and
*  and padding used
*
* @par Description:
*  Used to get reference picture buffer size for a given level and padding used
*  Each picture is padded on all four sides
*
* @param[in] pic_size
*  Number of luma samples (Width * Height)
*
* @param[in] level
*  Level
*
* @param[in] horz_pad
*  Total padding used in horizontal direction
*
* @param[in] vert_pad
*  Total padding used in vertical direction
*
* @returns  Total picture buffer size
*
* @remarks
*
*
*******************************************************************************
*/
WORD32 ih264e_get_total_pic_buf_size(WORD32 pic_size, WORD32 level,
                                     WORD32 horz_pad, WORD32 vert_pad,
                                     WORD32 num_ref_frames,
                                     WORD32 num_reorder_frames);

/**
*******************************************************************************
*
* @brief Returns MV bank buffer size for a given number of luma samples
*
* @par Description:
*  For given number of luma samples  one MV bank size is computed.
*  Each MV bank includes pu_map and enc_pu_t for all the min PUs(4x4) in a picture
*
* @param[in] num_luma_samples
*  Max number of luma pixels in the frame
*
* @returns  Total MV Bank size
*
* @remarks
*
*
*******************************************************************************
*/
WORD32 ih264e_get_pic_mv_bank_size(WORD32 num_luma_samples);

/**
*******************************************************************************
*
* @brief
*  Function to initialize ps_pic_buf structs add pic buffers to
*  buffer manager in case of non-shared mode
*
* @par Description:
*  Function to initialize ps_pic_buf structs add pic buffers to
*  buffer manager in case of non-shared mode
*  To be called once per stream or for every reset
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @returns  error status
*
* @remarks
*
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_pic_buf_mgr_add_bufs(codec_t *ps_codec);

/**
*******************************************************************************
*
* @brief Function to add buffers to MV Bank buffer manager
*
* @par Description:
*  Function to add buffers to MV Bank buffer manager.  To be called once per
*  stream or for every reset
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @returns  error status
*
* @remarks
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_mv_buf_mgr_add_bufs(codec_t *ps_codec);

/**
*******************************************************************************
*
* @brief Function to initialize quant params structure
*
* @par Description:
*  The forward quantization modules depends on qp/6, qp mod 6, forward scale
*  matrix, forward threshold matrix, weight list. The inverse quantization
*  modules depends on qp/6, qp mod 6, inverse scale matrix, weight list.
*  These params are initialized in this function.
*
* @param[in] ps_proc
*  pointer to process context
*
* @param[in] qp
*  quantization parameter
*
* @returns none
*
* @remarks
*
*******************************************************************************
*/
void ih264e_init_quant_params(process_ctxt_t *ps_proc, int qp);

/**
*******************************************************************************
*
* @brief
*  Initialize AIR mb frame Map
*
* @par Description:
*  Initialize AIR mb frame map
*  MB frame map indicates which frame an Mb should be coded as intra according to AIR
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @returns  error_status
*
* @remarks
*
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_init_air_map(codec_t *ps_codec);

/**
*******************************************************************************
*
* @brief
*  Codec level initializations
*
* @par Description:
*  Initializes the codec with parameters that needs to be set before encoding
*  first frame
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @param[in] ps_inp_buf
*  Pointer to input buffer context
*
* @returns  error_status
*
* @remarks
*
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_codec_init(codec_t *ps_codec);

/**
*******************************************************************************
*
* @brief
*  Picture level initializations
*
* @par Description:
*  Before beginning to encode the frame, the current function initializes all
*  the ctxts (proc, entropy, me, ...) basing on the input configured params.
*  It locates space for storing recon in the encoder picture buffer set, fetches
*  reference frame from encoder picture buffer set. Calls RC pre-enc to get
*  qp and pic type for the current frame. Queues proc jobs so that
*  the other threads can begin encoding. In brief, this function sets up the
*  tone for the entire encoder.
*
* @param[in] ps_codec
*  Pointer to codec context
*
* @param[in] ps_inp_buf
*  Pointer to input buffer context
*
* @returns  error_status
*
* @remarks
*
*
*******************************************************************************
*/
IH264E_ERROR_T ih264e_pic_init(codec_t *ps_codec, inp_buf_t *ps_inp_buf);

#endif /* IH264E_UTILS_H_ */
