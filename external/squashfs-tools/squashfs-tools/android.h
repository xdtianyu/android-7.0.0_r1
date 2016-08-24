/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ANDROID_H_
#define _ANDROID_H_

#include <stdint.h>
typedef void (*fs_config_func_t)(const char *path, int dir, const char *target_out_path,
                unsigned *uid, unsigned *gid, unsigned *mode, uint64_t *capabilities);

void alloc_mounted_path(const char *mount_point, const char *subpath, char **mounted_path);
void android_fs_config(fs_config_func_t fs_config_func, const char *path, struct stat *stat, const char *target_out_path, uint64_t *capabilities);
struct selabel_handle *get_sehnd(const char *context_file);
char *set_selabel(const char *path, unsigned int mode, struct selabel_handle *sehnd);
struct vfs_cap_data set_caps(uint64_t capabilities);

#endif
