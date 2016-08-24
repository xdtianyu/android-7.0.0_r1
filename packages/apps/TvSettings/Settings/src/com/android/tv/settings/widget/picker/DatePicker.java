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
import android.view.View;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.GregorianCalendar;

public class DatePicker extends Picker {

    private static final String EXTRA_START_YEAR = "start_year";
    private static final String EXTRA_YEAR_RANGE = "year_range";
    private static final String EXTRA_DEFAULT_TO_CURRENT = "default_to_current";
    private static final String EXTRA_FORMAT = "date_format";
    private static final int MAX_YEAR = 2037; // It's time to party like it's 1999
    private static final int DEFAULT_YEAR_RANGE = 67;

    private PickerConstants.Date mConstant;

    private String[] mYears;
    private int mStartYear;
    private int mYearRange;
    private String[] mDayStrings = null;
    private int mColMonthIndex = 0;
    private int mColDayIndex = 1;
    private int mColYearIndex = 2;

    private boolean mPendingDate = false;
    private int mInitYear;
    private int mInitMonth;
    private int mInitDay;

    private int mSelectedYear;
    private int mSelectedMonth;

    /**
     * Creates a new instance of DatePicker
     *
     * @param format         String containing a permutation of Y, M and D, indicating the order
     *                       of the fields Year, Month and Day to be displayed in the DatePicker.
     */
    public static DatePicker newInstance(String format) {
        return newInstance(format, MAX_YEAR - DEFAULT_YEAR_RANGE, DEFAULT_YEAR_RANGE, true);
    }

    /**
     * Creates a new instance of DatePicker
     *
     * @param format         String containing a permutation of Y, M and D, indicating the order
     *                       of the fields Year, Month and Day to be displayed in the DatePicker.
     * @param startYear      The lowest number to be displayed in the Year selector.
     * @param yearRange      Number of entries to be displayed in the Year selector.
     * @param startOnToday   Indicates if the date should be set to the current date by default.
     */
    public static DatePicker newInstance(String format, int startYear, int yearRange,
            boolean startOnToday) {
        DatePicker datePicker = new DatePicker();
        if (startYear <= 0) {
            throw new IllegalArgumentException("The start year must be > 0. Got " + startYear);
        }
        if (yearRange <= 0) {
            throw new IllegalArgumentException("The year range must be > 0. Got " + yearRange);
        }
        Bundle args = new Bundle();
        args.putString(EXTRA_FORMAT, format);
        args.putInt(EXTRA_START_YEAR, startYear);
        args.putInt(EXTRA_YEAR_RANGE, yearRange);
        args.putBoolean(EXTRA_DEFAULT_TO_CURRENT, startOnToday);
        datePicker.setArguments(args);
        return datePicker;
    }

    private void initYearsArray(int startYear, int yearRange) {
        mYears = new String[yearRange];
        for (int i = 0; i < yearRange; i++) {
            mYears[i] = String.format("%d", startYear + i);
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mConstant = PickerConstants.getDateInstance(getResources());

        mYearRange = getArguments().getInt(EXTRA_YEAR_RANGE, DEFAULT_YEAR_RANGE);
        mStartYear = getArguments().getInt(EXTRA_START_YEAR, MAX_YEAR - mYearRange);
        boolean startOnToday = getArguments().getBoolean(EXTRA_DEFAULT_TO_CURRENT, false);
        initYearsArray(mStartYear, mYearRange);

        mDayStrings = mConstant.days31;

        String format = getArguments().getString(EXTRA_FORMAT);
        if (format != null && !format.isEmpty()) {
            format = format.toUpperCase();

            int yIndex = format.indexOf('Y');
            int mIndex = format.indexOf('M');
            int dIndex = format.indexOf('D');
            if (yIndex < 0 || mIndex < 0 || dIndex < 0 || yIndex > 2 || mIndex > 2 || dIndex > 2) {
                // Badly formatted input. Use default order.
                mColMonthIndex = 0;
                mColDayIndex = 1;
                mColYearIndex = 2;
            } else {
                mColMonthIndex = mIndex;
                mColDayIndex = dIndex;
                mColYearIndex = yIndex;
            }
        }

        if (startOnToday) {
            mPendingDate = true;
            Calendar cal = Calendar.getInstance();
            mInitYear = cal.get(Calendar.YEAR);
            mInitMonth = cal.get(Calendar.MONTH);
            mInitDay = cal.get(Calendar.DATE);
        }
    }

    @Override
    public void onResume() {
        if (mPendingDate) {
            mPendingDate = false;
            setDate(mInitYear, mInitMonth, mInitDay);
        }
        super.onResume();
    }

    @Override
    protected ArrayList<PickerColumn> getColumns() {
        ArrayList<PickerColumn> ret = new ArrayList<>();
        PickerColumn months = new PickerColumn(mConstant.months);
        PickerColumn days = new PickerColumn(mDayStrings);
        PickerColumn years = new PickerColumn(mYears);

        for (int i = 0; i < 3; i++) {
            if (i == mColYearIndex) {
                ret.add(years);
            } else if (i == mColMonthIndex) {
                ret.add(months);
            } else if (i == mColDayIndex) {
                ret.add(days);
            }
        }

        return ret;
    }

    @Override
    protected String getSeparator() {
        return mConstant.dateSeparator;
    }

    protected boolean setDate(int year, int month, int day) {
        if (year < mStartYear || year > (mStartYear + mYearRange)) {
            return false;
        }

        // Test to see if this is a valid date
        try {
            GregorianCalendar cal = new GregorianCalendar(year, month, day);
            cal.setLenient(false);
            cal.getTime();
        } catch (IllegalArgumentException e) {
            return false;
        }

        mSelectedYear = year;
        mSelectedMonth = month;
        updateDayStrings(year, month);

        updateSelection(mColYearIndex, year - mStartYear);
        updateSelection(mColMonthIndex, month);
        updateSelection(mColDayIndex, day - 1);

        return true;
    }

    @Override
    public void onScroll(int column, View v, int position) {
        if (column == mColMonthIndex) {
            mSelectedMonth = position;
        } else if (column == mColYearIndex) {
            final String text = ((TextView) v).getText().toString();
            mSelectedYear = Integer.parseInt(text);
        } else {
            return;
        }
        updateDayStrings(mSelectedYear, mSelectedMonth);
    }

    private void updateDayStrings(int year, int month) {
        final String[] dayStrings;

        final GregorianCalendar calendar = new GregorianCalendar(year, month, 1);
        int numDays = calendar.getActualMaximum(GregorianCalendar.DAY_OF_MONTH);
        dayStrings = Arrays.copyOfRange(mConstant.days31, 0, numDays);
        if (!Arrays.equals(mDayStrings, dayStrings)) {
            mDayStrings = dayStrings;
            updateAdapter(mColDayIndex, new PickerColumn(mDayStrings));
        }
    }
}
