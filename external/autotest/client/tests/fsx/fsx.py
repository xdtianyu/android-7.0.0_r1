# This requires aio headers to build.
# Should work automagically out of deps now.

# NOTE - this should also have the ability to mount a filesystem,
# run the tests, unmount it, then fsck the filesystem
import os
from autotest_lib.client.bin import test, utils


class fsx(test.test):
    """Test to run fsx-linux."""
    version = 3

    def initialize(self):
        self.job.require_gcc()


    # http://www.zip.com.au/~akpm/linux/patches/stuff/ext3-tools.tar.gz
    def setup(self, tarball = 'ext3-tools.tar.gz'):
        self.tarball = utils.unmap_url(self.bindir, tarball, self.tmpdir)
        utils.extract_tarball_to_dir(self.tarball, self.srcdir)

        os.chdir(self.srcdir)
        for p in ['0001-Minor-fixes-to-PAGE_SIZE-handling.patch',
                  '0002-Enable-cross-compiling-for-fsx.patch',
                  '0003-Fix-Link-Options.patch']:
            utils.system('patch -p1 < ../%s' % p)
        utils.system('make fsx-linux')


    def run_once(self, dir=None, repeat=100000):
        args = '-N %s' % repeat
        if not dir:
            dir = self.tmpdir
        os.chdir(dir)
        utils.system(' '.join([os.path.join(self.srcdir,'/fsx-linux'),
                               args, 'poo']))
