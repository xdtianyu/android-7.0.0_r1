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

#ifndef FIXED_POINT_ERROR_BITS_H
#define FIXED_POINT_ERROR_BITS_H

typedef struct error_bits_t *error_bits_handle;

WORD32 irc_error_bits_num_fill_use_free_memtab(error_bits_handle *pps_error_bits,
                                               itt_memtab_t *ps_memtab,
                                               ITT_FUNC_TYPE_E e_func_type);

void irc_init_error_bits(error_bits_handle ps_error_bits,
                         WORD32 i4_max_tgt_frm_rate,
                         WORD32 i4_bitrate);

void irc_update_error_bits(error_bits_handle ps_error_bits);

WORD32 irc_get_error_bits(error_bits_handle ps_error_bits);

void irc_change_frm_rate_in_error_bits(error_bits_handle ps_error_bits,
                                       WORD32 i4_tgt_frm_rate);

void irc_change_bitrate_in_error_bits(error_bits_handle ps_error_bits,
                                      WORD32 i4_bitrate);

#endif

