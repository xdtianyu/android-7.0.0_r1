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

#ifndef _VBR_STORAGE_VBV_H_
#define _VBR_STORAGE_VBV_H_
/******************************************************************************
VBR STORAGE (VBV):
Max. buffer filling rate: Rmax
Max. buffer size: Bmax (as specified by level and profile)
Current Buffer Level: Bcur
Frame Rate: F

For a storage scenario, the initial buffer size is assumed to be max. For every
frame the Maximum bits filled in to the buffer is given by Rmaxfrm = Rmax/F. If
the buffer overflows then the buffer is thresholded to the max buffer size.

               (overflow)
   B(0)            /|
---|--------------/-|------------------------------ Bmax
   |             /  |
   |          /|/   |
   |  /|     /      |
   | / |  /|/       |
   |/  | /          | /|
       |/           |/ |
                       |
                       |
-----------------------|---------------------------
   |<->|               |
(1/F)=>1/frame_rate (underflow)


   B"(i) - Bits in buffer just before decoding a frame.
   B'(i) - Bits in buffer just after decoding a frame.


   B(0) (initBuffer size) = Bmax.
   B'(i) = B"(i) - bits_decoded
   B"(i) = Min( Bmax, B'(i-1) + Rmaxfrm)

Overflow Scenario: In VBR case, since we have only a max filling rate (or input bit rate)
buffer overflow is not a issue (since the buffer filling rate can be reduced to any value
below this rate)

Underflow Scenario: B'(i) should always be > 0. If not then, the buffer underflows. To
prevent this condition the number bits that needs to be decoded must be equal to B"(i)
which is equal to Min( Bmax, B'(i-1) + Rmaxfrm)
****************************************************************************************/

typedef struct vbr_storage_vbv_t* vbr_storage_vbv_handle;

WORD32 irc_vbr_vbv_num_fill_use_free_memtab(vbr_storage_vbv_handle *pps_vbr_storage_vbv,
                                            itt_memtab_t *ps_memtab,
                                            ITT_FUNC_TYPE_E e_func_type);

/* Initalises the vbv buffer status */
void irc_init_vbr_vbv(vbr_storage_vbv_handle ps_vbr_storage_vbv,
                      WORD32 max_bit_rate, /* In bits/sec*/
                      WORD32 max_frm_rate, /* In frames/1000 sec*/
                      WORD32 i4_max_vbv_buff_size); /* in bits*/

/* Updates the buffer after decoding a frame */
void irc_update_vbr_vbv(vbr_storage_vbv_handle ps_vbr_storage_vbv,
                        WORD32 i4_total_bits_decoded);

/* gets the max_number of bits that can be decoded out of the VBV without underflow */
WORD32 irc_get_max_target_bits(vbr_storage_vbv_handle ps_vbr_storage_vbv);

WORD32 irc_get_max_bits_inflow_per_frm_periode(vbr_storage_vbv_handle ps_vbr_storage_vbv);

WORD32 irc_get_max_bits_per_tgt_frm(vbr_storage_vbv_handle ps_vbr_storage_vbv);

WORD32 irc_get_cur_vbv_buf_size(vbr_storage_vbv_handle ps_vbr_storage_vbv);

/* Queries the VBV buffer for the buffer status */
vbv_buf_status_e irc_get_vbv_buffer_status(vbr_storage_vbv_handle ps_vbr_storage_vbv,
                                           WORD32 i4_total_frame_bits,
                                           WORD32 *pi4_num_bits_to_prevent_vbv_underflow);

UWORD8 irc_restrict_swing_dvd_comp(vbr_storage_vbv_handle ps_vbr_storage_vbv);

WORD32 irc_get_max_vbv_buf_size(vbr_storage_vbv_handle ps_vbr_storage_vbv);

WORD32 irc_vbv_get_vbv_buf_fullness(vbr_storage_vbv_handle ps_vbr_storage_vbv,
                                    UWORD32 u4_bits);

WORD32 irc_get_max_tgt_bits_dvd_comp(vbr_storage_vbv_handle ps_vbr_storage_vbv,
                                     WORD32 i4_rem_bits_in_gop,
                                     WORD32 i4_rem_frms_in_gop,
                                     picture_type_e e_pic_type);

/* Changing input values at run time */
void irc_change_vbr_vbv_bit_rate(vbr_storage_vbv_handle ps_vbr_storage_vbv,
                                 WORD32 i4_max_bit_rate);

void irc_change_vbr_vbv_frame_rate(vbr_storage_vbv_handle ps_vbr_storage_vbv,
                                   WORD32 i4_frm_rate);

void irc_change_vbr_max_bits_per_tgt_frm(vbr_storage_vbv_handle ps_vbr_storage_vbv,
                                         WORD32 i4_tgt_frm_rate);
#endif

