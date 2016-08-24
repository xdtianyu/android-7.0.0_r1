/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cutils/properties.h>
#include <dumpstate.h>

void dumpstate_board()
{
    /* ask init.dragon.rc to dump the charging state and wait */
    property_set("debug.bq25892", "dump");
    sleep(1);

    dump_file("EC Version", "/sys/class/chromeos/cros_ec/version");
    run_command("FW Version", 5, "fwtool", "vboot", NULL);
    dump_file("Charger chip registers", "/data/misc/fw_logs/bq25892.txt");
    dump_file("Battery gas gauge", "/sys/class/power_supply/bq27742-0/uevent");
    dump_file("Touchscreen firmware updater", "/data/misc/touchfwup/rmi4update.txt");
    dump_file("Ion heap", "/d/ion/heaps/system");
};
