libmpeg2d_cflags_x86_64 += -DX86 -DDISABLE_AVX2 -m64 -msse4.2 -mno-avx  -DDEFAULT_ARCH=D_ARCH_X86_SSE42

libmpeg2d_inc_dir_x86_64   +=  $(LOCAL_PATH)/decoder/x86
libmpeg2d_inc_dir_x86_64   +=  $(LOCAL_PATH)/common/x86

libmpeg2d_srcs_c_x86_64     +=  decoder/x86/impeg2d_function_selector.c
libmpeg2d_srcs_c_x86_64     +=  decoder/x86/impeg2d_function_selector_avx2.c
libmpeg2d_srcs_c_x86_64     +=  decoder/x86/impeg2d_function_selector_ssse3.c
libmpeg2d_srcs_c_x86_64     +=  decoder/x86/impeg2d_function_selector_sse42.c
libmpeg2d_srcs_c_x86_64     +=  common/x86/ideint_function_selector.c
libmpeg2d_srcs_c_x86_64     +=  common/x86/ideint_function_selector_ssse3.c
libmpeg2d_srcs_c_x86_64     +=  common/x86/ideint_function_selector_sse42.c

libmpeg2d_srcs_c_x86_64     +=  common/x86/icv_variance_ssse3.c
libmpeg2d_srcs_c_x86_64     +=  common/x86/icv_sad_ssse3.c
libmpeg2d_srcs_c_x86_64     +=  common/x86/ideint_cac_ssse3.c
libmpeg2d_srcs_c_x86_64     +=  common/x86/ideint_spatial_filter_ssse3.c

libmpeg2d_srcs_c_x86_64     +=  common/x86/impeg2_idct_recon_sse42_intr.c
libmpeg2d_srcs_c_x86_64     +=  common/x86/impeg2_inter_pred_sse42_intr.c
libmpeg2d_srcs_c_x86_64     +=  common/x86/impeg2_mem_func_sse42_intr.c

LOCAL_SRC_FILES_x86_64 += $(libmpeg2d_srcs_c_x86_64) $(libmpeg2d_srcs_asm_x86_64)
LOCAL_C_INCLUDES_x86_64 += $(libmpeg2d_inc_dir_x86_64)
LOCAL_CFLAGS_x86_64 += $(libmpeg2d_cflags_x86_64)



