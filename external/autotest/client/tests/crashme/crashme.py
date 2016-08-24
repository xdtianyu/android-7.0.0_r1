# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from autotest_lib.client.bin import test, utils


class crashme(test.test):
    """
    Runs the crashme random code test suite.

    crashme [+]<nbytes>[.inc] <srand> <ntrys> [nsub] [verbose]

      [NBYTES]
         The [NBYTES] should be an integer, specifying the size of
         the random data string in bytes. If given negative then the
         bytes are printed instead of being executed. If given with
         an explicit plus sign then the storage for the bytes is
         freshly malloc'ed each time. This can have an effect on
         machines with seperate I and D cache mechanisms. The
         argument can also have a dot in it, X.Y, in which case Y is
         a increment for a pointer into the random data. The buffer
         is recalculated only when the pointer gets near the end of
         the data.

      [SRAND]
         The [SRAND] is an input seed to the random number generator,
         passed to srand.

      [NTRIES]
         The [NTRIES] is how many times to loop before exiting
         normally from the program.

      [NSUB]
         The [NSUB] is optional, the number of vfork subprocesses
         running all at once. If negative run one after another. If
         given as a time hrs:mns:scs (hours, minutes, seconds) then
         one sub-process will be run to completion, followed by
         another, until the time limit has been reached. If this
         argument is given as the empty string or . then it is
         ignored.

         When in sequential-subprocess mode there is a 30 second time
         limit on each subprocess. This is to allow the
         instruction-set-space random walk to continue when a
         process bashes itself into an infinite loop. For example,
         the ntrys can be bashed to a very large number with nbytes
         bashed to zero. (10 second limit on Windows NT).

         The SRAND argument is incremented by one for each subprocess.

      [VERBOSE]
         The [VERBOSE] arg is optional. 0 is the least verbose, 5 the
         most.
"""
    version = 2

    def initialize(self):
        self.job.require_gcc()

    def setup(self, tarball = 'crashme_2.4.orig.tar.bz2'):
        tarball = utils.unmap_url(self.bindir, tarball, self.tmpdir)
        utils.extract_tarball_to_dir(tarball, self.srcdir)
        os.chdir(self.srcdir)
        utils.system('patch -p 1 <../crashme_2.4-9.diff')
        utils.make()

    def run_once(self, args_list=''):
        if args_list:
            args = args_list
        else:
            args = ''

        crashme_path = os.path.join(self.srcdir, 'crashme')
        utils.system("%s %s" % (crashme_path, args))
