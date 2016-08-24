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
*  iv2.h
*
* @brief
* This file contains all the necessary structure and  enumeration
* definitions needed for the Application  Program Interface(API) of the
* Ittiam Video codecs  This is version 2 of Ittiam Video API
*
* @author
* Ittiam
*
* @par List of Functions:
*
* @remarks
*  None
*
*******************************************************************************
*/

#ifndef _IV2_H_
#define _IV2_H_

/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/
#define IV_MAX_RAW_COMPONENTS 4

/*****************************************************************************/
/* Typedefs                                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Enums                                                                     */
/*****************************************************************************/


/** Function status */
typedef enum{
    IV_STATUS_NA                                = 0x7FFFFFFF,
    IV_SUCCESS                                  = 0x0,
    IV_FAIL                                     = 0x1,
}IV_STATUS_T;


/** Defines the types of memory */
typedef enum {
    IV_NA_MEM_TYPE                              = 0x7FFFFFFF,
    IV_EXTERNAL_CACHEABLE_PERSISTENT_MEM        = 0x0,
    IV_EXTERNAL_CACHEABLE_SCRATCH_MEM           = 0x1,
    IV_EXTERNAL_NONCACHEABLE_PERSISTENT_MEM     = 0x2,
    IV_EXTERNAL_NONCACHEABLE_SCRATCH_MEM        = 0x3,
    IV_INTERNAL_CACHEABLE_PERSISTENT_MEM        = 0x10,
    IV_INTERNAL_CACHEABLE_SCRATCH_MEM           = 0x11,
    IV_INTERNAL_NONCACHEABLE_PERSISTENT_MEM     = 0x12,
    IV_INTERNAL_NONCACHEABLE_SCRATCH_MEM        = 0x13,
}IV_MEM_TYPE_T;

/* The color formats used in video/image codecs */

typedef enum {
    IV_CHROMA_NA                            = 0x7FFFFFFF,
    IV_YUV_420P                             = 0x0,
    IV_YUV_420SP_UV                         = 0x1,
    IV_YUV_420SP_VU                         = 0x2,

    IV_YUV_422P                             = 0x10,
    IV_YUV_422IBE                           = 0x11,
    IV_YUV_422ILE                           = 0x12,

    IV_YUV_444P                             = 0x20,
    IV_YUV_411P                             = 0x21,

    IV_GRAY                                 = 0x30,

    IV_RGB_565                              = 0x31,
    IV_RGB_24                               = 0x32,
    IV_RGBA_8888                            = 0x33
}IV_COLOR_FORMAT_T;

/** Frame/Field coding types */
typedef enum {
    IV_NA_FRAME                             = 0x7FFFFFFF,
    IV_I_FRAME                              = 0x0,
    IV_P_FRAME                              = 0x1,
    IV_B_FRAME                              = 0x2,
    IV_IDR_FRAME                            = 0x3,
    IV_II_FRAME                             = 0x4,
    IV_IP_FRAME                             = 0x5,
    IV_IB_FRAME                             = 0x6,
    IV_PI_FRAME                             = 0x7,
    IV_PP_FRAME                             = 0x8,
    IV_PB_FRAME                             = 0x9,
    IV_BI_FRAME                             = 0xa,
    IV_BP_FRAME                             = 0xb,
    IV_BB_FRAME                             = 0xc,
    IV_MBAFF_I_FRAME                        = 0xd,
    IV_MBAFF_P_FRAME                        = 0xe,
    IV_MBAFF_B_FRAME                        = 0xf,
    IV_MBAFF_IDR_FRAME                      = 0x10,
    IV_NOT_CODED_FRAME                      = 0x11,
    IV_FRAMETYPE_DEFAULT                    = IV_I_FRAME
}IV_PICTURE_CODING_TYPE_T;

/** Field type */
typedef enum {
    IV_NA_FLD                               = 0x7FFFFFFF,
    IV_TOP_FLD                              = 0x0,
    IV_BOT_FLD                              = 0x1,
    IV_FLD_TYPE_DEFAULT                     = IV_TOP_FLD
}IV_FLD_TYPE_T;

