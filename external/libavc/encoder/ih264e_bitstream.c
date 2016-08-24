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
******************************************************************************
* @file
*  ih264e_bitstream.c
*
* @brief
*  This file contains function definitions related to bitstream generation
*
* @author
*  ittiam
*
* @par List of Functions:
*  - ih264e_bitstrm_init()
*  - ih264e_put_bits()
*  - ih264e_put_bit()
*  - ih264e_put_rbsp_trailing_bits()
*  - ih264e_put_uev()
*  - ih264e_put_sev()
*  - ih264e_put_nal_start_code_prefix()
*
******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* System include files */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <math.h>

/* User include files */
#include "ih264e_config.h"
#include "ih264_typedefs.h"
#include "ih264_platform_macros.h"
#include "ih264_debug.h"
#include "ih264e_error.h"
#include "ih264e_bitstream.h"
#include "ih264_defs.h"
#include "ih264_macros.h"


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
******************************************************************************
*
*  @brief Initializes the encoder bitstream engine
*
*  @par   Description
*  This routine needs to be called at start of slice/frame encode
*
*  @param[in]   ps_bitstrm
*  pointer to bitstream context (handle)
*
*  @param[in]   p1_bitstrm_buf
*  bitstream buffer pointer where the encoded stream is generated in byte order
*
*  @param[in]   u4_max_bitstrm_size
*  indicates maximum bitstream buffer size. (in bytes)
*  If actual stream size exceeds the maximum size, encoder should
*   1. Not corrupt data beyond u4_max_bitstrm_size bytes
*   2. Report an error back to application indicating overflow
*
*  @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_bitstrm_init(bitstrm_t *ps_bitstrm,
                                   UWORD8 *pu1_bitstrm_buf,
                                   UWORD32 u4_max_bitstrm_size)
{
    ps_bitstrm->pu1_strm_buffer  = pu1_bitstrm_buf;
    ps_bitstrm->u4_max_strm_size = u4_max_bitstrm_size;

    /* Default init values for other members of bitstream context */
    ps_bitstrm->u4_strm_buf_offset  = 0;
    ps_bitstrm->u4_cur_word         = 0;
    ps_bitstrm->i4_bits_left_in_cw  = WORD_SIZE;
    ps_bitstrm->i4_zero_bytes_run   = 0;

    return(IH264E_SUCCESS);
}

