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
/*  File Name         : irc_cbr_buffer_control.h                             */
/*                                                                           */
/*  Description       : This file contains all the necessary declarations    */
/*                      for cbr_buffer_control functions                     */
/*                                                                           */
/*                                                                           */
/*  List of Functions : <List the functions defined in this file>            */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         06 05 2008   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

#ifndef CBR_BUFFER_CONTROL_H
#define CBR_BUFFER_CONTROL_H

/* Macro for clipping a number between to extremes */
#define CLIP(Number,Max,Min)    if((Number) > (Max)) (Number) = (Max); \
                                else if((Number) < (Min)) (Number) = (Min);
/*****************************************************************************/
/* Structure                                                                 */
/*****************************************************************************/
typedef struct cbr_buffer_t *cbr_buffer_handle;

WORD32 irc_cbr_buffer_num_fill_use_free_memtab(cbr_buffer_handle *pps_cbr_buffer,
                                               itt_memtab_t *ps_memtab,
                                               ITT_FUNC_TYPE_E e_func_type);

/* Initialize the cbr Buffer*/
void irc_init_cbr_buffer(cbr_buffer_handle ps_cbr_buffer,
                         WORD32 i4_buffer_delay,
                         WORD32 i4_tgt_frm_rate,
                         WORD32 *i4_bit_rate,
                         UWORD32 *u4_num_pics_in_delay_prd,
                         UWORD32 u4_vbv_buf_size);

/* Check for tgt bits with in CBR buffer*/
WORD32 irc_cbr_buffer_constraint_check(cbr_buffer_handle ps_cbr_buffer,
                                       WORD32 i4_tgt_bits,
                                       picture_type_e e_pic_type);

/* Get the buffer status with the current consumed bits*/
vbv_buf_status_e irc_get_cbr_buffer_status(cbr_buffer_handle ps_cbr_buffer,
                                           WORD32 i4_tot_consumed_bits,
                                           WORD32 *pi4_num_bits_to_prevent_overflow,
                                           picture_type_e e_pic_type);

/* Update the CBR buffer at the end of the VOP*/
void irc_update_cbr_buffer(cbr_buffer_handle ps_cbr_buffer,
                           WORD32 i4_tot_consumed_bits,
                           picture_type_e e_pic_type);

/*Get the bits needed to stuff in case of Underflow*/
WORD32 irc_get_cbr_bits_to_stuff(cbr_buffer_handle ps_cbr_buffer,
                                 WORD32 i4_tot_consumed_bits,
                                 picture_type_e e_pic_type);

WORD32 irc_get_cbr_buffer_delay(cbr_buffer_handle ps_cbr_buffer);

WORD32 irc_get_cbr_buffer_size(cbr_buffer_handle ps_cbr_buffer);

WORD32 irc_vbr_stream_buffer_constraint_check(cbr_buffer_handle ps_cbr_buffer,
                                              WORD32 i4_tgt_bits,
                                              picture_type_e e_pic_type);

void irc_change_cbr_vbv_bit_rate(cbr_buffer_handle ps_cbr_buffer,
                                 WORD32 *i4_bit_rate);

void irc_change_cbr_vbv_tgt_frame_rate(cbr_buffer_handle ps_cbr_buffer,
                                       WORD32 i4_tgt_frm_rate);

void irc_change_cbr_vbv_num_pics_in_delay_period(cbr_buffer_handle ps_cbr_buffer,
                                                 UWORD32 *u4_num_pics_in_delay_prd);

void irc_change_cbr_buffer_delay(cbr_buffer_handle ps_cbr_buffer,
                                 WORD32 i4_buffer_delay);
#endif /* CBR_BUFFER_CONTROL_H */

