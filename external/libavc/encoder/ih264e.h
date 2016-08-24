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
/*  File Name         : ih264e.h                                    */
/*                                                                           */
/*  Description       : This file contains all the necessary structure and   */
/*                      enumeration definitions needed for the Application   */
/*                      Program Interface(API) of the Ittiam MPEG4        */
/*                      Encoder on Cortex A8 - Neon platform                 */
/*                                                                           */
/*  List of Functions : ih264e_api_function                              */
/*                                                                           */
/*  Issues / Problems : None                                                 */
/*                                                                           */
/*  Revision History  :                                                      */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         26 08 2010   100239(RCY)     Draft                                */
/*                                                                           */
/*****************************************************************************/

#ifndef _IH264E_H_
#define _IH264E_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "iv2.h"
#include "ive2.h"
/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/


/*****************************************************************************/
/* API Function Prototype                                                    */
/*****************************************************************************/
IV_STATUS_T ih264e_api_function(iv_obj_t *ps_handle, void *pv_api_ip,void *pv_api_op);

/*****************************************************************************/
/* Enums                                                                     */
/*****************************************************************************/
typedef enum
{
    IH264E_CMD_CTL_SET_ME_INFO_ENABLE,
}IH264E_CMD_CTL_SUB_CMDS;


/*****************************************************************************/
/* Extended Structures                                                       */
/*****************************************************************************/

/*****************************************************************************/
/*  Get Number of Memory Records                                             */
/*****************************************************************************/


typedef struct
{
    iv_num_mem_rec_ip_t                    s_ive_ip;
}ih264e_num_mem_rec_ip_t;


typedef struct
{
    iv_num_mem_rec_op_t                    s_ive_op;
}ih264e_num_mem_rec_op_t;


/*****************************************************************************/
/*  Fill Memory Records                                                      */
/*****************************************************************************/


typedef struct
{
    iv_fill_mem_rec_ip_t                   s_ive_ip;
}ih264e_fill_mem_rec_ip_t;


typedef struct
{
    iv_fill_mem_rec_op_t                   s_ive_op;
}ih264e_fill_mem_rec_op_t;

/*****************************************************************************/
/*  Retrieve Memory Records                                                  */
/*****************************************************************************/


typedef struct
{
    iv_retrieve_mem_rec_ip_t               s_ive_ip;
}ih264e_retrieve_mem_rec_ip_t;


typedef struct
{
    iv_retrieve_mem_rec_op_t               s_ive_op;
}ih264e_retrieve_mem_rec_op_t;


/*****************************************************************************/
/*   Initialize encoder                                                      */
/*****************************************************************************/

typedef struct
{
    ive_init_ip_t                           s_ive_ip;
}ih264e_init_ip_t;


typedef struct
{
    ive_init_op_t                           s_ive_op;
}ih264e_init_op_t;


/*****************************************************************************/
/*   Queue Input raw buffer - Send the YUV buffer to be encoded              */
/*****************************************************************************/
typedef struct
{
    ive_queue_inp_ip_t                      s_ive_ip;
}ih264e_queue_inp_ip_t;

typedef struct
{
    ive_queue_inp_op_t                      s_ive_op;
}ih264e_queue_inp_op_t;

/*****************************************************************************/
/*   Dequeue Input raw buffer - Get free YUV buffer from the encoder         */
/*****************************************************************************/
typedef struct
{
    ive_dequeue_inp_ip_t                      s_ive_ip;
}ih264e_dequeue_inp_ip_t;

typedef struct
{
    ive_dequeue_inp_op_t                      s_ive_op;
}ih264e_dequeue_inp_op_t;


/*****************************************************************************/
/*   Queue Output bitstream buffer - Send the bistream buffer to be filled   */
/*****************************************************************************/
typedef struct
{
    ive_queue_out_ip_t                      s_ive_ip;
}ih264e_queue_out_ip_t;

typedef struct
{
    ive_queue_out_op_t                      s_ive_op;
}ih264e_queue_out_op_t;

/*****************************************************************************/
/* Dequeue Output bitstream buffer - Get the bistream buffer filled          */
/*****************************************************************************/
typedef struct
{
    ive_dequeue_out_ip_t                      s_ive_ip;
}ih264e_dequeue_out_ip_t;

typedef struct
{
    ive_dequeue_out_op_t                      s_ive_op;
}ih264e_dequeue_out_op_t;


/*****************************************************************************/
/* Get Recon data - Get the reconstructed data from encoder                  */
/*****************************************************************************/
typedef struct
{
    ive_get_recon_ip_t                        s_ive_ip;
}ih264e_get_recon_ip_t;

typedef struct
{
    ive_get_recon_op_t                        s_ive_op;
}ih264e_get_recon_op_t;
/*****************************************************************************/
/*   Video control  Flush                                                    */
/*****************************************************************************/


