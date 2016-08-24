// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extents.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include <algorithm>
#include <limits>

namespace bsdiff {

/* The maximum accepted value for a given integer type when parsed as a signed
 * long long integer. This is defined to be the smaller of the maximum value
 * that can be represented by this type and LLONG_MAX. This bound allows us to
 * properly check that parsed values do not exceed the capacity of their
 * intended store, regardless of how its size relates to that of a signed long
 * long integer.  Note: this may mean that we are losing the most significant
 * bit of an unsigned 64-bit field (e.g. size_t on some platforms), however
 * still permitting values up to 2^62, which is more than enough for all
 * practical purposes. */
#define MAX_ALLOWED(t)                                            \
  (std::min(static_cast<uint64_t>(std::numeric_limits<t>::max()), \
            static_cast<uint64_t>(std::numeric_limits<long long>::max())))

/* Get the type of a struct field. */
#define FIELD_TYPE(t, f) decltype(((t*)0)->f)


/* Reads a long long integer from |s| into |*val_p|. Returns a pointer to the
 * character immediately following the specified |delim|, unless (a) parsing
 * failed (overflow or no valid digits); (b) the read value is less than
 * |min_val| or greater than |max_val|; (c) the delimiter character is not
 * |delim|, or the string ends although |may_end| is false. In any of these
 * cases, returns NULL. */
const char* read_llong(const char* s,
                       long long* val_p,
                       long long min_val,
                       long long max_val,
                       char delim,
                       int may_end) {
  assert(val_p);
  const char* next_s;
  errno = 0;
  long long val = strtoll(s, (char**)&next_s, 10);
  if (((val == LLONG_MAX || val == LLONG_MIN) && errno == ERANGE) ||
      next_s == s || val < min_val || val > max_val ||
      (*next_s ? *next_s != delim : !may_end))
    return NULL; /* bad value or delimiter */
  *val_p = val;
  if (*next_s)
    next_s++; /* skip delimeter */
  return next_s;
}


/* Reads a comma-separated list of "offset:length" extents from |ex_str|. If
 * |ex_arr| is NULL, then |ex_count| is ignored and it attempts to parse valid
 * extents until the end of the string is reached. Otherwise, stores up to
 * |ex_count| extents into |ex_arr|, which must be of at least this size.
 * Returns the number of correctly parsed extents, or -1 if a malformed extent
 * was found. */
static ssize_t extents_read(const char* ex_str, ex_t* ex_arr, size_t ex_count) {
  size_t i;
  size_t last_i = ex_count - 1;
  if (!ex_arr) {
    ex_count = SIZE_MAX;
    last_i = 0;
  }
  for (i = 0; *ex_str && i < ex_count; i++) {
    long long raw_off = 0, raw_len = 0;
    if (!((ex_str =
               read_llong(ex_str, &raw_off, -1,
                          MAX_ALLOWED(FIELD_TYPE(ex_t, off)), ':', false)) &&
          (ex_str = read_llong(ex_str, &raw_len, 1,
                               MAX_ALLOWED(FIELD_TYPE(ex_t, len)), ',',
                               i >= last_i))))
      return -1; /* parsing error */
    if (ex_arr) {
      ex_arr[i].off = raw_off;
      ex_arr[i].len = raw_len;
    }
  }
  return i;
}


bool ParseExtentStr(const char* ex_str, std::vector<ex_t>* extents) {
  // Sanity check: a string must be provided.
  if (!ex_str)
    return false;

  /* Parse string and count extents. */
  ssize_t ret = extents_read(ex_str, NULL, 0);
  if (ret < 0)
    return false;  // parsing error.

  // Input is good, commit to extent count.
  extents->resize(ret);
  if (ret == 0)
    return true;  // No extents, nothing to do.

  // Populate the extent array.
  extents_read(ex_str, extents->data(), extents->size());
  return true;
}

}  // namespace bsdiff
