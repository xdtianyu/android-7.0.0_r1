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
/* File Includes                                                             */
/*****************************************************************************/

/* System include files */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#ifndef IOS
#include <malloc.h>
#endif

#ifdef WINDOWS_TIMER
#include "windows.h"
#else
#include <sys/time.h>
#endif
/* User include files */
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264e.h"
#include "app.h"
#include "psnr.h"

/* Function declarations */
#ifndef MD5_DISABLE
void calc_md5_cksum(UWORD8 *pu1_inbuf,UWORD32 u4_stride,UWORD32 u4_width,UWORD32 u4_height,UWORD8 *pu1_cksum_p );
#else
#define calc_md5_cksum(a, b, c, d, e)
#endif

/*****************************************************************************/
/* Enums                                                                     */
/*****************************************************************************/
typedef enum
{
    INVALID,
    HELP,
    INPUT_FILE,
    OUTPUT_FILE,
    RECON_FILE,
    RECON_ENABLE,
    CHKSUM_ENABLE,
    CHKSUM_FILE,
    INPUT_CHROMA_FORMAT,
    RECON_CHROMA_FORMAT,
    MAX_WD,
    MAX_HT,
    WD,
    HT,
    MAX_LEVEL,
    ENC_SPEED,
    ME_SPEED,
    START_FRM,
    NUM_FRMS,
    MAX_FRAMERATE,
    SRC_FRAMERATE,
    TGT_FRAMERATE,
    RC,
    MAX_BITRATE,
    BITRATE,
    I_QP,
    P_QP,
    B_QP,
    I_QP_MAX,
    P_QP_MAX,
    B_QP_MAX,
    I_QP_MIN,
    P_QP_MIN,
    B_QP_MIN,
    ENTROPY,
    AIR,
    AIR_REFRESH_PERIOD,
    ARCH,
    SOC,
    NUMCORES,
    PRE_ENC_ME,
    PRE_ENC_IPE,
    HPEL,
    QPEL,
    SRCH_RNG_X,
    SRCH_RNG_Y,
    I_INTERVAL,
    IDR_INTERVAL,
    CONSTRAINED_INTRA_PRED,
    B_FRMS,
    NUM_B_FRMS,
    DISABLE_DBLK,
    PROFILE,
    FAST_SAD,
    ALT_REF,
    DISABLE_DEBLOCK_LEVEL,
    PSNR,
    SLICE_MODE,
    SLICE_PARAM,
    CONFIG,
    LOOPBACK,
    VBV_DELAY,
    VBV_SIZE,
    INTRA_4x4_ENABLE,
    MB_INFO_FILE,
    MB_INFO_TYPE,
    PIC_INFO_FILE,
    PIC_INFO_TYPE,
} ARGUMENT_T;

typedef struct
{
    CHAR        argument_shortname[8];
    CHAR        argument_name[128];
    ARGUMENT_T  argument;
    CHAR        description[512];
} argument_t;

static const argument_t argument_mapping[] =
        {
                { "--", "--help", HELP, "Print this help\n" },
                { "-i", "--input", INPUT_FILE, "Input file\n" },
                { "-o", "--output", OUTPUT_FILE, "Output file\n" },
                { "--", "--recon_enable", RECON_ENABLE, "Recon enable flag\n" },
                { "-r", "--recon", RECON_FILE, "Recon file \n" },
                { "--",  "--input_chroma_format",  INPUT_CHROMA_FORMAT,
                            "Input Chroma format Supported values YUV_420P, YUV_420SP_UV, YUV_420SP_VU\n" },
                { "--",  "--recon_chroma_format",  RECON_CHROMA_FORMAT,
                            "Recon Chroma format Supported values YUV_420P, YUV_420SP_UV, YUV_420SP_VU\n" },
                { "-w", "--width", WD, "Width of input  file\n" },
                { "-h", "--height", HT, "Height file\n" },
                { "--", "--start_frame", START_FRM,  "Starting frame number\n" },
                { "-f", "--num_frames", NUM_FRMS,  "Number of frames to be encoded\n" },
                { "--", "--rc", RC, "Rate control mode 0: Constant Qp, 1: Storage, 2: CBR non low delay, 3: CBR low delay \n" },
                { "--", "--max_framerate", MAX_FRAMERATE, "Maximum frame rate \n" },
                { "--", "--tgt_framerate", TGT_FRAMERATE, "Target frame rate \n" },
                { "--", "--src_framerate", SRC_FRAMERATE, "Source frame rate \n" },
                { "--", "--i_interval", I_INTERVAL,  "Intra frame interval \n" },
                { "--", "--idr_interval", IDR_INTERVAL,  "IDR frame interval \n" },
                { "--", "--constrained_intrapred", CONSTRAINED_INTRA_PRED,  "Constrained IntraPrediction Flag \n" },
                { "--", "--bframes", NUM_B_FRMS, "Maximum number of consecutive B frames \n" },
                { "--", "--speed", ENC_SPEED, "Encoder speed preset 0 (slowest) and 100 (fastest)\n" },
                { "--", "--me_speed", ME_SPEED, "Encoder speed preset 0 (slowest) and 100 (fastest)\n" },
                { "--", "--fast_sad", FAST_SAD, " Flag for faster sad execution\n" },
                { "--", "--alt_ref", ALT_REF , "Flag to enable alternate refernce frames"},
                { "--", "--hpel", HPEL, "Flag to enable/disable Quarter pel estimation \n" },
                { "--", "--qpel", QPEL, "Flag to enable/disable Quarter pel estimation \n" },
                { "--", "--disable_deblock_level",  DISABLE_DEBLOCK_LEVEL,
                        "Disable deblock level - 0 : Enables deblock completely, 1: enables for I and 8th frame , 2: Enables for I only, 3 : disables completely\n" },
                { "--", "--search_range_x", SRCH_RNG_X,     "Search range for X  \n" },
                { "--", "--search_range_y", SRCH_RNG_Y,     "Search range for Y \n" },
                { "--", "--psnr", PSNR, "Enable PSNR computation (Disable while benchmarking performance) \n" },
                { "--", "--pre_enc_me", PRE_ENC_ME, "Flag to enable/disable Pre Enc Motion Estimation\n" },
                { "--", "--pre_enc_ipe", PRE_ENC_IPE, "Flag to enable/disable Pre Enc Intra prediction Estimation\n" },
                { "-n", "--num_cores", NUMCORES, "Number of cores to be used\n" },
                { "--", "--adaptive_intra_refresh", AIR ,"Adaptive Intra Refresh enable/disable\n"},
                { "--", "--air_refresh_period", AIR_REFRESH_PERIOD,"adaptive intra refresh period\n"},
                { "--", "--slice", SLICE_MODE,  "Slice mode-  0 :No slice, 1: Bytes per slice, 2: MB/CTB per slice  \n" },
                { "--", "--slice_param",  SLICE_PARAM, "Slice param value based on slice mode. Slice mode of 1 implies number of bytes per slice, 2 implies number of MBs/CTBs, for 0 value is neglected \n" },
                { "--", "--max_wd",      MAX_WD,                "Maximum width (Default: 1920) \n" },
                { "--", "--max_ht",      MAX_HT,                "Maximum height (Default: 1088)\n" },
                { "--", "--max_level",   MAX_LEVEL,             "Maximum Level (Default: 50)\n" },
                { "--", "--arch", ARCH, "Set Architecture. Supported values  ARM_NONEON, ARM_A9Q, ARM_A7, ARM_A5, ARM_NEONINTR, X86_GENERIC, X86_SSSE3, X86_SSE4 \n" },
                { "--", "--soc", SOC, "Set SOC. Supported values  GENERIC, HISI_37X \n" },
                { "--", "--chksum",            CHKSUM_FILE,              "Save Check sum file for recon data\n" },
                { "--", "--chksum_enable",          CHKSUM_ENABLE,               "Recon MD5 Checksum file\n"},
                { "-c", "--config",      CONFIG,              "config file (Default: enc.cfg)\n" },
                { "--", "--loopback",      LOOPBACK,             "Enable encoding in a loop\n" },
                { "--", "--profile",      PROFILE,               "Profile mode: Supported values BASE, MAIN, HIGH\n" },
                { "--", "--max_bitrate",  MAX_BITRATE,           "Max bitrate\n"},
                { "--", "--bitrate",      BITRATE,               "Target bitrate\n"},
                { "--", "--qp_i",         I_QP,                  "QP for I frames\n"},
                { "--", "--qp_p",         P_QP,                  "QP for P frames\n"},
                { "--", "--qp_b",         B_QP,                  "QP for B frames\n"},
                { "--", "--qp_i_max",     I_QP_MAX,              "Max QP for I frames\n"},
                { "--", "--qp_p_max",     P_QP_MAX,              "Max QP for P frames\n"},
                { "--", "--qp_b_max",     B_QP_MAX,              "Max QP for B frames\n"},
                { "--", "--qp_i_min",     I_QP_MIN,              "Min QP for I frames\n"},
                { "--", "--qp_p_min",     P_QP_MIN,              "Min QP for P frames\n"},
                { "--", "--qp_b_min",     B_QP_MIN,              "Min QP for B frames\n"},
                { "--", "--entropy",      ENTROPY,              "Entropy coding mode(0: CAVLC or 1: CABAC)\n"},
                { "--", "--vbv_delay",    VBV_DELAY,             "VBV buffer delay\n"},
                { "--", "--vbv_size",     VBV_SIZE,              "VBV buffer size\n"},
                { "-i4", "--intra_4x4_enable", INTRA_4x4_ENABLE, "Intra 4x4 enable \n" },
                { "--", "--mb_info_file",     MB_INFO_FILE,              "MB info file\n"},
                { "--", "--mb_info_type",     MB_INFO_TYPE,              "MB info type\n"},
                { "--", "--pic_info_file",     PIC_INFO_FILE,              "Pic info file\n"},
                { "--", "--pic_info_type",     PIC_INFO_TYPE,              "Pic info type\n"},
        };



/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/



/*****************************************************************************/
/*  Function Definitions                                                     */
/*****************************************************************************/


#if(defined X86) && (defined X86_MINGW)
/*****************************************************************************/
/* Function to print library calls                                           */
/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*  Function Name : memalign                                                 */
/*                                                                           */
/*  Description   : Returns malloc data. Ideally should return aligned memory*/
/*                  support alignment will be added later                    */
/*                                                                           */
/*  Inputs        : alignment                                                */
/*                  size                                                     */
/*  Globals       :                                                          */
/*  Processing    :                                                          */
/*                                                                           */
/*  Outputs       :                                                          */
/*  Returns       :                                                          */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         07 09 2012   100189          Initial Version                      */
/*                                                                           */
/*****************************************************************************/

void * ih264a_aligned_malloc(WORD32 alignment, WORD32 size)
{
    return _aligned_malloc(size, alignment);
}

void ih264a_aligned_free(void *pv_buf)
{
    _aligned_free(pv_buf);
    return;
}

#elif IOS

void * ih264a_aligned_malloc(WORD32 alignment, WORD32 size)
{
    return malloc(size);
}

void ih264a_aligned_free(void *pv_buf)
{
    free(pv_buf);
    return;
}

#else

void * ih264a_aligned_malloc(WORD32 alignment, WORD32 size)
{
    return memalign(alignment, size);
}

void ih264a_aligned_free(void *pv_buf)
{
    free(pv_buf);
    return;
}

#endif

