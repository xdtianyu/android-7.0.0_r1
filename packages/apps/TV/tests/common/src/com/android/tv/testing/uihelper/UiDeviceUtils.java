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
package com.android.tv.testing.uihelper;

import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.view.KeyEvent;

/**
 * Static utility methods for {@link UiDevice}.
 */
public final class UiDeviceUtils {

    public static void pressDpad(UiDevice uiDevice, Direction direction) {
        switch (direction) {
            case UP:
                uiDevice.pressDPadUp();
                break;
            case DOWN:
                uiDevice.pressDPadDown();
                break;
            case LEFT:
                uiDevice.pressDPadLeft();
                break;
            case RIGHT:
                uiDevice.pressDPadRight();
                break;
            default:
                throw new IllegalArgumentException(direction.toString());
        }
    }


    public static void pressKeys(UiDevice uiDevice, int... keyCodes) {
        for (int k : keyCodes) {
            uiDevice.pressKeyCode(k);
        }
    }

    /**
     * Parses the string and sends the corresponding individual key preses.
     * <p>
     * <b>Note:</b> only handles 0-9, '.', and '-'.
     */
    public static void pressKeys(UiDevice uiDevice, String keys) {
        for (char c : keys.toCharArray()) {
            if (c >= '0' && c <= '9') {
                uiDevice.pressKeyCode(KeyEvent.KEYCODE_0 + c - '0');
            } else if (c == '-') {
                uiDevice.pressKeyCode(KeyEvent.KEYCODE_MINUS);
            } else if (c == '.') {
                uiDevice.pressKeyCode(KeyEvent.KEYCODE_PERIOD);
            } else {
                throw new IllegalArgumentException(c + " is not supported");
            }
        }
    }

    private UiDeviceUtils() {
    }
}
