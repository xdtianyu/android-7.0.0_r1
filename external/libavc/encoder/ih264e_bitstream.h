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
*  ih264e_bitstream.h
*
* @brief
*  This file contains encoder bitstream engine related structures and
*  interface prototypes
*
* @author
*  ittiam
*
* @remarks
*  none
*
*******************************************************************************
*/

#ifndef IH264E_BITSTREAM_H_
#define IH264E_BITSTREAM_H_

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief      defines the maximum number of bits in a bitstream word
******************************************************************************
 */
#define WORD_SIZE         32

/**
******************************************************************************
 *  @brief  The number of consecutive zero bytes for emulation prevention check
******************************************************************************
 */
#define EPB_ZERO_BYTES      2

/**
******************************************************************************
 *  @brief  Emulation prevention insertion byte
******************************************************************************
 */
#define EPB_BYTE            0x03


/**
******************************************************************************
 *  @brief  Stream buffer allocated per frame should be atleast MIN_STREAM_SIZE
******************************************************************************
 */
#define MIN_STREAM_SIZE            0x800


/*****************************************************************************/
/* Function Macros                                                           */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief   Macro to check if emulation prevention byte insertion is required
******************************************************************************
 */
#define INSERT_EPB(zero_run, next_byte)                                       \
    ((zero_run) == EPB_ZERO_BYTES) && (0 == ((next_byte) & 0xFC))

/**
******************************************************************************
 *  @brief   returns the bit position of a leading 1 (msb) in a code value
******************************************************************************
 */
#if !MSVC
#define GETRANGE(r,value)   \
{                           \
    r = 0;                  \
    if(0 == value)          \
        r = 1;              \
    else                    \
    {                       \
        r = 32-CLZ(value);  \
    }\
}
#else
#define GETRANGE(r,value)                 \
{                                         \
    unsigned long  msb_one_bit = 0;       \
    r = _BitScanReverse(&msb_one_bit, value) ? (UWORD32)(msb_one_bit + 1) : 1 ; \
}
#endif

/**
******************************************************************************
 *  @brief   returns bits required to code a value
******************************************************************************
 */
#define UE_LENGTH(bits,x)        \
{                                \
    UWORD32 r_bit;               \
    GETRANGE(r_bit,x+1)          \
    bits =(((r_bit - 1) << 1)+1);\
}                                \

/**
******************************************************************************
 *  @brief  Inserts 1 byte and Emulation Prevention Byte(if any) into bitstream
 *          Increments the stream offset and zero run correspondingly
******************************************************************************
 */
#define PUTBYTE_EPB(ptr,off,byte,zero_run)                      \
{                                                               \
    if( INSERT_EPB(zero_run, byte) )                            \
    {                                                           \
        ptr[off] = EPB_BYTE;                                    \
        off++;                                                  \
        zero_run = 0;                                           \
    }                                                           \
                                                                \
    ptr[off] = byte;                                            \
    off++;                                                      \
    zero_run = byte ? 0 : zero_run+1;                           \
}                                                               \

/**
******************************************************************************
 *  @brief  Ensures Byte alignment of the slice header
******************************************************************************
 */
#define BYTE_ALIGNMENT(ps_bitstrm) ih264e_put_rbsp_trailing_bits(ps_bitstrm)

/**
******************************************************************************
 *  @brief  Gets number of  bits coded
******************************************************************************
 */

#define GET_NUM_BITS(ps_bitstream) ((ps_bitstream->u4_strm_buf_offset << 3) \
                                    + 32 - ps_bitstream->i4_bits_left_in_cw);



/**
******************************************************************************
 *  @macro Align bitstream to byte - Remainig bits are filled with '1'
******************************************************************************
*/
#define BITSTREAM_BYTE_ALIGN(ps_bitstrm)                                    \
   if (ps_bitstrm->i4_bits_left_in_cw & 0x07)                               \
   {                                                                        \
       const WORD32 len = (WORD32)((ps_bitstrm->i4_bits_left_in_cw) & 0x07);\
       ih264e_put_bits(ps_bitstrm, (UWORD32)((1 << len) - 1), len);         \
   }


