/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Driver for using rootdev.c from the commandline
 */
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "rootdev.h"

static void print_help(const char *progname) {
  fprintf(stderr,
    "%s [OPTIONS] [PATH]\n"
    "Outputs the containing device for the specified PATH.\n"
    "With no arguments, '/' is assumed.\n"
    "\n"
    "Options:\n"
    "  -h\tthis message.\n"
    "\n"
    "  -c\tcreate the /dev node if it cannot be found\n"
    "  -d\treturn the block device only if possible\n"
    "  -i\treturn path even if the node doesn't exist\n"
    "  -s\tif possible, return the first slave of the root device\n"
    "\n"
    "  --block [path]\tset the path to block under the sys mount point\n"
    "  --dev [path]\tset the path to dev mount point\n"
    "  --major [num]\tset the major number of the rootdev\n"
    "  --minor [num]\tset the minor number of the rootdev\n",
    progname);
}

static int flag_help = 0;
static int flag_use_slave = 0;
static int flag_strip_partition = 0;
static int flag_ignore = 0;
static int flag_create = 0;
static int flag_major = 0;
static int flag_minor = 0;
static const char *flag_path = "/";
static char *flag_block_path = NULL;
static char *flag_dev_path = NULL;

static void parse_args(int argc, char **argv) {
  while (1) {
    int c;
    int option_index = 0;
    static const struct option long_options[] = {
      {"c", no_argument, &flag_create, 1},
      {"d", no_argument, &flag_strip_partition, 1},
      {"h", no_argument, &flag_help, 1},
      {"i", no_argument, &flag_ignore, 1},
      {"s", no_argument, &flag_use_slave, 1},
      /* Long arguments for testing. */
      {"block", required_argument, NULL, 'b'},
      {"dev", required_argument, NULL, 'd'},
      {"major", required_argument, NULL, 'M'},
      {"minor", required_argument, NULL, 'm'},
      {0, 0, 0, 0}
    };
    c = getopt_long_only(argc, argv, "", long_options, &option_index);

    if (c == -1)
      break;

    if (c == '?') {
      flag_help = 1;
      break;
    }

    switch (c) {
    case 'b':
      flag_block_path = optarg;
      break;
    case 'd':
      flag_dev_path = optarg;
      break;
    case 'M':
      flag_major = atoi(optarg);
      break;
    case 'm':
      flag_minor = atoi(optarg);
      break;
    }

  }

  if (flag_create && flag_strip_partition) {
    flag_help = 1;
    warnx("-c and -d are incompatible at present.");
    return;
  }

  if (optind < argc) {
    flag_path = argv[optind++];
  }

  if (optind < argc) {
    fprintf(stderr, "Too many free arguments: %d\n", argc - optind);
    flag_help = 1;
   }
}

int main(int argc, char **argv) {
  struct stat path_stat;
  char path[PATH_MAX];
  int ret;
  dev_t root_dev;
  parse_args(argc, argv);

  if (flag_help) {
    print_help(argv[0]);
    return 1;
  }

  if (flag_major || flag_minor) {
    root_dev = makedev(flag_major, flag_minor);
  } else {
    /* Yields the containing dev_t in st_dev. */
    if (stat(flag_path, &path_stat) != 0)
      err(1, "Cannot stat(%s)", flag_path);
    root_dev = path_stat.st_dev;
  }

  path[0] = '\0';
  ret = rootdev_wrapper(path, sizeof(path),
                        flag_use_slave,
                        flag_strip_partition,
                        &root_dev,
                        flag_block_path,
                        flag_dev_path);

  if (ret == 1 && flag_create) {
    /* TODO(wad) add flag_force to allow replacement */
    ret = 0;
    if (mknod(path, S_IFBLK | S_IRUSR | S_IWUSR, root_dev) && errno != EEXIST) {
      warn("failed to create %s", path);
      ret = 1;
    }
  }

  if (flag_ignore && ret > 0)
    ret = 0;

  if (path[0] != '\0')
    printf("%s\n", path);

  return ret;
}
