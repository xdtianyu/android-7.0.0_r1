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

import android.icu.text.DateFormatSymbols;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

@RunWith(JUnit4.class)
public class DateFormatSymbolsTest {
    @Test
    public void testSetEraNames_defensivelyCopies() {
        DateFormatSymbols symbols = DateFormatSymbols.getInstance();
        String[] eraNamesInput = new String[]{"longAgo", "notSoLongAgo"};
        symbols.setEraNames(eraNamesInput);
        assertArrayEquals(eraNamesInput, symbols.getEraNames());
        eraNamesInput[0] = "longLongAgo";
        assertEquals("longAgo", symbols.getEraNames()[0]);
    }


    @Test
    public void testYearNames_defensivelyCopies() {
        DateFormatSymbols symbols = DateFormatSymbols.getInstance();
        String[] yearNamesInput = new String[]{"aYear", "anotherYear"};
        symbols.setYearNames(
                yearNamesInput, DateFormatSymbols.FORMAT, DateFormatSymbols.ABBREVIATED);
        assertArrayEquals(yearNamesInput,
                symbols.getYearNames(DateFormatSymbols.FORMAT, DateFormatSymbols.ABBREVIATED));
        yearNamesInput[0] = "aDifferentYear";
        assertEquals("aYear",
                symbols.getYearNames(DateFormatSymbols.FORMAT, DateFormatSymbols.ABBREVIATED)[0]);
    }
}
