# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)
cblas_SRC_FILES:= \
	src/cblas_caxpy.c \
	src/cblas_ccopy.c \
	src/cblas_cdotc_sub.c \
	src/cblas_cdotu_sub.c \
	src/cblas_cgbmv.c \
	src/cblas_cgemm.c \
	src/cblas_cgemv.c \
	src/cblas_cgerc.c \
	src/cblas_cgeru.c \
	src/cblas_chbmv.c \
	src/cblas_chemm.c \
	src/cblas_chemv.c \
	src/cblas_cher.c \
	src/cblas_cher2.c \
	src/cblas_cher2k.c \
	src/cblas_cherk.c \
	src/cblas_chpmv.c \
	src/cblas_chpr.c \
	src/cblas_chpr2.c \
	src/cblas_cscal.c \
	src/cblas_csscal.c \
	src/cblas_cswap.c \
	src/cblas_csymm.c \
	src/cblas_csyr2k.c \
	src/cblas_csyrk.c \
	src/cblas_ctbmv.c \
	src/cblas_ctbsv.c \
	src/cblas_ctpmv.c \
	src/cblas_ctpsv.c \
	src/cblas_ctrmm.c \
	src/cblas_ctrmv.c \
	src/cblas_ctrsm.c \
	src/cblas_ctrsv.c \
	src/cblas_dasum.c \
	src/cblas_daxpy.c \
	src/cblas_dcopy.c \
	src/cblas_ddot.c \
	src/cblas_dgbmv.c \
	src/cblas_dgemm.c \
	src/cblas_dgemv.c \
	src/cblas_dger.c \
	src/cblas_dnrm2.c \
	src/cblas_drot.c \
	src/cblas_drotg.c \
	src/cblas_drotm.c \
	src/cblas_drotmg.c \
	src/cblas_dsbmv.c \
	src/cblas_dscal.c \
	src/cblas_dsdot.c \
	src/cblas_dspmv.c \
	src/cblas_dspr.c \
	src/cblas_dspr2.c \
	src/cblas_dswap.c \
	src/cblas_dsymm.c \
	src/cblas_dsymv.c \
	src/cblas_dsyr.c \
	src/cblas_dsyr2.c \
	src/cblas_dsyr2k.c \
	src/cblas_dsyrk.c \
	src/cblas_dtbmv.c \
	src/cblas_dtbsv.c \
	src/cblas_dtpmv.c \
	src/cblas_dtpsv.c \
	src/cblas_dtrmm.c \
	src/cblas_dtrmv.c \
	src/cblas_dtrsm.c \
	src/cblas_dtrsv.c \
	src/cblas_dzasum.c \
	src/cblas_dznrm2.c \
	src/cblas_globals.c \
	src/cblas_icamax.c \
	src/cblas_idamax.c \
	src/cblas_isamax.c \
	src/cblas_izamax.c \
	src/cblas_sasum.c \
	src/cblas_saxpy.c \
	src/cblas_scasum.c \
	src/cblas_scnrm2.c \
	src/cblas_scopy.c \
	src/cblas_sdot.c \
	src/cblas_sdsdot.c \
	src/cblas_sgbmv.c \
	src/cblas_sgemm.c \
	src/cblas_sgemv.c \
	src/cblas_sger.c \
	src/cblas_snrm2.c \
	src/cblas_srot.c \
	src/cblas_srotg.c \
	src/cblas_srotm.c \
	src/cblas_srotmg.c \
	src/cblas_ssbmv.c \
	src/cblas_sscal.c \
	src/cblas_sspmv.c \
	src/cblas_sspr.c \
	src/cblas_sspr2.c \
	src/cblas_sswap.c \
	src/cblas_ssymm.c \
	src/cblas_ssymv.c \
	src/cblas_ssyr.c \
	src/cblas_ssyr2.c \
	src/cblas_ssyr2k.c \
	src/cblas_ssyrk.c \
	src/cblas_stbmv.c \
	src/cblas_stbsv.c \
	src/cblas_stpmv.c \
	src/cblas_stpsv.c \
	src/cblas_strmm.c \
	src/cblas_strmv.c \
	src/cblas_strsm.c \
	src/cblas_strsv.c \
	src/cblas_xerbla.c \
	src/cblas_zaxpy.c \
	src/cblas_zcopy.c \
	src/cblas_zdotc_sub.c \
	src/cblas_zdotu_sub.c \
	src/cblas_zdscal.c \
	src/cblas_zgbmv.c \
	src/cblas_zgemm.c \
	src/cblas_zgemv.c \
	src/cblas_zgerc.c \
	src/cblas_zgeru.c \
	src/cblas_zhbmv.c \
	src/cblas_zhemm.c \
	src/cblas_zhemv.c \
	src/cblas_zher.c \
	src/cblas_zher2.c \
	src/cblas_zher2k.c \
	src/cblas_zherk.c \
	src/cblas_zhpmv.c \
	src/cblas_zhpr.c \
	src/cblas_zhpr2.c \
	src/cblas_zscal.c \
	src/cblas_zswap.c \
	src/cblas_zsymm.c \
	src/cblas_zsyr2k.c \
	src/cblas_zsyrk.c \
	src/cblas_ztbmv.c \
	src/cblas_ztbsv.c \
	src/cblas_ztpmv.c \
	src/cblas_ztpsv.c \
	src/cblas_ztrmm.c \
	src/cblas_ztrmv.c \
	src/cblas_ztrsm.c \
	src/cblas_ztrsv.c \
	src/xerbla.c


LOCAL_CLANG := true
LOCAL_MODULE := libblas
LOCAL_SRC_FILES := $(cblas_SRC_FILES)

LOCAL_C_INCLUDES += external/cblas/include

LOCAL_STATIC_LIBRARIES := libF77blas

include $(BUILD_SHARED_LIBRARY)


# Build libblas using API 9 toolchain for RS Support lib.
include $(CLEAR_VARS)

LOCAL_CLANG := true
LOCAL_MODULE := libblasV8
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := c++_static
LOCAL_LDFLAGS += -ldl -Wl,--exclude-libs,libc++_static.a

LOCAL_SRC_FILES := $(cblas_SRC_FILES)

LOCAL_C_INCLUDES += external/cblas/include

LOCAL_STATIC_LIBRARIES := libF77blasV8

include $(BUILD_SHARED_LIBRARY)

