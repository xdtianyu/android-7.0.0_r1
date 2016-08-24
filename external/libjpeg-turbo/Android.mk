LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# By default, the build system generates ARM target binaries in thumb mode,
# where each instruction is 16 bits wide.  Defining this variable as arm
# forces the build system to generate object files in 32-bit arm mode.  This
# is the same setting previously used by libjpeg.
# TODO (msarett): Run performance tests to determine whether arm mode is still
#                 preferred to thumb mode for libjpeg-turbo.
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
    jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c \
    jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c \
    jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c \
    jdatadst.c jdatasrc.c jdcoefct.c  jdcolor.c jddctmgr.c jdhuff.c \
    jdinput.c jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c \
    jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c \
    jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c jmemmgr.c \
    jmemnobs.c jquant1.c jquant2.c jutils.c

# ARM v7 NEON
LOCAL_SRC_FILES_arm += simd/jsimd_arm_neon.S simd/jsimd_arm.c

# If we are certain that the ARM v7 device has NEON (and there is no need for
# a runtime check), we can indicate that with a flag.
ifeq ($(strip $(TARGET_ARCH)),arm)
  ifeq ($(ARCH_ARM_HAVE_NEON),true)
    LOCAL_CFLAGS += -D__ARM_HAVE_NEON__
  endif
endif

# ARM v8 64-bit NEON
LOCAL_SRC_FILES_arm64 += simd/jsimd_arm64_neon.S simd/jsimd_arm64.c

# x86 MMX and SSE2
LOCAL_SRC_FILES_x86 += \
      simd/jsimd_i386.c simd/jccolor-mmx.asm simd/jccolor-sse2.asm \
      simd/jcgray-mmx.asm  simd/jcgray-sse2.asm simd/jcsample-mmx.asm \
      simd/jcsample-sse2.asm simd/jdcolor-mmx.asm simd/jdcolor-sse2.asm \
      simd/jdmerge-mmx.asm simd/jdmerge-sse2.asm simd/jdsample-mmx.asm \
      simd/jdsample-sse2.asm simd/jfdctflt-3dn.asm simd/jfdctflt-sse.asm \
      simd/jfdctfst-mmx.asm simd/jfdctfst-sse2.asm simd/jfdctint-mmx.asm \
      simd/jfdctint-sse2.asm simd/jidctflt-3dn.asm simd/jidctflt-sse2.asm \
      simd/jidctflt-sse.asm simd/jidctfst-mmx.asm simd/jidctfst-sse2.asm \
      simd/jidctint-mmx.asm simd/jidctint-sse2.asm simd/jidctred-mmx.asm \
      simd/jidctred-sse2.asm simd/jquant-3dn.asm simd/jquantf-sse2.asm \
      simd/jquanti-sse2.asm simd/jquant-mmx.asm simd/jquant-sse.asm \
      simd/jsimdcpu.asm
LOCAL_ASFLAGS_x86 += -DPIC -DELF
LOCAL_C_INCLUDES_x86 += $(LOCAL_PATH)/simd

# x86-64 SSE2
LOCAL_SRC_FILES_x86_64 += \
      simd/jsimd_x86_64.c simd/jccolor-sse2-64.asm simd/jcgray-sse2-64.asm \
      simd/jcsample-sse2-64.asm simd/jdcolor-sse2-64.asm \
      simd/jdmerge-sse2-64.asm simd/jdsample-sse2-64.asm \
      simd/jfdctflt-sse-64.asm simd/jfdctfst-sse2-64.asm \
      simd/jfdctint-sse2-64.asm simd/jidctflt-sse2-64.asm \
      simd/jidctfst-sse2-64.asm simd/jidctint-sse2-64.asm \
      simd/jidctred-sse2-64.asm simd/jquantf-sse2-64.asm \
      simd/jquanti-sse2-64.asm
LOCAL_ASFLAGS_x86_64 += -D__x86_64__ -DPIC -DELF
LOCAL_C_INCLUDES_x86_64 += $(LOCAL_PATH)/simd

LOCAL_SRC_FILES_mips += jsimd_none.c
LOCAL_SRC_FILES_mips64 += jsimd_none.c

LOCAL_CFLAGS += -O3 -fstrict-aliasing
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

ifneq (,$(TARGET_BUILD_APPS))
  # Unbundled branch, built against NDK.
  LOCAL_SDK_VERSION := 17
endif

# Build as a static library.
LOCAL_MODULE := libjpeg_static
include $(BUILD_STATIC_LIBRARY)

# Also build as a shared library.
include $(CLEAR_VARS)

ifneq (,$(TARGET_BUILD_APPS))
  # Unbundled branch, built against NDK.
  LOCAL_SDK_VERSION := 17
endif

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_WHOLE_STATIC_LIBRARIES = libjpeg_static
LOCAL_MODULE := libjpeg
include $(BUILD_SHARED_LIBRARY)