/**
******************************************************************************
*
*  @brief puts a code with specified number of bits into the bitstream
*
*  @par   Description
*  inserts code_len number of bits from lsb of code_val into the
*  bitstream. updates context members like u4_cur_word, u4_strm_buf_offset and
*  i4_bits_left_in_cw. If the total words (u4_strm_buf_offset) exceeds max
*  available size (u4_max_strm_size), returns error without corrupting data
*  beyond it
*
*  @param[in]    ps_bitstrm
*  pointer to bitstream context (handle)
*
*  @param[in]    u4_code_val
*  code value that needs to be inserted in the stream.
*
*  @param[in]    code_len
*  indicates code length (in bits) of code_val that would be inserted in
*  bitstream buffer size. Range of length[1:WORD_SIZE]
*
*  @remarks     Assumptions: all bits from bit position code_len to msb of
*   code_val shall be zero
*
*  @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_put_bits(bitstrm_t *ps_bitstrm,
                               UWORD32 u4_code_val,
                               WORD32 code_len)
{
    UWORD32 u4_cur_word = ps_bitstrm->u4_cur_word;
    WORD32  bits_left_in_cw = ps_bitstrm->i4_bits_left_in_cw;


    /* check assumptions made in the module */
    ASSERT(code_len > 0 && code_len <= WORD_SIZE);

    if(code_len < WORD_SIZE)
        ASSERT((u4_code_val >> code_len) == 0);

    /* sanity check on the bitstream engine state */
    ASSERT(bits_left_in_cw > 0 && bits_left_in_cw <= WORD_SIZE);

    ASSERT(ps_bitstrm->i4_zero_bytes_run <= EPB_ZERO_BYTES);

    ASSERT(ps_bitstrm->pu1_strm_buffer != NULL);


    if(bits_left_in_cw > code_len)
    {
        /*******************************************************************/
        /* insert the code in local bitstream word and return              */
        /* code is inserted in position of bits left (post decrement)      */
        /*******************************************************************/
        bits_left_in_cw -= code_len;
        u4_cur_word     |= (u4_code_val << bits_left_in_cw);

        ps_bitstrm->u4_cur_word         = u4_cur_word;
        ps_bitstrm->i4_bits_left_in_cw  = bits_left_in_cw;

        return(IH264E_SUCCESS);
    }
    else
    {
        /********************************************************************/
        /* 1. insert partial code corresponding to bits left in cur word    */
        /* 2. flush all the bits of cur word to bitstream                   */
        /* 3. insert emulation prevention bytes while flushing the bits     */
        /* 4. insert remaining bits of code starting from msb of cur word   */
        /* 5. update bitsleft in current word and stream buffer offset      */
        /********************************************************************/
        UWORD32 u4_strm_buf_offset  = ps_bitstrm->u4_strm_buf_offset;

        UWORD32 u4_max_strm_size    = ps_bitstrm->u4_max_strm_size;

        WORD32  zero_run            = ps_bitstrm->i4_zero_bytes_run;

        UWORD8* pu1_strm_buf        = ps_bitstrm->pu1_strm_buffer;

        WORD32  i, rem_bits = (code_len - bits_left_in_cw);


        /*********************************************************************/
        /* Bitstream overflow check                                          */
        /* NOTE: corner case of epb bytes (max 2 for 32bit word) not handled */
        /*********************************************************************/
        if((u4_strm_buf_offset + (WORD_SIZE>>3)) >= u4_max_strm_size)
        {
            /* return without corrupting the buffer beyond its size */
            return(IH264E_BITSTREAM_BUFFER_OVERFLOW);
        }

        /* insert parital code corresponding to bits left in cur word */
        u4_cur_word |= u4_code_val >> rem_bits;

        for(i = WORD_SIZE; i > 0; i -= 8)
        {
            /* flush the bits in cur word byte by byte and copy to stream */
            UWORD8   u1_next_byte = (u4_cur_word >> (i-8)) & 0xFF;

            PUTBYTE_EPB(pu1_strm_buf, u4_strm_buf_offset, u1_next_byte, zero_run);
        }

        /* insert the remaining bits from code val into current word */
        u4_cur_word = rem_bits ? (u4_code_val << (WORD_SIZE - rem_bits)) : 0;

        /* update the state variables and return success */
        ps_bitstrm->u4_cur_word         = u4_cur_word;
        ps_bitstrm->i4_bits_left_in_cw  = WORD_SIZE - rem_bits;
        ps_bitstrm->i4_zero_bytes_run   = zero_run;
        ps_bitstrm->u4_strm_buf_offset  = u4_strm_buf_offset;
        return (IH264E_SUCCESS);
    }
}

/**
******************************************************************************
*
*  @brief inserts a 1-bit code into the bitstream
*
*  @par   Description
*  inserts 1bit lsb of code_val into the bitstream
*  updates context members like u4_cur_word, u4_strm_buf_offset and
*  i4_bits_left_in_cw. If the total words (u4_strm_buf_offset) exceeds max
*  available size (u4_max_strm_size), returns error without corrupting data
*  beyond it
*
*  @param[in]    ps_bitstrm
*  pointer to bitstream context (handle)
*
*  @param[in]    u4_code_val
*  code value that needs to be inserted in the stream.
*
*  @remarks     Assumptions: all bits from bit position 1 to msb of code_val
*  shall be zero
*
*  @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_put_bit(bitstrm_t *ps_bitstrm, UWORD32 u4_code_val)
{
    /* call the put bits function for 1 bit and return */
    return(ih264e_put_bits(ps_bitstrm, u4_code_val, 1));
}

