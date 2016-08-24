/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BandwidthControllerTest.cpp - unit tests for BandwidthController.cpp
 */

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <android-base/strings.h>

#include "BandwidthController.h"
#include "IptablesBaseTest.h"

FILE *fake_popen(const char *, const char *) {
    return NULL;
};

class BandwidthControllerTest : public IptablesBaseTest {
public:
    BandwidthControllerTest() {
        BandwidthController::execFunction = fake_android_fork_exec;
        BandwidthController::popenFunction = fake_popen;
        BandwidthController::iptablesRestoreFunction = fakeExecIptablesRestore;
    }
    BandwidthController mBw;
};

TEST_F(BandwidthControllerTest, TestSetupIptablesHooks) {
    mBw.setupIptablesHooks();
    std::vector<std::string> expected = {
        "*filter\n"
        ":bw_INPUT -\n"
        ":bw_OUTPUT -\n"
        ":bw_FORWARD -\n"
        ":bw_happy_box -\n"
        ":bw_penalty_box -\n"
        ":bw_data_saver -\n"
        ":bw_costly_shared -\n"
        "COMMIT\n"
        "*raw\n"
        ":bw_raw_PREROUTING -\n"
        "COMMIT\n"
        "*mangle\n"
        ":bw_mangle_POSTROUTING -\n"
        "COMMIT\n\x04"
    };
    expectIptablesRestoreCommands(expected);
}

TEST_F(BandwidthControllerTest, TestEnableBandwidthControl) {
    mBw.enableBandwidthControl(false);
    std::string expectedFlush =
        "*filter\n"
        ":bw_INPUT -\n"
        ":bw_OUTPUT -\n"
        ":bw_FORWARD -\n"
        ":bw_happy_box -\n"
        ":bw_penalty_box -\n"
        ":bw_data_saver -\n"
        ":bw_costly_shared -\n"
        "COMMIT\n"
        "*raw\n"
        ":bw_raw_PREROUTING -\n"
        "COMMIT\n"
        "*mangle\n"
        ":bw_mangle_POSTROUTING -\n"
        "COMMIT\n\x04";
     std::string expectedAccounting =
        "*filter\n"
        "-A bw_INPUT -m owner --socket-exists\n"
        "-A bw_OUTPUT -m owner --socket-exists\n"
        "-A bw_costly_shared --jump bw_penalty_box\n"
        "-A bw_penalty_box --jump bw_happy_box\n"
        "-A bw_happy_box --jump bw_data_saver\n"
        "-A bw_data_saver -j RETURN\n"
        "-I bw_happy_box -m owner --uid-owner 0-9999 --jump RETURN\n"
        "COMMIT\n"
        "*raw\n"
        "-A bw_raw_PREROUTING -m owner --socket-exists\n"
        "COMMIT\n"
        "*mangle\n"
        "-A bw_mangle_POSTROUTING -m owner --socket-exists\n"
        "COMMIT\n\x04";

    expectIptablesRestoreCommands({ expectedFlush, expectedAccounting });
}

TEST_F(BandwidthControllerTest, TestDisableBandwidthControl) {
    mBw.disableBandwidthControl();
    const std::string expected =
        "*filter\n"
        ":bw_INPUT -\n"
        ":bw_OUTPUT -\n"
        ":bw_FORWARD -\n"
        ":bw_happy_box -\n"
        ":bw_penalty_box -\n"
        ":bw_data_saver -\n"
        ":bw_costly_shared -\n"
        "COMMIT\n"
        "*raw\n"
        ":bw_raw_PREROUTING -\n"
        "COMMIT\n"
        "*mangle\n"
        ":bw_mangle_POSTROUTING -\n"
        "COMMIT\n\x04";
    expectIptablesRestoreCommands({ expected });
}

TEST_F(BandwidthControllerTest, TestEnableDataSaver) {
    mBw.enableDataSaver(true);
    std::vector<std::string> expected = {
        "-R bw_data_saver 1 --jump REJECT",
    };
    expectIptablesCommands(expected);

    mBw.enableDataSaver(false);
    expected = {
        "-R bw_data_saver 1 --jump RETURN",
    };
    expectIptablesCommands(expected);
}
