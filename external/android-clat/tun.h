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
 * tun.h - tun device functions
 */
#ifndef __TUN_H__
#define __TUN_H__

#include <linux/if.h>

#include "clatd.h"
#include "ring.h"

struct tun_data {
  char device4[IFNAMSIZ];
  int read_fd6, write_fd6, fd4;
  struct packet_ring ring;
};

int tun_open();
int tun_alloc(char *dev, int fd);
int send_tun(int fd, clat_packet out, int iov_len);
int set_nonblocking(int fd);

#endif
