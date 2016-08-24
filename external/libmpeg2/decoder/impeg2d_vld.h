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
#ifndef __IMPEG2D_VLD_H__
#define __IMPEG2D_VLD_H__


WORD16 impeg2d_dec_vld_symbol(stream_t *stream,const WORD16 codeTable[][2],
                 UWORD16 maxLen);
WORD16 impeg2d_fast_dec_vld_symbol(stream_t *stream,
                     const WORD16  codeTable[][2],
                     const UWORD16 indexTable[][2],
                     UWORD16 maxLen);
IMPEG2D_ERROR_CODES_T impeg2d_vld_decode(dec_state_t *dec, WORD16 *outAddr, /*!< Address where decoded symbols will be stored */
                                            const UWORD8 *scan, /*!< Scan table to be used */
                                            UWORD8 *pu1_pos, /*!< Scan table to be used */
                                            UWORD16 intraFlag, /*!< Intra Macroblock or not */
                                            UWORD16 chromaFlag, /*!< Chroma Block or not */
                                            UWORD16 dPicture, /*!< D Picture or not */
                                            UWORD16 intraVlcFormat, /*!< Intra VLC format */
                                            UWORD16 mpeg2, /*!< MPEG-2 or not */
                                            WORD32 *pi4_num_coeffs /*!< Returns the number of coeffs in block */
                                            );

pf_vld_inv_quant_t impeg2d_vld_inv_quant_mpeg1;
pf_vld_inv_quant_t impeg2d_vld_inv_quant_mpeg2;


pf_inv_quant_t impeg2d_inv_quant_mpeg1;
pf_inv_quant_t impeg2d_inv_quant_mpeg2;


#endif /* #ifndef __IMPEG2D_VLD_H__ */