typedef struct
{
    ive_ctl_flush_ip_t                      s_ive_ip;
}ih264e_ctl_flush_ip_t;


typedef struct
{
    ive_ctl_flush_op_t                      s_ive_op;
}ih264e_ctl_flush_op_t;

/*****************************************************************************/
/*   Video control reset                                                     */
/*****************************************************************************/


typedef struct
{
    ive_ctl_reset_ip_t                      s_ive_ip;
}ih264e_ctl_reset_ip_t;


typedef struct
{
    ive_ctl_reset_op_t                      s_ive_op;
}ih264e_ctl_reset_op_t;


/*****************************************************************************/
/*   Video control:Get Buf Info                                              */
/*****************************************************************************/


typedef struct
{
    ive_ctl_getbufinfo_ip_t             s_ive_ip;
}ih264e_ctl_getbufinfo_ip_t;



typedef struct
{
    ive_ctl_getbufinfo_op_t             s_ive_op;
}ih264e_ctl_getbufinfo_op_t;



/*****************************************************************************/
/*   Video control:Get Version Info                                          */
/*****************************************************************************/


typedef struct
{
    ive_ctl_getversioninfo_ip_t         s_ive_ip;
}ih264e_ctl_getversioninfo_ip_t;



typedef struct
{
    ive_ctl_getversioninfo_op_t         s_ive_op;
}ih264e_ctl_getversioninfo_op_t;

/*****************************************************************************/
/*   Video control:Set default params                                       */
/*****************************************************************************/


typedef struct
{
    ive_ctl_setdefault_ip_t         s_ive_ip;
}ih264e_ctl_setdefault_ip_t;



typedef struct
{
    ive_ctl_setdefault_op_t         s_ive_op;
}ih264e_ctl_setdefault_op_t;

/*****************************************************************************/
/*   Video control  Set IPE params                                           */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_ipe_params_ip_t     s_ive_ip;
}ih264e_ctl_set_ipe_params_ip_t;

typedef struct
{
    ive_ctl_set_ipe_params_op_t     s_ive_op;
}ih264e_ctl_set_ipe_params_op_t;

/*****************************************************************************/
/*   Video control  Set Frame dimensions                                     */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_dimensions_ip_t     s_ive_ip;
}ih264e_ctl_set_dimensions_ip_t;

typedef struct
{
    ive_ctl_set_dimensions_op_t     s_ive_op;
}ih264e_ctl_set_dimensions_op_t;

/*****************************************************************************/
/*   Video control  Set Frame rates                                          */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_frame_rate_ip_t     s_ive_ip;
}ih264e_ctl_set_frame_rate_ip_t;
typedef struct
{
    ive_ctl_set_frame_rate_op_t     s_ive_op;
}ih264e_ctl_set_frame_rate_op_t;


/*****************************************************************************/
/*   Video control  Set Bitrate                                              */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_bitrate_ip_t        s_ive_ip;
}ih264e_ctl_set_bitrate_ip_t;

typedef struct
{
    ive_ctl_set_bitrate_op_t        s_ive_op;
}ih264e_ctl_set_bitrate_op_t;


/*****************************************************************************/
/*   Video control  Set Frame type                                           */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_frame_type_ip_t     s_ive_ip;
}ih264e_ctl_set_frame_type_ip_t;

typedef struct
{
    ive_ctl_set_frame_type_op_t     s_ive_op;
}ih264e_ctl_set_frame_type_op_t;

/*****************************************************************************/
/*   Video control  Set Encode mode                                          */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_enc_mode_ip_t       s_ive_ip;
}ih264e_ctl_set_enc_mode_ip_t;

typedef struct
{
    ive_ctl_set_enc_mode_op_t       s_ive_op;
}ih264e_ctl_set_enc_mode_op_t;

/*****************************************************************************/
/*   Video control  Set QP                                                   */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_qp_ip_t             s_ive_ip;
}ih264e_ctl_set_qp_ip_t;

typedef struct
{
    ive_ctl_set_qp_op_t             s_ive_op;
}ih264e_ctl_set_qp_op_t;

/*****************************************************************************/
/*   Video control  Set AIR params                                           */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_air_params_ip_t     s_ive_ip;
}ih264e_ctl_set_air_params_ip_t;

typedef struct
{
    ive_ctl_set_air_params_op_t     s_ive_op;
}ih264e_ctl_set_air_params_op_t;

/*****************************************************************************/
/*   Video control  Set VBV params                                           */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_vbv_params_ip_t     s_ive_ip;
}ih264e_ctl_set_vbv_params_ip_t;

typedef struct
{
    ive_ctl_set_vbv_params_op_t     s_ive_op;
}ih264e_ctl_set_vbv_params_op_t;

/*****************************************************************************/
/*   Video control  Set Processor Details                                    */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_num_cores_ip_t      s_ive_ip;
}ih264e_ctl_set_num_cores_ip_t;

