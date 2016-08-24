
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, shutil, re
from autotest_lib.client.bin import utils, test

class ddtest(test.test):
    version = 2


    def setup(self, tarball='ddtest.tar.gz'):
        tarball = utils.unmap_url(self.bindir, tarball, self.tmpdir)
        utils.extract_tarball_to_dir(tarball, self.srcdir)
        os.chdir(self.srcdir)
        utils.system('make build')


    def initialize(self):
        self.job.require_gcc()
        self.results = []
        self.job.drop_caches_between_iterations = True


    def run_once(self, dir=None, blocksize=1024, blocknum=262144, threads=20):
        if not dir:
           dir = os.path.join(self.srcdir, 'rdir')
           shutil.rmtree(dir, True)
           os.mkdir(dir)
        args = '-D ' + dir
        args += ' -b %d' % blocksize
        args += ' -n %d' % blocknum
        args += ' -t %d' % threads
        self.results.append(utils.system_output(os.path.join(self.srcdir,
                            'ddtest') + ' ' + args))


    def postprocess(self):
        pattern = re.compile(r"throughput is (.*?) MB/sec")
        for throughput in pattern.findall("\n".join(self.results)):
            self.write_perf_keyval({'throughput':throughput})
