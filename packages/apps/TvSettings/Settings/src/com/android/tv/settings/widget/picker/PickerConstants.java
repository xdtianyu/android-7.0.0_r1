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

package com.android.tv.settings.widget.picker;

import android.content.res.Resources;

import com.android.tv.settings.R;

import java.text.DateFormatSymbols;

/**
 * Picker related constants
 */
public class PickerConstants {

    public static class Date {
        public final String[] months;
        public final String[] days31;
        public final String dateSeparator;

        private Date(Resources resources) {
            months = new DateFormatSymbols().getShortMonths();
            days31 = createStringIntArrays(31, false, 2);
            dateSeparator = resources.getString(R.string.date_separator);
        }
    }

    public static class Time {
        public final String[] hours12;
        public final String[] hours24;
        public final String[] minutes;
        public final String[] ampm;
        public final String timeSeparator;

        private Time(Resources resources) {
            hours12 = createStringIntArrays(12, false, 2);
            hours24 = createStringIntArrays(23, true, 2);
            minutes = createStringIntArrays(59, true, 2);
            ampm = resources.getStringArray(R.array.ampm);
            timeSeparator = resources.getString(R.string.time_separator);
        }
    }

    private PickerConstants() {}

    private static String[] createStringIntArrays(int lastNumber, boolean startAtZero, int minLen) {
        int range = startAtZero ? (lastNumber + 1) : lastNumber;
        String format = "%0" + minLen + "d";
        String[] array = new String[range];
        for (int i = 0; i < range; i++) {
            if (minLen > 0) {
                array[i] = String.format(format, startAtZero ? i : (i + 1));
            } else {
                array[i] = String.valueOf(startAtZero ? i : (i + 1));
            }
        }
        return array;
    }

    public static PickerConstants.Date getDateInstance(Resources resources) {
        return new Date(resources);
    }

    public static PickerConstants.Time getTimeInstance(Resources resources) {
        return new Time(resources);
    }
}
