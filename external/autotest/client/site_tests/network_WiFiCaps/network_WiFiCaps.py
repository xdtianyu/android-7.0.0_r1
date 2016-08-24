# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re, string

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class network_WiFiCaps(test.test):
    version = 1

    def setup(self):
        self.job.setup_dep(['iwcap'])
        # create a empty srcdir to prevent the error that checks .version
        if not os.path.exists(self.srcdir):
            os.mkdir(self.srcdir)


    def __parse_iwcap(self, lines):
        """Parse the iwcap output"""

        results = {}
        parse_re = re.compile(r'([a-z0-9]*):[ ]*(.*)')
        for line in lines.split('\n'):
            line = line.rstrip()
            logging.info('==> %s' %line)
            match = parse_re.search(line)
            if match:
                results[match.group(1)] = match.group(2)
                continue
        return results


    def __run_iwcap(self, phy, caps):
        dir = os.path.join(self.autodir, 'deps', 'iwcap', 'iwcap')
        iwcap = utils.run(dir + ' ' + phy + ' ' + string.join(caps))
        return self.__parse_iwcap(iwcap.stdout)

    def run_once(self):
        phy = utils.system_output("iw list | awk '/^Wiphy/ {print $2}'")
        if not phy or 'phy' not in phy:
            raise error.TestFail('WiFi Physical interface not found')

        requiredCaps = {
            'sta'    : 'true',        # station mode

            '24ghz'  : 'true',        # 2.4GHz band
            '11b'    : 'true',
            '11g'    : 'true',

            '5ghz'   : 'true',        # 5GHz band
            '11a'    : 'true',

            '11n'    : 'true',        # 802.11n (both bands)
            'ht40'   : 'true',        # HT40
            'sgi40'  : 'true',        # Short GI in HT40
        }

        dep = 'iwcap'
        dep_dir = os.path.join(self.autodir, 'deps', dep)
        self.job.install_pkg(dep, 'dep', dep_dir)

        results = self.__run_iwcap(phy, requiredCaps.keys())
        for cap in requiredCaps:
            if not cap in results:
                raise error.TestFail('Internal error, ' +
                    'capability "%s" not handled' % cap)
            if results[cap] != requiredCaps[cap]:
                raise error.TestFail('Requirement not met: ' +
                    'cap "%s" is "%s" but expected "%s"'
                    % (cap, results[cap], requiredCaps[cap]))
