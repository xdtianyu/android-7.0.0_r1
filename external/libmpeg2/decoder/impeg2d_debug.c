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
#include <string.h>
#include <stdio.h>

#include "iv_datatypedef.h"
#include "iv.h"
#include "ivd.h"
#include "impeg2d.h"

#include "impeg2_buf_mgr.h"
#include "impeg2_disp_mgr.h"
#include "impeg2_macros.h"
#include "impeg2_defs.h"
#include "impeg2_platform_macros.h"
#include "impeg2_inter_pred.h"
#include "impeg2_idct.h"
#include "impeg2_format_conv.h"
#include "impeg2_mem_func.h"
#include "impeg2_globals.h"

#include "impeg2d_bitstream.h"
#include "impeg2d_api.h"
#include "impeg2d_structs.h"
#include "impeg2d_debug.h"

#if STATISTICS
WORD32 gai4_impeg2d_idct_inp_last_nonzero_histogram[64] = {0};
WORD32 gai4_impeg2d_idct_inp_num_nonzero_histogram[64] = {0};
WORD32 gai4_impeg2d_idct_inp_last_non_zero_row_histogram[8] = {0};

WORD32 gai4_impeg2d_iqnt_inp_last_nonzero_histogram[64] = {0};
WORD32 gai4_impeg2d_iqnt_inp_num_nonzero_histogram[64] = {0};
WORD32 gai4_impeg2d_iqnt_inp_last_non_zero_row_histogram[8] = {0};

WORD32 gi4_impeg2d_idct_inp_only_first_coeff = 0;
WORD32 gi4_impeg2d_idct_inp_only_last_coeff = 0;
WORD32 gi4_impeg2d_idct_inp_only_first_n_last_coeff = 0;
WORD32 gi4_impeg2d_idct_cnt = 0;


WORD32 gi4_impeg2d_iqnt_inp_only_first_coeff = 0;
WORD32 gi4_impeg2d_iqnt_inp_only_last_coeff = 0;
WORD32 gi4_impeg2d_iqnt_inp_only_first_n_last_coeff = 0;
WORD32 gi4_impeg2d_iqnt_cnt = 0;


void impeg2d_iqnt_inp_statistics(WORD16 *pi2_iqnt_inp,
                                 WORD32 i4_non_zero_cols,
                                 WORD32 i4_non_zero_rows)
{
    WORD32 i, j;
    WORD32 i4_last_row = 0, i4_last_col = 0;
    WORD32 i4_num_non_zero = 0;
    WORD32 i4_non_zero_cols_computed = 0;
    WORD32 i4_non_zero_rows_computed = 0;

    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(pi2_iqnt_inp[i * 8 + j])
            {
                i4_non_zero_cols_computed |= (1 << j);
                i4_non_zero_rows_computed |= (1 << i);
            }
        }
    }

    if(i4_non_zero_cols_computed != i4_non_zero_cols)
    {
        printf("IQ Input: Invalid non_zero_cols 0x%x non_zero_cols_computed 0x%x\n", i4_non_zero_cols, i4_non_zero_cols_computed);
    }
    if(i4_non_zero_rows_computed != i4_non_zero_rows)
    {
        printf("IQ Input: Invalid non_zero_rows 0x%x non_zero_rows_computed 0x%x\n", i4_non_zero_rows, i4_non_zero_rows_computed);
    }
    {
        WORD32 last_non_zero_row = 32 - CLZ(i4_non_zero_rows);
        gai4_impeg2d_iqnt_inp_last_non_zero_row_histogram[last_non_zero_row - 1]++;
    }
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(pi2_iqnt_inp[i * 8 + j])
            {
                i4_last_col = MAX(i4_last_col, j);
                i4_last_row = MAX(i4_last_row, i);
                i4_num_non_zero++;
            }
        }
    }
    gai4_impeg2d_iqnt_inp_last_nonzero_histogram[i4_last_row * 8 + i4_last_col]++;
    gai4_impeg2d_iqnt_inp_num_nonzero_histogram[i4_num_non_zero]++;
    gi4_impeg2d_iqnt_cnt++;
    /* Check if only (0,0) and (7,7) are non zero */
    if(i4_num_non_zero == 1)
    {
        if(pi2_iqnt_inp[7 * 8 + 7])
            gi4_impeg2d_iqnt_inp_only_last_coeff++;
    }
    if(i4_num_non_zero == 1)
    {
        if(pi2_iqnt_inp[0])
            gi4_impeg2d_iqnt_inp_only_first_coeff++;
    }

    if(i4_num_non_zero == 2)
    {
        if((pi2_iqnt_inp[0]) && (1 == pi2_iqnt_inp[7 * 8 + 7]))
            gi4_impeg2d_iqnt_inp_only_first_n_last_coeff++;
    }
}