typedef struct
{
    ive_ctl_set_num_cores_op_t      s_ive_op;
}ih264e_ctl_set_num_cores_op_t;

/*****************************************************************************/
/*   Video control  Set Motion estimation params                             */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_me_params_ip_t      s_ive_ip;
}ih264e_ctl_set_me_params_ip_t;

typedef struct
{
    ive_ctl_set_me_params_op_t      s_ive_op;
}ih264e_ctl_set_me_params_op_t;

/*****************************************************************************/
/*   Video control  Set GOP params                                           */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_gop_params_ip_t     s_ive_ip;
}ih264e_ctl_set_gop_params_ip_t;

typedef struct
{
    ive_ctl_set_gop_params_op_t     s_ive_op;
}ih264e_ctl_set_gop_params_op_t;

/*****************************************************************************/
/*   Video control  Set Deblock params                                       */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_deblock_params_ip_t s_ive_ip;
}ih264e_ctl_set_deblock_params_ip_t;

typedef struct
{
    ive_ctl_set_deblock_params_op_t s_ive_op;
}ih264e_ctl_set_deblock_params_op_t;

/*****************************************************************************/
/*   Video control  Set Profile params                                       */
/*****************************************************************************/
typedef struct
{
    ive_ctl_set_profile_params_ip_t s_ive_ip;
}ih264e_ctl_set_profile_params_ip_t;

typedef struct
{
    ive_ctl_set_profile_params_op_t s_ive_op;
}ih264e_ctl_set_profile_params_op_t;

/*****************************************************************************/
/*   Synchronous video encode call                                           */
/*****************************************************************************/
typedef struct
{
    ive_video_encode_ip_t s_ive_ip;
}ih264e_video_encode_ip_t;

typedef struct
{
    ive_video_encode_op_t s_ive_op;
}ih264e_video_encode_op_t;


/* The enum values should not have greater than 8 bits as this is assigned to WORD8 */
typedef enum
{
    INTRA16x16 = 0,
    INTRA4x4,
    INTER16x16
}IV_MB_TYPE_T;

/*****************************************************************************/
/*   Pic info structures                                                     */
/*****************************************************************************/
typedef struct
{
    /** Qp  */
    UWORD32                                     u4_qp;

    /** Pic Type */
    IV_PICTURE_CODING_TYPE_T                    e_frame_type;

}ih264e_pic_info1_t;

typedef struct
{
    /** Qp  */
    UWORD32                                     u4_qp;

    /** Pic Type */
    IV_PICTURE_CODING_TYPE_T                    e_frame_type;

    /** Disable deblock level (0: Enable completely, 3: Disable completely */
    UWORD32                                     u4_disable_deblock_level;

}ih264e_pic_info2_t;


/*****************************************************************************/
/*   MB info structures                                                     */
/*****************************************************************************/
typedef struct
{
    /** MV X    */
    WORD16                                  i2_mv_x;

    /** MV Y    */
    WORD16                                  i2_mv_y;
}ih264e_mv_t;

typedef struct
{
    /** Intra / Inter    */
    WORD8                                       i1_mb_type;
    union
    {
        ih264e_mv_t                                 as_mv[1];

        /** Intra mode */
        WORD8                                       ai1_intra_mode[1];
    };
}ih264e_mb_info1_t;

typedef struct
{
    /** Intra / Inter    */
    WORD8                                       i1_mb_type;


    /** SAD     */
    UWORD16                                     u2_sad;

    union
    {
        ih264e_mv_t                                 as_mv[1];

        /** Intra mode */
        WORD8                                       ai1_intra_mode[1];
    };


}ih264e_mb_info2_t;

typedef struct
{
    /** Intra / Inter    */
    WORD8                                       i1_mb_type;

    union
    {
        ih264e_mv_t                                 as_mv[4];

        /** Intra mode */
        WORD8                                       ai1_intra_mode[16];
    };

}ih264e_mb_info3_t;

typedef struct
{
    /** Intra / Inter    */
    WORD8                                       i1_mb_type;

    /** Intra Mode      */
    WORD8                                       i1_intra_mode;

    /** SAD     */
    UWORD16                                     u2_sad;

    union
    {
        ih264e_mv_t                                 as_mv[16];

        /** Intra mode */
        WORD8                                       ai1_intra_mode[16];
    };



}ih264e_mb_info4_t;

/* Add any new structures to the following union. It is used to calculate the max size needed for allocation of memory */
typedef struct
{
    union
    {
        ih264e_mb_info1_t               s_mb_info1;
        ih264e_mb_info2_t               s_mb_info2;
        ih264e_mb_info3_t               s_mb_info3;
        ih264e_mb_info4_t               s_mb_info4;
    };
}ih264e_mb_info_t;

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
#endif /* _IH264E_H_ */
