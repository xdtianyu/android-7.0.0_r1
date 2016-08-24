# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS_32 += -DBITS_PER_LONG=32 -DCONFIG_64BIT
LOCAL_CFLAGS_64 += -DBITS_PER_LONG=64 -DCONFIG_32BIT

main_src_files := gettime.c fio.c ioengines.c init.c stat.c log.c time.c \
                  filesetup.c eta.c verify.c memory.c io_u.c parse.c mutex.c options.c \
                  smalloc.c filehash.c helpers.c profile.c debug.c backend.c \
                  cconv.c client.c filelock.c flow.c gettime-thread.c idletime.c io_u_queue.c \
                  iolog.c json.c libfio.c memalign.c profiles/act.c profiles/tiobench.c server.c \
                  td_error.c diskutil.c blktrace.c trim.c fifo.c cgroup.c

lib_src_files := lib/rbtree.c lib/flist_sort.c lib/getrusage.c lib/hweight.c lib/ieee754.c lib/lfsr.c \
                 lib/num2str.c lib/prio_tree.c lib/rand.c lib/zipf.c lib/inet_aton.c lib/axmap.c \
                 lib/bloom.c lib/linux-dev-lookup.c lib/tp.c

crc_src_files := crc/crc7.c crc/crc16.c crc/crc32.c crc/crc64.c crc/crc32c.c crc/crc32c-intel.c \
                 crc/sha1.c crc/sha256.c crc/sha512.c crc/md5.c crc/test.c crc/xxhash.c \
                 crc/fnv.c crc/murmur3.c

engines_src_files := engines/cpu.c engines/mmap.c engines/null.c engines/net.c \
                     engines/sg.c engines/sync.c

engines_src_files_64 := engines/splice.c

LOCAL_SRC_FILES := $(main_src_files) \
                   $(lib_src_files) \
                   $(crc_src_files) \
                   $(engines_src_files) \

LOCAL_SRC_FILES_64 += $(engines_src_files_64)

LOCAL_MODULE := fio
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := debug

LOCAL_SHARED_LIBRARIES := libdl
LOCAL_STATIC_LIBRARIES := libcutils libz

LOCAL_CFLAGS += -DFIO_VERSION="\"fio-2.2.6\"" \
                -DCONFIG_3ARG_AFFINITY \
                -DCONFIG_CLOCK_GETTIME \
                -DCONFIG_CLOCK_MONOTONIC \
                -DCONFIG_FDATASYNC \
                -DCONFIG_GETOPT_LONG_ONLY \
                -DCONFIG_GETTIMEOFDAY \
                -DCONFIG_IPV6 \
                -DCONFIG_LINUX_FALLOCATE \
                -DCONFIG_LITTLE_ENDIAN \
                -DCONFIG_RLIMIT_MEMLOCK \
                -DCONFIG_RUSAGE_THREAD \
                -DCONFIG_SCHED_IDLE \
                -DCONFIG_SETVBUF \
                -DCONFIG_SFAA \
                -DCONFIG_SOCKLEN_T \
                -DCONFIG_STRCASESTR \
                -DCONFIG_STRSEP \
                -DCONFIG_TCP_NODELAY \
                -DCONFIG_TLS_THREAD \
                -DCONFIG_ZLIB \
                -DFIO_HAVE_CGROUPS \
                -DFIO_INC_DEBUG \
                -DFIO_INTERNAL \
                -DNO_GETMNTENT_R \
                -DNO_INET_NETWORK \
                -D_FILE_OFFSET_BITS=64 \
                -D_FORTIFY_SOURCE=2 \
                -D_GNU_SOURCE \
                -D_LARGEFILE_SOURCE \
                -D__ANDROID__ \
                -O3 \
                -Wall \
                -Wdeclaration-after-statement \
                -Wwrite-strings \
                -ffast-math \
                -fno-omit-frame-pointer \
                -g \
                -rdynamic \
                -std=gnu99 \
                -Wno-pointer-arith \
                -Wno-sign-compare \
                -Wno-unused-parameter \
                -Wno-unused-variable \

LOCAL_CFLAGS_64 += \
                -DCONFIG_LINUX_SPLICE \

include $(BUILD_EXECUTABLE)
