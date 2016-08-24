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
/*  File Name         : impeg2d_api.h                                 */
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

#ifndef __IMPEG2D_API_H__
#define __IMPEG2D_API_H__


/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/


#define DEBUG_PRINT printf




#define NUM_MEM_RECORDS                 4 * MAX_THREADS + NUM_INT_FRAME_BUFFERS + 5 + 2


#define SETBIT(a,i)   ((a) |= (1 << i))


/*********************/
/* Codec Versioning  */
/*********************/




/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

IV_API_CALL_STATUS_T impeg2d_api_num_mem_rec(void *pv_api_ip, void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_fill_mem_rec(void *pv_api_ip, void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_init(iv_obj_t *DECHDL,
                                      void *ps_ip,
                                      void *ps_op);

IV_API_CALL_STATUS_T impeg2d_api_set_display_frame(iv_obj_t *DECHDL,
                                          void *pv_api_ip,
                                          void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_rel_display_frame(iv_obj_t *DECHDL,
                                                   void *pv_api_ip,
                                                   void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_retrieve_mem_rec(iv_obj_t *DECHDL,
                                                        void *pv_api_ip,
                                                        void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_ctl(iv_obj_t *DECHDL,
                                       void *pv_api_ip,
                                       void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_get_version(iv_obj_t *DECHDL,
                                               void *pv_api_ip,
                                               void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_get_buf_info(iv_obj_t *DECHDL,
                                                    void *pv_api_ip,
                                                    void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_set_flush_mode(iv_obj_t *DECHDL,
                                                      void *pv_api_ip,
                                                      void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_set_default(iv_obj_t *DECHDL,
                                                   void *pv_api_ip,
                                                   void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_reset(iv_obj_t *DECHDL,
                                             void *pv_api_ip,
                                             void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_set_params(iv_obj_t *DECHDL,
                                                  void *pv_api_ip,
                                                  void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_get_status(iv_obj_t *DECHDL,
                                                  void *pv_api_ip,
                                                  void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_set_params(iv_obj_t *DECHDL,
                                                  void *pv_api_ip,
                                                  void *pv_api_op);

void impeg2d_fill_mem_rec(impeg2d_fill_mem_rec_ip_t *ps_ip,
                            impeg2d_fill_mem_rec_op_t *ps_op);

void impeg2d_dec_frm(void *dec,
                                impeg2d_video_decode_ip_t *ps_ip,
                                impeg2d_video_decode_op_t *ps_op);

void impeg2d_dec_hdr(void *dec,
                               impeg2d_video_decode_ip_t *ps_ip,
                               impeg2d_video_decode_op_t *ps_op);

IV_API_CALL_STATUS_T impeg2d_api_entity(iv_obj_t *DECHDL,
                                        void *pv_api_ip,
                                        void *pv_api_op);

IV_API_CALL_STATUS_T impeg2d_api_check_struct_sanity(iv_obj_t *ps_handle,
                                                     void *pv_api_ip,
                                                     void *pv_api_op);




#endif /* __IMPEG2D_API_H__ */

