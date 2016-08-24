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
 * limitations under the License
 */

package com.android.tv.settings.system;

import android.app.AlarmManager;
import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.text.format.DateFormat;

import com.android.tv.settings.R;
import com.android.tv.settings.dialog.old.ContentFragment;
import com.android.tv.settings.dialog.old.DialogActivity;
import com.android.tv.settings.widget.picker.DatePicker;
import com.android.tv.settings.widget.picker.Picker;
import com.android.tv.settings.widget.picker.PickerConstants;
import com.android.tv.settings.widget.picker.TimePicker;

import java.util.Calendar;
import java.util.List;

public class SetDateTimeActivity extends DialogActivity {

    private static final String EXTRA_PICKER_TYPE = "SetDateTimeActivity.pickerType";
    private static final String TYPE_DATE = "date";
    private static final String TYPE_TIME = "time";

    private static final int HOURS_IN_HALF_DAY = 12;

    private class DatePickerListener implements Picker.ResultListener {
        @Override
        public void onCommitResult(List<String> result) {
            String formatOrder = new String(
                    DateFormat.getDateFormatOrder(SetDateTimeActivity.this)).toUpperCase();
            int yIndex = formatOrder.indexOf('Y');
            int mIndex = formatOrder.indexOf('M');
            int dIndex = formatOrder.indexOf('D');
            if (yIndex < 0 || mIndex < 0 || dIndex < 0 ||
                    yIndex > 2 || mIndex > 2 || dIndex > 2) {
                // Badly formatted input. Use default order.
                mIndex = 0;
                dIndex = 1;
                yIndex = 2;
            }
            String month = result.get(mIndex);
            int day = Integer.parseInt(result.get(dIndex));
            int year = Integer.parseInt(result.get(yIndex));
            int monthInt = 0;
            String[] months = PickerConstants.getDateInstance(getResources()).months;
            int totalMonths = months.length;
            for (int i = 0; i < totalMonths; i++) {
                if (months[i].equals(month)) {
                    monthInt = i;
                }
            }

            setDate(SetDateTimeActivity.this, year, monthInt, day);
            finish();
        }
    }

    private class TimePickerListener implements Picker.ResultListener {
        @Override
        public void onCommitResult(List<String> result) {
            boolean is24hFormat = isTimeFormat24h(SetDateTimeActivity.this);
            final TimePicker.ColumnOrder columnOrder = new TimePicker.ColumnOrder(is24hFormat);
            int hour = Integer.parseInt(result.get(columnOrder.hours));
            int minute = Integer.parseInt(result.get(columnOrder.minutes));
            if (!is24hFormat) {
                String ampm = result.get(columnOrder.amPm);
                if (ampm.equals(getResources().getStringArray(R.array.ampm)[1])) {
                    // PM case, valid hours: 12-23
                    hour = (hour % HOURS_IN_HALF_DAY) + HOURS_IN_HALF_DAY;
                } else {
                    // AM case, valid hours: 0-11
                    hour = hour % HOURS_IN_HALF_DAY;
                }
            }

            setTime(SetDateTimeActivity.this, hour, minute);
            finish();
        }
    }

    public static Intent getSetDateIntent(Context context) {
        final Intent intent = new Intent(context, SetDateTimeActivity.class);
        intent.putExtra(EXTRA_PICKER_TYPE, TYPE_DATE);
        return intent;
    }

    public static Intent getSetTimeIntent(Context context) {
        final Intent intent = new Intent(context, SetDateTimeActivity.class);
        intent.putExtra(EXTRA_PICKER_TYPE, TYPE_TIME);
        return intent;
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final String pickerType = getIntent().getStringExtra(EXTRA_PICKER_TYPE);
        final Picker.ResultListener resultListener;
        if (TYPE_DATE.equals(pickerType)) {
            resultListener = new DatePickerListener();
        } else if (TYPE_TIME.equals(pickerType)) {
            resultListener = new TimePickerListener();
        } else {
            throw new IllegalArgumentException("Must specify " + EXTRA_PICKER_TYPE + " in intent");
        }

        if (savedInstanceState == null) {
            final Fragment contentFragment;
            final Fragment actionFragment;

            if (TYPE_DATE.equals(pickerType)) {
                contentFragment = ContentFragment.newInstance(getString(R.string.system_set_date),
                        getString(R.string.system_date), null, R.drawable.ic_access_time_132dp,
                        getColor(R.color.icon_background));
                actionFragment = DatePicker
                        .newInstance(new String(DateFormat.getDateFormatOrder(this)));
            } else {
                contentFragment = ContentFragment.newInstance(getString(R.string.system_set_time),
                        getString(R.string.system_time), null, R.drawable.ic_access_time_132dp,
                        getColor(R.color.icon_background));
                actionFragment = TimePicker.newInstance(isTimeFormat24h(this), true);
            }

            setContentAndActionFragments(contentFragment, actionFragment);
        }

        getFragmentManager().executePendingTransactions();

        final Picker picker = (Picker) getActionFragment();
        picker.setResultListener(resultListener);
    }

    public static void setDate(Context context, int year, int month, int day) {
        Calendar c = Calendar.getInstance();

        c.set(Calendar.YEAR, year);
        c.set(Calendar.MONTH, month);
        c.set(Calendar.DAY_OF_MONTH, day);
        long when = c.getTimeInMillis();

        ((AlarmManager) context.getSystemService(Context.ALARM_SERVICE)).setTime(when);
    }

    public static void setTime(Context context, int hourOfDay, int minute) {
        Calendar c = Calendar.getInstance();

        c.set(Calendar.HOUR_OF_DAY, hourOfDay);
        c.set(Calendar.MINUTE, minute);
        c.set(Calendar.SECOND, 0);
        c.set(Calendar.MILLISECOND, 0);
        long when = c.getTimeInMillis();

        ((AlarmManager) context.getSystemService(Context.ALARM_SERVICE)).setTime(when);
    }

    private static boolean isTimeFormat24h(Context context) {
        return DateFormat.is24HourFormat(context);
    }
}
