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
package android.icu.cts.coverage.lang;

import android.icu.cts.coverage.rules.ULocaleDefault;
import android.icu.cts.coverage.rules.ULocaleDefaultRule;
import android.icu.lang.UCharacter;
import java.util.Locale;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static org.junit.Assert.assertEquals;

/**
 * Extra tests to improve CTS Test Coverage.
 */
@RunWith(JUnit4.class)
public class UCharacterTest {

    @Rule
    public ULocaleDefaultRule uLocaleDefaultRule = new ULocaleDefaultRule();

    @Test
    public void testNameAliasing() {
        int input = '\u01a2';
        String alias = UCharacter.getNameAlias(input);
        assertEquals("LATIN CAPITAL LETTER GHA", alias);
        int output = UCharacter.getCharFromNameAlias(alias);
        assertEquals("alias for '" + input + "'", input, output);
    }

    @Test
    public void testToTitleCase_Locale_String_BreakIterator_I() {
        String titleCase = UCharacter.toTitleCase(Locale.forLanguageTag("nl"), "ijsland", null,
                UCharacter.FOLD_CASE_DEFAULT);
        assertEquals("IJsland", titleCase);
    }

    @ULocaleDefault(languageTag = "nl")
    @Test
    public void testToTitleCase_String_BreakIterator_nl() {
        String titleCase = UCharacter.toTitleCase("ijsland", null);
        assertEquals("IJsland", titleCase);
    }

    @ULocaleDefault(languageTag = "en")
    @Test
    public void testToTitleCase_String_BreakIterator_en() {
        String titleCase = UCharacter.toTitleCase("ijsland", null);
        assertEquals("Ijsland", titleCase);
    }
}
