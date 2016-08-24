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
/*  File Name         : impeg2d_bitstream.h                                       */
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
/*         10 01 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
#ifndef __IMPEG2D_BITSTREAM_H__
#define __IMPEG2D_BITSTREAM_H__



/* Structure for the stream */
typedef struct _stream_t
{
    void    *pv_bs_buf;               /* Pointer to buffer containing the
                                        bitstream                    */

    UWORD32  *pu4_buf_aligned;         /* Pointer to the buffer after alignment correction,
                                         It points to the currently usable buffer */

    UWORD32  u4_offset;                  /* Offset in the buffer for the current bit */

    UWORD32  u4_buf;                  /* Buffer storing the current word */

    UWORD32  u4_buf_nxt;              /* Buffer storing the next Word */

    UWORD32  u4_max_offset;            /* Max Bit stream buffer offset in bytes for error checks */
} stream_t;

#define GET_MARKER_BIT(dec,stream)                                             \
{                                                                              \
    if (impeg2d_bit_stream_get(stream,1) != 0x1) {                             \
    /* No need to return error if marker is not present. */                    \
    }                                                                          \
}

/* Define A macro for inlining of FlushBits */
#define     FLUSH_BITS(u4_offset,u4_buf,u4_buf_nxt,u4_no_bits,pu4_buf_aligned) \
{                                                                              \
        UWORD32     u4_temp;                                                   \
                                                                               \
        if (((u4_offset & 0x1f) + u4_no_bits)>= 32)                            \
        {                                                                      \
            u4_buf              = u4_buf_nxt;                                  \
                                                                               \
            u4_temp             = *(pu4_buf_aligned)++;                        \
                                                                               \
            CONV_LE_TO_BE(u4_buf_nxt,u4_temp)                                  \
        }                                                                      \
        u4_offset               += u4_no_bits;                                 \
}

/* Macro to initialize the variables from stream */
#define GET_TEMP_STREAM_DATA(u4_buf,u4_buf_nxt,u4_offset,pu4_buf_aligned,stream)    \
{                                                                                   \
    u4_buf = stream->u4_buf;                                                        \
    u4_buf_nxt = stream->u4_buf_nxt;                                                \
    u4_offset = stream->u4_offset;                                                     \
    pu4_buf_aligned = stream->pu4_buf_aligned;                                      \
}

/* Macro to put the stream variable values back */
#define PUT_TEMP_STREAM_DATA(u4_buf,u4_buf_nxt,u4_offset,pu4_buf_aligned,stream)    \
{                                                                                   \
    stream->u4_buf = u4_buf;                                                        \
    stream->u4_buf_nxt = u4_buf_nxt;                                                \
    stream->u4_offset = u4_offset;                                                     \
    stream->pu4_buf_aligned = pu4_buf_aligned;                                      \
}

/* Macro to implement the get bits inline (ibits_nxt_inline) */
#define IBITS_NXT(u4_buf, u4_buf_nxt, u4_offset, u4_bits, no_of_bits)              \
{                                                                                   \
    UWORD8 u4_bit_ptr;                                                              \
    UWORD32 u4_temp;                                                                \
                                                                                    \
    u4_bit_ptr  = u4_offset & 0x1F;                                                 \
    u4_bits     = u4_buf << u4_bit_ptr;                                             \
                                                                                    \
    u4_bit_ptr  += no_of_bits;                                                      \
                                                                                    \
    if(32 < u4_bit_ptr)                                                             \
    {                                                                               \
        /*  Read bits from the next word if necessary */                            \
        u4_temp     = u4_buf_nxt;                                           \
        u4_bit_ptr  &= (BITS_IN_INT - 1);                                           \
                                                                                    \
        u4_temp     = (u4_temp >> (BITS_IN_INT - u4_bit_ptr));                      \
                                                                                    \
    /* u4_temp consists of bits,if any that had to be read from the next word*/     \
    /* of the buffer.The bits read from both the words are concatenated and*/       \
    /* moved to the least significant positions of 'u4_bits'*/                      \
            u4_bits = (u4_bits >> (32 - no_of_bits)) | u4_temp;                     \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            u4_bits = (u4_bits >> (32 - no_of_bits));                               \
        }                                                                           \
}

/* Macro to implement the get bits inline (ibits_get_inline) */
#define IBITS_GET(u4_buf,u4_buf_nxt,u4_offset,u4_bits,pu4_buf_aligned,no_of_bits)   \
{                                                                                   \
    IBITS_NXT(u4_buf, u4_buf_nxt, u4_offset, u4_bits, no_of_bits)                   \
    FLUSH_BITS(u4_offset,u4_buf,u4_buf_nxt,no_of_bits,pu4_buf_aligned)              \
}

void impeg2d_bit_stream_init(stream_t *stream,
                             UWORD8 *byteBuf,
                             UWORD32 u4_max_offset);
INLINE UWORD8 impeg2d_bit_stream_get_bit(stream_t *stream);
INLINE void impeg2d_bit_stream_flush(void* ctxt, UWORD32 NoOfBits);
INLINE void impeg2d_bit_stream_flush_to_byte_boundary(void* ctxt);
INLINE UWORD32 impeg2d_bit_stream_nxt(stream_t *stream, WORD32 NoOfBits);

INLINE UWORD32 impeg2d_bit_stream_get(void* ctxt, UWORD32 numBits);
INLINE UWORD32 impeg2d_bit_stream_num_bits_read(void* ctxt);







#endif /* __IMPEG2D_BITSTREAM_H__ */
