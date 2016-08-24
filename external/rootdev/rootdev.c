/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Implements root device discovery via sysfs with optional bells and whistles.
 */

#include "rootdev.h"

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Limit prevents endless looping to find slave.
 * We currently have at most 2 levels, this allows
 * for future growth.
 */
#define MAX_SLAVE_DEPTH 8

static const char *kDefaultSearchPath = "/sys/block";
static const char *kDefaultDevPath = "/dev/block";

/* Encode the root device structuring here for Chromium OS */
static const char kActiveRoot[] = "/dev/ACTIVE_ROOT";
static const char kRootDev[] = "/dev/ROOT";
static const char kRootA[] = "/dev/ROOT0";
static const char kRootB[] = "/dev/ROOT1";

struct part_config {
  const char *name;
  int offset;
};

#define CHROMEOS_PRIMARY_PARTITION 3
static const struct part_config kPrimaryPart[] = { { kRootA,    0 },
                                                   { kRootDev, -3 },
                                                   { kRootB,    2 } };
#define CHROMEOS_SECONDARY_PARTITION 5
static const struct part_config kSecondaryPart[] = { { kRootB,    0 },
                                                     { kRootDev, -5 },
                                                     { kRootA,   -2 } };

/* The number of entries in a part_config so we could add RootC easily. */
static const int kPartitionEntries = 3;

/* Converts a file of %u:%u -> dev_t. */
static dev_t devt_from_file(const char *file) {
  char candidate[10];  /* TODO(wad) system-provided constant? */
  ssize_t bytes = 0;
  unsigned int major_num = 0;
  unsigned int minor_num = 0;
  dev_t dev = 0;
  int fd = -1;

  /* Never hang. Either get the data or return 0. */
  fd = open(file, O_NONBLOCK | O_RDONLY);
  if (fd < 0)
    return 0;
  bytes = read(fd, candidate, sizeof(candidate));
  close(fd);

  /* 0:0 should be considered the minimum size. */
  if (bytes < 3)
    return 0;
  candidate[bytes] = 0;
  if (sscanf(candidate, "%u:%u", &major_num, &minor_num) == 2) {
    /* candidate's size artificially limits the size of the converted
     * %u to safely convert to a signed int. */
    dev = makedev(major_num, minor_num);
  }
  return dev;
}

/* Walks sysfs and recurses into any directory/link that represents
 * a block device to find sub-devices (partitions) for dev.
 * If dev == 0, the name fo the first device in the directory will be returned.
 * Returns the device's name in "name" */
static int match_sysfs_device(char *name, size_t name_len,
                              const char *basedir, dev_t *dev, int depth) {
  int found = -1;
  size_t basedir_len;
  DIR *dirp = NULL;
  struct dirent *entry = NULL;
  struct dirent *next = NULL;
  char *working_path = NULL;
  long working_path_size = 0;

  if (!name || !name_len || !basedir || !dev) {
    warnx("match_sysfs_device: invalid arguments supplied");
    return -1;
  }
  basedir_len = strlen(basedir);
  if (!basedir_len) {
    warnx("match_sysfs_device: basedir must not be empty");
    return -1;
  }

  errno = 0;
  dirp = opendir(basedir);
  if (!dirp) {
     /* Don't complain if the directory doesn't exist. */
     if (errno != ENOENT)
       warn("match_sysfs_device:opendir(%s)", basedir);
     return found;
  }

  /* Grab a platform appropriate path to work with.
   * Ideally, this won't vary under sys/block. */
  working_path_size = pathconf(basedir, _PC_NAME_MAX) + 1;
  /* Fallback to PATH_MAX on any pathconf error. */
  if (working_path_size < 0)
    working_path_size = PATH_MAX;

  working_path = malloc(working_path_size);
  if (!working_path) {
    warn("malloc(dirent)");
    closedir(dirp);
    return found;
  }

  /* Allocate a properly sized entry. */
  entry = malloc(offsetof(struct dirent, d_name) + working_path_size);
  if (!entry) {
    warn("malloc(dirent)");
    free(working_path);
    closedir(dirp);
    return found;
  }

  while (readdir_r(dirp, entry, &next) == 0 && next) {
    size_t candidate_len = strlen(entry->d_name);
    size_t path_len = 0;
    dev_t found_devt = 0;
    /* Ignore the usual */
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;
    /* TODO(wad) determine how to best bubble up this case. */
    if (candidate_len > name_len)
      continue;
    /* Only traverse directories or symlinks (to directories ideally) */
    switch (entry->d_type) {
    case DT_UNKNOWN:
    case DT_DIR:
    case DT_LNK:
      break;
    default:
      continue;
    }
    /* Determine path to block device number */
    path_len = snprintf(working_path, working_path_size, "%s/%s/dev",
                        basedir, entry->d_name);
    /* Ignore if truncation occurs. */
    if (path_len != candidate_len + basedir_len + 5)
      continue;

    found_devt = devt_from_file(working_path);
    /* *dev == 0 is a wildcard. */
    if (!*dev || found_devt == *dev) {
      snprintf(name, name_len, "%s", entry->d_name);
      *dev = found_devt;
      found = 1;
      break;
    }

    /* Prevent infinite recursion on symlink loops by limiting depth. */
    if (depth > 5)
      break;

    /* Recurse one level for devices that may have a matching partition. */
    if (major(found_devt) == major(*dev) && minor(*dev) > minor(found_devt)) {
      sprintf(working_path, "%s/%s", basedir, entry->d_name);
      found = match_sysfs_device(name, name_len, working_path, dev, depth + 1);
      if (found > 0)
        break;
    }
  }

  free(working_path);
  free(entry);
  closedir(dirp);
  return found;
}