/** Video content type progressive/interlaced etc */
typedef enum {
    IV_CONTENTTYPE_NA                       = 0x7FFFFFFF,
    IV_PROGRESSIVE                          = 0x0,
    IV_INTERLACED                           = 0x1,
    IV_PROGRESSIVE_FRAME                    = 0x2,
    IV_INTERLACED_FRAME                     = 0x3,
    IV_INTERLACED_TOPFIELD                  = 0x4,
    IV_INTERLACED_BOTTOMFIELD               = 0x5,
    IV_CONTENTTYPE_DEFAULT                  = IV_PROGRESSIVE,
}IV_CONTENT_TYPE_T;

/** Profile */
typedef enum
{
    IV_PROFILE_NA                           = 0x7FFFFFFF,
    IV_PROFILE_BASE                         = 0x0,
    IV_PROFILE_MAIN                         = 0x1,
    IV_PROFILE_HIGH                         = 0x2,


    IV_PROFILE_SIMPLE                       = 0x100,
    IV_PROFILE_ADVSIMPLE                    = 0x101,
    IV_PROFILE_DEFAULT                      = IV_PROFILE_BASE,
}IV_PROFILE_T;


/** Architecture Enumeration                               */
typedef enum
{
    ARCH_NA                 =   0x7FFFFFFF,
    ARCH_ARM_NONEON         =   0x0,
    ARCH_ARM_A9Q,
    ARCH_ARM_A9A,
    ARCH_ARM_A9,
    ARCH_ARM_A7,
    ARCH_ARM_A5,
    ARCH_ARM_A15,
    ARCH_ARM_NEONINTR,
    ARCH_X86_GENERIC,
    ARCH_X86_SSSE3,
    ARCH_X86_SSE42,
    ARCH_ARM_A53,
    ARCH_ARM_A57,
    ARCH_ARM_V8_NEON
}IV_ARCH_T;

/** SOC Enumeration                               */
typedef enum
{
    SOC_NA                  = 0x7FFFFFFF,
    SOC_GENERIC             = 0x0,
    SOC_HISI_37X
}IV_SOC_T;


/** API command type */
typedef enum {
    IV_CMD_NA                           = 0x7FFFFFFF,
    IV_CMD_GET_NUM_MEM_REC              = 0x0,
    IV_CMD_FILL_NUM_MEM_REC             = 0x1,
    IV_CMD_RETRIEVE_MEMREC              = 0x2,
    IV_CMD_INIT                         = 0x3,
    /* Do not add anything after the following entry */
    IV_CMD_EXTENSIONS                   = 0x100
}IV_API_COMMAND_TYPE_T;

/*****************************************************************************/
/* Structure Definitions                                                     */
/*****************************************************************************/

/** This structure defines the handle for the codec instance            */

typedef struct{
    /** size of the structure                                           */
    UWORD32                                     u4_size;
    /** Pointer to the API function pointer table of the codec          */
    void                                        *pv_fxns;
    /** Pointer to the handle of the codec                              */
    void                                        *pv_codec_handle;
}iv_obj_t;

/** This structure defines the memory record holder which will          *
 * be used by the codec to communicate its memory requirements to the   *
 * application through appropriate API functions                        */

typedef struct {
    /** size of the structure                                           */
    UWORD32                                     u4_size;
    /** Pointer to the memory allocated by the application              */
    void                                        *pv_base;
    /** u4_size of the memory to be allocated                           */
    UWORD32                                     u4_mem_size;
    /** Alignment of the memory pointer                                 */
    UWORD32                                     u4_mem_alignment;
    /** Type of the memory to be allocated                              */
    IV_MEM_TYPE_T                               e_mem_type;
}iv_mem_rec_t;

/** This structure defines attributes for the raw buffer                */
typedef struct {
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Color format                                                    */
    IV_COLOR_FORMAT_T                           e_color_fmt;

    /** Pointer to each component                                       */
    void                                        *apv_bufs[IV_MAX_RAW_COMPONENTS];

    /** Width of each component                                         */
    UWORD32                                     au4_wd[IV_MAX_RAW_COMPONENTS];

    /** Height of each component                                        */
    UWORD32                                     au4_ht[IV_MAX_RAW_COMPONENTS];

    /** Stride of each component                                        */
    UWORD32                                     au4_strd[IV_MAX_RAW_COMPONENTS];

}iv_raw_buf_t;

