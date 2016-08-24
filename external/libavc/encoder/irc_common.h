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

#ifndef _RC_COMMON_H_
#define _RC_COMMON_H_

/****************************************************************************
 NOTE : Put only those things into this file which are common across many
 files, say I_TO_P_BIT_RATIO macro is used across irc_bit_allocation.c
 and irc_rate_control_api.c.If anything is exclusive only to one file,
 define it in the same file

 This file is an RC private file. It should not be exported to Codec
 ****************************************************************************/

#define UNUSED(x) ((void)(x))

typedef float number_t;

#define mult32_var_q(a,b,c) *c = a * b

#define div32_var_q(a,b,c) (*c = ((b == 0)? a : (a / b)))

#define add32_var_q(a,b,c) *c = a + b

#define sub32_var_q(a,b,c) *c = a - b

#define sqrt32_var_q(a, c) *c = sqrt(a)

#define number_t_to_word32(num_a, a) *a = (WORD32)num_a

#define convert_float_to_fix(a_f, a) *a = (WORD32)a_f

#define convert_fix_to_float(a, a_f) *a_f = (float) a

#define SET_VAR_Q(a,b,c) {a = (float) b;}


/* Defines the maximum and the minimum quantizer allowed in the stream.*/
#define MAX_MPEG2_QP        255 /* 127*/

/* Bits ratio between I and P frame */
#define I_TO_P_BIT_RATIO 5

/* Calculates P = (X*Y/Z) (Assuming all the four are in integers)*/
#define X_PROD_Y_DIV_Z(X1,Y1,Z1,P1)\
{\
    number_t vq_a,vq_b,vq_c;\
    SET_VAR_Q(vq_a,(X1),0);\
    SET_VAR_Q(vq_b,(Y1),0);\
    SET_VAR_Q(vq_c,(Z1),0);\
    mult32_var_q(vq_a,vq_b,&vq_a);\
    div32_var_q(vq_a,vq_c,&vq_a);\
    number_t_to_word32(vq_a,&(P1));\
}
#define VQ_A_LT_VQ_B(A,B, Z) Z = A < B;
#define VQ_A_GT_VQ_B(A,B, Z) Z = A > B;

/* Z=MAX(A,B) where A, B  and Z are var_q variables */
#define MAX_VARQ(A,B, Z)\
{\
    WORD32 a_gt_b;\
    VQ_A_GT_VQ_B((A), (B), a_gt_b);\
    (Z) = (a_gt_b) ? (A) : (B);\
}

/* Z=MIN(A,B) where A, B  and Z are var_q variables */
#define MIN_VARQ(A,B, Z)\
{\
    WORD32 a_lt_b;\
    VQ_A_LT_VQ_B((A), (B), a_lt_b);\
    (Z) = (a_lt_b) ? (A) : (B);\
}

/* Maximum number of drain-rates supported. Currently a maximum of only 2
 drain-rates supported. One for
 I pictures and the other for P & B pictures */
#define MAX_NUM_DRAIN_RATES 2

/* The ratios between I to P and P to B Qp is specified here */
#define K_Q 4
#define I_TO_P_RATIO (19) /* In K_Q Q factor */
#define P_TO_B_RATIO (32) /* In K_Q Q factor */
#define P_TO_I_RATIO (13) /* In K_Q Q factor */

#endif /* _RC_COMMON_H_ */