void impeg2d_idct_inp_statistics(WORD16 *pi2_idct_inp,
                                 WORD32 i4_non_zero_cols,
                                 WORD32 i4_non_zero_rows)
{
    WORD32 i, j;
    WORD32 i4_last_row = 0, i4_last_col = 0;
    WORD32 i4_num_non_zero = 0;
    WORD32 i4_non_zero_cols_computed = 0;
    WORD32 i4_non_zero_rows_computed = 0;

    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(pi2_idct_inp[i * 8 + j])
            {
                i4_non_zero_cols_computed |= (1 << j);
                i4_non_zero_rows_computed |= (1 << i);
            }
        }
    }

    if(i4_non_zero_cols_computed != i4_non_zero_cols)
    {
        printf("IDCT Input: Invalid non_zero_cols 0x%x non_zero_cols_computed 0x%x\n", i4_non_zero_cols, i4_non_zero_cols_computed);
    }
    if(i4_non_zero_rows_computed != i4_non_zero_rows)
    {
        printf("IDCT Input: Invalid non_zero_rows 0x%x non_zero_rows_computed 0x%x\n", i4_non_zero_rows, i4_non_zero_rows_computed);
    }

    {
        WORD32 last_non_zero_row = 32 - CLZ(i4_non_zero_rows);
        gai4_impeg2d_idct_inp_last_non_zero_row_histogram[last_non_zero_row - 1]++;
    }

    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(pi2_idct_inp[i * 8 + j])
            {
                i4_last_col = MAX(i4_last_col, j);
                i4_last_row = MAX(i4_last_row, i);
                i4_num_non_zero++;
            }
        }
    }
    gai4_impeg2d_idct_inp_last_nonzero_histogram[i4_last_row * 8 + i4_last_col]++;
    gai4_impeg2d_idct_inp_num_nonzero_histogram[i4_num_non_zero]++;
    gi4_impeg2d_idct_cnt++;
    /* Check if only (0,0) and (7,7) are non zero */
    if(i4_num_non_zero == 1)
    {
        if(pi2_idct_inp[7 * 8 + 7])
            gi4_impeg2d_idct_inp_only_last_coeff++;
    }
    if(i4_num_non_zero == 1)
    {
        if(pi2_idct_inp[0])
            gi4_impeg2d_idct_inp_only_first_coeff++;
    }

    if(i4_num_non_zero == 2)
    {
        if((pi2_idct_inp[0]) && (1 == pi2_idct_inp[7 * 8 + 7]))
            gi4_impeg2d_idct_inp_only_first_n_last_coeff++;
    }
}
void impeg2d_print_idct_inp_statistics()
{
    WORD32 i, j;
    WORD32 i4_sum;
    WORD32 i4_accumulator;
    i4_sum = 0;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            i4_sum += gai4_impeg2d_idct_inp_last_nonzero_histogram[i * 8 + j];
        }
    }
    printf("IDCT input : Only last coeff non-zero %8.2f\n", (gi4_impeg2d_idct_inp_only_last_coeff * 100.0) / gi4_impeg2d_idct_cnt);
    printf("IDCT input : Only first coeff non-zero (Includes DC + mismatch) %8.2f\n", (gi4_impeg2d_idct_inp_only_first_coeff * 100.0) / gi4_impeg2d_idct_cnt);

    printf("IDCT input : Last non-zero coeff histogram\n");
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            double val = gai4_impeg2d_idct_inp_last_nonzero_histogram[i * 8 + j] * 100.0 / i4_sum;
            printf("%8.2f \t", val);

        }
        printf("\n");
    }

    printf("IDCT input : Cumulative Last non-zero coeff histogram\n");
    i4_accumulator = 0;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            double val;
            i4_accumulator += gai4_impeg2d_idct_inp_last_nonzero_histogram[i * 8 + j];
            val = i4_accumulator * 100.0 / i4_sum;

            printf("%8.2f \t", val);

        }
        printf("\n");
    }



    printf("IDCT input : Number of non-zero coeff histogram\n");
    i4_sum = 0;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            i4_sum += gai4_impeg2d_idct_inp_num_nonzero_histogram[i * 8 + j];
        }
    }
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            double val = gai4_impeg2d_idct_inp_num_nonzero_histogram[i * 8 + j] * 100.0 / i4_sum;
            printf("%8.2f \t", val);

        }
        printf("\n");
    }

    printf("IDCT input : Cumulative number of non-zero coeffs histogram\n");
    i4_accumulator = 0;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            double val;
            i4_accumulator += gai4_impeg2d_idct_inp_num_nonzero_histogram[i * 8 + j];
            val = i4_accumulator * 100.0 / i4_sum;
            printf("%8.2f \t", val);

        }
        printf("\n");
    }

    printf("IDCT input : Last non-zero row histogram\n");


    {
        i4_accumulator = 0;
        for(i = 0; i < 8; i++)
        {
            i4_accumulator += gai4_impeg2d_idct_inp_last_non_zero_row_histogram[i];
        }
        for(i = 0; i < 8; i++)
        {
            double val = gai4_impeg2d_idct_inp_last_non_zero_row_histogram[i] * 100.0 / i4_accumulator;
            printf("%8.2f \t", val);
        }
        printf("\n");
    }




}