/*****************************************************************************/
/*                                                                           */
/*  Function Name : codec_exit                                               */
/*                                                                           */
/*  Description   : handles unrecoverable errors                             */
/*  Inputs        : Error message                                            */
/*  Globals       : None                                                     */
/*  Processing    : Prints error message to console and exits.               */
/*  Outputs       : Error message to the console                             */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         07 06 2006   Sankar          Creation                             */
/*                                                                           */
/*****************************************************************************/
void codec_exit(CHAR *pc_err_message)
{
    printf("%s\n", pc_err_message);
    exit(-1);
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : codec_exit                                               */
/*                                                                           */
/*  Description   : handles unrecoverable errors                             */
/*  Inputs        : Error message                                            */
/*  Globals       : None                                                     */
/*  Processing    : Prints error message to console and exits.               */
/*  Outputs       : Error mesage to the console                              */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         07 06 2006   Sankar          Creation                             */
/*                                                                           */
/*****************************************************************************/
IV_COLOR_FORMAT_T get_chroma_fmt(CHAR *value)
{
    IV_COLOR_FORMAT_T e_chroma_format;
    if((strcmp(value, "YUV_420P")) == 0)
        e_chroma_format = IV_YUV_420P;
    else if((strcmp(value, "YUV_422ILE")) == 0)
        e_chroma_format = IV_YUV_422ILE;
    else if((strcmp(value, "RGB_565")) == 0)
        e_chroma_format = IV_RGB_565;
    else if((strcmp(value, "RGBA_8888")) == 0)
        e_chroma_format = IV_RGBA_8888;
    else if((strcmp(value, "YUV_420SP_UV")) == 0)
        e_chroma_format = IV_YUV_420SP_UV;
    else if((strcmp(value, "YUV_420SP_VU")) == 0)
        e_chroma_format = IV_YUV_420SP_VU;
    else
    {
        printf("\nInvalid colour format setting it to IV_YUV_420P\n");
        e_chroma_format = IV_YUV_420P;
    }
    return e_chroma_format;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : codec_exit                                               */
/*                                                                           */
/*  Description   : handles unrecoverable errors                             */
/*  Inputs        : Error message                                            */
/*  Globals       : None                                                     */
/*  Processing    : Prints error message to console and exits.               */
/*  Outputs       : Error mesage to the console                              */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         07 06 2006   Sankar          Creation                             */
/*                                                                           */
/*****************************************************************************/
IVE_SPEED_CONFIG get_speed_preset(CHAR *value)
{
    IVE_SPEED_CONFIG e_enc_speed_preset;
    if((strcmp(value, "CONFIG")) == 0)
        e_enc_speed_preset = IVE_CONFIG;
    else if((strcmp(value, "SLOWEST")) == 0)
        e_enc_speed_preset = IVE_SLOWEST;
    else if((strcmp(value, "NORMAL")) == 0)
        e_enc_speed_preset = IVE_NORMAL;
    else if((strcmp(value, "FAST")) == 0)
          e_enc_speed_preset = IVE_FAST;
    else if((strcmp(value, "HIGH_SPEED")) == 0)
        e_enc_speed_preset = IVE_HIGH_SPEED;
    else if((strcmp(value, "FASTEST")) == 0)
        e_enc_speed_preset = IVE_FASTEST;
    else
    {
        printf("\nInvalid speed preset, setting it to IVE_FASTEST\n");
        e_enc_speed_preset = IVE_FASTEST;
    }
    return e_enc_speed_preset;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : print_usage                                              */
/*                                                                           */
/*  Description   : Prints argument format                                   */
/*                                                                           */
/*                                                                           */
/*  Inputs        :                                                          */
/*  Globals       :                                                          */
/*  Processing    : Prints argument format                                   */
/*                                                                           */
/*  Outputs       :                                                          */
/*  Returns       :                                                          */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         07 09 2012   100189          Initial Version                      */
/*                                                                           */
/*****************************************************************************/

void print_usage(void)
{
    WORD32 i = 0;
    WORD32 num_entries = sizeof(argument_mapping) / sizeof(argument_t);
    printf("\nUsage:\n");
    while(i < num_entries)
    {
        printf("%-32s\t %s", argument_mapping[i].argument_name,
               argument_mapping[i].description);
        i++;
    }
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : get_argument                                             */
/*                                                                           */
/*  Description   : Gets argument for a given string                         */
/*                                                                           */
/*                                                                           */
/*  Inputs        : name                                                     */
/*  Globals       :                                                          */
/*  Processing    : Searches the given string in the array and returns       */
/*                  appropriate argument ID                                  */
/*                                                                           */
/*  Outputs       : Argument ID                                              */
/*  Returns       : Argument ID                                              */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         07 09 2012   100189          Initial Version                      */
/*                                                                           */
/*****************************************************************************/
ARGUMENT_T get_argument(CHAR *name)
{
    WORD32 i = 0;
    WORD32 num_entries = sizeof(argument_mapping) / sizeof(argument_t);
    while(i < num_entries)
    {
        if((0 == strcmp(argument_mapping[i].argument_name, name))       ||
          ((0 == strcmp(argument_mapping[i].argument_shortname, name))  &&
           (0 != strcmp(argument_mapping[i].argument_shortname, "--"))))
        {
            return argument_mapping[i].argument;
        }
        i++;
    }
    return INVALID;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : get_argument                                             */
/*                                                                           */
/*  Description   : Gets argument for a given string                         */
/*                                                                           */
/*                                                                           */
/*  Inputs        : name                                                     */
/*  Globals       :                                                          */
/*  Processing    : Searches the given string in the array and returns       */
/*                  appropriate argument ID                                  */
/*                                                                           */
/*  Outputs       : Argument ID                                              */
/*  Returns       : Argument ID                                              */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         07 09 2012   100189          Initial Version                      */
/*                                                                           */
/*****************************************************************************/
void parse_argument(app_ctxt_t *ps_app_ctxt, CHAR *argument, CHAR *value)
{
    ARGUMENT_T arg;

    arg = get_argument(argument);
    switch(arg)
    {
      case HELP:
        print_usage();
        exit(-1);
        break;
      case SLICE_MODE:
        sscanf(value, "%d", &ps_app_ctxt->u4_slice_mode);
        break;
      case SLICE_PARAM:
        sscanf(value, "%d", &ps_app_ctxt->u4_slice_param);
        break;
      case INPUT_FILE:
        sscanf(value, "%s", ps_app_ctxt->ac_ip_fname);
        break;

      case OUTPUT_FILE:
        sscanf(value, "%s", ps_app_ctxt->ac_op_fname);
        break;

      case RECON_FILE:
        sscanf(value, "%s", ps_app_ctxt->ac_recon_fname);
        break;

      case RECON_ENABLE:
        sscanf(value, "%d", &ps_app_ctxt->u4_recon_enable);
        break;

      case CHKSUM_FILE:
          sscanf(value, "%s", ps_app_ctxt->ac_chksum_fname);
          break;

      case CHKSUM_ENABLE:
          sscanf(value, "%d", &ps_app_ctxt->u4_chksum_enable);
          break;

      case MB_INFO_FILE:
        sscanf(value, "%s", ps_app_ctxt->ac_mb_info_fname);
        break;

      case MB_INFO_TYPE:
        sscanf(value, "%d", &ps_app_ctxt->u4_mb_info_type);
        break;

      case PIC_INFO_FILE:
        sscanf(value, "%s", ps_app_ctxt->ac_pic_info_fname);
        break;

      case PIC_INFO_TYPE:
        sscanf(value, "%d", &ps_app_ctxt->u4_pic_info_type);
        break;

      case INPUT_CHROMA_FORMAT:
        ps_app_ctxt->e_inp_color_fmt = get_chroma_fmt(value);
        break;

      case RECON_CHROMA_FORMAT:
        ps_app_ctxt->e_recon_color_fmt = get_chroma_fmt(value);
        break;

      case MAX_WD:
        sscanf(value, "%d", &ps_app_ctxt->u4_max_wd);
        break;

      case MAX_HT:
        sscanf(value, "%d", &ps_app_ctxt->u4_max_ht);
        break;

      case WD:
        sscanf(value, "%d", &ps_app_ctxt->u4_wd);
        break;

      case HT:
        sscanf(value, "%d", &ps_app_ctxt->u4_ht);
        break;

      case MAX_LEVEL:
        sscanf(value, "%d", &ps_app_ctxt->u4_max_level);
        break;

      case ENC_SPEED:
        ps_app_ctxt->u4_enc_speed = get_speed_preset(value);
        break;

      case ME_SPEED:
        sscanf(value, "%d", &ps_app_ctxt->u4_me_speed);
        break;

      case START_FRM:
        sscanf(value, "%d", &ps_app_ctxt->u4_start_frm);
        break;

      case NUM_FRMS:
        sscanf(value, "%d", &ps_app_ctxt->u4_max_num_frms);
        break;

      case MAX_FRAMERATE:
          sscanf(value, "%d", &ps_app_ctxt->u4_max_frame_rate);
          if(ps_app_ctxt->u4_max_frame_rate <= 0)
              ps_app_ctxt->u4_max_frame_rate = DEFAULT_MAX_FRAMERATE;
          break;

      case SRC_FRAMERATE:
        sscanf(value, "%d", &ps_app_ctxt->u4_src_frame_rate);
        if(ps_app_ctxt->u4_src_frame_rate <= 0)
            ps_app_ctxt->u4_src_frame_rate = DEFAULT_SRC_FRAME_RATE;
        break;

      case TGT_FRAMERATE:
        sscanf(value, "%d", &ps_app_ctxt->u4_tgt_frame_rate);
        if(ps_app_ctxt->u4_tgt_frame_rate <= 0)
            ps_app_ctxt->u4_tgt_frame_rate = DEFAULT_TGT_FRAME_RATE;
        break;

      case RC:
        sscanf(value, "%d", &ps_app_ctxt->u4_rc);
        break;

      case MAX_BITRATE:
          sscanf(value, "%d", &ps_app_ctxt->u4_max_bitrate);
          break;

      case BITRATE:
        sscanf(value, "%d", &ps_app_ctxt->u4_bitrate);
        break;

      case I_QP:
          sscanf(value, "%d", &ps_app_ctxt->u4_i_qp);
          break;

      case I_QP_MAX:
          sscanf(value, "%d", &ps_app_ctxt->u4_i_qp_max);
          break;

      case I_QP_MIN:
          sscanf(value, "%d", &ps_app_ctxt->u4_i_qp_min);
          break;

      case P_QP:
          sscanf(value, "%d", &ps_app_ctxt->u4_p_qp);
          break;

      case P_QP_MAX:
          sscanf(value, "%d", &ps_app_ctxt->u4_p_qp_max);
          break;

      case P_QP_MIN:
          sscanf(value, "%d", &ps_app_ctxt->u4_p_qp_min);
          break;

      case B_QP:
          sscanf(value, "%d", &ps_app_ctxt->u4_b_qp);
          break;

      case B_QP_MAX:
          sscanf(value, "%d", &ps_app_ctxt->u4_b_qp_max);
          break;

      case B_QP_MIN:
          sscanf(value, "%d", &ps_app_ctxt->u4_b_qp_min);
          break;

      case ENTROPY:
          sscanf(value, "%d", &ps_app_ctxt->u4_entropy_coding_mode);
          break;

      case AIR:
          sscanf(value, "%d", &ps_app_ctxt->u4_air);
          break;

      case ARCH:
          if((strcmp(value, "ARM_NONEON")) == 0)
              ps_app_ctxt->e_arch = ARCH_ARM_NONEON;
          else if((strcmp(value, "ARM_A9Q")) == 0)
              ps_app_ctxt->e_arch = ARCH_ARM_A9Q;
          else if((strcmp(value, "ARM_A7")) == 0)
              ps_app_ctxt->e_arch = ARCH_ARM_A7;
          else if((strcmp(value, "ARM_A5")) == 0)
              ps_app_ctxt->e_arch = ARCH_ARM_A5;
          else if((strcmp(value, "ARM_NEONINTR")) == 0)
              ps_app_ctxt->e_arch = ARCH_ARM_NEONINTR;
          else if((strcmp(value, "X86_GENERIC")) == 0)
              ps_app_ctxt->e_arch = ARCH_X86_GENERIC;
          else if((strcmp(value, "X86_SSSE3")) == 0)
              ps_app_ctxt->e_arch = ARCH_X86_SSSE3;
          else if((strcmp(value, "X86_SSE42")) == 0)
              ps_app_ctxt->e_arch = ARCH_X86_SSE42;
          else if((strcmp(value, "ARM_A53")) == 0)
              ps_app_ctxt->e_arch = ARCH_ARM_A53;
          else if((strcmp(value, "ARM_A57")) == 0)
              ps_app_ctxt->e_arch = ARCH_ARM_A57;
          else if((strcmp(value, "ARM_V8_NEON")) == 0)
              ps_app_ctxt->e_arch = ARCH_ARM_V8_NEON;
          else
          {
              printf("\nInvalid Arch. Setting it to ARM_A9Q\n");
              ps_app_ctxt->e_arch = ARCH_ARM_A9Q;
          }

          break;
      case SOC:
          if((strcmp(value, "GENERIC")) == 0)
              ps_app_ctxt->e_soc = SOC_GENERIC;
          else if((strcmp(value, "HISI_37X")) == 0)
              ps_app_ctxt->e_soc = SOC_HISI_37X;
          else
          {
              ps_app_ctxt->e_soc = SOC_GENERIC;
          }
          break;

      case NUMCORES:
        sscanf(value, "%d", &ps_app_ctxt->u4_num_cores);
        break;

      case LOOPBACK:
        sscanf(value, "%d", &ps_app_ctxt->u4_loopback);
        break;

      case PRE_ENC_ME:
        sscanf(value, "%d", &ps_app_ctxt->u4_pre_enc_me);
        break;

      case PRE_ENC_IPE:
        sscanf(value, "%d", &ps_app_ctxt->u4_pre_enc_ipe);
        break;

      case HPEL:
        sscanf(value, "%d", &ps_app_ctxt->u4_hpel);
        break;

      case QPEL:
        sscanf(value, "%d", &ps_app_ctxt->u4_qpel);
        break;

      case SRCH_RNG_X:
        sscanf(value, "%d", &ps_app_ctxt->u4_srch_rng_x);
        break;

      case SRCH_RNG_Y:
        sscanf(value, "%d", &ps_app_ctxt->u4_srch_rng_y);
        break;

      case I_INTERVAL:
        sscanf(value, "%d", &ps_app_ctxt->u4_i_interval);
        break;

      case IDR_INTERVAL:
        sscanf(value, "%d", &ps_app_ctxt->u4_idr_interval);
        break;

      case CONSTRAINED_INTRA_PRED:
        sscanf(value, "%d", &ps_app_ctxt->u4_constrained_intra_pred);
        break;

      case NUM_B_FRMS:
        sscanf(value, "%d", &ps_app_ctxt->u4_num_bframes);
        break;

      case DISABLE_DEBLOCK_LEVEL:
        sscanf(value, "%d", &ps_app_ctxt->u4_disable_deblk_level);
        break;

      case VBV_DELAY:
         sscanf(value, "%d", &ps_app_ctxt->u4_vbv_buffer_delay);
         break;

      case VBV_SIZE:
         sscanf(value, "%d", &ps_app_ctxt->u4_vbv_buf_size);
         break;

      case FAST_SAD:
          sscanf(value, "%d", &ps_app_ctxt->u4_enable_fast_sad);
          break;

      case ALT_REF:
          sscanf(value, "%d", &ps_app_ctxt->u4_enable_alt_ref);
          break;

      case AIR_REFRESH_PERIOD:
          sscanf(value, "%d", &ps_app_ctxt->u4_air_refresh_period);
                   break;

      case PROFILE:
            if((strcmp(value, "BASE")) == 0)
                ps_app_ctxt->e_profile = IV_PROFILE_BASE;
            else if((strcmp(value, "MAIN")) == 0)
              ps_app_ctxt->e_profile = IV_PROFILE_MAIN;
            else if((strcmp(value, "HIGH")) == 0)
              ps_app_ctxt->e_profile = IV_PROFILE_HIGH;
            else
            {
                printf("\nInvalid profile. Setting it to BASE\n");
                ps_app_ctxt->e_profile = IV_PROFILE_BASE;
            }
            break;

      case PSNR:
          sscanf(value, "%d", &ps_app_ctxt->u4_psnr_enable);
          break;

      case INTRA_4x4_ENABLE:
          sscanf(value, "%d", &ps_app_ctxt->u4_enable_intra_4x4);
          break;


      case INVALID:
        default:
            printf("Ignoring argument :  %s\n", argument);
            break;
    }
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : read_cfg_file                                            */
/*                                                                           */
/*  Description   : Reads arguments from a configuration file                */
/*                                                                           */
/*                                                                           */
/*  Inputs        : ps_app_ctxt  : Application context                        */
/*                  fp_cfg_file : Configuration file handle                  */
/*  Globals       :                                                          */
/*  Processing    : Parses the arguments and fills in the application context*/
/*                                                                           */
/*  Outputs       : Arguments parsed                                         */
/*  Returns       : None                                                     */
/*                                                                           */
/*  Issues        :                                                          */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         07 09 2012   100189          Initial Version                      */
/*                                                                           */
/*****************************************************************************/
void read_cfg_file(app_ctxt_t *ps_app_ctxt, FILE *fp_cfg)
{
    CHAR line[STRLENGTH];
    CHAR description[STRLENGTH];
    CHAR value[STRLENGTH];
    CHAR argument[STRLENGTH];

    while(0 == (feof(fp_cfg)))
    {
        line[0] = '\0';
        fgets(line, STRLENGTH, fp_cfg);
        argument[0] = '\0';
        /* Reading Input File Name */
        sscanf(line, "%s %s %s", argument, value, description);
        if(argument[0] == '\0')
            continue;

        parse_argument(ps_app_ctxt, argument, value);
    }
}

void invalid_argument_exit(CHAR *pc_err_message)
{
    print_usage();
    codec_exit(pc_err_message);
}

void validate_params(app_ctxt_t *ps_app_ctxt)
{
    CHAR ac_error[STRLENGTH];

    if(ps_app_ctxt->ac_ip_fname[0] == '\0')
    {
        invalid_argument_exit("Specify input file");
    }
    if(ps_app_ctxt->ac_op_fname[0] == '\0')
    {
        invalid_argument_exit("Specify output file");
    }
    if((1 == ps_app_ctxt->u4_recon_enable) && (ps_app_ctxt->ac_recon_fname[0] == '\0'))
    {
        invalid_argument_exit("Specify recon file");
    }
    if((1 == ps_app_ctxt->u4_chksum_enable) && (ps_app_ctxt->ac_chksum_fname[0] == '\0'))
    {
        invalid_argument_exit("Specify checksum file");
    }
    if(0 >= (WORD32)ps_app_ctxt->u4_wd)
    {
        sprintf(ac_error, "Invalid width: %d", ps_app_ctxt->u4_wd);
        invalid_argument_exit(ac_error);
    }
    if(0 >= (WORD32)ps_app_ctxt->u4_ht)
    {
        sprintf(ac_error, "Invalid height: %d", ps_app_ctxt->u4_ht);
        invalid_argument_exit(ac_error);
    }

    if(0 == (WORD32)ps_app_ctxt->u4_max_num_frms)
    {
        sprintf(ac_error, "Invalid number of frames to be encoded: %d", ps_app_ctxt->u4_max_num_frms);
        invalid_argument_exit(ac_error);
    }
    if ((0 != (WORD32)ps_app_ctxt->u4_entropy_coding_mode)
                    && (1 != (WORD32)ps_app_ctxt->u4_entropy_coding_mode))
    {
        sprintf(ac_error, "Invalid entropy codeing mode: %d",
                ps_app_ctxt->u4_entropy_coding_mode);
        invalid_argument_exit(ac_error);
    }
    return;
}

void init_default_params(app_ctxt_t *ps_app_ctxt)
{

    ps_app_ctxt->ps_enc                  = NULL;
    ps_app_ctxt->ps_mem_rec              = NULL;
    ps_app_ctxt->u4_num_mem_rec          = DEFAULT_MEM_REC_CNT;
    ps_app_ctxt->u4_recon_enable         = DEFAULT_RECON_ENABLE;
    ps_app_ctxt->u4_chksum_enable        = DEFAULT_CHKSUM_ENABLE;
    ps_app_ctxt->u4_mb_info_type         = 0;
    ps_app_ctxt->u4_pic_info_type        = 0;
    ps_app_ctxt->u4_mb_info_size         = 0;
    ps_app_ctxt->u4_pic_info_size        = 0;
    ps_app_ctxt->u4_start_frm            = DEFAULT_START_FRM;
    ps_app_ctxt->u4_max_num_frms         = DEFAULT_NUM_FRMS;
    ps_app_ctxt->avg_time                = 0;
    ps_app_ctxt->u4_total_bytes          = 0;
    ps_app_ctxt->u4_pics_cnt             = 0;
    ps_app_ctxt->e_inp_color_fmt         = DEFAULT_INP_COLOR_FMT;
    ps_app_ctxt->e_recon_color_fmt       = DEFAULT_RECON_COLOR_FMT;
    ps_app_ctxt->e_arch                  = ARCH_ARM_A9Q;
    ps_app_ctxt->e_soc                   = SOC_GENERIC;
    ps_app_ctxt->header_generated        = 0;
    ps_app_ctxt->pv_codec_obj            = NULL;
    ps_app_ctxt->u4_num_cores            = DEFAULT_NUM_CORES;
    ps_app_ctxt->u4_pre_enc_me           = 0;
    ps_app_ctxt->u4_pre_enc_ipe          = 0;
    ps_app_ctxt->ac_ip_fname[0]          = '\0';
    ps_app_ctxt->ac_op_fname[0]          = '\0';
    ps_app_ctxt->ac_recon_fname[0]       = '\0';
    ps_app_ctxt->ac_chksum_fname[0]      = '\0';
    ps_app_ctxt->ac_mb_info_fname[0]     = '\0';
    ps_app_ctxt->fp_ip                   = NULL;
    ps_app_ctxt->fp_op                   = NULL;
    ps_app_ctxt->fp_recon                = NULL;
    ps_app_ctxt->fp_chksum               = NULL;
    ps_app_ctxt->fp_psnr_ip              = NULL;
    ps_app_ctxt->fp_mb_info              = NULL;
    ps_app_ctxt->fp_pic_info             = NULL;
    ps_app_ctxt->u4_loopback             = DEFAULT_LOOPBACK;
    ps_app_ctxt->u4_max_frame_rate       = DEFAULT_MAX_FRAMERATE;
    ps_app_ctxt->u4_src_frame_rate       = DEFAULT_SRC_FRAME_RATE;
    ps_app_ctxt->u4_tgt_frame_rate       = DEFAULT_TGT_FRAME_RATE;
    ps_app_ctxt->u4_max_wd               = DEFAULT_MAX_WD;
    ps_app_ctxt->u4_max_ht               = DEFAULT_MAX_HT;
    ps_app_ctxt->u4_max_level            = DEFAULT_MAX_LEVEL;
    ps_app_ctxt->u4_strd                 = DEFAULT_STRIDE;
    ps_app_ctxt->u4_wd                   = DEFAULT_WD;
    ps_app_ctxt->u4_ht                   = DEFAULT_HT;
    ps_app_ctxt->u4_psnr_enable          = DEFAULT_PSNR_ENABLE;
    ps_app_ctxt->u4_enc_speed            = IVE_FASTEST;
    ps_app_ctxt->u4_me_speed             = DEFAULT_ME_SPEED;
    ps_app_ctxt->u4_enable_fast_sad      = DEFAULT_ENABLE_FAST_SAD;
    ps_app_ctxt->u4_enable_alt_ref       = DEFAULT_ENABLE_ALT_REF;
    ps_app_ctxt->u4_rc                   = DEFAULT_RC;
    ps_app_ctxt->u4_max_bitrate          = DEFAULT_MAX_BITRATE;
    ps_app_ctxt->u4_num_bframes          = DEFAULT_NUM_BFRAMES;
    ps_app_ctxt->u4_bitrate              = DEFAULT_BITRATE;
    ps_app_ctxt->u4_i_qp                 = DEFAULT_I_QP;
    ps_app_ctxt->u4_p_qp                 = DEFAULT_P_QP;
    ps_app_ctxt->u4_b_qp                 = DEFAULT_B_QP;
    ps_app_ctxt->u4_i_qp_min             = DEFAULT_QP_MIN;
    ps_app_ctxt->u4_i_qp_max             = DEFAULT_QP_MAX;
    ps_app_ctxt->u4_p_qp_min             = DEFAULT_QP_MIN;
    ps_app_ctxt->u4_p_qp_max             = DEFAULT_QP_MAX;
    ps_app_ctxt->u4_b_qp_min             = DEFAULT_QP_MIN;
    ps_app_ctxt->u4_b_qp_max             = DEFAULT_QP_MAX;
    ps_app_ctxt->u4_air                  = DEFAULT_AIR;
    ps_app_ctxt->u4_air_refresh_period   = DEFAULT_AIR_REFRESH_PERIOD;
    ps_app_ctxt->u4_srch_rng_x           = DEFAULT_SRCH_RNG_X;
    ps_app_ctxt->u4_srch_rng_y           = DEFAULT_SRCH_RNG_Y;
    ps_app_ctxt->u4_i_interval           = DEFAULT_I_INTERVAL;
    ps_app_ctxt->u4_idr_interval         = DEFAULT_IDR_INTERVAL;
    ps_app_ctxt->u4_constrained_intra_pred  = DEFAULT_CONSTRAINED_INTRAPRED;
    ps_app_ctxt->u4_disable_deblk_level  = DEFAULT_DISABLE_DEBLK_LEVEL;
    ps_app_ctxt->u4_hpel                 = DEFAULT_HPEL;
    ps_app_ctxt->u4_qpel                 = DEFAULT_QPEL;
    ps_app_ctxt->u4_enable_intra_4x4     = DEFAULT_I4;
    ps_app_ctxt->e_profile               = DEFAULT_EPROFILE;
    ps_app_ctxt->u4_slice_mode           = DEFAULT_SLICE_MODE;
    ps_app_ctxt->u4_slice_param          = DEFAULT_SLICE_PARAM;
    ps_app_ctxt->pv_input_thread_handle  = NULL;
    ps_app_ctxt->pv_output_thread_handle = NULL;
    ps_app_ctxt->pv_recon_thread_handle  = NULL;
    ps_app_ctxt->u4_vbv_buf_size         = 0;
    ps_app_ctxt->u4_vbv_buffer_delay     = 1000;
    ps_app_ctxt->adbl_psnr[0]            = 0.0;
    ps_app_ctxt->adbl_psnr[1]            = 0.0;
    ps_app_ctxt->adbl_psnr[2]            = 0.0;
    ps_app_ctxt->u4_psnr_cnt             = 0;
    ps_app_ctxt->pu1_psnr_buf            = NULL;
    ps_app_ctxt->u4_psnr_buf_size        = 0;
    ps_app_ctxt->u4_entropy_coding_mode  = DEFAULT_ENTROPY_CODING_MODE;

    return;
}

void set_dimensions(app_ctxt_t *ps_app_ctxt,
                    UWORD32 u4_timestamp_low,
                    UWORD32 u4_timestamp_high)
{
    ih264e_ctl_set_dimensions_ip_t s_frame_dimensions_ip;
    ih264e_ctl_set_dimensions_op_t s_frame_dimensions_op;
    IV_STATUS_T status;

    s_frame_dimensions_ip.s_ive_ip.e_cmd = IVE_CMD_VIDEO_CTL;
    s_frame_dimensions_ip.s_ive_ip.e_sub_cmd = IVE_CMD_CTL_SET_DIMENSIONS;

    s_frame_dimensions_ip.s_ive_ip.u4_ht = ps_app_ctxt->u4_ht;
    s_frame_dimensions_ip.s_ive_ip.u4_wd = ps_app_ctxt->u4_wd;

    s_frame_dimensions_ip.s_ive_ip.u4_timestamp_high = u4_timestamp_high;
    s_frame_dimensions_ip.s_ive_ip.u4_timestamp_low = u4_timestamp_low;

    s_frame_dimensions_ip.s_ive_ip.u4_size =
                    sizeof(ih264e_ctl_set_dimensions_ip_t);
    s_frame_dimensions_op.s_ive_op.u4_size =
                    sizeof(ih264e_ctl_set_dimensions_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc,
                                 &s_frame_dimensions_ip,
                                 &s_frame_dimensions_op);
    if(status != IV_SUCCESS)
    {
        CHAR ac_error[STRLENGTH];
        sprintf(ac_error, "Unable to set frame dimensions = 0x%x\n",
                s_frame_dimensions_op.s_ive_op.u4_error_code);
        codec_exit(ac_error);
    }
    return;
}

void set_frame_rate(app_ctxt_t *ps_app_ctxt,
                    UWORD32 u4_timestamp_low,
                    UWORD32 u4_timestamp_high)
{
    ih264e_ctl_set_frame_rate_ip_t s_frame_rate_ip;
    ih264e_ctl_set_frame_rate_op_t s_frame_rate_op;
    IV_STATUS_T status;

    s_frame_rate_ip.s_ive_ip.e_cmd  =   IVE_CMD_VIDEO_CTL;
    s_frame_rate_ip.s_ive_ip.e_sub_cmd  =   IVE_CMD_CTL_SET_FRAMERATE;

    s_frame_rate_ip.s_ive_ip.u4_src_frame_rate  =
                    ps_app_ctxt->u4_src_frame_rate;
    s_frame_rate_ip.s_ive_ip.u4_tgt_frame_rate  =
                    ps_app_ctxt->u4_tgt_frame_rate;

    s_frame_rate_ip.s_ive_ip.u4_timestamp_high  =   u4_timestamp_high;
    s_frame_rate_ip.s_ive_ip.u4_timestamp_low   =   u4_timestamp_low;

    s_frame_rate_ip.s_ive_ip.u4_size    =   sizeof(ih264e_ctl_set_frame_rate_ip_t);
    s_frame_rate_op.s_ive_op.u4_size    =   sizeof(ih264e_ctl_set_frame_rate_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc,&s_frame_rate_ip,&s_frame_rate_op);
    if(status != IV_SUCCESS)
       {
           CHAR ac_error[STRLENGTH];
           sprintf(ac_error, "Unable to set frame rate = 0x%x\n",
                   s_frame_rate_op.s_ive_op.u4_error_code);
           codec_exit(ac_error);
       }
       return;
}


void set_ipe_params(app_ctxt_t *ps_app_ctxt,
                    UWORD32 u4_timestamp_low,
                    UWORD32 u4_timestamp_high)
{
    ih264e_ctl_set_ipe_params_ip_t s_ipe_params_ip;
    ih264e_ctl_set_ipe_params_op_t s_ipe_params_op;
    IV_STATUS_T status;

    s_ipe_params_ip.s_ive_ip.e_cmd  =   IVE_CMD_VIDEO_CTL;
    s_ipe_params_ip.s_ive_ip.e_sub_cmd  =   IVE_CMD_CTL_SET_IPE_PARAMS;

    s_ipe_params_ip.s_ive_ip.u4_enable_intra_4x4  = ps_app_ctxt->u4_enable_intra_4x4;
    s_ipe_params_ip.s_ive_ip.u4_enc_speed_preset  = ps_app_ctxt->u4_enc_speed;

    s_ipe_params_ip.s_ive_ip.u4_timestamp_high  =   u4_timestamp_high;
    s_ipe_params_ip.s_ive_ip.u4_timestamp_low   =   u4_timestamp_low;

    s_ipe_params_ip.s_ive_ip.u4_size    =   sizeof(ih264e_ctl_set_ipe_params_ip_t);
    s_ipe_params_op.s_ive_op.u4_size    =   sizeof(ih264e_ctl_set_ipe_params_op_t);

    s_ipe_params_ip.s_ive_ip.u4_constrained_intra_pred =
                                            ps_app_ctxt->u4_constrained_intra_pred;

    status = ih264e_api_function(ps_app_ctxt->ps_enc,&s_ipe_params_ip,&s_ipe_params_op);
    if(status != IV_SUCCESS)
    {
        CHAR ac_error[STRLENGTH];
        sprintf(ac_error, "Unable to set ipe params = 0x%x\n",
                s_ipe_params_op.s_ive_op.u4_error_code);
        codec_exit(ac_error);
    }
    return;
}

void set_bit_rate(app_ctxt_t *ps_app_ctxt,
                  UWORD32 u4_timestamp_low, UWORD32 u4_timestamp_high)
{
    ih264e_ctl_set_bitrate_ip_t s_bitrate_ip;
    ih264e_ctl_set_bitrate_op_t s_bitrate_op;
    IV_STATUS_T status;

    s_bitrate_ip.s_ive_ip.e_cmd  =   IVE_CMD_VIDEO_CTL;
    s_bitrate_ip.s_ive_ip.e_sub_cmd  =   IVE_CMD_CTL_SET_BITRATE;

    s_bitrate_ip.s_ive_ip.u4_target_bitrate  =   ps_app_ctxt->u4_bitrate;

    s_bitrate_ip.s_ive_ip.u4_timestamp_high  =   u4_timestamp_high;
    s_bitrate_ip.s_ive_ip.u4_timestamp_low   =   u4_timestamp_low;

    s_bitrate_ip.s_ive_ip.u4_size    =   sizeof(ih264e_ctl_set_bitrate_ip_t);
    s_bitrate_op.s_ive_op.u4_size    =   sizeof(ih264e_ctl_set_bitrate_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc,&s_bitrate_ip,&s_bitrate_op);
    if(status != IV_SUCCESS)
       {
           CHAR ac_error[STRLENGTH];
           sprintf(ac_error, "Unable to set bit rate = 0x%x\n",
                   s_bitrate_op.s_ive_op.u4_error_code);
           codec_exit(ac_error);
       }
       return;
}


void set_frame_type(app_ctxt_t *ps_app_ctxt,
                    UWORD32 u4_timestamp_low,
                    UWORD32 u4_timestamp_high,
                    IV_PICTURE_CODING_TYPE_T  e_frame_type)
{
    ih264e_ctl_set_frame_type_ip_t s_frame_type_ip;
    ih264e_ctl_set_frame_type_op_t s_frame_type_op;
    IV_STATUS_T status;

    s_frame_type_ip.s_ive_ip.e_cmd  =   IVE_CMD_VIDEO_CTL;
    s_frame_type_ip.s_ive_ip.e_sub_cmd  =   IVE_CMD_CTL_SET_FRAMETYPE;

    s_frame_type_ip.s_ive_ip.e_frame_type  =   e_frame_type;

    s_frame_type_ip.s_ive_ip.u4_timestamp_high  =   u4_timestamp_high;
    s_frame_type_ip.s_ive_ip.u4_timestamp_low   =   u4_timestamp_low;

    s_frame_type_ip.s_ive_ip.u4_size    =   sizeof(ih264e_ctl_set_frame_type_ip_t);
    s_frame_type_op.s_ive_op.u4_size    =   sizeof(ih264e_ctl_set_frame_type_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc,&s_frame_type_ip,&s_frame_type_op);
    if(status != IV_SUCCESS)
    {
        CHAR ac_error[STRLENGTH];
        sprintf(ac_error, "Unable to set frame type = 0x%x\n",
                s_frame_type_op.s_ive_op.u4_error_code);
        codec_exit(ac_error);
    }
    return;
}

void set_qp(app_ctxt_t *ps_app_ctxt,
            UWORD32 u4_timestamp_low, UWORD32 u4_timestamp_high)
{
    ih264e_ctl_set_qp_ip_t s_qp_ip;
    ih264e_ctl_set_qp_op_t s_qp_op;
    IV_STATUS_T status;

    s_qp_ip.s_ive_ip.e_cmd  =   IVE_CMD_VIDEO_CTL;
    s_qp_ip.s_ive_ip.e_sub_cmd  =   IVE_CMD_CTL_SET_QP;

    s_qp_ip.s_ive_ip.u4_i_qp = ps_app_ctxt->u4_i_qp;
    s_qp_ip.s_ive_ip.u4_i_qp_max = ps_app_ctxt->u4_i_qp_max;
    s_qp_ip.s_ive_ip.u4_i_qp_min = ps_app_ctxt->u4_i_qp_min;

    s_qp_ip.s_ive_ip.u4_p_qp = ps_app_ctxt->u4_p_qp;
    s_qp_ip.s_ive_ip.u4_p_qp_max = ps_app_ctxt->u4_p_qp_max;
    s_qp_ip.s_ive_ip.u4_p_qp_min = ps_app_ctxt->u4_p_qp_min;

    s_qp_ip.s_ive_ip.u4_b_qp = ps_app_ctxt->u4_b_qp;
    s_qp_ip.s_ive_ip.u4_b_qp_max = ps_app_ctxt->u4_b_qp_max;
    s_qp_ip.s_ive_ip.u4_b_qp_min = ps_app_ctxt->u4_b_qp_min;

    s_qp_ip.s_ive_ip.u4_timestamp_high  =   u4_timestamp_high;
    s_qp_ip.s_ive_ip.u4_timestamp_low   =   u4_timestamp_low;

    s_qp_ip.s_ive_ip.u4_size    =   sizeof(ih264e_ctl_set_qp_ip_t);
    s_qp_op.s_ive_op.u4_size    =   sizeof(ih264e_ctl_set_qp_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc,&s_qp_ip,&s_qp_op);
    if(status != IV_SUCCESS)
       {
           CHAR ac_error[STRLENGTH];
           sprintf(ac_error, "Unable to set qp 0x%x\n",
                   s_qp_op.s_ive_op.u4_error_code);
           codec_exit(ac_error);
       }
       return;
}

void set_enc_mode(app_ctxt_t *ps_app_ctxt,
                  UWORD32 u4_timestamp_low, UWORD32 u4_timestamp_high,
                  IVE_ENC_MODE_T e_enc_mode)
{
    IV_STATUS_T status;

    ih264e_ctl_set_enc_mode_ip_t s_enc_mode_ip;
    ih264e_ctl_set_enc_mode_op_t s_enc_mode_op;

    s_enc_mode_ip.s_ive_ip.e_cmd = IVE_CMD_VIDEO_CTL;
    s_enc_mode_ip.s_ive_ip.e_sub_cmd = IVE_CMD_CTL_SET_ENC_MODE;

    s_enc_mode_ip.s_ive_ip.e_enc_mode = e_enc_mode;

    s_enc_mode_ip.s_ive_ip.u4_timestamp_high = u4_timestamp_high;
    s_enc_mode_ip.s_ive_ip.u4_timestamp_low = u4_timestamp_low;

    s_enc_mode_ip.s_ive_ip.u4_size = sizeof(ih264e_ctl_set_enc_mode_ip_t);
    s_enc_mode_op.s_ive_op.u4_size = sizeof(ih264e_ctl_set_enc_mode_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc, &s_enc_mode_ip,
                                      &s_enc_mode_op);
    if(status != IV_SUCCESS)
    {
        CHAR ac_error[STRLENGTH];
        sprintf(ac_error, "Unable to set in header encode mode = 0x%x\n",
                s_enc_mode_op.s_ive_op.u4_error_code);
        codec_exit(ac_error);
    }
    return;
}


void set_vbv_params(app_ctxt_t *ps_app_ctxt,
                    UWORD32 u4_timestamp_low,
                    UWORD32 u4_timestamp_high)
{
    ih264e_ctl_set_vbv_params_ip_t s_vbv_ip;
    ih264e_ctl_set_vbv_params_op_t s_vbv_op;
    IV_STATUS_T status;

    s_vbv_ip.s_ive_ip.e_cmd  =   IVE_CMD_VIDEO_CTL;
    s_vbv_ip.s_ive_ip.e_sub_cmd  =   IVE_CMD_CTL_SET_VBV_PARAMS;

    s_vbv_ip.s_ive_ip.u4_vbv_buf_size = ps_app_ctxt->u4_vbv_buf_size;
    s_vbv_ip.s_ive_ip.u4_vbv_buffer_delay  =
                    ps_app_ctxt->u4_vbv_buffer_delay;

    s_vbv_ip.s_ive_ip.u4_timestamp_high  =   u4_timestamp_high;
    s_vbv_ip.s_ive_ip.u4_timestamp_low   =   u4_timestamp_low;

    s_vbv_ip.s_ive_ip.u4_size    =   sizeof(ih264e_ctl_set_vbv_params_ip_t);
    s_vbv_op.s_ive_op.u4_size    =   sizeof(ih264e_ctl_set_vbv_params_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc,&s_vbv_ip,&s_vbv_op);
    if(status != IV_SUCCESS)
    {
        CHAR ac_error[STRLENGTH];
        sprintf(ac_error, "Unable to set VBC params = 0x%x\n",
                s_vbv_op.s_ive_op.u4_error_code);
        codec_exit(ac_error);
    }
    return;
}

void set_air_params(app_ctxt_t *ps_app_ctxt,
                    UWORD32 u4_timestamp_low,
                    UWORD32 u4_timestamp_high)
{
    ih264e_ctl_set_air_params_ip_t s_air_ip;
    ih264e_ctl_set_air_params_op_t s_air_op;
    IV_STATUS_T status;

    s_air_ip.s_ive_ip.e_cmd  =   IVE_CMD_VIDEO_CTL;
    s_air_ip.s_ive_ip.e_sub_cmd  =   IVE_CMD_CTL_SET_AIR_PARAMS;

    s_air_ip.s_ive_ip.e_air_mode = ps_app_ctxt->u4_air;
    s_air_ip.s_ive_ip.u4_air_refresh_period = ps_app_ctxt->u4_air_refresh_period;

    s_air_ip.s_ive_ip.u4_timestamp_high  =   u4_timestamp_high;
    s_air_ip.s_ive_ip.u4_timestamp_low   =   u4_timestamp_low;

    s_air_ip.s_ive_ip.u4_size    =   sizeof(ih264e_ctl_set_air_params_ip_t);
    s_air_op.s_ive_op.u4_size    =   sizeof(ih264e_ctl_set_air_params_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc,&s_air_ip,&s_air_op);
    if(status != IV_SUCCESS)
       {
           CHAR ac_error[STRLENGTH];
           sprintf(ac_error, "Unable to set air params = 0x%x\n",
                   s_air_op.s_ive_op.u4_error_code);
           codec_exit(ac_error);
       }
       return;
}

void set_me_params(app_ctxt_t *ps_app_ctxt,
                    UWORD32 u4_timestamp_low,
                    UWORD32 u4_timestamp_high)
{
    IV_STATUS_T status;

    ih264e_ctl_set_me_params_ip_t s_me_params_ip;
    ih264e_ctl_set_me_params_op_t s_me_params_op;

    s_me_params_ip.s_ive_ip.e_cmd = IVE_CMD_VIDEO_CTL;
    s_me_params_ip.s_ive_ip.e_sub_cmd = IVE_CMD_CTL_SET_ME_PARAMS;

    s_me_params_ip.s_ive_ip.u4_enable_fast_sad = ps_app_ctxt->u4_enable_fast_sad;
    s_me_params_ip.s_ive_ip.u4_enable_alt_ref = ps_app_ctxt->u4_enable_alt_ref;

    s_me_params_ip.s_ive_ip.u4_enable_hpel  =   ps_app_ctxt->u4_hpel;
    s_me_params_ip.s_ive_ip.u4_enable_qpel  =   ps_app_ctxt->u4_qpel;
    s_me_params_ip.s_ive_ip.u4_me_speed_preset  =   ps_app_ctxt->u4_me_speed;
    s_me_params_ip.s_ive_ip.u4_srch_rng_x   =   ps_app_ctxt->u4_srch_rng_x;
    s_me_params_ip.s_ive_ip.u4_srch_rng_y   =   ps_app_ctxt->u4_srch_rng_y;

    s_me_params_ip.s_ive_ip.u4_timestamp_high = u4_timestamp_high;
    s_me_params_ip.s_ive_ip.u4_timestamp_low = u4_timestamp_low;

    s_me_params_ip.s_ive_ip.u4_size = sizeof(ih264e_ctl_set_me_params_ip_t);
    s_me_params_op.s_ive_op.u4_size = sizeof(ih264e_ctl_set_me_params_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc, &s_me_params_ip,
                                 &s_me_params_op);
    if(status != IV_SUCCESS)
    {
        CHAR ac_error[STRLENGTH];
        sprintf(ac_error, "Unable to set me params = 0x%x\n",
                s_me_params_op.s_ive_op.u4_error_code);
        codec_exit(ac_error);
    }
    return;
}


void set_gop_params(app_ctxt_t *ps_app_ctxt,
                    UWORD32 u4_timestamp_low,
                    UWORD32 u4_timestamp_high)
{
    IV_STATUS_T status;

    ih264e_ctl_set_gop_params_ip_t s_gop_params_ip;
    ih264e_ctl_set_gop_params_op_t s_gop_params_op;

    s_gop_params_ip.s_ive_ip.e_cmd = IVE_CMD_VIDEO_CTL;
    s_gop_params_ip.s_ive_ip.e_sub_cmd = IVE_CMD_CTL_SET_GOP_PARAMS;

    s_gop_params_ip.s_ive_ip.u4_i_frm_interval = ps_app_ctxt->u4_i_interval;
    s_gop_params_ip.s_ive_ip.u4_idr_frm_interval = ps_app_ctxt->u4_idr_interval;

    s_gop_params_ip.s_ive_ip.u4_timestamp_high = u4_timestamp_high;
    s_gop_params_ip.s_ive_ip.u4_timestamp_low = u4_timestamp_low;

    s_gop_params_ip.s_ive_ip.u4_size = sizeof(ih264e_ctl_set_gop_params_ip_t);
    s_gop_params_op.s_ive_op.u4_size = sizeof(ih264e_ctl_set_gop_params_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc, &s_gop_params_ip,
                                      &s_gop_params_op);
    if(status != IV_SUCCESS)
    {
        CHAR ac_error[STRLENGTH];
        sprintf(ac_error, "Unable to set ME params = 0x%x\n",
                s_gop_params_op.s_ive_op.u4_error_code);
        codec_exit(ac_error);
    }
    return;
}

void set_profile_params(app_ctxt_t *ps_app_ctxt,
                    UWORD32 u4_timestamp_low,
                    UWORD32 u4_timestamp_high)
{
    IV_STATUS_T status;

    ih264e_ctl_set_profile_params_ip_t s_profile_params_ip;
    ih264e_ctl_set_profile_params_op_t s_profile_params_op;

    s_profile_params_ip.s_ive_ip.e_cmd = IVE_CMD_VIDEO_CTL;
    s_profile_params_ip.s_ive_ip.e_sub_cmd = IVE_CMD_CTL_SET_PROFILE_PARAMS;

    s_profile_params_ip.s_ive_ip.e_profile = ps_app_ctxt->e_profile;

    s_profile_params_ip.s_ive_ip.u4_entropy_coding_mode = ps_app_ctxt->u4_entropy_coding_mode;

    s_profile_params_ip.s_ive_ip.u4_timestamp_high = u4_timestamp_high;
    s_profile_params_ip.s_ive_ip.u4_timestamp_low = u4_timestamp_low;

    s_profile_params_ip.s_ive_ip.u4_size = sizeof(ih264e_ctl_set_profile_params_ip_t);
    s_profile_params_op.s_ive_op.u4_size = sizeof(ih264e_ctl_set_profile_params_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc, &s_profile_params_ip,
                                      &s_profile_params_op);
    if(status != IV_SUCCESS)
    {
        CHAR ac_error[STRLENGTH];
        sprintf(ac_error, "Unable to set profile params = 0x%x\n",
                s_profile_params_op.s_ive_op.u4_error_code);
        codec_exit(ac_error);
    }
    return;
}

void set_deblock_params(app_ctxt_t *ps_app_ctxt,
                    UWORD32 u4_timestamp_low,
                    UWORD32 u4_timestamp_high)
{
    IV_STATUS_T status;

    ih264e_ctl_set_deblock_params_ip_t s_deblock_params_ip;
    ih264e_ctl_set_deblock_params_op_t s_deblock_params_op;

    s_deblock_params_ip.s_ive_ip.e_cmd = IVE_CMD_VIDEO_CTL;
    s_deblock_params_ip.s_ive_ip.e_sub_cmd = IVE_CMD_CTL_SET_DEBLOCK_PARAMS;

    s_deblock_params_ip.s_ive_ip.u4_disable_deblock_level =
                    ps_app_ctxt->u4_disable_deblk_level;

    s_deblock_params_ip.s_ive_ip.u4_timestamp_high = u4_timestamp_high;
    s_deblock_params_ip.s_ive_ip.u4_timestamp_low = u4_timestamp_low;

    s_deblock_params_ip.s_ive_ip.u4_size = sizeof(ih264e_ctl_set_deblock_params_ip_t);
    s_deblock_params_op.s_ive_op.u4_size = sizeof(ih264e_ctl_set_deblock_params_op_t);

    status = ih264e_api_function(ps_app_ctxt->ps_enc, &s_deblock_params_ip,
                                      &s_deblock_params_op);
    if(status != IV_SUCCESS)
    {
        CHAR ac_error[STRLENGTH];
        sprintf(ac_error, "Unable to enable/disable deblock params = 0x%x\n",
                s_deblock_params_op.s_ive_op.u4_error_code);
        codec_exit(ac_error);
    }
    return;
}

#define PEAK_WINDOW_SIZE    8

void synchronous_encode(iv_obj_t *ps_enc, app_ctxt_t *ps_app_ctxt)
{
    ih264e_video_encode_ip_t ih264e_video_encode_ip;
    ih264e_video_encode_op_t ih264e_video_encode_op;

    ive_video_encode_ip_t *ps_video_encode_ip = &ih264e_video_encode_ip.s_ive_ip;
    ive_video_encode_op_t *ps_video_encode_op = &ih264e_video_encode_op.s_ive_op;

    iv_raw_buf_t *ps_inp_raw_buf = &ps_video_encode_ip->s_inp_buf;

    IV_STATUS_T status = IV_SUCCESS;

    WORD32 i, is_last = 0, buff_size = 0, num_bytes = 0;
    UWORD32 u4_total_time = 0;
    UWORD8 *pu1_buf = NULL;
    UWORD32 u4_timestamp_low, u4_timestamp_high;
    void *pv_mb_info = NULL, *pv_pic_info = NULL;

    TIMER curtime ;
#ifdef WINDOWS_TIMER
    TIMER frequency;
#endif
    WORD32 peak_window[PEAK_WINDOW_SIZE] = {0};
    WORD32 peak_window_idx = 0;
    WORD32 peak_avg_max = 0, timetaken = 0;
    iv_raw_buf_t s_inp_buf, s_recon_buf;
    CHAR ac_error[STRLENGTH];
    WORD32 end_of_frames=0;
    WORD32 i4_inp_done =0;

    u4_timestamp_low = 0;
    u4_timestamp_high = 0;

    /*************************************************************************/
    /*                         Allocate I/O Buffers                          */
    /*************************************************************************/
    allocate_input(ps_app_ctxt);
    allocate_output(ps_app_ctxt);
    allocate_recon(ps_app_ctxt);

    /* init psnr */
    init_psnr(ps_app_ctxt);

    /* open file pointers */
    ps_app_ctxt->fp_ip = fopen(ps_app_ctxt->ac_ip_fname, "rb");
    if(NULL == ps_app_ctxt->fp_ip)
    {
        sprintf(ac_error, "Unable to open input file for reading: %s", ps_app_ctxt->ac_ip_fname);
        invalid_argument_exit(ac_error);
    }

    ps_app_ctxt->fp_op = fopen(ps_app_ctxt->ac_op_fname, "wb");
    if(NULL == ps_app_ctxt->fp_op)
    {
        sprintf(ac_error, "Unable to open output file for writing: %s", ps_app_ctxt->ac_op_fname);
        invalid_argument_exit(ac_error);
    }

    if(1 == ps_app_ctxt->u4_recon_enable)
    {
        ps_app_ctxt->fp_recon = fopen(ps_app_ctxt->ac_recon_fname, "wb");
        if(NULL == ps_app_ctxt->fp_recon)
        {
            sprintf(ac_error, "Unable to open recon file for writing: %s", ps_app_ctxt->ac_recon_fname);
            invalid_argument_exit(ac_error);
        }
    }

    if(1 == ps_app_ctxt->u4_chksum_enable)
    {
        ps_app_ctxt->fp_chksum               = fopen(ps_app_ctxt->ac_chksum_fname, "wb");
        if(NULL == ps_app_ctxt->fp_chksum)
        {
            sprintf(ac_error, "Unable to open checksum file for writing: %s", ps_app_ctxt->ac_chksum_fname);
            invalid_argument_exit(ac_error);
        }
    }

    /* If PSNR is enabled, open input file again and hold a different file pointer
     * This makes it easy to compute PSNR without adding dependency between input and recon threads
     */
    if(1 == ps_app_ctxt->u4_psnr_enable)
    {
        ps_app_ctxt->fp_psnr_ip              = fopen(ps_app_ctxt->ac_ip_fname, "rb");
        if(NULL == ps_app_ctxt->fp_psnr_ip)
        {
            sprintf(ac_error, "Unable to open input file for reading: %s", ps_app_ctxt->ac_ip_fname);
            invalid_argument_exit(ac_error);
        }
    }

    if(0 != ps_app_ctxt->u4_mb_info_type)
    {
        ps_app_ctxt->fp_mb_info  = fopen(ps_app_ctxt->ac_mb_info_fname, "rb");
        if(NULL == ps_app_ctxt->fp_mb_info)
        {
            sprintf(ac_error, "Unable to open MB info file for reading: %s", ps_app_ctxt->ac_mb_info_fname);
            invalid_argument_exit(ac_error);
        }
    }
    if (ps_app_ctxt->u4_pic_info_type)
    {
        ps_app_ctxt->fp_pic_info  = fopen(ps_app_ctxt->ac_pic_info_fname, "rb");
        if(NULL == ps_app_ctxt->fp_pic_info)
        {
            sprintf(ac_error, "Unable to open Pic info file for reading: %s", ps_app_ctxt->ac_pic_info_fname);
            invalid_argument_exit(ac_error);
        }
    }

    GETTIME(&ps_app_ctxt->enc_start_time);
    ps_app_ctxt->enc_last_time = ps_app_ctxt->enc_start_time;

    while(1)
    {

        /******************************************************************************/
        /****************** Input Initialization **************************************/
        /******************************************************************************/

        for(i = 0; i < DEFAULT_MAX_INPUT_BUFS; i++)
        {
            if(ps_app_ctxt->as_input_buf[i].u4_is_free)
            {
                pu1_buf = ps_app_ctxt->as_input_buf[i].pu1_buf;
                pv_mb_info = ps_app_ctxt->as_input_buf[i].pv_mb_info;
                pv_pic_info = ps_app_ctxt->as_input_buf[i].pv_pic_info;
                ps_app_ctxt->as_input_buf[i].u4_is_free = 0;
                break;
            }
        }

        if (i == DEFAULT_MAX_INPUT_BUFS)
        {
            printf("\n Unable to find a free input buffer!!");
            exit(0);
        }

        ps_video_encode_ip->u4_size = sizeof(ih264e_video_encode_ip_t);
        ps_video_encode_op->u4_size = sizeof(ih264e_video_encode_op_t);

        ps_video_encode_ip->e_cmd = IVE_CMD_VIDEO_ENCODE;
        ps_video_encode_ip->pv_bufs = pu1_buf;
        ps_video_encode_ip->pv_mb_info = pv_mb_info;
        ps_video_encode_ip->pv_pic_info = pv_pic_info;
        ps_video_encode_ip->u4_pic_info_type = ps_app_ctxt->u4_pic_info_type;
        /*
         * Since the buffers are used for reading,
         * And after each row we have a stride we nned to calculate
         * the luma size according to the stride
         */
        ps_inp_raw_buf->e_color_fmt = ps_app_ctxt->e_inp_color_fmt;

        /* Initialize for 420SP */
        if(IV_YUV_420SP_UV == ps_app_ctxt->e_inp_color_fmt||
                        IV_YUV_420SP_VU == ps_app_ctxt->e_inp_color_fmt)
        {
            /*init luma buffer*/
            ps_inp_raw_buf->apv_bufs[0] = pu1_buf;

            /*Init chroma buffer*/
            pu1_buf += ps_app_ctxt->u4_strd * ps_app_ctxt->u4_ht;
            ps_inp_raw_buf->apv_bufs[1] = pu1_buf;

            ps_inp_raw_buf->au4_wd[0] =  ps_app_ctxt->u4_wd;
            ps_inp_raw_buf->au4_wd[1] =  ps_app_ctxt->u4_wd;

            ps_inp_raw_buf->au4_ht[0] =  ps_app_ctxt->u4_ht;
            ps_inp_raw_buf->au4_ht[1] =  ps_app_ctxt->u4_ht / 2;

            ps_inp_raw_buf->au4_strd[0] =  ps_app_ctxt->u4_strd;
            ps_inp_raw_buf->au4_strd[1] =  ps_app_ctxt->u4_strd;
        }
        else if(IV_YUV_420P == ps_app_ctxt->e_inp_color_fmt)
        {
            /* init buffers */
            ps_inp_raw_buf->apv_bufs[0] = pu1_buf;
            pu1_buf += (ps_app_ctxt->u4_wd) * ps_app_ctxt->u4_ht;
            ps_inp_raw_buf->apv_bufs[1] = pu1_buf;
            pu1_buf += (ps_app_ctxt->u4_wd >> 1) * (ps_app_ctxt->u4_ht >> 1);
            ps_inp_raw_buf->apv_bufs[2] = pu1_buf;

            ps_inp_raw_buf->au4_wd[0] =  ps_app_ctxt->u4_wd;
            ps_inp_raw_buf->au4_wd[1] =  ps_app_ctxt->u4_wd / 2;
            ps_inp_raw_buf->au4_wd[2] =  ps_app_ctxt->u4_wd / 2;

            ps_inp_raw_buf->au4_ht[0] =  ps_app_ctxt->u4_ht;
            ps_inp_raw_buf->au4_ht[1] =  ps_app_ctxt->u4_ht / 2;
            ps_inp_raw_buf->au4_ht[2] =  ps_app_ctxt->u4_ht / 2;

            ps_inp_raw_buf->au4_strd[0] =  ps_app_ctxt->u4_strd;
            ps_inp_raw_buf->au4_strd[1] =  ps_app_ctxt->u4_strd / 2;
            ps_inp_raw_buf->au4_strd[2] =  ps_app_ctxt->u4_strd / 2;

        }
        else if(IV_YUV_422ILE == ps_app_ctxt->e_inp_color_fmt)
        {
            /*init luma buffer*/
            ps_inp_raw_buf->apv_bufs[0] = pu1_buf;

            ps_inp_raw_buf->au4_wd[0] =  ps_app_ctxt->u4_wd * 2;

            ps_inp_raw_buf->au4_ht[0] =  ps_app_ctxt->u4_ht;

            ps_inp_raw_buf->au4_strd[0] = ps_app_ctxt->u4_strd *2;
        }

        /*
         * Here we read input and other associated buffers. Regardless of success
         * we will proceed from here as we will need extra calls to flush out
         * input queue in encoder. Note that this is not necessary. You can just
         * send encode calls till with valid output and recon buffers till the
         * queue is flushed.
         */
        while(1)
        {
            IV_STATUS_T mb_info_status = IV_SUCCESS, pic_info_status = IV_SUCCESS;

            status = read_input(ps_app_ctxt->fp_ip, ps_inp_raw_buf);

            if (ps_app_ctxt->u4_mb_info_type != 0)
            {
                mb_info_status = read_mb_info(ps_app_ctxt, pv_mb_info);
            }
            if (ps_app_ctxt->u4_pic_info_type != 0)
            {
                pic_info_status = read_pic_info(ps_app_ctxt, pv_pic_info);
            }
            if((IV_SUCCESS != status) || (IV_SUCCESS != mb_info_status)
                            || (IV_SUCCESS != pic_info_status))
            {
                if(0 == ps_app_ctxt->u4_loopback)
                {
                    is_last = 1;
                    break;
                }
                else
                    fseek(ps_app_ctxt->fp_ip, 0, SEEK_SET);
            }
            break;
        }

        /******************************************************************************/
        /****************** Output Initialization *************************************/
        /******************************************************************************/

        for(i = 0; i < DEFAULT_MAX_OUTPUT_BUFS; i++)
        {
            if(ps_app_ctxt->as_output_buf[i].u4_is_free)
            {
                pu1_buf = ps_app_ctxt->as_output_buf[i].pu1_buf;
                buff_size = ps_app_ctxt->as_output_buf[i].u4_buf_size;
                ps_app_ctxt->as_output_buf[i].u4_is_free = 0;
                break;
            }
        }
        ps_video_encode_ip->s_out_buf.pv_buf = pu1_buf;
        ps_video_encode_ip->s_out_buf.u4_bytes = 0;
        ps_video_encode_ip->s_out_buf.u4_bufsize = buff_size;

        /******************************************************************************/
        /****************** Recon Initialization **************************************/
        /******************************************************************************/
        init_raw_buf_descr(ps_app_ctxt, &s_recon_buf, ps_app_ctxt->as_recon_buf[0].pu1_buf, ps_app_ctxt->e_recon_color_fmt);

        if(ps_app_ctxt->u4_psnr_enable)
            init_raw_buf_descr(ps_app_ctxt, &s_inp_buf, ps_app_ctxt->pu1_psnr_buf, ps_app_ctxt->e_inp_color_fmt);

        ps_video_encode_ip->s_recon_buf = s_recon_buf;

        /******************************************************************************/
        /************************* Un Initialized *************************************/
        /******************************************************************************/
        if(0 == ps_app_ctxt->u4_loopback)
        {
            /* If input file is read completely and loopback is not enabled,
             *  then exit the loop */
            if(feof(ps_app_ctxt->fp_ip))
            {
                is_last = 1;
            }
        }


        /* If last frame, send input null to get back encoded frames */
        if ( is_last == 1 || ((ps_app_ctxt->u4_max_num_frms) <= u4_timestamp_low) )
        {
            is_last = 1;
            ps_inp_raw_buf->apv_bufs[0] = NULL;
            ps_inp_raw_buf->apv_bufs[1] = NULL;
            ps_inp_raw_buf->apv_bufs[2] = NULL;
        }

        ps_video_encode_ip->u4_is_last = is_last;
        ps_video_encode_ip->u4_mb_info_type = ps_app_ctxt->u4_mb_info_type;
        ps_video_encode_ip->u4_pic_info_type = ps_app_ctxt->u4_pic_info_type;;
        ps_video_encode_op->s_out_buf.pv_buf= NULL;
        ps_video_encode_ip->u4_timestamp_high = u4_timestamp_high;
        ps_video_encode_ip->u4_timestamp_low = u4_timestamp_low;


        GETTIME(&ps_app_ctxt->enc_last_time);

        status = ih264e_api_function(ps_enc, &ih264e_video_encode_ip, &ih264e_video_encode_op);

        if (IV_SUCCESS != status)
        {
            printf("Encode Frame failed = 0x%x\n", ih264e_video_encode_op.s_ive_op.u4_error_code);
            break;
        }

#ifdef WINDOWS_TIMER
        QueryPerformanceFrequency ( &frequency);
#endif
        GETTIME(&curtime);
        ELAPSEDTIME(ps_app_ctxt->enc_last_time, curtime, timetaken, frequency);
        ps_app_ctxt->enc_last_time = curtime;

#ifdef PROFILE_ENABLE
        {
            WORD32 peak_avg, id;
            u4_total_time += timetaken;
            peak_window[peak_window_idx++] = timetaken;
            if(peak_window_idx == PEAK_WINDOW_SIZE)
                peak_window_idx = 0;
            peak_avg = 0;
            for(id = 0; id < PEAK_WINDOW_SIZE; id++)
            {
                peak_avg += peak_window[id];
            }
            peak_avg /= PEAK_WINDOW_SIZE;
            if (peak_avg > peak_avg_max)
                peak_avg_max = peak_avg;
        }
#endif

        /******************************************************************************/
        /****************** Writing Output ********************************************/
        /******************************************************************************/
        num_bytes = 0;

        if(1 == ps_video_encode_op->output_present)
        {
            num_bytes = ps_video_encode_op->s_out_buf.u4_bytes;
            buff_size = ps_video_encode_op->s_out_buf.u4_bufsize;
            pu1_buf = (UWORD8*)ps_video_encode_op->s_out_buf.pv_buf;

            status = write_output(ps_app_ctxt->fp_op, pu1_buf, num_bytes);
            if(IV_SUCCESS != status)
            {
                printf("Error: Unable to write to output file\n");
                break;
            }
        }

        /* free input bufer if codec returns a valid input buffer */
        if (ps_video_encode_op->s_inp_buf.apv_bufs[0])
        {
            /* Reuse of freed input buffer */
            for(i = 0; i < DEFAULT_MAX_INPUT_BUFS; i++)
            {
                if(ps_app_ctxt->as_input_buf[i].pu1_buf == ps_video_encode_op->s_inp_buf.apv_bufs[0])
                {
                    ps_app_ctxt->as_input_buf[i].u4_is_free = 1;
                    break;
                }
            }
        }

        /* free output buffer if codec returns a valid output buffer */
        // if(ps_video_encode_op->s_out_buf.pv_buf)
        {
            for(i = 0; i < DEFAULT_MAX_OUTPUT_BUFS; i++)
            {
                if(ps_app_ctxt->as_output_buf[i].pu1_buf == ps_video_encode_op->s_out_buf.pv_buf)
                {
                    ps_app_ctxt->as_output_buf[i].u4_is_free = 1;
                    break;
                }
            }
        }

        /**********************************************************************
         *  Print stats
         **********************************************************************/
        {
            UWORD8 u1_pic_type[][5] =
                { "IDR", "I", "P", "B", "NA" };
            WORD32 lookup_idx = 0;

            if (ih264e_video_encode_op.s_ive_op.u4_encoded_frame_type
                            == IV_IDR_FRAME)
            {
                lookup_idx = 0;
            }
            else if(ih264e_video_encode_op.s_ive_op.u4_encoded_frame_type
                            == IV_I_FRAME)
            {
                lookup_idx = 1;
            }
            else if(ih264e_video_encode_op.s_ive_op.u4_encoded_frame_type
                            == IV_P_FRAME)
            {
                lookup_idx = 2;
            }
            else if(ih264e_video_encode_op.s_ive_op.u4_encoded_frame_type
                            == IV_B_FRAME)
            {
                lookup_idx = 3;
            }
            else if(ih264e_video_encode_op.s_ive_op.u4_encoded_frame_type
                            == IV_NA_FRAME)
            {
                lookup_idx = 4;
            }

            if (ih264e_video_encode_op.s_ive_op.u4_encoded_frame_type
                            != IV_NA_FRAME)
            {
                ps_app_ctxt->u4_pics_cnt++;
                ps_app_ctxt->avg_time = u4_total_time / ps_app_ctxt->u4_pics_cnt;
                ps_app_ctxt->u4_total_bytes += num_bytes;
            }

            if (ps_app_ctxt->u4_psnr_enable == 0)
            {
                printf("[%s] PicNum %4d Bytes Generated %6d TimeTaken(microsec): %6d AvgTime: %6d PeakAvgTimeMax: %6d\n",
                       u1_pic_type[lookup_idx], ps_app_ctxt->u4_pics_cnt,
                       num_bytes, timetaken, ps_app_ctxt->avg_time,
                       peak_avg_max);
            }
        }


        /* For psnr computation, we need to read the correct input frame and
         * compare with recon. The difficulty with doing it is that we only know
         * that the frame number of recon is monotonically increasing. There
         * may be gaps in the recon if any pre or post enc skip happens. There are
         * 3 senarios
         *  1) A frame is encoded -> returns the pic type
         *  2) A frame is not encoded -> Encoder is waiting, the frame may get
         *     encoded later
         *  3) A frame is not encoded -> A post enc or pre enc skip happend. The
         *     frame is not going to be encoded
         *
         *     The 1st and 2nd scenarios are easy, since we just needs to increment
         *     recon cnt whenever we get a valid recon. This cnt can we used to
         *     sync the recon and input
         *     3rd scenario in conjuction with 2nd will pose problems. Even if
         *     the returning frame is NA, we donot know we should increment the
         *     recon cnt or not becasue it can be case 2 or case 3.
         *
         *  Solutions:
         *  -------------------------
         *   One way to over come this will be to return more information as of
         *   the frame type. We can send if a frame was skipped as a part of the
         *   return frame type.
         *   This will not work. Since the output and recon are not in sync, we
         *   cannot use the current output frame type to determine if a recon
         *   is present currently or not. We need some other way to acheive this.
         *
         *   Other way to do this which is cleaner and maintains the seperation
         *   between recon and the ouptut is to set the width [& height] of output recon
         *   buffer to be zero. Hence we will in effect be saying :"look there
         *   is a recon, but due to frame not being encoded it is having a width 0".
         *   To be more clear we need to make height also to be zero.
         *
         *   But are we using these variables for allocating and deallocating
         *   the buffers some where ? No we are not. The buffer gets re-init
         *   at every encode call
         *
         *   Fixes
         *   ------------------------
         *   Currently the recon buff width and height are set in the encoder.
         *   This will not work now because since recon and input are not
         *   in sync. Hence a recon buff sent at time stamp x will get used to
         *   fill recon of input at time stamp y (x > y). If we reduced the
         *   frame dimensions in between, the recon buffer will not have enough
         *   space. Hence we need to set the with and height appropriatley inside
         *   lib itself.
         */

        if (ps_app_ctxt->u4_recon_enable || ps_app_ctxt->u4_chksum_enable
                        || ps_app_ctxt->u4_psnr_enable)
        {
            if (ps_video_encode_op->dump_recon)
            {
                s_recon_buf = ps_video_encode_op->s_recon_buf;

                /* Read input for psnr computuation */
                if (ps_app_ctxt->u4_psnr_enable)
                    read_input(ps_app_ctxt->fp_psnr_ip, &s_inp_buf);

                /* if we have a valid recon buffer do the assocated tasks */
                if (s_recon_buf.au4_wd[0])
                {
                    /* Dump recon when enabled, and output bytes != 0 */
                    if (ps_app_ctxt->u4_recon_enable)
                    {
                        status = write_recon(ps_app_ctxt->fp_recon, &s_recon_buf);
                        if (IV_SUCCESS != status)
                        {
                            printf("Error: Unable to write to recon file\n");
                            break;
                        }
                    }

                    if (ps_app_ctxt->u4_psnr_enable)
                    {
                        compute_psnr(ps_app_ctxt, &s_recon_buf, &s_inp_buf);
                    }


                    if (ps_app_ctxt->u4_chksum_enable)
                    {
                        WORD32 comp, num_comp = 2;

                        if (IV_YUV_420P == s_recon_buf.e_color_fmt)
                            num_comp = 3;

                        for (comp = 0; comp < num_comp; comp++)
                        {
                            UWORD8 au1_chksum[16];
                            calc_md5_cksum((UWORD8 *)s_recon_buf.apv_bufs[comp],
                                           s_recon_buf.au4_strd[comp],
                                           s_recon_buf.au4_wd[comp],
                                           s_recon_buf.au4_ht[comp],
                                           au1_chksum);
                            fwrite(au1_chksum, sizeof(UWORD8), 16, ps_app_ctxt->fp_chksum);
                        }
                    }
                }
            }
        }

        u4_timestamp_low++;

        /* Break if all the encoded frames are taken from encoder */
        if (1 == ps_video_encode_op->u4_is_last)
        {
            break;
        }
    }

    /* Pic count is 1 more than actual num frames encoded, because last call is to just get the output  */
    ps_app_ctxt->u4_pics_cnt--;

    if(ps_app_ctxt->u4_psnr_enable)
    {
        print_average_psnr(ps_app_ctxt);
    }

    /* house keeping operations */
    fclose(ps_app_ctxt->fp_ip);
    fclose(ps_app_ctxt->fp_op);
    if(1 == ps_app_ctxt->u4_recon_enable)
    {
        fclose(ps_app_ctxt->fp_recon);
    }
    if(1 == ps_app_ctxt->u4_chksum_enable)
    {
        fclose(ps_app_ctxt->fp_chksum);
    }
    if(1 == ps_app_ctxt->u4_psnr_enable)
    {
        fclose(ps_app_ctxt->fp_psnr_ip);
    }

    if(0 != ps_app_ctxt->u4_mb_info_type)
    {
        fclose(ps_app_ctxt->fp_mb_info);
    }
    if (ps_app_ctxt->u4_pic_info_type)
    {
        fclose(ps_app_ctxt->fp_pic_info);
    }

    free_input(ps_app_ctxt);
    free_output(ps_app_ctxt);
    free_recon(ps_app_ctxt);
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : main                                                     */
/*                                                                           */
/*  Description   : Application to demonstrate codec API                     */
/*                                                                           */
/*                                                                           */
/*  Inputs        : argc    - Number of arguments                            */
/*                  argv[]  - Arguments                                      */
/*  Globals       :                                                          */
/*  Processing    : Shows how to use create, process, control and delete     */
/*                                                                           */
/*  Outputs       : Codec output in a file                                   */
/*  Returns       :                                                          */
/*                                                                           */
/*  Issues        : Assumes both PROFILE_ENABLE to be                        */
/*                  defined for multithread decode-display working           */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes                              */
/*         20 11 2013   100189          Initial Version                      */
/*****************************************************************************/
#ifdef IOS
int h264enc_main(char * homedir,char *documentdir, int screen_wd, int screen_ht)
#else
int main(int argc, char *argv[])
#endif
{
    /* Config Parameters for Encoding */
    app_ctxt_t s_app_ctxt;

    /* error string */
    CHAR ac_error[STRLENGTH];

    /* config file name */
    CHAR ac_cfg_fname[STRLENGTH];

    /* error status */
    IV_STATUS_T status = IV_SUCCESS;
#ifdef IOS
    /* temp var */
    CHAR filename_with_path[STRLENGTH];
#endif
    WORD32 num_mem_recs;
    iv_obj_t *ps_enc;
    WORD32 i;
    FILE *fp_cfg = NULL;

#ifdef X86_MINGW

    /* For getting printfs without any delay in eclipse */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

#endif

    init_default_params(&s_app_ctxt);

#ifndef IOS

    /* Usage */
    if(argc < 2)
    {
        printf("Using enc.cfg as configuration file \n");
        strcpy(ac_cfg_fname, "enc.cfg");
    }
    else if(argc == 2)
    {
        if (!strcmp(argv[1], "--help"))
        {
            print_usage();
            exit(-1);
        }
        strcpy(ac_cfg_fname, argv[1]);
    }

#else
    strcpy(ac_cfg_fname, "test.cfg");

#endif

    /*************************************************************************/
    /* Parse arguments                                                       */
    /*************************************************************************/

#ifndef IOS

    /* Read command line arguments */
    if(argc > 2)
    {
        for(i = 1; i + 1 < argc; i += 2)
        {
            if(CONFIG == get_argument(argv[i]))
            {
                strcpy(ac_cfg_fname, argv[i + 1]);
                if((fp_cfg = fopen(ac_cfg_fname, "r")) == NULL)
                {
                    sprintf(ac_error,
                            "Could not open Configuration file %s",
                            ac_cfg_fname);
                    codec_exit(ac_error);
                }
                read_cfg_file(&s_app_ctxt, fp_cfg);
                fclose(fp_cfg);
            }
            else
            {
                parse_argument(&s_app_ctxt, argv[i], argv[i + 1]);
            }
        }
    }
    else
    {
        if((fp_cfg = fopen(ac_cfg_fname, "r")) == NULL)
        {
            sprintf(ac_error, "Could not open Configuration file %s",
                    ac_cfg_fname);
            codec_exit(ac_error);
        }
        read_cfg_file(&s_app_ctxt, fp_cfg);
        fclose(fp_cfg);
    }

#else

    sprintf(filename_with_path, "%s/%s", homedir, "enc.cfg");
    if((fp_cfg = fopen(filename_with_path, "r")) == NULL)
    {
        sprintf(ac_error, "Could not open Configuration file %s",
                ac_cfg_fname);
        codec_exit(ac_error);

    }
    read_cfg_file(&s_app_ctxt, fp_cfg);
    fclose(fp_cfg);

#endif


    validate_params(&s_app_ctxt);


    /*************************************************************************/
    /*                      Getting Number of MemRecords                     */
    /*************************************************************************/
    {
        ih264e_num_mem_rec_ip_t s_num_mem_rec_ip;
        ih264e_num_mem_rec_op_t s_num_mem_rec_op;

        s_num_mem_rec_ip.s_ive_ip.u4_size = sizeof(ih264e_num_mem_rec_ip_t);
        s_num_mem_rec_op.s_ive_op.u4_size = sizeof(ih264e_num_mem_rec_op_t);

        s_num_mem_rec_ip.s_ive_ip.e_cmd = IV_CMD_GET_NUM_MEM_REC;

        status = ih264e_api_function(0, &s_num_mem_rec_ip, &s_num_mem_rec_op);

        if(status != IV_SUCCESS)
        {
            sprintf(ac_error, "Get number of memory records failed = 0x%x\n", s_num_mem_rec_op.s_ive_op.u4_error_code);
            codec_exit(ac_error);
        }

        s_app_ctxt.u4_num_mem_rec = num_mem_recs = s_num_mem_rec_op.s_ive_op.u4_num_mem_rec;
    }

    /* Allocate array to hold memory records */
    s_app_ctxt.ps_mem_rec = (iv_mem_rec_t *) malloc(num_mem_recs * sizeof(iv_mem_rec_t));
    if(NULL == s_app_ctxt.ps_mem_rec)
    {

        sprintf(ac_error, "Unable to allocate memory for hold memory records: Size %d", (WORD32)(num_mem_recs * sizeof(iv_mem_rec_t)));
        codec_exit(ac_error);
    }

    {
        iv_mem_rec_t *ps_mem_rec;
        ps_mem_rec = s_app_ctxt.ps_mem_rec;
        for(i = 0; i < num_mem_recs; i++)
        {
            ps_mem_rec->u4_size = sizeof(iv_mem_rec_t);
            ps_mem_rec->pv_base = NULL;
            ps_mem_rec->u4_mem_size = 0;
            ps_mem_rec->u4_mem_alignment = 0;
            ps_mem_rec->e_mem_type = IV_NA_MEM_TYPE;

            ps_mem_rec++;
        }
    }

    /*************************************************************************/
    /*                      Getting MemRecords Attributes                    */
    /*************************************************************************/
    {
        ih264e_fill_mem_rec_ip_t s_fill_mem_rec_ip;
        ih264e_fill_mem_rec_op_t s_fill_mem_rec_op;

        s_fill_mem_rec_ip.s_ive_ip.u4_size = sizeof(ih264e_fill_mem_rec_ip_t);
        s_fill_mem_rec_op.s_ive_op.u4_size = sizeof(ih264e_fill_mem_rec_op_t);

        s_fill_mem_rec_ip.s_ive_ip.e_cmd = IV_CMD_FILL_NUM_MEM_REC;
        s_fill_mem_rec_ip.s_ive_ip.ps_mem_rec = s_app_ctxt.ps_mem_rec;
        s_fill_mem_rec_ip.s_ive_ip.u4_num_mem_rec = s_app_ctxt.u4_num_mem_rec;
        s_fill_mem_rec_ip.s_ive_ip.u4_max_wd = s_app_ctxt.u4_max_wd;
        s_fill_mem_rec_ip.s_ive_ip.u4_max_ht = s_app_ctxt.u4_max_ht;
        s_fill_mem_rec_ip.s_ive_ip.u4_max_level = s_app_ctxt.u4_max_level;
        s_fill_mem_rec_ip.s_ive_ip.e_color_format = DEFAULT_INP_COLOR_FMT;
        s_fill_mem_rec_ip.s_ive_ip.u4_max_ref_cnt = DEFAULT_MAX_REF_FRM;
        s_fill_mem_rec_ip.s_ive_ip.u4_max_reorder_cnt = DEFAULT_MAX_REORDER_FRM;
        s_fill_mem_rec_ip.s_ive_ip.u4_max_srch_rng_x = DEFAULT_MAX_SRCH_RANGE_X;
        s_fill_mem_rec_ip.s_ive_ip.u4_max_srch_rng_y = DEFAULT_MAX_SRCH_RANGE_Y;

        status = ih264e_api_function(0, &s_fill_mem_rec_ip, &s_fill_mem_rec_op);

        if(status != IV_SUCCESS)
        {
            sprintf(ac_error, "Fill memory records failed = 0x%x\n",
                    s_fill_mem_rec_op.s_ive_op.u4_error_code);
            codec_exit(ac_error);
        }
    }

    /*************************************************************************/
    /*                      Allocating Memory for Mem Records                */
    /*************************************************************************/
    {
        WORD32 total_size;
        iv_mem_rec_t *ps_mem_rec;
        total_size = 0;

        ps_mem_rec = s_app_ctxt.ps_mem_rec;
        for(i = 0; i < num_mem_recs; i++)
        {
            ps_mem_rec->pv_base = ih264a_aligned_malloc(ps_mem_rec->u4_mem_alignment,
                                           ps_mem_rec->u4_mem_size);
            if(ps_mem_rec->pv_base == NULL)
            {
                sprintf(ac_error, "Allocation failure for mem record id %d size %d\n",
                        i, ps_mem_rec->u4_mem_size);
                codec_exit(ac_error);
            }
            total_size += ps_mem_rec->u4_mem_size;

            ps_mem_rec++;
        }
        printf("\nTotal memory for codec %d\n", total_size);
    }


    /*************************************************************************/
    /*                        Codec Instance Creation                        */
    /*************************************************************************/
    {
        ih264e_init_ip_t s_init_ip;
        ih264e_init_op_t s_init_op;

        ps_enc = s_app_ctxt.ps_mem_rec[0].pv_base;
        ps_enc->u4_size = sizeof(iv_obj_t);
        ps_enc->pv_fxns = ih264e_api_function;
        s_app_ctxt.ps_enc = ps_enc;

        s_init_ip.s_ive_ip.u4_size = sizeof(ih264e_init_ip_t);
        s_init_op.s_ive_op.u4_size = sizeof(ih264e_init_op_t);

        s_init_ip.s_ive_ip.e_cmd = IV_CMD_INIT;
        s_init_ip.s_ive_ip.u4_num_mem_rec = s_app_ctxt.u4_num_mem_rec;
        s_init_ip.s_ive_ip.ps_mem_rec = s_app_ctxt.ps_mem_rec;
        s_init_ip.s_ive_ip.u4_max_wd = s_app_ctxt.u4_max_wd;
        s_init_ip.s_ive_ip.u4_max_ht = s_app_ctxt.u4_max_ht;
        s_init_ip.s_ive_ip.u4_max_ref_cnt = DEFAULT_MAX_REF_FRM;
        s_init_ip.s_ive_ip.u4_max_reorder_cnt = DEFAULT_MAX_REORDER_FRM;
        s_init_ip.s_ive_ip.u4_max_level = s_app_ctxt.u4_max_level;
        s_init_ip.s_ive_ip.e_inp_color_fmt = s_app_ctxt.e_inp_color_fmt;
        if(s_app_ctxt.u4_recon_enable || s_app_ctxt.u4_psnr_enable || s_app_ctxt.u4_chksum_enable)
        {
            s_init_ip.s_ive_ip.u4_enable_recon           = 1;
        }
        else
        {
            s_init_ip.s_ive_ip.u4_enable_recon           = 0;
        }
        s_init_ip.s_ive_ip.e_recon_color_fmt    = s_app_ctxt.e_recon_color_fmt;
        s_init_ip.s_ive_ip.e_rc_mode            = s_app_ctxt.u4_rc;
        s_init_ip.s_ive_ip.u4_max_framerate     = s_app_ctxt.u4_max_frame_rate;
        s_init_ip.s_ive_ip.u4_max_bitrate       = s_app_ctxt.u4_max_bitrate;
        s_init_ip.s_ive_ip.u4_num_bframes       = s_app_ctxt.u4_num_bframes;
        s_init_ip.s_ive_ip.e_content_type       = IV_PROGRESSIVE;
        s_init_ip.s_ive_ip.u4_max_srch_rng_x    = DEFAULT_MAX_SRCH_RANGE_X;
        s_init_ip.s_ive_ip.u4_max_srch_rng_y    = DEFAULT_MAX_SRCH_RANGE_Y;
        s_init_ip.s_ive_ip.e_slice_mode         = s_app_ctxt.u4_slice_mode;
        s_init_ip.s_ive_ip.u4_slice_param       = s_app_ctxt.u4_slice_param;
        s_init_ip.s_ive_ip.e_arch               = s_app_ctxt.e_arch;
        s_init_ip.s_ive_ip.e_soc                = s_app_ctxt.e_soc;

        status = ih264e_api_function(ps_enc, &s_init_ip, &s_init_op);

        if(status != IV_SUCCESS)
        {
            sprintf(ac_error, "Init memory records failed = 0x%x\n",
                    s_init_op.s_ive_op.u4_error_code);
            codec_exit(ac_error);
        }
    }

    /*************************************************************************/
    /*                        set processor details                          */
    /*************************************************************************/
    {
        ih264e_ctl_set_num_cores_ip_t s_ctl_set_num_cores_ip;
        ih264e_ctl_set_num_cores_op_t s_ctl_set_num_cores_op;
        s_ctl_set_num_cores_ip.s_ive_ip.e_cmd = IVE_CMD_VIDEO_CTL;
        s_ctl_set_num_cores_ip.s_ive_ip.e_sub_cmd = IVE_CMD_CTL_SET_NUM_CORES;
        s_ctl_set_num_cores_ip.s_ive_ip.u4_num_cores = s_app_ctxt.u4_num_cores;
        s_ctl_set_num_cores_ip.s_ive_ip.u4_timestamp_high = 0;
        s_ctl_set_num_cores_ip.s_ive_ip.u4_timestamp_low = 0;
        s_ctl_set_num_cores_ip.s_ive_ip.u4_size = sizeof(ih264e_ctl_set_num_cores_ip_t);

        s_ctl_set_num_cores_op.s_ive_op.u4_size = sizeof(ih264e_ctl_set_num_cores_op_t);

        status = ih264e_api_function(ps_enc, (void *) &s_ctl_set_num_cores_ip,
                (void *) &s_ctl_set_num_cores_op);
        if(status != IV_SUCCESS)
        {
            sprintf(ac_error, "Unable to set processor params = 0x%x\n",
                    s_ctl_set_num_cores_op.s_ive_op.u4_error_code);
            codec_exit(ac_error);
        }

    }

    /*************************************************************************/
    /*                        Get Codec Version                              */
    /*************************************************************************/
    {
        ih264e_ctl_getversioninfo_ip_t s_ctl_set_getversioninfo_ip;
        ih264e_ctl_getversioninfo_op_t s_ctl_set_getversioninfo_op;
        CHAR ac_version_string[STRLENGTH];
        s_ctl_set_getversioninfo_ip.s_ive_ip.e_cmd = IVE_CMD_VIDEO_CTL;
        s_ctl_set_getversioninfo_ip.s_ive_ip.e_sub_cmd = IVE_CMD_CTL_GETVERSION;
        s_ctl_set_getversioninfo_ip.s_ive_ip.pu1_version = (UWORD8 *)ac_version_string;
        s_ctl_set_getversioninfo_ip.s_ive_ip.u4_version_bufsize = sizeof(ac_version_string);
        s_ctl_set_getversioninfo_ip.s_ive_ip.u4_size = sizeof(ih264e_ctl_getversioninfo_ip_t);
        s_ctl_set_getversioninfo_op.s_ive_op.u4_size = sizeof(ih264e_ctl_getversioninfo_op_t);

        status = ih264e_api_function(ps_enc, (void *) &s_ctl_set_getversioninfo_ip,
                (void *) &s_ctl_set_getversioninfo_op);
        if(status != IV_SUCCESS)
        {
            sprintf(ac_error, "Unable to get codec version = 0x%x\n",
                    s_ctl_set_getversioninfo_op.s_ive_op.u4_error_code);
            codec_exit(ac_error);
        }
        printf("CODEC VERSION %s\n", ac_version_string);
    }

    /*************************************************************************/
    /*                      Get I/O Buffer Requirement                       */
    /*************************************************************************/
    {
        ih264e_ctl_getbufinfo_ip_t s_get_buf_info_ip;
        ih264e_ctl_getbufinfo_op_t s_get_buf_info_op;

        s_get_buf_info_ip.s_ive_ip.u4_size = sizeof(ih264e_ctl_getbufinfo_ip_t);
        s_get_buf_info_op.s_ive_op.u4_size = sizeof(ih264e_ctl_getbufinfo_op_t);

        s_get_buf_info_ip.s_ive_ip.e_cmd = IVE_CMD_VIDEO_CTL;
        s_get_buf_info_ip.s_ive_ip.e_sub_cmd = IVE_CMD_CTL_GETBUFINFO;
        s_get_buf_info_ip.s_ive_ip.u4_max_ht = s_app_ctxt.u4_max_ht;
        s_get_buf_info_ip.s_ive_ip.u4_max_wd = s_app_ctxt.u4_max_wd;
        s_get_buf_info_ip.s_ive_ip.e_inp_color_fmt = s_app_ctxt.e_inp_color_fmt;

        status = ih264e_api_function(ps_enc, &s_get_buf_info_ip, &s_get_buf_info_op);

        if (status != IV_SUCCESS)
        {
            sprintf(ac_error, "Unable to get I/O buffer requirements = 0x%x\n",
                    s_get_buf_info_op.s_ive_op.u4_error_code);
            codec_exit(ac_error);
        }
        s_app_ctxt.s_get_buf_info_op = s_get_buf_info_op;
    }

    /*****************************************************************************/
    /* Add the following initializations based on the parameters in context      */
    /*****************************************************************************/


    /*****************************************************************************/
    /*   Video control  Set Frame dimensions                                     */
    /*****************************************************************************/
    s_app_ctxt.u4_strd = s_app_ctxt.u4_wd;
    set_dimensions(&s_app_ctxt, 0, 0);

    /*****************************************************************************/
    /*   Video control  Set Frame rates                                          */
    /*****************************************************************************/
    set_frame_rate(&s_app_ctxt, 0, 0);

    /*****************************************************************************/
    /*   Video control  Set IPE Params                                           */
    /*****************************************************************************/
    set_ipe_params(&s_app_ctxt, 0, 0);

    /*****************************************************************************/
    /*   Video control  Set Bitrate                                              */
    /*****************************************************************************/
    set_bit_rate(&s_app_ctxt, 0, 0);

    /*****************************************************************************/
    /*   Video control  Set QP                                                   */
    /*****************************************************************************/
    set_qp(&s_app_ctxt,0,0);

    /*****************************************************************************/
    /*   Video control  Set AIR params                                           */
    /*****************************************************************************/
    set_air_params(&s_app_ctxt,0,0);

    /*****************************************************************************/
    /*   Video control  Set VBV params                                           */
    /*****************************************************************************/
    set_vbv_params(&s_app_ctxt,0,0);

    /*****************************************************************************/
    /*   Video control  Set Motion estimation params                             */
    /*****************************************************************************/
    set_me_params(&s_app_ctxt,0,0);

    /*****************************************************************************/
    /*   Video control  Set GOP params                                           */
    /*****************************************************************************/
    set_gop_params(&s_app_ctxt, 0, 0);

    /*****************************************************************************/
    /*   Video control  Set Deblock params                                       */
    /*****************************************************************************/
    set_deblock_params(&s_app_ctxt, 0, 0);

    /*****************************************************************************/
    /*   Video control  Set Profile params                                       */
    /*****************************************************************************/
    set_profile_params(&s_app_ctxt, 0, 0);

    /*****************************************************************************/
    /*   Video control Set in Encode header mode                                 */
    /*****************************************************************************/
    set_enc_mode(&s_app_ctxt, 0, 0, IVE_ENC_MODE_PICTURE);

#ifdef IOS
    /* Correct file paths */
    sprintf(filename_with_path, "%s/%s", documentdir, s_app_ctxt.ac_ip_fname);
    strcpy (s_app_ctxt.ac_ip_fname, filename_with_path);

    sprintf(filename_with_path, "%s/%s", documentdir, s_app_ctxt.ac_op_fname);
    strcpy (s_app_ctxt.ac_op_fname, filename_with_path);

    sprintf(filename_with_path, "%s/%s", documentdir, s_app_ctxt.ac_recon_fname);
    strcpy (s_app_ctxt.ac_recon_fname, filename_with_path);

    sprintf(filename_with_path, "%s/%s", documentdir, s_app_ctxt.ac_chksum_fname);
    strcpy (s_app_ctxt.ac_chksum_fname, filename_with_path);

    sprintf(filename_with_path, "%s/%s", documentdir, s_app_ctxt.ac_mb_info_fname);
    strcpy (s_app_ctxt.ac_mb_info_fname, filename_with_path);

    sprintf(filename_with_path, "%s/%s", documentdir, s_app_ctxt.ac_pic_info_fname);
    strcpy (s_app_ctxt.ac_pic_info_fname, filename_with_path);
#endif

    /*************************************************************************/
    /*               begin encoding                                          */
    /*************************************************************************/

    synchronous_encode(ps_enc, &s_app_ctxt);

    {
        DOUBLE bytes_per_frame;
        DOUBLE bytes_per_second;
        WORD32 achieved_bitrate;

        if(s_app_ctxt.u4_pics_cnt != 0)
        {
            bytes_per_frame = (s_app_ctxt.u4_total_bytes) / (s_app_ctxt.u4_pics_cnt);
        }
        else
        {
            bytes_per_frame = 0;
        }
        bytes_per_second = (bytes_per_frame * s_app_ctxt.u4_tgt_frame_rate);

        achieved_bitrate = bytes_per_second * 8;

        printf("\nEncoding Completed\n");
        printf("Summary\n");
        printf("Input filename                  : %s\n", s_app_ctxt.ac_ip_fname);
        printf("Output filename                 : %s\n", s_app_ctxt.ac_op_fname);
        printf("Output Width                    : %-4d\n", s_app_ctxt.u4_wd);
        printf("Output Height                   : %-4d\n", s_app_ctxt.u4_ht);
        printf("Target Bitrate (bps)            : %-4d\n", s_app_ctxt.u4_bitrate);
        printf("Achieved Bitrate (bps)          : %-4d\n", achieved_bitrate);
        printf("Average Time per Frame          : %-4d\n", s_app_ctxt.avg_time);
        printf("Achieved FPS                    : %-4.2f\n", 1000000.0 / s_app_ctxt.avg_time);
    }


    /*************************************************************************/
    /*                         Close Codec Instance                         */
    /*************************************************************************/
    {
        ih264e_retrieve_mem_rec_ip_t s_retrieve_mem_ip;
        ih264e_retrieve_mem_rec_op_t s_retrieve_mem_op;
        iv_mem_rec_t *ps_mem_rec;
        s_retrieve_mem_ip.s_ive_ip.u4_size =
                        sizeof(ih264e_retrieve_mem_rec_ip_t);
        s_retrieve_mem_op.s_ive_op.u4_size =
                        sizeof(ih264e_retrieve_mem_rec_op_t);

        s_retrieve_mem_ip.s_ive_ip.e_cmd = IV_CMD_RETRIEVE_MEMREC;
        s_retrieve_mem_ip.s_ive_ip.ps_mem_rec = s_app_ctxt.ps_mem_rec;

        status = ih264e_api_function(ps_enc, &s_retrieve_mem_ip,
                                          &s_retrieve_mem_op);

        if(status != IV_SUCCESS)
        {
            sprintf(ac_error, "Unable to retrieve memory records = 0x%x\n",
                    s_retrieve_mem_op.s_ive_op.u4_error_code);
            codec_exit(ac_error);
        }

        /* Free memory records */
        ps_mem_rec = s_app_ctxt.ps_mem_rec;
        for(i = 0; i < num_mem_recs; i++)
        {
            ih264a_aligned_free(ps_mem_rec->pv_base);
            ps_mem_rec++;
        }

        free(s_app_ctxt.ps_mem_rec);

    }

    return 0;
}


#ifdef  ANDROID_NDK
int raise(int a)
{
    printf("Divide by zero\n");
    return 0;
}
void __aeabi_assert(const char *assertion, const char *file, unsigned int line)
{
    return;
}
#endif
