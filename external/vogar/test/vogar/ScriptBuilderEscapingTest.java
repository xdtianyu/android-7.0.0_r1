/*
 * Copyright (C) 2015 The Android Open Source Project
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

package vogar;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import vogar.Target.ScriptBuilder;

/**
 * Tests the {@link ScriptBuilder#escape(String)} method.
 */
public class ScriptBuilderEscapingTest {

    public static Test suite() {
        TestSuite suite = new TestSuite(ScriptBuilderEscapingTest.class.getName());
        char[] chars = " '\"<>&|$".toCharArray();
        for (char c : chars) {
            suite.addTest(new SingleCharacterEscapeTest(c));
        }
        suite.addTest(new MixedCharacterEscapeTest());
        return suite;
    }

    private static class SingleCharacterEscapeTest extends TestCase {

        private final String uc;
        private final String qc;

        public SingleCharacterEscapeTest(char c) {
            this.uc = Character.toString(c);
            this.qc = "\\" + c;
            setName("Escape '" + uc + "' as '" + qc + "'");
        }

        @Override
        protected void runTest() throws Throwable {
            assertEquals(qc, ScriptBuilder.escape(uc));
            assertEquals("a" + qc, ScriptBuilder.escape("a" + uc));
            assertEquals(qc + "b", ScriptBuilder.escape(uc + "b"));
            assertEquals("a" + qc + "b", ScriptBuilder.escape("a" + uc + "b"));
            assertEquals(qc + "a" + qc + qc + qc + "b" + qc,
                    ScriptBuilder.escape(uc + "a" + uc + uc + uc + "b" + uc));

        }
    }

    private static class MixedCharacterEscapeTest extends TestCase {

        public MixedCharacterEscapeTest() {
            super("mixed character escape test");
        }

        @Override
        protected void runTest() throws Throwable {
            assertEquals("\\ \\'\\\"\\<\\>\\&\\|\\$",
                    ScriptBuilder.escape(" '\"<>&|$"));
        }
    }
}
