# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
print_time() {
  unix_time_from_date="$1"
  awk "BEGIN { printf \""$(printf "%x" $1 | \
       sed -e 's/\(..\)\(..\)\(..\)\(..\)/\\x\4\\x\3\\x\2\\x\1/g')"\" }"
}
