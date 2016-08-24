# Output variables:
# libvpx_config_dir_arm64
# libvpx_codec_srcs_c_arm64
# libvpx_codec_srcs_asm_arm64

libvpx_target := config/arm64

LOCAL_ARM_MODE := arm

libvpx_config_dir_arm64 := $(LOCAL_PATH)/$(libvpx_target)
libvpx_codec_srcs := $(sort $(shell cat $(libvpx_config_dir_arm64)/libvpx_srcs.txt))

# vpx_config.c is an auto-generated file in $(libvpx_target).
libvpx_codec_srcs_c_arm64 := $(addprefix libvpx/, $(filter-out vpx_config.c, \
    $(filter %.c, $(libvpx_codec_srcs)))) \
    $(libvpx_target)/vpx_config.c

libvpx_codec_srcs_asm_arm64 := $(filter %.asm.s, $(libvpx_codec_srcs))
