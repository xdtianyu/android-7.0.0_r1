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

import android.icu.text.RelativeDateTimeFormatter;
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
public class RelativeDateTimeFormatterTest {

    /**
     * Ensure that it behaves the same with an implicit default locale as it does with explicitly
     * specifying the default locale.
     */
    @Test
    public void testGetInstance() {
        RelativeDateTimeFormatter withImplicitDefaultLocale
                = RelativeDateTimeFormatter.getInstance(Locale.CANADA);
        RelativeDateTimeFormatter withExplicitDefaultLocale
                = RelativeDateTimeFormatter.getInstance(ULocale.getDefault());

        String formatWithImplicitDefaultLocale =
                withImplicitDefaultLocale.format(5,
                        RelativeDateTimeFormatter.Direction.NEXT,
                        RelativeDateTimeFormatter.RelativeUnit.MINUTES);

        String formatWithExplicitDefaultLocale =
                withExplicitDefaultLocale.format(5,
                        RelativeDateTimeFormatter.Direction.NEXT,
                        RelativeDateTimeFormatter.RelativeUnit.MINUTES);

        assertEquals(formatWithExplicitDefaultLocale, formatWithImplicitDefaultLocale);
    }
}
