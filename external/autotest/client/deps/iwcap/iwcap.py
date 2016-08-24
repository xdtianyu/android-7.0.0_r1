#!/usr/bin/python

import os, common
from autotest_lib.client.bin import utils

version = 1

def setup(topdir):
    srcdir = os.path.join(topdir, 'src')
    os.chdir(srcdir)
    utils.system('make BINDIR=%s install' % (topdir))
    os.chdir(topdir)

pwd = os.getcwd()
utils.update_version(pwd + '/src', True, version, setup, pwd)
