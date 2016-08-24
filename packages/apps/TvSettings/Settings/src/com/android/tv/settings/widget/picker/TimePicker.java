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

import android.os.Bundle;
import android.text.format.DateFormat;
import android.text.TextUtils;
import android.view.View;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Locale;


public class TimePicker extends Picker {

    private static final String EXTRA_24H_FORMAT = "24h_format";
    private static final String EXTRA_DEFAULT_TO_CURRENT = "delault_to_current";

    private static final int HOURS_IN_HALF_DAY = 12;

    private PickerConstants.Time mConstant;

    private boolean mIs24hFormat = false;
    private boolean mPendingTime = false;
    private int mInitHour;
    private int mInitMinute;
    private boolean mInitIsPm;

    // Column order varies by locale
    public static class ColumnOrder {
        public int hours = 0;
        public int minutes = 1;
        public int amPm = 2;

        public ColumnOrder(boolean is24hFormat) {
            Locale locale = Locale.getDefault();
            String hmaPattern = DateFormat.getBestDateTimePattern(locale, "hma");
            boolean isAmPmAtEnd = hmaPattern.indexOf("a") > hmaPattern.indexOf("m");
            boolean isRtl = TextUtils.getLayoutDirectionFromLocale(locale) == View.LAYOUT_DIRECTION_RTL;
            // Note ordering of calculation below is important

            if (isRtl) {
                // Hours and minutes are always shown with hours on the left
                hours = 1;
                minutes = 0;
            }
            if (!isAmPmAtEnd && !is24hFormat) {
                // Rotate AM/PM indicator to front, if visible
                amPm = 0;
                hours++;
                minutes++;
            }
        }
    }

    private ColumnOrder mColumnOrder;

    public static TimePicker newInstance() {
        return newInstance(true, true);
    }

    public static TimePicker newInstance(boolean is24hFormat, boolean defaultToCurrentTime) {
        TimePicker picker = new TimePicker();
        Bundle args = new Bundle();
        args.putBoolean(EXTRA_24H_FORMAT, is24hFormat);
        args.putBoolean(EXTRA_DEFAULT_TO_CURRENT, defaultToCurrentTime);
        picker.setArguments(args);
        return picker;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mIs24hFormat = getArguments().getBoolean(EXTRA_24H_FORMAT, false);
        boolean useCurrent = getArguments().getBoolean(EXTRA_DEFAULT_TO_CURRENT, false);
        mColumnOrder = new ColumnOrder(mIs24hFormat);

        super.onCreate(savedInstanceState);

        mConstant = PickerConstants.getTimeInstance(getResources());

        if (useCurrent) {
            mPendingTime = true;
            Calendar cal = Calendar.getInstance();
            mInitHour = cal.get(Calendar.HOUR_OF_DAY);

            if (!mIs24hFormat) {
                if (mInitHour >= HOURS_IN_HALF_DAY) {
                    // PM case, valid hours: 12-23
                    mInitIsPm = true;
                    if (mInitHour > HOURS_IN_HALF_DAY) {
                        mInitHour = mInitHour - HOURS_IN_HALF_DAY;
                    }
                } else {
                    // AM case, valid hours: 0-11
                    mInitIsPm = false;
                    if (mInitHour == 0) {
                        mInitHour = HOURS_IN_HALF_DAY;
                    }
                }
            }

            mInitMinute = cal.get(Calendar.MINUTE);
        }
    }

    @Override
    public void onResume() {
        if (mPendingTime) {
            mPendingTime = false;
            setTime(mInitHour, mInitMinute, mInitIsPm);
        }
        super.onResume();
    }

    protected boolean setTime(int hour, int minute, boolean isPm) {
        if (minute < 0 || minute > 59) {
            return false;
        }

        if (mIs24hFormat) {
            if (hour < 0 || hour > 23) {
                return false;
            }
        } else {
            if (hour < 1 || hour > 12) {
                return false;
            }
        }

        updateSelection(mColumnOrder.hours, mIs24hFormat ? hour : (hour - 1));
        updateSelection(mColumnOrder.minutes, minute);
        if (!mIs24hFormat) {
            updateSelection(mColumnOrder.amPm, isPm ? 1 : 0);
        }

        return true;
    }

    @Override
    protected ArrayList<PickerColumn> getColumns() {
        ArrayList<PickerColumn> ret = new ArrayList<>();
        // Fill output with nulls, then place actual columns according to order
        int capacity = mIs24hFormat ? 2 : 3;
        for (int i = 0; i < capacity; i++) {
            ret.add(null);
        }
        PickerColumn hours = new PickerColumn(mIs24hFormat ? mConstant.hours24 : mConstant.hours12);
        PickerColumn minutes = new PickerColumn(mConstant.minutes);
        ret.set(mColumnOrder.hours, hours);
        ret.set(mColumnOrder.minutes, minutes);

        if (!mIs24hFormat) {
            PickerColumn ampm = new PickerColumn(mConstant.ampm);
            ret.set(mColumnOrder.amPm, ampm);
        }
        return ret;
    }

    @Override
    protected String getSeparator() {
        return mConstant.timeSeparator;
    }
}
