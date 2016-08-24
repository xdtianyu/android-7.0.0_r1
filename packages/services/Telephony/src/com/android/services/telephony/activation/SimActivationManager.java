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
import android.telephony.TelephonyManager;

/**
 * Handles SIM activation requests and runs the appropriate activation process until it completes
 * or fails. When done, sends back a response if needed.
 */
public class SimActivationManager {
    public static final class Triggers {
        public static final int SYSTEM_START = 1;
        public static final int EXPLICIT_REQUEST = 2;
    }

    public interface Response {
        /**
         * @param status See {@link android.telephony.TelephonyManager} for SIM_ACTIVATION_RESULT_*
         *               constants.
         */
        void onResponse(int status);
    }

    public void runActivation(int trigger, Response response) {
        Activator activator = selectActivator(trigger);

        activator.onActivate();

        // TODO: Specify some way to determine if activation is even necessary.

        // TODO: specify some way to return the result.

        if (response != null) {
            response.onResponse(TelephonyManager.SIM_ACTIVATION_RESULT_COMPLETE);
        }
    }

    private Activator selectActivator(int trigger) {
        // TODO: Select among all activator types

        // For now, pick a do-nothing activator
        return new Activator() {

            /** ${inheritDoc} */
                @Override
            public void onActivate() {
                // do something
            }
        };
    }
}
