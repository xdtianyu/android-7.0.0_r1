/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Interface for root device discovery via sysfs with optional
 * bells and whistles.
 */
#ifndef ROOTDEV_ROOTDEV_H_
#define ROOTDEV_ROOTDEV_H_

#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * rootdev: returns the path to the root device in @path
 * @path: pre-allocated char array the result will be written to
 * @size: size of @path
 * @full: whether to try to do full resolution. E.g., device-mapper
 * @strip: whether to remove the partition # or not.
 *
 * Returns 0 on success, non-zero on error.
 */
int rootdev(char *path, size_t size, bool full, bool strip);

/* All interface below this point will most definitely be C specific. If
 * we rewrite this as a C++ class, only the above generic interface should
 * still be provided.
 */

/**
 * rootdev_wrapper: rootdev equivalent with paths can be substituted.
 */
int rootdev_wrapper(char *path, size_t size,
                    bool full, bool strip,
                    dev_t *dev,
                    const char *search, const char *dev_path);
/**
 * rootdev_get_device: finds the /dev path for @dev
 * @dst: destination char array
 * @size: size of @dst
 * @dev: dev_t specifying the known root device
 * @search: path to search under. NULL for default.
 *
 * Returns 0 on success, non-zero on error.
 *
 * The name of the devices is placed in @dst. It will not
 * be qualified with /dev/ by default.
 */
int rootdev_get_device(char *dst, size_t size, dev_t dev,
                       const char *search);

/**
 * rootdev_get_device_slave: returns the first device under @device/slaves
 * @slave: destination char array for storing the result
 * @size: size of @slave
 * @dev: pointer to a dev_t to populate
 * @device: name of the device to probe, like "sdb"
 * @search: path to search under. NULL for default.
 *
 * It is safe for @device == @slave.
 */
void rootdev_get_device_slave(char *slave, size_t size, dev_t *dev,
                              const char *device, const char *search);

/**
 * rootdev_get_path: converts a device name to a path in the device tree
 * @path: char array to store the path
 * @size: size of @devpath
 * @device: name of the device
 * @dev_path: path to dev tree. NULL for default (/dev)
 *
 * A @dev of 0 is ignored.
 *
 * @path is populated for all return codes.
 * Returns 0 on success and non-zero on error:
 * -1 on unexpected errors (@path may be invalid)
 *
 * Nb, this function does NOT search /dev for a match.  It performs a normal
 *     string concatenation.
 *     We can't check if the device actually exists as vendors may create an
 *     SELinux context we don't know about for it (in which case, this function
 *     would always fail).
 */
int rootdev_get_path(char *path, size_t size, const char *device,
                     const char *dev_path);

const char *rootdev_get_partition(const char *dst, size_t len);
void rootdev_strip_partition(char *dst, size_t len);
int rootdev_symlink_active(const char *path);
int rootdev_create_devices(const char *name, dev_t dev, bool symlink);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ROOTDEV_ROOTDEV_H_ */
