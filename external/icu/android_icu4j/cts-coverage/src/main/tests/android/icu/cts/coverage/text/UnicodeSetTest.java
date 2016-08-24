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

import android.icu.text.UnicodeSet;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.Collections;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

@RunWith(JUnit4.class)
public class UnicodeSetTest {
    @Test
    public void testAddAll_CharacterSequences() {
        UnicodeSet unicodeSet = new UnicodeSet();
        unicodeSet.addAll("a", "b");
        assertEquals("[ab]", unicodeSet.toPattern(true));
        unicodeSet.addAll("b", "x");
        assertEquals("[abx]", unicodeSet.toPattern(true));
        unicodeSet.addAll(new CharSequence[]{new StringBuilder("foo"), new StringBuffer("bar")});
        assertEquals("[abx{bar}{foo}]", unicodeSet.toPattern(true));
    }

    @Test
    public void testCompareTo() {
        assertEquals(0, UnicodeSet.EMPTY.compareTo(Collections.emptyList()));
        assertEquals(0, UnicodeSet.fromAll("a").compareTo(Collections.singleton("a")));

        // Longer is bigger
        assertTrue(UnicodeSet.ALL_CODE_POINTS.compareTo(Collections.emptyList()) > 0);
        assertTrue(UnicodeSet.EMPTY.compareTo(Collections.singleton("a")) < 0);

        // Equal length compares on first difference.
        assertTrue(UnicodeSet.fromAll("a").compareTo(Collections.singleton("b")) < 0);
        assertTrue(UnicodeSet.fromAll("ab").compareTo(Arrays.asList("a", "c")) < 0);
        assertTrue(UnicodeSet.fromAll("b").compareTo(Collections.singleton("a")) > 0);
    }
}
