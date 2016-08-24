/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.compatibility.common.tradefed.testtype;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IRuntimeHintProvider;

import java.io.File;
import java.util.Set;

/**
 * Container for Compatibility test info.
 */
public interface IModuleDef extends Comparable<IModuleDef>, IBuildReceiver, IDeviceTest,
        IRemoteTest, IRuntimeHintProvider {

    /**
     * @return The name of this module.
     */
    String getName();

    /**
     * @return a {@link String} to uniquely identify this module.
     */
    String getId();

    /**
     * @return the abi of this test module.
     */
    IAbi getAbi();

    /**
     * @return the {@link Set} of tokens a device must have in order to run this module.
     */
    Set<String> getTokens();

    /**
     * @return the {@link IRemoteTest} that runs the tests.
     */
    IRemoteTest getTest();

    /**
     * Set a list of preparers to allow to run before or after a test.
     * If this list is empty, then all configured preparers will run.
     *
     * @param a list containing the simple name of the preparer to run.
     */
    void setPreparerWhitelist(Set<String> preparerWhitelist);

    /**
     * Runs the module's precondition checks and setup tasks.
     */
    void prepare(boolean skipPrep) throws DeviceNotAvailableException;

}
