#!/usr/bin/env python3.4

from setuptools import setup
from setuptools import find_packages
import sys


install_requires = [
    'contextlib2',
    'future',
    # mock-1.0.1 is the last version compatible with setuptools <17.1,
    # which is what comes with Ubuntu 14.04 LTS.
    'mock<=1.0.1',
    'pyserial',
]
if sys.version_info < (3,):
    install_requires.append('enum34')

setup(
    name='acts',
    version = '0.9',
    description = 'Android Comms Test Suite',
    license = 'Apache2.0',
    packages = find_packages(),
    include_package_data = False,
    install_requires = install_requires,
    scripts = ['acts/bin/act.py','acts/bin/monsoon.py'],
    url = "http://www.android.com/"
)
