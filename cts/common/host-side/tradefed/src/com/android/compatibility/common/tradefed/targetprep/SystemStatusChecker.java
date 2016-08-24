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

package com.android.compatibility.common.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.ITargetCleaner;
import com.android.tradefed.targetprep.TargetSetupError;

/**
 * An {@link ITargetCleaner} that performs checks on system status and returns a boolean to indicate
 * if the system is in an expected state. Such check maybe performed either prior to or after a
 * module execution.
 * <p>Note: the checker must be reentrant: meaning that the same instance will be called multiple
 * times for each module executed, so it should not leave a state so as to interfere with the checks
 * to be performed for the following modules.
 */
public abstract class SystemStatusChecker implements ITargetCleaner {

    private String mFailureMessage = null;

    /**
     * {@inheritDoc}
     */
   @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        boolean check = preExecutionCheck(device);
        if (!check) {
            CLog.w("Failed pre-module-execution status check, message: %s", mFailureMessage);
        } else {
            CLog.d("Passed system status check");
        }
    }

   /**
    * {@inheritDoc}
    */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable t)
            throws DeviceNotAvailableException {
        boolean check = postExecutionCheck(device);
        if (!check) {
            CLog.w("Failed post-module-execution status check, message: %s", mFailureMessage);
        } else {
            CLog.d("Passed system status check");
        }
    }

    /**
     * Check system condition before test module execution. Subclass should override this method if
     * a check is desirable here. Implementation must return a <code>boolean</code> value to
     * indicate if the status check has passed or failed.
     * <p>It's strongly recommended that system status be checked <strong>after</strong> module
     * execution, and this method may be used for the purpose of caching certain system state
     * prior to module execution.
     *
     * @return result of system status check
     * @throws DeviceNotAvailableException
     */
    public boolean preExecutionCheck(ITestDevice device) throws DeviceNotAvailableException {
        return true;
    }

    /**
     * Check system condition after test module execution. Subclass should override this method if
     * a check is desirable here. Implementation must return a <code>boolean</code> value to
     * indicate if the status check has passed or failed.
     *
     * @return result of system status check
     * @throws DeviceNotAvailableException
     */
    public boolean postExecutionCheck(ITestDevice device) throws DeviceNotAvailableException {
        return true;
    }

    /**
     * Sets failure message when a system status check failed for reporting purpose
     * @param failureMessage
     */
    protected void setFailureMessage(String failureMessage) {
        mFailureMessage = failureMessage;
    }

    /**
     * Returns failure message set by the failed system status check
     * @return
     */
    public String getFailureMessage() {
        return mFailureMessage;
    }
}
