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

#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <linux/if.h>
#include <linux/if_packet.h>

#include "logging.h"
#include "ring.h"
#include "translate.h"
#include "tun.h"

int ring_create(struct tun_data *tunnel) {
  int packetsock = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IPV6));
  if (packetsock < 0) {
    logmsg(ANDROID_LOG_FATAL, "packet socket failed: %s", strerror(errno));
    return -1;
  }

  int ver = TPACKET_V2;
  if (setsockopt(packetsock, SOL_PACKET, PACKET_VERSION, (void *) &ver, sizeof(ver))) {
    logmsg(ANDROID_LOG_FATAL, "setsockopt(PACKET_VERSION, %d) failed: %s", ver, strerror(errno));
    return -1;
  }

  int on = 1;
  if (setsockopt(packetsock, SOL_PACKET, PACKET_LOSS, (void *) &on, sizeof(on))) {
    logmsg(ANDROID_LOG_WARN, "PACKET_LOSS failed: %s", strerror(errno));
  }

  struct packet_ring *ring = &tunnel->ring;
  ring->numblocks = TP_NUM_BLOCKS;

  int total_frames = TP_FRAMES * ring->numblocks;

  struct tpacket_req req = {
      .tp_frame_size = TP_FRAME_SIZE,  // Frame size.
      .tp_block_size = TP_BLOCK_SIZE,  // Frames per block.
      .tp_block_nr = ring->numblocks,  // Number of blocks.
      .tp_frame_nr = total_frames,     // Total frames.
  };

  if (setsockopt(packetsock, SOL_PACKET, PACKET_RX_RING, &req, sizeof(req)) < 0) {
    logmsg(ANDROID_LOG_FATAL, "PACKET_RX_RING failed: %s", strerror(errno));
    return -1;
  }

  size_t buflen = TP_BLOCK_SIZE * ring->numblocks;
  ring->base = mmap(NULL, buflen, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED|MAP_POPULATE,
                    packetsock, 0);
  if (ring->base == MAP_FAILED) {
    logmsg(ANDROID_LOG_FATAL, "mmap %lu failed: %s", buflen, strerror(errno));
    return -1;
  }

  ring->block = 0;
  ring->slot = 0;
  ring->numslots = TP_BLOCK_SIZE / TP_FRAME_SIZE;
  ring->next = (struct tpacket2_hdr *) ring->base;

  logmsg(ANDROID_LOG_INFO, "Using ring buffer with %d frames (%d bytes) at %p",
         total_frames, buflen, ring->base);

  return packetsock;
}

/* function: ring_advance
 * advances to the next position in the packet ring
 * ring - packet ring buffer
 */
static struct tpacket2_hdr* ring_advance(struct packet_ring *ring) {
  uint8_t *next = (uint8_t *) ring->next;

  ring->slot++;
  next += TP_FRAME_SIZE;

  if (ring->slot == ring->numslots) {
    ring->slot = 0;
    ring->block++;

    if (ring->block < ring->numblocks) {
      next += TP_FRAME_GAP;
    } else {
      ring->block = 0;
      next = (uint8_t *) ring->base;
    }
  }

  ring->next = (struct tpacket2_hdr *) next;
  return ring->next;
}

/* function: ring_read
 * reads a packet from the ring buffer and translates it
 * read_fd  - file descriptor to read original packet from
 * write_fd - file descriptor to write translated packet to
 * to_ipv6  - whether the packet is to be translated to ipv6 or ipv4
 */
void ring_read(struct packet_ring *ring, int write_fd, int to_ipv6) {
  struct tpacket2_hdr *tp = ring->next;
  if (tp->tp_status & TP_STATUS_USER) {
    uint8_t *packet = ((uint8_t *) tp) + tp->tp_net;
    translate_packet(write_fd, to_ipv6, packet, tp->tp_len);
    tp->tp_status = TP_STATUS_KERNEL;
    tp = ring_advance(ring);
  }
}