/**
******************************************************************************
*
*  @brief inserts rbsp trailing bits at the end of stream buffer (NAL)
*
*  @par   Description
*  inserts rbsp trailing bits, updates context members like u4_cur_word and
*  i4_bits_left_in_cw and flushes the same in the bitstream buffer. If the
*  total words (u4_strm_buf_offset) exceeds max available size
*  (u4_max_strm_size), returns error without corrupting data beyond it
*
*  @param[in]    ps_bitstrm
*  pointer to bitstream context (handle)
*
*  @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_put_rbsp_trailing_bits(bitstrm_t *ps_bitstrm)
{
    WORD32 i;
    UWORD32 u4_cur_word = ps_bitstrm->u4_cur_word;
    WORD32  bits_left_in_cw = ps_bitstrm->i4_bits_left_in_cw;
    WORD32  bytes_left_in_cw = (bits_left_in_cw - 1) >> 3;

    UWORD32 u4_strm_buf_offset  = ps_bitstrm->u4_strm_buf_offset;
    UWORD32 u4_max_strm_size    = ps_bitstrm->u4_max_strm_size;
    WORD32  zero_run            = ps_bitstrm->i4_zero_bytes_run;
    UWORD8* pu1_strm_buf        = ps_bitstrm->pu1_strm_buffer;

    /*********************************************************************/
    /* Bitstream overflow check                                          */
    /* NOTE: corner case of epb bytes (max 2 for 32bit word) not handled */
    /*********************************************************************/
    if((u4_strm_buf_offset + (WORD_SIZE>>3) - bytes_left_in_cw) >=
        u4_max_strm_size)
    {
        /* return without corrupting the buffer beyond its size */
        return(IH264E_BITSTREAM_BUFFER_OVERFLOW);
    }

    /* insert a 1 at the end of current word and flush all the bits */
    u4_cur_word |= (1 << (bits_left_in_cw - 1));

    /* get the bits to be inserted in msbdb of the word */
    //u4_cur_word <<= (WORD_SIZE - bytes_left_in_cw + 1);

    for(i = WORD_SIZE; i > (bytes_left_in_cw*8); i -= 8)
    {
        /* flush the bits in cur word byte by byte  and copy to stream */
        UWORD8   u1_next_byte = (u4_cur_word >> (i-8)) & 0xFF;

        PUTBYTE_EPB(pu1_strm_buf, u4_strm_buf_offset, u1_next_byte, zero_run);
    }

    /* update the stream offset */
    ps_bitstrm->u4_strm_buf_offset  = u4_strm_buf_offset;

    /* Default init values for scratch variables of bitstream context */
    ps_bitstrm->u4_cur_word         = 0;
    ps_bitstrm->i4_bits_left_in_cw  = WORD_SIZE;
    ps_bitstrm->i4_zero_bytes_run   = 0;

    return (IH264E_SUCCESS);
}

/**
******************************************************************************
*
*  @brief puts exponential golomb code of a unsigned integer into bitstream
*
*  @par   Description
*  computes uev code for given syntax element and inserts the same into
*  bitstream by calling ih264e_put_bits() interface.
*
*  @param[in]    ps_bitstrm
*  pointer to bitstream context (handle)
*
*  @param[in]    u4_code_num
*  unsigned integer input whose golomb code is written in stream
*
*  @remarks     Assumptions: code value can be represented in less than 16bits
*
*  @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_put_uev(bitstrm_t *ps_bitstrm, UWORD32 u4_code_num)
{
    UWORD32 u4_bit_str, u4_range;
    IH264E_ERROR_T e_error;

    /* convert the codenum to exp-golomb bit code: Table 9-2 JCTVC-J1003_d7 */
    u4_bit_str = u4_code_num + 1;

    /* get range of the bit string and put using put_bits()                 */
    GETRANGE(u4_range, u4_bit_str);

    e_error = ih264e_put_bits(ps_bitstrm, u4_bit_str, (2 * u4_range - 1));

    return(e_error);
}

