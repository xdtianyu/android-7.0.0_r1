/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _MSM_MDP_EXT_H_
#define _MSM_MDP_EXT_H_
#include <linux/msm_mdp.h>
#define MDP_IOCTL_MAGIC 'S'
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MSMFB_ATOMIC_COMMIT _IOWR(MDP_IOCTL_MAGIC, 128, void *)
#define MSMFB_ASYNC_POSITION_UPDATE _IOWR(MDP_IOCTL_MAGIC, 129, struct mdp_position_update)
#define MDP_LAYER_FLIP_LR 0x1
#define MDP_LAYER_FLIP_UD 0x2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MDP_LAYER_ENABLE_PIXEL_EXT 0x4
#define MDP_LAYER_FORGROUND 0x8
#define MDP_LAYER_SECURE_SESSION 0x10
#define MDP_LAYER_SOLID_FILL 0x20
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MDP_LAYER_DEINTERLACE 0x40
#define MDP_LAYER_BWC 0x80
#define MDP_LAYER_ASYNC 0x100
#define MDP_LAYER_PP 0x200
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MDP_LAYER_SECURE_DISPLAY_SESSION 0x400
#define MDP_VALIDATE_LAYER 0x01
#define MDP_COMMIT_WAIT_FOR_FINISH 0x02
#define MDP_COMMIT_SYNC_FENCE_WAIT 0x04
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MDP_COMMIT_VERSION_1_0 0x00010000
struct mdp_layer_plane {
  int fd;
  uint32_t offset;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t stride;
};
struct mdp_layer_buffer {
  uint32_t width;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t height;
  uint32_t format;
  struct mdp_layer_plane planes[MAX_PLANES];
  uint32_t plane_count;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct mult_factor comp_ratio;
  int fence;
  uint32_t reserved;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct mdp_input_layer {
  uint32_t flags;
  uint32_t pipe_ndx;
  uint8_t horz_deci;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint8_t vert_deci;
  uint8_t alpha;
  uint16_t z_order;
  uint32_t transp_mask;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t bg_color;
  enum mdss_mdp_blend_op blend_op;
  enum mdp_color_space color_space;
  struct mdp_rect src_rect;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct mdp_rect dst_rect;
  struct mdp_scale_data __user * scale;
  struct mdp_layer_buffer buffer;
  void __user * pp_info;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int error_code;
  uint32_t reserved[6];
};
struct mdp_output_layer {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t flags;
  uint32_t writeback_ndx;
  struct mdp_layer_buffer buffer;
  uint32_t reserved[6];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct mdp_layer_commit_v1 {
  uint32_t flags;
  int release_fence;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct mdp_rect left_roi;
  struct mdp_rect right_roi;
  struct mdp_input_layer __user * input_layers;
  uint32_t input_layer_cnt;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct mdp_output_layer __user * output_layer;
  int retire_fence;
  uint32_t reserved[6];
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct mdp_layer_commit {
  uint32_t version;
  union {
    struct mdp_layer_commit_v1 commit_v1;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  };
};
struct mdp_point {
  uint32_t x;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t y;
};
struct mdp_async_layer {
  uint32_t flags;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t pipe_ndx;
  struct mdp_point src;
  struct mdp_point dst;
  int error_code;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t reserved[3];
};
struct mdp_position_update {
  struct mdp_async_layer __user * input_layers;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  uint32_t input_layer_cnt;
};
#endif

