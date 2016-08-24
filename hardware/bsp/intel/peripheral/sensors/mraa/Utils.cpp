/*
 * Copyright (C) 2015 Intel Corporation
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

#include "Utils.hpp"

static int64_t get_timestamp(clockid_t clock_id)
{
  struct timespec ts = {0, 0};

  if (!clock_gettime(clock_id, &ts))
    return 1000000000LL * ts.tv_sec + ts.tv_nsec;
  else  /* in this case errno is set appropriately */
    return -1;
}

int64_t get_timestamp_monotonic()
{
  return get_timestamp(CLOCK_MONOTONIC);
}

void set_timestamp(struct timespec *out, int64_t target_ns)
{
  out->tv_sec  = target_ns / 1000000000LL;
  out->tv_nsec = target_ns % 1000000000LL;
}
