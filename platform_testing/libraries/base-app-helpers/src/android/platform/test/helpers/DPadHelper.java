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
 * limitations under the License
 */

package android.platform.test.helpers;

import android.app.Instrumentation;
import android.os.SystemClock;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;

public class DPadHelper {

    private static final String TAG = DPadHelper.class.getSimpleName();
    private static final long DPAD_DEFAULT_WAIT_TIME_MS = 1000; // 1 sec
    private static DPadHelper mInstance;
    private UiDevice mDevice;


    private DPadHelper(Instrumentation instrumentation) {
        mDevice = UiDevice.getInstance(instrumentation);
    }

    public static DPadHelper getInstance(Instrumentation instrumentation) {
        if (mInstance == null) {
            mInstance = new DPadHelper(instrumentation);
        }
        return mInstance;
    }

    public void pressDPad(Direction direction) {
        pressDPad(direction, 1, DPAD_DEFAULT_WAIT_TIME_MS);
    }

    public void pressDPad(Direction direction, long repeat) {
        pressDPad(direction, repeat, DPAD_DEFAULT_WAIT_TIME_MS);
    }

    /**
     * Presses DPad button of the same direction for the count times.
     * It sleeps between each press for DPAD_DEFAULT_WAIT_TIME_MS.
     *
     * @param direction the direction of the button to press.
     * @param repeat the number of times to press the button.
     * @param timeout the timeout for the wait.
     */
    public void pressDPad(Direction direction, long repeat, long timeout) {
        int iteration = 0;
        while (iteration++ < repeat) {
            switch (direction) {
                case LEFT:
                    mDevice.pressDPadLeft();
                    break;
                case RIGHT:
                    mDevice.pressDPadRight();
                    break;
                case UP:
                    mDevice.pressDPadUp();
                    break;
                case DOWN:
                    mDevice.pressDPadDown();
                    break;
            }
            SystemClock.sleep(timeout);
        }
    }
}
