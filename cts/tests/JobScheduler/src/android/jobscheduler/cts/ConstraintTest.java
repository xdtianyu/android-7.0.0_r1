/*
 * Copyright (C) 2014 The Android Open Source Project
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
package android.jobscheduler.cts;

import android.annotation.TargetApi;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.jobscheduler.MockJobService;
import android.jobscheduler.TriggerContentJobService;
import android.test.AndroidTestCase;

/**
 * Common functionality from which the other test case classes derive.
 */
@TargetApi(21)
public abstract class ConstraintTest extends AndroidTestCase {
    /** Force the scheduler to consider the device to be on stable charging. */
    private static final Intent EXPEDITE_STABLE_CHARGING =
            new Intent("com.android.server.task.controllers.BatteryController.ACTION_CHARGING_STABLE");

    /** Environment that notifies of JobScheduler callbacks. */
    static MockJobService.TestEnvironment kTestEnvironment =
            MockJobService.TestEnvironment.getTestEnvironment();
    static TriggerContentJobService.TestEnvironment kTriggerTestEnvironment =
            TriggerContentJobService.TestEnvironment.getTestEnvironment();
    /** Handle for the service which receives the execution callbacks from the JobScheduler. */
    static ComponentName kJobServiceComponent;
    static ComponentName kTriggerContentServiceComponent;
    JobScheduler mJobScheduler;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        kTestEnvironment.setUp();
        kTriggerTestEnvironment.setUp();
        kJobServiceComponent = new ComponentName(getContext(), MockJobService.class);
        kTriggerContentServiceComponent = new ComponentName(getContext(),
                TriggerContentJobService.class);
        mJobScheduler = (JobScheduler) getContext().getSystemService(Context.JOB_SCHEDULER_SERVICE);
        mJobScheduler.cancelAll();
    }

    /**
     * The scheduler will usually only flush its queue of unexpired jobs when the device is
     * considered to be on stable power - that is, plugged in for a period of 2 minutes.
     * Rather than wait for this to happen, we cheat and send this broadcast instead.
     */
    protected void sendExpediteStableChargingBroadcast() {
        getContext().sendBroadcast(EXPEDITE_STABLE_CHARGING);
    }
}