const char *rootdev_get_partition(const char *dst, size_t len) {
  const char *end = dst + strnlen(dst, len);
  const char *part = end - 1;
  if (!len)
    return NULL;

  if (!isdigit(*part--))
    return NULL;

  while (part > dst && isdigit(*part)) part--;
  part++;

  if (part >= end)
    return NULL;

  return part;
}

void rootdev_strip_partition(char *dst, size_t len) {
  char *part = (char *)rootdev_get_partition(dst, len);
  if (!part)
    return;
  /* For devices that end with a digit, the kernel uses a 'p'
   * as a separator. E.g., mmcblk1p2. */
  if (*(part - 1) == 'p')
    part--;
  *part = '\0';
}

int rootdev_symlink_active(const char *path) {
  int ret = 0;
  /* Don't overwrite an existing link. */
  errno = 0;
  if ((symlink(path, kActiveRoot)) && errno != EEXIST) {
    warn("failed to symlink %s -> %s", kActiveRoot, path);
    ret = -1;
  }
  return ret;
}

int rootdev_get_device(char *dst, size_t size, dev_t dev,
                       const char *search) {
  struct stat active_root_statbuf;

  if (search == NULL)
    search = kDefaultSearchPath;

  /* Check if the -s symlink exists. */
  if ((stat(kActiveRoot, &active_root_statbuf) == 0) &&
      active_root_statbuf.st_rdev == dev) {
    /* Note, if the link is not fully qualified, this won't be
     * either. */
    ssize_t len = readlink(kActiveRoot, dst, PATH_MAX);
    if (len > 0) {
      dst[len] = 0;
      return 0;
    }
    /* If readlink fails or is empty, fall through */
  }

  snprintf(dst, size, "%s", search);
  if (match_sysfs_device(dst, size, dst, &dev, 0) <= 0) {
    fprintf (stderr, "unable to find match\n");
    return 1;
  }

  return 0;
}

/*
 * rootdev_get_device_slave returns results in slave which
 * may be the original device or the name of the slave.
 *
 * Because slave and device may point to the same data,
 * must be careful how they are handled because slave
 * is modified (can't use snprintf).
 */
void rootdev_get_device_slave(char *slave, size_t size, dev_t *dev,
                              const char *device, const char *search) {
  char dst[PATH_MAX];
  int len = 0;
  int i;

  if (search == NULL)
    search = kDefaultSearchPath;

  /*
   * With stacked device mappers, we have to chain through all the levels
   * and find the last device. For example, verity can be stacked on bootcache
   * that is stacked on a disk partition.
   */
  if (slave != device)
    strncpy(slave, device, size);
  slave[size - 1] = '\0';
  for (i = 0; i < MAX_SLAVE_DEPTH; i++) {
    len = snprintf(dst, sizeof(dst), "%s/%s/slaves", search, slave);
    if (len != strlen(device) + strlen(search) + 8) {
      warnx("rootdev_get_device_slave: device name too long");
      return;
    }
    *dev = 0;
    if (match_sysfs_device(slave, size, dst, dev, 0) <= 0) {
      return;
    }
  }
  warnx("slave depth greater than %d at %s", i, slave);
}

