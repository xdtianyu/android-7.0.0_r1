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
package android.icu.cts.coverage.text;

import android.icu.cts.coverage.rules.ULocaleDefault;
import android.icu.cts.coverage.rules.ULocaleDefaultRule;
import android.icu.text.DateFormat;
import android.icu.text.DateIntervalFormat;
import android.icu.text.DateIntervalInfo;
import android.icu.util.Calendar;
import android.icu.util.DateInterval;
import android.icu.util.GregorianCalendar;
import android.icu.util.ULocale;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.text.ParseException;
import java.util.Locale;

import static org.junit.Assert.assertEquals;

@RunWith(JUnit4.class)
public class DateIntervalFormatTest {
    @Rule
    public ULocaleDefaultRule uLocaleDefaultRule = new ULocaleDefaultRule();

    @Test
    @ULocaleDefault(languageTag = "en")
    public void testGetInstance_String_DateIntervalInfo() {
        DateIntervalInfo dateIntervalInfo = new DateIntervalInfo(new ULocale("ca"));
        DateIntervalFormat dateIntervalFormat =
                DateIntervalFormat.getInstance(DateFormat.YEAR_MONTH, dateIntervalInfo);
        Calendar from = Calendar.getInstance();
        from.set(2000, Calendar.JANUARY, 1, 12, 0);
        Calendar to = Calendar.getInstance();
        to.set(2001, Calendar.FEBRUARY, 1, 12, 0);
        DateInterval interval = new DateInterval(from.getTimeInMillis(), to.getTimeInMillis());
        dateIntervalFormat.setTimeZone(from.getTimeZone());

        // Month names are default (English), format is Catalan
        assertEquals("January de 2000 – February de 2001", dateIntervalFormat.format(interval));
    }

    @Test
    @ULocaleDefault(languageTag = "en")
    public void testGetInstance_String_Locale_DateIntervalInfo() {
        DateIntervalInfo dateIntervalInfo = new DateIntervalInfo(new ULocale("ca"));
        DateIntervalFormat dateIntervalFormat = DateIntervalFormat.getInstance(
                DateFormat.YEAR_MONTH, Locale.GERMAN, dateIntervalInfo);
        Calendar from = Calendar.getInstance();
        from.set(2000, Calendar.JANUARY, 1, 12, 0);
        Calendar to = Calendar.getInstance();
        to.set(2001, Calendar.FEBRUARY, 1, 12, 0);
        DateInterval interval = new DateInterval(from.getTimeInMillis(), to.getTimeInMillis());
        dateIntervalFormat.setTimeZone(from.getTimeZone());

        // Month names are German, format is Catalan
        assertEquals("Januar de 2000 – Februar de 2001", dateIntervalFormat.format(interval));
    }

    @Test(expected = UnsupportedOperationException.class)
    public void testParseObject_notSupported() throws ParseException {
        DateIntervalFormat.getInstance(DateFormat.YEAR_MONTH).parseObject("");
    }
}
