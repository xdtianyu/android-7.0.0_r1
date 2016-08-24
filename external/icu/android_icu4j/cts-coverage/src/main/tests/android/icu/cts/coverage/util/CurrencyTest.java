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
package android.icu.cts.coverage.util;

import android.icu.util.Currency;
import android.icu.util.ULocale;
import java.util.Locale;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static org.junit.Assert.assertEquals;

/**
 * Extra tests to improve CTS Test Coverage.
 */
@RunWith(JUnit4.class)
public class CurrencyTest {

    @Test
    public void testGetName_Locale_Int_String_BooleanArray() {
        Currency currency = Currency.getInstance(ULocale.CHINA);
        boolean[] isChoiceFormat = new boolean[1];
        int nameStyle = Currency.LONG_NAME;
        String pluralCount = "";
        String ulocaleName =
                currency.getName(ULocale.CANADA, nameStyle, pluralCount, isChoiceFormat);
        assertEquals("Chinese Yuan", ulocaleName);
        String localeName = currency.getName(Locale.CANADA, nameStyle, pluralCount, isChoiceFormat);
        assertEquals("currency name mismatch", ulocaleName, localeName);
    }

    @Test
    public void testGetDefaultFractionDigits_CurrencyUsage() {
        Currency currency = Currency.getInstance(ULocale.CHINA);
        int cashFractionDigits = currency.getDefaultFractionDigits(Currency.CurrencyUsage.CASH);
        assertEquals(2, cashFractionDigits);
    }

    @Test
    public void testGetRoundingIncrement() {
        Currency currency = Currency.getInstance(ULocale.JAPAN);
        // It appears as though this always returns 0 irrespective of the currency.
        double roundingIncrement = currency.getRoundingIncrement();
        assertEquals(0, roundingIncrement, 0);
    }

    @Test
    public void testGetRoundingIncrement_CurrencyUsage() {
        Currency currency = Currency.getInstance(ULocale.JAPAN);
        // It appears as though this always returns 0 irrespective of the currency or usage.
        double roundingIncrement = currency.getRoundingIncrement(Currency.CurrencyUsage.CASH);
        assertEquals(0, roundingIncrement, 0);
    }
}
