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

public abstract class AbstractTuneInHelper extends AbstractStandardAppHelper {

    public AbstractTuneInHelper(Instrumentation instr) {
        super(instr);
    }

    /**
     * Setup expectation: TuneIn app is open, originally on Browse Page
     *
     * This method attempts a few times until go back to Browse Page
     * and assert fails if it doesn't end up on Browse Page
     */
    public abstract void goToBrowsePage();

    /**
     * Setup expectation: TuneIn is on Browse page
     *
     * This method blocks until on local radio page
     */
    public abstract void goToLocalRadio();

    /**
     * Setup expectation: TuneIn is on Local Radio page
     *
     * This method selects the ith FM from the radio list
     * and goes to radio profile page
     * @param i ith FM
     */
    public abstract void selectFM(int i);

    /**
     * Setup expectation: TuneIn is on radio profile page
     *
     * This method starts playing the radio channel
     */
    public abstract void startChannel();

    /**
     * Setup expectation: TuneIn is on channel page
     *
     * This method stops the channel and stays on the page
     */
    public abstract void stopChannel();
}