void impeg2d_print_iqnt_inp_statistics()
{
    WORD32 i, j;
    WORD32 i4_sum;
    WORD32 i4_accumulator;
    i4_sum = 0;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            i4_sum += gai4_impeg2d_iqnt_inp_last_nonzero_histogram[i * 8 + j];
        }
    }
    printf("IQnt input : Only last coeff non-zero %8.2f\n", (gi4_impeg2d_iqnt_inp_only_last_coeff * 100.0) / gi4_impeg2d_iqnt_cnt);
    printf("IQnt input : Only first coeff non-zero (Includes DC + mismatch) %8.2f\n", (gi4_impeg2d_iqnt_inp_only_first_coeff * 100.0) / gi4_impeg2d_idct_cnt);

    printf("IQnt input : Last non-zero coeff histogram\n");
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            double val = gai4_impeg2d_iqnt_inp_last_nonzero_histogram[i * 8 + j] * 100.0 / i4_sum;
            printf("%8.2f \t", val);

        }
        printf("\n");
    }

    printf("IQnt input : Cumulative Last non-zero coeff histogram\n");
    i4_accumulator = 0;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            double val;
            i4_accumulator += gai4_impeg2d_iqnt_inp_last_nonzero_histogram[i * 8 + j];
            val = i4_accumulator * 100.0 / i4_sum;

            printf("%8.2f \t", val);

        }
        printf("\n");
    }



    printf("IQnt input : Number of non-zero coeff histogram\n");
    i4_sum = 0;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            i4_sum += gai4_impeg2d_iqnt_inp_num_nonzero_histogram[i * 8 + j];
        }
    }
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            double val = gai4_impeg2d_iqnt_inp_num_nonzero_histogram[i * 8 + j] * 100.0 / i4_sum;
            printf("%8.2f \t", val);

        }
        printf("\n");
    }

    printf("IQnt input : Cumulative number of non-zero coeffs histogram\n");
    i4_accumulator = 0;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            double val;
            i4_accumulator += gai4_impeg2d_iqnt_inp_num_nonzero_histogram[i * 8 + j];
            val = i4_accumulator * 100.0 / i4_sum;
            printf("%8.2f \t", val);

        }
        printf("\n");
    }

    printf("IQnt input : Last non-zero row histogram\n");


    {
        i4_accumulator = 0;
        for(i = 0; i < 8; i++)
        {
            i4_accumulator += gai4_impeg2d_iqnt_inp_last_non_zero_row_histogram[i];
        }
        for(i = 0; i < 8; i++)
        {
            double val = gai4_impeg2d_iqnt_inp_last_non_zero_row_histogram[i] * 100.0 / i4_accumulator;
            printf("%8.2f \t", val);
        }
        printf("\n");
    }

}

