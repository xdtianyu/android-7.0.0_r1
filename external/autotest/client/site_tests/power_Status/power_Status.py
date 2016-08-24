# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time
from autotest_lib.client.bin import test
from autotest_lib.client.cros import power_status



class power_Status(test.test):
    version = 1


    def run_once(self):
        status = power_status.get_status()
        statomatic = power_status.StatoMatic()
        meas = [power_status.SystemPower(status.battery_path)]
        plog = power_status.PowerLogger(meas, seconds_period=1)
        tlog = power_status.TempLogger(None, seconds_period=1)
        plog.start()
        tlog.start()
        time.sleep(2)

        logging.info("battery_energy: %f" % status.battery[0].energy)
        logging.info("linepower_online: %s" % status.on_ac())

        keyvals = plog.calc()
        keyvals.update(tlog.calc())
        keyvals.update(statomatic.publish())
        for k in sorted(keyvals.keys()):
            logging.info("%s: %s" , k, keyvals[k])
        plog.save_results(self.resultsdir)
        tlog.save_results(self.resultsdir)
