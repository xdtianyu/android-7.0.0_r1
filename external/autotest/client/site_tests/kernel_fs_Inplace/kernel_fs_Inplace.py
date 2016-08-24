
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, shutil, re
from autotest_lib.client.bin import utils, test

class kernel_fs_Inplace(test.test):
    version = 2


    def setup(self, tarball='kernel_fs_Inplace.tar.gz'):
        tarball = utils.unmap_url(self.bindir, tarball, self.tmpdir)
        utils.extract_tarball_to_dir(tarball, self.srcdir)
        os.chdir(self.srcdir)
        utils.system('make build')


    def initialize(self):
        self.job.require_gcc()
        self.results = []
        self.job.drop_caches_between_iterations = True


    def run_once(self, dir=None, iosize=4096, num_iter=100000,
      scratch=None, results_file=None):
        if not dir:
           dir = os.path.join(self.srcdir, 'rdir')
           shutil.rmtree(dir, True)
           os.mkdir(dir)
        if not scratch:
           scratch = dir + '/.scratch'
        if not results_file:
           results_file = dir + '/kernel_fs_Inplace.results'
        args  = ' %d' % iosize
        args += ' %d' % num_iter
        args += ' ' + scratch
        args += ' ' + results_file
        self.results.append(utils.system_output(os.path.join(self.srcdir,
                            'inplace') + ' ' + args))
