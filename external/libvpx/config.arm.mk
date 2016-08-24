# Output variables:
# libvpx_config_dir_arm
# libvpx_codec_srcs_c_arm
# libvpx_codec_srcs_asm_arm

ifeq ($(ARCH_ARM_HAVE_NEON),true)
libvpx_target := config/arm-neon
else ifeq ($(ARCH_ARM_HAVE_ARMV7A),true)
libvpx_target := config/arm
else
libvpx_target := config/generic
endif

LOCAL_ARM_MODE := arm

libvpx_config_dir_arm := $(LOCAL_PATH)/$(libvpx_target)
libvpx_codec_srcs := $(sort $(shell cat $(libvpx_config_dir_arm)/libvpx_srcs.txt))

# vpx_config.c is an auto-generated file in $(libvpx_target).
libvpx_codec_srcs_c_arm := $(addprefix libvpx/, $(filter-out vpx_config.c, \
    $(filter %.c, $(libvpx_codec_srcs)))) \
    $(libvpx_target)/vpx_config.c

libvpx_codec_srcs_asm_arm := $(filter %.asm, $(libvpx_codec_srcs))
