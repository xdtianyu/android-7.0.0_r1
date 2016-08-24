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
*  ih264e_version.c
*
* @brief
*  Contains version info for H264 encoder
*
* @author
*  ittiam
*
* @par List of Functions:
* - ih264e_get_version()
*
* @remarks
*  None
*
*******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
/* system include files */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* user include files */
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ih264e.h"
#include "ih264_defs.h"
#include "ih264_debug.h"
#include "ih264_structs.h"
#include "ih264e_version.h"


/*****************************************************************************/
/* Constant Macros                                                           */
/*****************************************************************************/

/**
 * Name of the codec and target platform (All Cortex A processors in this case)
 */
#define CODEC_NAME              "H264ENC"
/**
 * Codec release type, production or evaluation
 */
#define CODEC_RELEASE_TYPE      "production"
/**
 * Version string. First two digits signify major version and last two minor
 */
#define CODEC_RELEASE_VER       "01.00"
/**
 * Vendor name
 */
#define CODEC_VENDOR            "ITTIAM"

#define MAX_STRLEN              511
/**
*******************************************************************************
* Concatenates various strings to form a version string
*******************************************************************************
*/
#ifdef __ANDROID__
#define VERSION(version_string, codec_name, codec_release_type, codec_release_ver, codec_vendor)    \
    snprintf(version_string, MAX_STRLEN,                                                            \
             "@(#)Id:%s_%s Ver:%s Released by %s",                                                  \
             codec_name, codec_release_type, codec_release_ver, codec_vendor)
#else
#define VERSION(version_string, codec_name, codec_release_type, codec_release_ver, codec_vendor)    \
    snprintf(version_string, MAX_STRLEN,                                                            \
             "@(#)Id:%s_%s Ver:%s Released by %s Build: %s @ %s",                                   \
             codec_name, codec_release_type, codec_release_ver, codec_vendor, __DATE__, __TIME__)
#endif

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief
*  Fills the version info in the given char pointer
*
* @par Description:
*  Fills the version info in the given char pointer
*
* @param[in] pc_version
*  Pointer to hold version info
*
* @param[in] u4_version_bufsize
*  Size of the buffer passed
*
* @returns error status
*
* @remarks none
*
*******************************************************************************
*/
IV_STATUS_T ih264e_get_version(CHAR *pc_version, UWORD32 u4_version_bufsize)
{
    CHAR ac_version_tmp[MAX_STRLEN];

    VERSION(ac_version_tmp, CODEC_NAME, CODEC_RELEASE_TYPE, CODEC_RELEASE_VER,
            CODEC_VENDOR);

    if (u4_version_bufsize >= (strlen(ac_version_tmp) + 1))
    {
        memcpy(pc_version, ac_version_tmp, (strlen(ac_version_tmp) + 1));
        return IV_SUCCESS;
    }
    else
    {
        return IV_FAIL;
    }
}
