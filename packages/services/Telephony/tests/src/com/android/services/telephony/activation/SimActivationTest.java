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

package com.android.services.telephony.activation;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.services.telephony.Log;

public class SimActivationTest extends AndroidTestCase {

    private static Object mActivationLock = new Object();
    private ResponseReceiver mResponseReceiver;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mResponseReceiver = new ResponseReceiver(mActivationLock);
        mResponseReceiver.register(context());
    }

    /** ${inheritDoc} */
    @Override
    protected void tearDown() throws Exception {
        mResponseReceiver.unregister();
        super.tearDown();
    }

    @SmallTest
    public void testSimActivationResponse() throws Exception {
        Log.i(this, "Running activation test");

        Intent responseIntent = new Intent(ResponseReceiver.ACTION_ACTIVATION_RESPONSE);
        responseIntent.setPackage(context().getPackageName());
        PendingIntent pendingResponse = PendingIntent.getBroadcast(
                context(), 0, responseIntent, 0);

        Intent intent = new Intent(Intent.ACTION_SIM_ACTIVATION_REQUEST);
        intent.putExtra(Intent.EXTRA_SIM_ACTIVATION_RESPONSE, pendingResponse);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        Log.i(this, "sent intent");
        context().startActivity(intent);
        synchronized (mActivationLock) {
            Log.i(this, "waiting ");
            mActivationLock.wait(5000);
            Log.i(this, "unwaiting");
        }
        assertTrue(ResponseReceiver.responseReceived);
    }

    private Context context() {
        return getContext();
    }
}
