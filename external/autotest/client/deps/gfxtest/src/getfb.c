// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  const char* device_name = "/dev/fb0";
  struct fb_var_screeninfo info;
  uint32_t screen_size = 0;
  void *ptr = NULL;
  int fd = -1;
  FILE *file = NULL;
  int ret = -1;

  if (argc < 2) {
    printf("Usage: getfb [filename]\n");
    printf("Writes the active framebuffer to output file [filename].\n");
    return 0;
  }

  // Open the file for reading and writing
  fd = open(device_name, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Cannot open framebuffer device %s\n", device_name);
    goto exit;
  }
  printf("The framebuffer device was opened successfully.\n");

  // Get fixed screen information
  if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1) {
    fprintf(stderr, "Error reading variable screen information.\n");
    goto exit;
  }

  printf("Framebuffer info: %dx%d, %dbpp\n", info.xres, info.yres,
         info.bits_per_pixel);

  // Figure out the size of the screen in bytes
  screen_size = info.xres * info.yres * info.bits_per_pixel / 8;

  // Map the device to memory
  ptr = mmap(0, screen_size, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    fprintf(stderr, "Error: failed to map framebuffer device to memory.\n");
    goto exit;
  }
  printf("The framebuffer device was mapped to memory successfully.\n");

  // Write it to output file.
  file = fopen(argv[1], "w");
  if (!file) {
    fprintf(stderr, "Could not open file %s for writing.\n", argv[1]);
    goto exit;
  }

  if (fwrite(ptr, screen_size, 1, file) < 1) {
    fprintf(stderr, "Error while writing framebuffer to file.\n");
    goto exit;
  }

  ret = 0;

exit:
  if (file)
    fclose(file);

  if (ptr != MAP_FAILED && ptr)
    munmap(ptr, screen_size);

  if (fd >= 0)
    close(fd);

  return ret;
}
