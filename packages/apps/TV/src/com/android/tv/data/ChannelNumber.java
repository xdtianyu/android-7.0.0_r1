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

package com.android.tv.data;

import android.support.annotation.NonNull;
import android.view.KeyEvent;

/**
 * A convenience class to handle channel number.
 */
public final class ChannelNumber implements Comparable<ChannelNumber> {
    public static final String PRIMARY_CHANNEL_DELIMITER = "-";
    public static final String[] CHANNEL_DELIMITERS = {"-", ".", " "};

    private static final int[] CHANNEL_DELIMITER_KEYCODES = {
        KeyEvent.KEYCODE_MINUS, KeyEvent.KEYCODE_NUMPAD_SUBTRACT, KeyEvent.KEYCODE_PERIOD,
        KeyEvent.KEYCODE_NUMPAD_DOT, KeyEvent.KEYCODE_SPACE
    };

    public String majorNumber;
    public boolean hasDelimiter;
    public String minorNumber;

    public ChannelNumber() {
        reset();
    }

    public ChannelNumber(String major, boolean hasDelimiter, String minor) {
        setChannelNumber(major, hasDelimiter, minor);
    }

    public void reset() {
        setChannelNumber("", false, "");
    }

    public void setChannelNumber(String majorNumber, boolean hasDelimiter, String minorNumber) {
        this.majorNumber = majorNumber;
        this.hasDelimiter = hasDelimiter;
        this.minorNumber = minorNumber;
    }

    @Override
    public String toString() {
        if (hasDelimiter) {
            return majorNumber + PRIMARY_CHANNEL_DELIMITER + minorNumber;
        }
        return majorNumber;
    }

    @Override
    public int compareTo(@NonNull ChannelNumber another) {
        int major = Integer.parseInt(majorNumber);
        int minor = hasDelimiter ? Integer.parseInt(minorNumber) : 0;

        int opponentMajor = Integer.parseInt(another.majorNumber);
        int opponentMinor = another.hasDelimiter
                ? Integer.parseInt(another.minorNumber) : 0;
        if (major == opponentMajor) {
            return minor - opponentMinor;
        }
        return major - opponentMajor;
    }

    public static boolean isChannelNumberDelimiterKey(int keyCode) {
        for (int delimiterKeyCode : CHANNEL_DELIMITER_KEYCODES) {
            if (delimiterKeyCode == keyCode) {
                return true;
            }
        }
        return false;
    }

    public static ChannelNumber parseChannelNumber(String number) {
        if (number == null) {
            return null;
        }
        ChannelNumber ret = new ChannelNumber();
        int indexOfDelimiter = -1;
        for (String delimiter : CHANNEL_DELIMITERS) {
            indexOfDelimiter = number.indexOf(delimiter);
            if (indexOfDelimiter >= 0) {
                break;
            }
        }
        if (indexOfDelimiter == 0 || indexOfDelimiter == number.length() - 1) {
            return null;
        }
        if (indexOfDelimiter < 0) {
            ret.majorNumber = number;
            if (!isInteger(ret.majorNumber)) {
                return null;
            }
        } else {
            ret.hasDelimiter = true;
            ret.majorNumber = number.substring(0, indexOfDelimiter);
            ret.minorNumber = number.substring(indexOfDelimiter + 1);
            if (!isInteger(ret.majorNumber) || !isInteger(ret.minorNumber)) {
                return null;
            }
        }
        return ret;
    }

    public static int compare(String lhs, String rhs) {
        ChannelNumber lhsNumber = parseChannelNumber(lhs);
        ChannelNumber rhsNumber = parseChannelNumber(rhs);
        if (lhsNumber == null && rhsNumber == null) {
            return 0;
        } else if (lhsNumber == null /* && rhsNumber != null */) {
            return -1;
        } else if (lhsNumber != null && rhsNumber == null) {
            return 1;
        }
        return lhsNumber.compareTo(rhsNumber);
    }

    public static boolean isInteger(String string) {
        try {
            Integer.parseInt(string);
        } catch(NumberFormatException e) {
            return false;
        } catch(NullPointerException e) {
            return false;
        }
        return true;
    }
}
