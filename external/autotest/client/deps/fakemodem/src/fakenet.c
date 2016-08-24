/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/if.h>
#include <linux/if_tun.h>

static int
tun_alloc(char *dev)
{
  struct ifreq ifr;
  int fd, err;

  if ((fd = open ("/dev/net/tun", O_RDWR)) < 0) {
    printf ("Error opening /dev/net/tun: %s\n", strerror (errno));
    return -1;
  }

  memset (&ifr, 0, sizeof (ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_TAP   - TAP device
   *
   *        IFF_NO_PI - Do not provide packet information
   */
  ifr.ifr_flags = IFF_TAP;
  if (*dev)
    strncpy (ifr.ifr_name, dev, IFNAMSIZ);

  if ((err = ioctl (fd, TUNSETIFF, (void *) &ifr)) < 0) {
    printf ("Error calling TUNSETIFF: %s\n", strerror (errno));
    close (fd);
    return err;
  }
  strncpy (dev, ifr.ifr_name, IFNAMSIZ);
  return fd;
}

int
main (int argc, const char *argv[])
{

  int fd;
  char namebuf[IFNAMSIZ];

  strcpy (namebuf, "pseudo-modem%d");
  fd = tun_alloc (namebuf);
  if (fd == -1)
    exit (1);

  printf ("%s\n", namebuf);
  fflush(stdout);

  while (1)
    sleep (3600);

  return 0;
}
