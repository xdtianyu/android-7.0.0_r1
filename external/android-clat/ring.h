/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ring.c - packet ring buffer functions
 */
#ifndef __RING_H__
#define __RING_H__

#include <linux/if.h>
#include <linux/if_packet.h>

#include "clatd.h"

struct tun_data;

// Frame size. Must be a multiple of TPACKET_ALIGNMENT (=16)
// Why the 16? http://lxr.free-electrons.com/source/net/packet/af_packet.c?v=3.4#L1764
#define TP_FRAME_SIZE (TPACKET_ALIGN(MAXMTU) + TPACKET_ALIGN(TPACKET2_HDRLEN) + 16)

// Block size. Must be a multiple of the page size, and a power of two for efficient memory use.
#define TP_BLOCK_SIZE 65536

// In order to save memory, our frames are not an exact divider of the block size. Therefore, the
// mmaped region will have gaps corresponding to the empty space at the end of each block.
#define TP_FRAMES (TP_BLOCK_SIZE / TP_FRAME_SIZE)
#define TP_FRAME_GAP (TP_BLOCK_SIZE % TP_FRAME_SIZE)

// TODO: Make this configurable. This requires some refactoring because the packet socket is
// opened before we drop privileges, but the configuration file is read after. A value of 16
// results in 656 frames (1048576 bytes).
#define TP_NUM_BLOCKS 16

struct packet_ring {
  uint8_t *base;
  struct tpacket2_hdr *next;
  int slot, numslots;
  int block, numblocks;
};

int ring_create(struct tun_data *tunnel);
void ring_read(struct packet_ring *ring, int write_fd, int to_ipv6);

#endif