/** This structure defines attributes for the bitstream buffer                */
typedef struct
{
    /** size of the structure                                           */
    UWORD32                                     u4_size;

    /** Pointer to buffer                                               */
    void                                        *pv_buf;

    /** Number of valid bytes in the buffer                             */
    UWORD32                                     u4_bytes;

    /** Allocated size of the buffer                                    */
    UWORD32                                     u4_bufsize;

}iv_bits_buf_t;
/*****************************************************************************/
/*  Get Number of Memory Records                                             */
/*****************************************************************************/

/** Input structure : Get number of memory records                     */
typedef struct {
    /** size of the structure                                          */
    UWORD32                                     u4_size;

    /** Command type                                                   */
    IV_API_COMMAND_TYPE_T                       e_cmd;
}iv_num_mem_rec_ip_t;

/** Output structure : Get number of memory records                    */
typedef struct{
    /** size of the structure                                          */
    UWORD32                                     u4_size;

    /** Return error code                                              */
    UWORD32                                     u4_error_code;

    /** Number of memory records that will be used by the codec        */
    UWORD32                                     u4_num_mem_rec;
}iv_num_mem_rec_op_t;


/*****************************************************************************/
/*  Fill Memory Records                                                      */
/*****************************************************************************/

/** Input structure : Fill memory records                              */

typedef struct {
    /** size of the structure                                          */
    UWORD32                                     u4_size;

    /** Command type                                                   */
    IV_API_COMMAND_TYPE_T                       e_cmd;

    /** Number of memory records                                       */
    UWORD32                                     u4_num_mem_rec;

    /** pointer to array of memrecords structures should be filled by codec
    with details of memory resource requirements */
    iv_mem_rec_t                                *ps_mem_rec;

    /** maximum width for which codec should request memory requirements */
    UWORD32                                     u4_max_wd;

    /** maximum height for which codec should request memory requirements*/
    UWORD32                                     u4_max_ht;

    /** Maximum number of reference frames                               */
    UWORD32                                     u4_max_ref_cnt;

    /** Maximum number of reorder frames                                 */
    UWORD32                                     u4_max_reorder_cnt;

    /** Maximum level supported                                          */
    UWORD32                                     u4_max_level;

    /** Color format that codec supports for input/output                */
    IV_COLOR_FORMAT_T                           e_color_format;

    /** Maximum search range to be used in X direction                      */
    UWORD32                                     u4_max_srch_rng_x;

    /** Maximum search range to be used in Y direction                      */
    UWORD32                                     u4_max_srch_rng_y;

}iv_fill_mem_rec_ip_t;


/** Output structure : Fill memory records                               */
typedef struct{
    /** size of the structure                                            */
    UWORD32                                     u4_size;

    /** Return error code                                                */
    UWORD32                                     u4_error_code;

    /** no of memory record structures which are filled by codec         */
    UWORD32                                     u4_num_mem_rec;
}iv_fill_mem_rec_op_t;


/*****************************************************************************/
/*  Retrieve Memory Records                                                  */
/*****************************************************************************/

/** Input structure : Retrieve memory records                                */

typedef struct {
    /** size of the structure                                          */
    UWORD32                                     u4_size;

    /** Command type                                                   */
    IV_API_COMMAND_TYPE_T                       e_cmd;

    /** array of structures where codec should fill with all memory  requested earlier */
    iv_mem_rec_t                                *ps_mem_rec;
}iv_retrieve_mem_rec_ip_t;


typedef struct{
    /** size of the structure                                            */
    UWORD32                                     u4_size;

    /** Return error code                                                */
    UWORD32                                     u4_error_code;

    /** no of memory record structures which are filled by codec         */
    UWORD32                                     u4_num_mem_rec_filled;
}iv_retrieve_mem_rec_op_t;

#endif /* _IV2_H_ */

