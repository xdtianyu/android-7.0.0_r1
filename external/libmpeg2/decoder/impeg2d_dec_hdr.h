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
/*  File Name         : impeg2d_dec_hdr.h                                   */
/*                                                                           */
/*  Description       : This file contains all the necessary examples to     */
/*                      establish a consistent use of Ittiam C coding        */
/*                      standards (based on Indian Hill C Standards)         */
/*                                                                           */
/*  List of Functions : <List the functions defined in this file>            */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         10 10 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

#ifndef __IMPEG2D_DEC_HDR_H__
#define __IMPEG2D_DEC_HDR_H__

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/
IMPEG2D_ERROR_CODES_T impeg2d_process_video_header(dec_state_t *dec);

IMPEG2D_ERROR_CODES_T impeg2d_process_video_bit_stream(dec_state_t *dec);


#endif /* __IMPEG2D_DEC_HDR_H__ */

