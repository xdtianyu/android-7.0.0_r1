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

package android.widget.cts;

import android.annotation.ColorInt;
import android.app.Instrumentation;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.MediumTest;
import android.widget.CalendarView;
import android.widget.cts.util.TestUtils;

import java.util.Calendar;
import java.util.GregorianCalendar;

@MediumTest
public class CalendarViewTest extends ActivityInstrumentationTestCase2<CalendarViewCtsActivity> {
    private CalendarViewCtsActivity mActivity;
    private CalendarView mCalendarViewMaterial;
    private CalendarView mCalendarViewHolo;

    public CalendarViewTest() {
        super("android.widget.cts", CalendarViewCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        mCalendarViewMaterial = (CalendarView) mActivity.findViewById(R.id.calendar_view_material);
        mCalendarViewHolo = (CalendarView) mActivity.findViewById(R.id.calendar_view_holoyolo);

        // Initialize both calendar views to the current date
        final long currentDate = new GregorianCalendar().getTime().getTime();
        getInstrumentation().runOnMainSync(() -> {
            mCalendarViewMaterial.setDate(currentDate);
            mCalendarViewHolo.setDate(currentDate);
        });
    }

    public void testConstructor() {
        new CalendarView(mActivity);

        new CalendarView(mActivity, null);

        new CalendarView(mActivity, null, android.R.attr.calendarViewStyle);

        new CalendarView(mActivity, null, 0, android.R.style.Widget_Material_Light_CalendarView);
    }

    public void testAccessDate() {
        final Instrumentation instrumentation = getInstrumentation();

        // Go back one year
        final Calendar newCalendar = new GregorianCalendar();
        newCalendar.set(Calendar.YEAR, newCalendar.get(Calendar.YEAR) - 1);
        final long yearAgoDate = newCalendar.getTime().getTime();

        instrumentation.runOnMainSync(
                () -> mCalendarViewMaterial.setDate(yearAgoDate));
        assertEquals(yearAgoDate, mCalendarViewMaterial.getDate());

        // Go forward two years (one year from current date in aggregate)
        newCalendar.set(Calendar.YEAR, newCalendar.get(Calendar.YEAR) + 2);
        final long yearHenceDate = newCalendar.getTime().getTime();

        instrumentation.runOnMainSync(
                () -> mCalendarViewMaterial.setDate(yearHenceDate, true, false));
        assertEquals(yearHenceDate, mCalendarViewMaterial.getDate());
    }

    public void testAccessMinMaxDate() {
        final Instrumentation instrumentation = getInstrumentation();

        // Use a range of minus/plus one year as min/max dates
        final Calendar minCalendar = new GregorianCalendar();
        minCalendar.set(Calendar.YEAR, minCalendar.get(Calendar.YEAR) - 1);
        final Calendar maxCalendar = new GregorianCalendar();
        maxCalendar.set(Calendar.YEAR, maxCalendar.get(Calendar.YEAR) + 1);

        final long minDate = minCalendar.getTime().getTime();
        final long maxDate = maxCalendar.getTime().getTime();

        instrumentation.runOnMainSync(() -> {
            mCalendarViewMaterial.setMinDate(minDate);
            mCalendarViewMaterial.setMaxDate(maxDate);
        });

        assertEquals(mCalendarViewMaterial.getMinDate(), minDate);
        assertEquals(mCalendarViewMaterial.getMaxDate(), maxDate);
    }

    public void testAppearanceMaterial() {
        // The logic in this method is performed on a Material-styled CalendarView and
        // non-deprecated attributes / visual appearance APIs

        // Test the initial appearance defined in the layout XML
        assertEquals(2, mCalendarViewMaterial.getFirstDayOfWeek());
        assertEquals(R.style.TextAppearance_WithColor,
                mCalendarViewMaterial.getDateTextAppearance());
        assertEquals(R.style.TextAppearance_WithColorGreen,
                mCalendarViewMaterial.getWeekDayTextAppearance());

        final Instrumentation instrumentation = getInstrumentation();

        // Change the visual appearance of the widget
        instrumentation.runOnMainSync(() -> {
            mCalendarViewMaterial.setFirstDayOfWeek(3);
            mCalendarViewMaterial.setDateTextAppearance(R.style.TextAppearance_WithColorBlue);
            mCalendarViewMaterial.setWeekDayTextAppearance(R.style.TextAppearance_WithColorMagenta);
        });

        assertEquals(3, mCalendarViewMaterial.getFirstDayOfWeek());
        assertEquals(R.style.TextAppearance_WithColorBlue,
                mCalendarViewMaterial.getDateTextAppearance());
        assertEquals(R.style.TextAppearance_WithColorMagenta,
                mCalendarViewMaterial.getWeekDayTextAppearance());
    }

    public void testAppearanceHolo() {
        // All the logic in this method is performed on a Holo-styled CalendarView, as
        // under Material design we are ignoring most of these decorative attributes

        // Test the initial appearance defined in the layout XML
        assertEquals(3, mCalendarViewHolo.getFirstDayOfWeek());
        assertEquals(5, mCalendarViewHolo.getShownWeekCount());
        assertFalse(mCalendarViewHolo.getShowWeekNumber());
        assertEquals(R.style.TextAppearance_WithColor,
                mCalendarViewHolo.getDateTextAppearance());
        assertEquals(R.style.TextAppearance_WithColorGreen,
                mCalendarViewHolo.getWeekDayTextAppearance());
        assertEquals(mActivity.getColor(R.color.calendarview_week_background),
                mCalendarViewHolo.getSelectedWeekBackgroundColor());
        assertEquals(mActivity.getColor(R.color.calendarview_focusedmonthdate),
                mCalendarViewHolo.getFocusedMonthDateColor());
        assertEquals(mActivity.getColor(R.color.calendarview_unfocusedmonthdate),
                mCalendarViewHolo.getUnfocusedMonthDateColor());
        TestUtils.assertAllPixelsOfColor("Selected date vertical bar blue",
                mCalendarViewHolo.getSelectedDateVerticalBar(), 40, 40, true, 0xFF0000FF, 1, true);

        final Instrumentation instrumentation = getInstrumentation();

        // Change the visual appearance of the widget
        final @ColorInt int newSelectedWeekBackgroundColor =
                mActivity.getColor(R.color.calendarview_week_background_new);
        final @ColorInt int newFocusedMonthDateColor =
                mActivity.getColor(R.color.calendarview_focusedmonthdate_new);
        final @ColorInt int newUnfocusedMonthDataColor =
                mActivity.getColor(R.color.calendarview_unfocusedmonthdate_new);
        final @ColorInt int newWeekNumberColor =
                mActivity.getColor(R.color.calendarview_week_number_new);
        final @ColorInt int newWeekSeparatorLineColor =
                mActivity.getColor(R.color.calendarview_week_separatorline_new);

        instrumentation.runOnMainSync(() -> {
            mCalendarViewHolo.setFirstDayOfWeek(1);
            mCalendarViewHolo.setShownWeekCount(4);
            mCalendarViewHolo.setShowWeekNumber(true);
            mCalendarViewHolo.setDateTextAppearance(R.style.TextAppearance_WithColorBlue);
            mCalendarViewHolo.setWeekDayTextAppearance(R.style.TextAppearance_WithColorMagenta);
            mCalendarViewHolo.setSelectedWeekBackgroundColor(newSelectedWeekBackgroundColor);
            mCalendarViewHolo.setFocusedMonthDateColor(newFocusedMonthDateColor);
            mCalendarViewHolo.setUnfocusedMonthDateColor(newUnfocusedMonthDataColor);
            mCalendarViewHolo.setWeekNumberColor(newWeekNumberColor);
            mCalendarViewHolo.setWeekSeparatorLineColor(newWeekSeparatorLineColor);
        });

        assertEquals(1, mCalendarViewHolo.getFirstDayOfWeek());
        assertEquals(4, mCalendarViewHolo.getShownWeekCount());
        assertTrue(mCalendarViewHolo.getShowWeekNumber());
        assertEquals(R.style.TextAppearance_WithColorBlue,
                mCalendarViewHolo.getDateTextAppearance());
        assertEquals(R.style.TextAppearance_WithColorMagenta,
                mCalendarViewHolo.getWeekDayTextAppearance());
        assertEquals(newSelectedWeekBackgroundColor,
                mCalendarViewHolo.getSelectedWeekBackgroundColor());
        assertEquals(newFocusedMonthDateColor,
                mCalendarViewHolo.getFocusedMonthDateColor());
        assertEquals(newUnfocusedMonthDataColor,
                mCalendarViewHolo.getUnfocusedMonthDateColor());
        assertEquals(newWeekNumberColor,
                mCalendarViewHolo.getWeekNumberColor());
        assertEquals(newWeekSeparatorLineColor,
                mCalendarViewHolo.getWeekSeparatorLineColor());

        instrumentation.runOnMainSync(
                () -> mCalendarViewHolo.setSelectedDateVerticalBar(R.drawable.yellow_fill));
        TestUtils.assertAllPixelsOfColor("Selected date vertical bar yellow",
                mCalendarViewHolo.getSelectedDateVerticalBar(), 40, 40, true, 0xFFFFFF00, 1, true);

        instrumentation.runOnMainSync(
                () -> mCalendarViewHolo.setSelectedDateVerticalBar(
                        mActivity.getDrawable(R.drawable.magenta_fill)));
        TestUtils.assertAllPixelsOfColor("Selected date vertical bar magenta",
                mCalendarViewHolo.getSelectedDateVerticalBar(), 40, 40, true, 0xFFFF00FF, 1, true);
    }
}
