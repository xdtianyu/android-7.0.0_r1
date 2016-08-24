/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.platform.test.helpers;

import android.app.Instrumentation;

public abstract class AbstractSettingsHelper extends AbstractStandardAppHelper {

    public AbstractSettingsHelper(Instrumentation instr) {
        super(instr);
    }

    /**
     * Setup expectation: Settings is open.
     *
     * This method will fling through the settings list numberOfFlings
     * number of times or until the bottom of Settings is reached,
     * whichever happens first.
     * @param numberOfFlings number of flings needed
     */
    public abstract void scrollThroughSettings(int numberOfFlings) throws Exception;

    /**
     * Setup expectation: Settings is open.
     *
     * This method will fling through the settings list until it
     * reaches the top of the list.
     */
    public abstract void flingSettingsToStart() throws Exception;
}