int rootdev_create_devices(const char *name, dev_t dev, bool symlink) {
  int ret = 0;
  unsigned int major_num = major(dev);
  unsigned int minor_num = minor(dev);
  int i;
  const struct part_config *config;
  const char *part_s = rootdev_get_partition(name, strlen(name));

  if (part_s == NULL) {
    warnx("create_devices: unable to determine partition");
    return -1;
  }

  switch (atoi(part_s)) {
  case CHROMEOS_PRIMARY_PARTITION:
    config = kPrimaryPart;
    break;
  case CHROMEOS_SECONDARY_PARTITION:
    config = kSecondaryPart;
    break;
  default:
    warnx("create_devices: unable to determine partition: %s",
          part_s);
    return -1;
  }

  for (i = 0; i < kPartitionEntries; ++i) {
    dev = makedev(major_num, minor_num + config[i].offset);
    errno = 0;
    if (mknod(config[i].name,
              S_IFBLK | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
              dev) && errno != EEXIST) {
      warn("failed to create %s", config[i].name);
      return -1;
    }
  }

  if (symlink)
    ret = rootdev_symlink_active(config[0].name);
  return ret;
}

int rootdev_get_path(char *path, size_t size, const char *device,
                     const char *dev_path) {
  int path_len;

  if (!dev_path)
    dev_path = kDefaultDevPath;

  if (!path || !size || !device)
    return -1;

  path_len = snprintf(path, size, "%s/%s", dev_path, device);
  if (path_len != strlen(dev_path) + 1 + strlen(device))
    return -1;

  // TODO(bsimonnet): We should check that |path| exists and is the right
  // device. We don't do this currently as OEMs can add custom SELinux rules
  // which may prevent us from accessing this.
  // See b/24267261.

  return 0;
}

int rootdev_wrapper(char *path, size_t size,
                    bool full, bool strip,
                    dev_t *dev,
                    const char *search, const char *dev_path) {
  int res = 0;
  char devname[PATH_MAX];
  if (!search)
    search = kDefaultSearchPath;
  if (!dev_path)
   dev_path = kDefaultDevPath;
  if (!dev)
    return -1;

  res = rootdev_get_device(devname, sizeof(devname), *dev, search);
  if (res != 0)
    return res;

  if (full)
    rootdev_get_device_slave(devname, sizeof(devname), dev, devname,
                             search);

  /* TODO(wad) we should really just track the block dev, partition number, and
   *           dev path.  When we rewrite this, we can track all the sysfs info
   *           in the class. */
  if (strip) {
    /* When we strip the partition, we don't want get_path to return non-zero
     * because of dev mismatch.  Passing in 0 tells it to not test. */
    *dev = 0;
    rootdev_strip_partition(devname, size);
  }

  res = rootdev_get_path(path, size, devname, dev_path);

  return res;
}

int rootdev(char *path, size_t size, bool full, bool strip) {
  struct stat root_statbuf;
  dev_t _root_dev, *root_dev = &_root_dev;

  /* Yields the containing dev_t in st_dev. */
  if (stat("/data", &root_statbuf) != 0)
    return -1;

  /* Some ABIs (like mips o32) are broken and the st_dev field isn't actually
   * a dev_t.  In that case, pass a pointer to a local dev_t who we took care
   * of truncating the value into.  On sane arches, gcc can optimize this to
   * the same code, so should only be a penalty when the ABI is broken. */
  if (sizeof(root_statbuf.st_dev) == sizeof(*root_dev)) {
    /* Cast is OK since we verified size here. */
    root_dev = (dev_t *)&root_statbuf.st_dev;
  } else {
    *root_dev = root_statbuf.st_dev;
  }

  return rootdev_wrapper(path,
                         size,
                         full,
                         strip,
                         root_dev,
                         NULL,  /* default /sys dir */
                         NULL);  /* default /dev dir */
}