/**
******************************************************************************
* flush the bits in cur word byte by byte  and copy to stream                *
* (current word is assumed to be byte aligned)                               *
******************************************************************************
*/
#define  BITSTREAM_FLUSH(ps_bitstrm)                                           \
{                                                                              \
    WORD32 i;                                                                  \
    for (i = WORD_SIZE; i > ps_bitstrm->i4_bits_left_in_cw; i -= 8)            \
    {                                                                          \
       UWORD8 u1_next_byte = (ps_bitstrm->u4_cur_word >> (i - 8)) & 0xFF;      \
       PUTBYTE_EPB(ps_bitstrm->pu1_strm_buffer, ps_bitstrm->u4_strm_buf_offset,\
                   u1_next_byte, ps_bitstrm->i4_zero_bytes_run);               \
    }                                                                          \
    ps_bitstrm->u4_cur_word = 0;                                               \
    ps_bitstrm->i4_bits_left_in_cw = WORD_SIZE;                                \
}                                                                              \




/*****************************************************************************/
/* Structures                                                                */
/*****************************************************************************/

/**
******************************************************************************
 *  @brief      Bitstream context for encoder
******************************************************************************
 */
typedef struct bitstrm
{
    /** points to start of stream buffer.    */
    UWORD8  *pu1_strm_buffer;

    /**
     *  max bitstream size (in bytes).
     *  Encoded stream shall not exceed this size.
     */
    UWORD32 u4_max_strm_size;

    /**
     *  byte offset (w.r.t pu1_strm_buffer) where next byte would be written
     *  Bitstream engine makes sure it would not corrupt data beyond
     *  u4_max_strm_size bytes
                                 */
    UWORD32 u4_strm_buf_offset;

    /**
     *  current bitstream word; It is a scratch word containing max of
     *  WORD_SIZE bits. Will be copied to stream buffer when the word is
     *  full
                                 */
    UWORD32 u4_cur_word;

    /**
     *  signifies number of bits available in u4_cur_word
     *  bits from msb to i4_bits_left_in_cw of u4_cur_word have already been
     *  inserted next bits would be inserted from pos [i4_bits_left_in_cw-1]
     *  Range of this variable [1 : WORD_SIZE]
                                 */
    WORD32  i4_bits_left_in_cw;

    /**
     *  signifies the number of consecutive zero bytes propogated from previous
     *  word. It is used for emulation prevention byte insertion in the stream
                                 */
    WORD32  i4_zero_bytes_run;

} bitstrm_t;


/*****************************************************************************/
/* Extern Function Declarations                                              */
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
IH264E_ERROR_T    ih264e_bitstrm_init
        (
            bitstrm_t   *ps_bitstrm,
            UWORD8      *pu1_bitstrm_buf,
            UWORD32     u4_max_bitstrm_size
        );

/**
******************************************************************************
*
*  @brief puts a code with specified number of bits into the bitstream
*
*  @par   Description
*  inserts code_len number of bits from lsb of code_val into the
*  bitstream.  If the total bytes (u4_strm_buf_offset) exceeds max
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
*  bitstream buffer size.
*
*  @remarks     Assumptions: all bits from bit position code_len to msb of
*   code_val shall be zero
*
*  @return      success or failure error code
*
******************************************************************************
*/
IH264E_ERROR_T    ih264e_put_bits
        (
            bitstrm_t   *ps_bitstrm,
            UWORD32     u4_code_val,
            WORD32      code_len
        );

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
IH264E_ERROR_T    ih264e_put_bit
        (
            bitstrm_t   *ps_bitstrm,
            UWORD32     u4_code_val
        );

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
IH264E_ERROR_T    ih264e_put_rbsp_trailing_bits
        (
            bitstrm_t   *ps_bitstrm
        );

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
IH264E_ERROR_T    ih264e_put_uev
        (
            bitstrm_t   *ps_bitstrm,
            UWORD32     u4_code_num
        );

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
IH264E_ERROR_T    ih264e_put_sev
        (
            bitstrm_t   *ps_bitstrm,
            WORD32      syntax_elem
        );

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
IH264E_ERROR_T    ih264e_put_nal_start_code_prefix
        (
            bitstrm_t   *ps_bitstrm,
            WORD32      insert_leading_zero_8bits
        );

#endif /* IH264E_BITSTREAM_H_ */
