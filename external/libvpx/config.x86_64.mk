# Output variables:
# libvpx_config_dir_x86_64
# libvpx_codec_srcs_c_x86_64
# libvpx_codec_srcs_asm_x86_64

libvpx_target := config/generic

libvpx_config_dir_x86_64 := $(LOCAL_PATH)/$(libvpx_target)
libvpx_codec_srcs := $(sort $(shell cat $(libvpx_config_dir_x86_64)/libvpx_srcs.txt))

# vpx_config.c is an auto-generated file in $(libvpx_target).
libvpx_codec_srcs_c_x86_64 := $(addprefix libvpx/, $(filter-out vpx_config.c, \
    $(filter %.c, $(libvpx_codec_srcs)))) \
    $(libvpx_target)/vpx_config.c

# X86_64 asm files are processed by the system and sent to yasm
libvpx_codec_srcs_c_x86_64 += $(addprefix libvpx/, $(filter %.asm, $(libvpx_codec_srcs)))
