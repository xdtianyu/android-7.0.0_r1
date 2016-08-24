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

#ifndef UTILS_HPP
#define UTILS_HPP

#include <time.h>

/**
 * Get current timestamp in nanoseconds
 * @return time in nanoseconds
 */
int64_t get_timestamp_monotonic();

/**
 * Populates a timespec data structure from a int64_t timestamp
 * @param out what timespec to populate
 * @param target_ns timestamp in nanoseconds
 */
void set_timestamp(struct timespec *out, int64_t target_ns);

#endif  // UTILS_HPP
