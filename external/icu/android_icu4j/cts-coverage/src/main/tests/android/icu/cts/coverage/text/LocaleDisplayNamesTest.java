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

import android.icu.impl.LocaleDisplayNamesImpl;
import android.icu.lang.UScript;
import android.icu.text.DisplayContext;
import android.icu.text.LocaleDisplayNames;
import android.icu.util.ULocale;
import java.util.Locale;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static org.junit.Assert.assertEquals;

/**
 * There is only one implementation we care about which is {@link LocaleDisplayNamesImpl} as the
 * other one shouldn't be used.
 */
@RunWith(JUnit4.class)
public class LocaleDisplayNamesTest {

    @Test
    public void testGetInstance() {
        DisplayContext capitalization = DisplayContext.CAPITALIZATION_FOR_BEGINNING_OF_SENTENCE;
        DisplayContext dialectHandling = DisplayContext.STANDARD_NAMES;
        LocaleDisplayNamesImpl names = (LocaleDisplayNamesImpl) LocaleDisplayNames.getInstance(
                Locale.CANADA, capitalization, dialectHandling);
        assertEquals(capitalization, names.getContext(DisplayContext.Type.CAPITALIZATION));
        assertEquals(dialectHandling, names.getContext(DisplayContext.Type.DIALECT_HANDLING));
    }

    @Test
    public void testGetLocale() {
        LocaleDisplayNamesImpl names = (LocaleDisplayNamesImpl) LocaleDisplayNames.getInstance(
                Locale.CANADA);
        assertEquals(ULocale.CANADA, names.getLocale());
    }

    @Test
    public void testKeyDisplayName() {
        LocaleDisplayNamesImpl names = (LocaleDisplayNamesImpl) LocaleDisplayNames.getInstance(
                Locale.GERMANY, DisplayContext.CAPITALIZATION_FOR_BEGINNING_OF_SENTENCE);
        assertEquals("Währung", names.keyDisplayName("currency"));
    }

    @Test
    public void testKeyValueDisplayName() {
        LocaleDisplayNamesImpl names = (LocaleDisplayNamesImpl) LocaleDisplayNames.getInstance(
                Locale.CANADA, DisplayContext.CAPITALIZATION_FOR_UI_LIST_OR_MENU);
        assertEquals("Sterling", names.keyValueDisplayName("currency", "sterling"));
    }

    @Test
    public void testLocaleDisplayName() {
        LocaleDisplayNamesImpl names = (LocaleDisplayNamesImpl) LocaleDisplayNames.getInstance(
                Locale.FRANCE, DisplayContext.CAPITALIZATION_FOR_STANDALONE);
        assertEquals("coréen (Corée du Sud)", names.localeDisplayName(Locale.KOREA));
    }

    @Test
    public void testScriptDisplayName_Int() {
        LocaleDisplayNamesImpl names = (LocaleDisplayNamesImpl) LocaleDisplayNames.getInstance(
                Locale.CHINA, DisplayContext.CAPITALIZATION_FOR_MIDDLE_OF_SENTENCE);
        assertEquals("巴厘文", names.scriptDisplayName(UScript.BALINESE));
    }

    @Test
    public void testScriptDisplayName_String() {
        LocaleDisplayNamesImpl names = (LocaleDisplayNamesImpl) LocaleDisplayNames.getInstance(
                Locale.CHINA, DisplayContext.CAPITALIZATION_FOR_MIDDLE_OF_SENTENCE);
        assertEquals("巴厘文", names.scriptDisplayName(UScript.BALINESE));
    }

    @Test
    public void testVariantDisplayName() {
        LocaleDisplayNamesImpl names = (LocaleDisplayNamesImpl) LocaleDisplayNames.getInstance(
                Locale.CHINA, DisplayContext.CAPITALIZATION_NONE);
        assertEquals("拼音罗马字", names.variantDisplayName("PINYIN"));
    }
}
