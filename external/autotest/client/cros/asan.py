# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import utils


_ASAN_SYMBOL = "__asan_init"


def running_on_asan(binary="debugd"):
    """Returns whether we're running on ASan.

    @param binary: file to test for ASan symbols.
    """
    # -q, --quiet         * Only output 'bad' things
    # -F, --format <arg>  * Use specified format for output
    # -g, --gmatch        * Use regex rather than string compare (with -s)
    # -s, --symbol <arg>  * Find a specified symbol
    scanelf_command = "scanelf -qF'%s#F'"
    scanelf_command += " -gs %s `which %s`" % (_ASAN_SYMBOL, binary)
    symbol = utils.system_output(scanelf_command)
    logging.debug("running_on_asan(): symbol: '%s', _ASAN_SYMBOL: '%s'",
                  symbol, _ASAN_SYMBOL)
    return symbol != ""