void impeg2d_print_statistics()
{
    impeg2d_print_idct_inp_statistics();
    impeg2d_print_iqnt_inp_statistics();
}


#endif

#if DEBUG_MB

static UWORD32  u4_debug_frm = 12;
static UWORD32  u4_debug_mb_x = 3;
static UWORD32  u4_debug_mb_y = 0;

static UWORD32  u4_debug_frm_num = 0;

/*****************************************************************************/
/*                                                                           */
/*  Function Name : example_of_a_function                                    */
/*                                                                           */
/*  Description   : This function illustrates the use of C coding standards. */
/*                  switch/case, if, for, block comments have been shown     */
/*                  here.                                                    */
/*  Inputs        : <What inputs does the function take?>                    */
/*  Globals       : <Does it use any global variables?>                      */
/*  Processing    : <Describe how the function operates - include algorithm  */
/*                  description>                                             */
/*  Outputs       : <What does the function produce?>                        */
/*  Returns       : <What does the function return?>                         */
/*                                                                           */
/*  Issues        : <List any issues or problems with this function>         */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         13 07 2002   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void impeg2d_trace_mb_start(UWORD32 u4_mb_x, UWORD32 u4_mb_y)
{
    UWORD32 u4_frm_num = impeg2d_frm_num_get();

   if(u4_frm_num == u4_debug_frm && u4_mb_x == u4_debug_mb_x &&  u4_mb_y == u4_debug_mb_y)
   {
//       printf("");
   }
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : example_of_a_function                                    */
/*                                                                           */
/*  Description   : This function illustrates the use of C coding standards. */
/*                  switch/case, if, for, block comments have been shown     */
/*                  here.                                                    */
/*  Inputs        : <What inputs does the function take?>                    */
/*  Globals       : <Does it use any global variables?>                      */
/*  Processing    : <Describe how the function operates - include algorithm  */
/*                  description>                                             */
/*  Outputs       : <What does the function produce?>                        */
/*  Returns       : <What does the function return?>                         */
/*                                                                           */
/*  Issues        : <List any issues or problems with this function>         */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         13 07 2002   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void impeg2d_frm_num_set(void)
{
    u4_debug_frm_num++;
}


/*****************************************************************************/
/*                                                                           */
/*  Function Name : example_of_a_function                                    */
/*                                                                           */
/*  Description   : This function illustrates the use of C coding standards. */
/*                  switch/case, if, for, block comments have been shown     */
/*                  here.                                                    */
/*  Inputs        : <What inputs does the function take?>                    */
/*  Globals       : <Does it use any global variables?>                      */
/*  Processing    : <Describe how the function operates - include algorithm  */
/*                  description>                                             */
/*  Outputs       : <What does the function produce?>                        */
/*  Returns       : <What does the function return?>                         */
/*                                                                           */
/*  Issues        : <List any issues or problems with this function>         */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         13 07 2002   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
UWORD32 impeg2d_frm_num_get(void)
{
    return(u4_debug_frm_num);
}

#endif