/**
******************************************************************************
*
*  @brief puts exponential golomb code of a signed integer into bitstream
*
*  @par   Description
*  computes sev code for given syntax element and inserts the same into
*  bitstream by calling ih264e_put_bits() interface.
*
*  @param[in]    ps_bitstrm
*  pointer to bitstream context (handle)
*
*  @param[in]    syntax_elem
*  signed integer input whose golomb code is written in stream
*
*  @remarks     Assumptions: code value can be represented in less than 16bits
*
*  @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_put_sev(bitstrm_t *ps_bitstrm, WORD32 syntax_elem)
{
    UWORD32 u4_code_num, u4_bit_str, u4_range;
    IH264E_ERROR_T e_error;

    /************************************************************************/
    /* convert the codenum to exp-golomb bit code for signed syntax element */
    /* See Table9-2 and Table 9-3 of standard JCTVC-J1003_d7                */
    /************************************************************************/
    if(syntax_elem <= 0)
    {
        /* codeNum for non-positive integer =  2*abs(x) : Table9-3  */
        u4_code_num = ((-syntax_elem) << 1);
    }
    else
    {
        /* codeNum for positive integer     =  2x-1     : Table9-3  */
        u4_code_num = (syntax_elem << 1) - 1;
    }

    /* convert the codenum to exp-golomb bit code: Table 9-2 JCTVC-J1003_d7 */
    u4_bit_str = u4_code_num + 1;

    /* get range of the bit string and put using put_bits()                 */
    GETRANGE(u4_range, u4_bit_str);

    e_error = ih264e_put_bits(ps_bitstrm, u4_bit_str, (2 * u4_range - 1));

    return(e_error);
}

/**
******************************************************************************
*
*  @brief insert NAL start code prefix (0x000001) into bitstream with an option
*  of inserting leading_zero_8bits (which makes startcode prefix as 0x00000001)
*
*  @par   Description
*  Although start code prefix could have been put by calling ih264e_put_bits(),
*  ih264e_put_nal_start_code_prefix() is specially added to make sure emulation
*  prevention insertion is not done for the NAL start code prefix which will
*  surely happen otherwise by calling ih264e_put_bits() interface.
*
*  @param[in]    ps_bitstrm
*  pointer to bitstream context (handle)
*
*  @param[in]    insert_leading_zero_8bits
*  flag indicating if one more zero bytes needs to prefixed before start code
*
*  @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_put_nal_start_code_prefix(bitstrm_t *ps_bitstrm,
                                                WORD32 insert_leading_zero_8bits)
{
    UWORD32 u4_strm_buf_offset  = ps_bitstrm->u4_strm_buf_offset;
    UWORD8* pu1_strm_buf        = ps_bitstrm->pu1_strm_buffer;

    /* Bitstream buffer overflow check assuming worst case of 4 bytes */
    if((u4_strm_buf_offset + 4) >= ps_bitstrm->u4_max_strm_size)
    {
        return(IH264E_BITSTREAM_BUFFER_OVERFLOW);
    }

    /* Insert leading zero 8 bits conditionally */
    if(insert_leading_zero_8bits)
    {
        pu1_strm_buf[u4_strm_buf_offset] = 0x00;
        u4_strm_buf_offset++;
    }

    /* Insert NAL start code prefix 0x00 00 01 */
    pu1_strm_buf[u4_strm_buf_offset] = 0x00;
    u4_strm_buf_offset++;

    pu1_strm_buf[u4_strm_buf_offset] = 0x00;
    u4_strm_buf_offset++;

    pu1_strm_buf[u4_strm_buf_offset] = 0x01;
    u4_strm_buf_offset++;

    /* update the stream offset */
    ps_bitstrm->u4_strm_buf_offset = u4_strm_buf_offset;

    return (IH264E_SUCCESS);
}

