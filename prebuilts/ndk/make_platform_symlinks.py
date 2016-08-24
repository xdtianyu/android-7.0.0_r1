#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import os


THIS_DIR = os.path.realpath(os.path.dirname(__file__))


def api_str(api_level):
    return 'android-{}'.format(api_level)


def symlink_gaps(first, last):
    for api in xrange(first, last + 1):
        if os.path.exists(api_str(api)):
            continue

        # Not all API levels have a platform directory. Make a symlink to the
        # previous API level. For example, symlink android-10 to android-9.
        assert api != 3
        os.symlink(api_str(api - 1), api_str(api))


def main():
    os.chdir(os.path.join(THIS_DIR, 'current/platforms'))

    first_api = 3
    first_multiarch_api = 9
    first_lp64_api = 21
    latest_api = 23

    for api in xrange(first_api, first_multiarch_api):
        if not os.path.exists(api_str(api)):
            continue

        for arch in ('arch-x86', 'arch-mips'):
            src = os.path.join('..', api_str(first_multiarch_api), arch)
            dst = os.path.join(api_str(api), arch)
            if os.path.islink(dst):
                os.unlink(dst)
            os.symlink(src, dst)

    for api in xrange(first_api, first_lp64_api):
        if not os.path.exists(api_str(api)):
            continue

        for arch in ('arch-arm64', 'arch-mips64', 'arch-x86_64'):
            src = os.path.join('..', api_str(first_lp64_api), arch)
            dst = os.path.join(api_str(api), arch)
            if os.path.islink(dst):
                os.unlink(dst)
            os.symlink(src, dst)

    symlink_gaps(first_api, latest_api)


if __name__ == '__main__':
    main()
