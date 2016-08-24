#!/bin/bash
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e
echo "This assumes you have python 2.4 or later installed"
for json in *.json; do
   echo "Validating $json"
   python -c "import json; json.load(open('$json'))" || \
      printf "\n\n$json is broken!!!!\n\n"
done
