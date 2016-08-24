# Output variables:
# libvpx_config_dir_mips64
# libvpx_codec_srcs_c_mips64

libvpx_target := config/generic

libvpx_config_dir_mips64 := $(LOCAL_PATH)/$(libvpx_target)
libvpx_codec_srcs := $(sort $(shell cat $(libvpx_config_dir_mips64)/libvpx_srcs.txt))

# vpx_config.c is an auto-generated file in $(libvpx_target).
libvpx_codec_srcs_c_mips64 := $(addprefix libvpx/, $(filter-out vpx_config.c, \
    $(filter %.c, $(libvpx_codec_srcs)))) \
    $(libvpx_target)/vpx_config.c
