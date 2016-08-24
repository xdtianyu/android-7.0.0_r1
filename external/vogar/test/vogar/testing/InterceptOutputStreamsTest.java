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

package vogar.testing;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.junit.runners.model.Statement;
import vogar.testing.InterceptOutputStreams.Stream;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * Tests for {@link InterceptOutputStreams}.
 */
@RunWith(JUnit4.class)
public class InterceptOutputStreamsTest {

    @Rule
    public InterceptOutputStreams iosRule = new InterceptOutputStreams(Stream.OUT, Stream.ERR);

    @Before
    public void setUp() {
        System.out.println("Before Tests OUT");
        System.err.println("Before Tests ERR");
    }

    @After
    public void tearDown() {
        System.out.println("After Tests OUT");
        System.err.println("After Tests ERR");
        assertTrue(iosRule.contents(Stream.OUT).endsWith("\nAfter Tests OUT\n"));
        assertTrue(iosRule.contents(Stream.ERR).endsWith("\nAfter Tests ERR\n"));

    }

    @Test
    public void testApply() throws Throwable {
        final InterceptOutputStreams ios = new InterceptOutputStreams(Stream.OUT, Stream.ERR);

        Statement statement = new Statement() {
            @Override
            public void evaluate() throws Throwable {
                System.out.println("Hello");
                System.err.println("World");
                assertEquals("Hello\n", ios.contents(Stream.OUT));
                assertEquals("World\n", ios.contents(Stream.ERR));

                // Make sure that the outer rule doesn't see the output,
                assertEquals("Before Tests OUT\n", iosRule.contents(Stream.OUT));
                assertEquals("Before Tests ERR\n", iosRule.contents(Stream.ERR));
            }
        };

        try {
            ios.contents(Stream.OUT);
            fail("did not detect attempt to access content from outside test");
        } catch(IllegalStateException e) {
            assertEquals("Attempting to access stream contents outside the test", e.getMessage());
        }

        statement = ios.apply(statement, Description.EMPTY);
        statement.evaluate();

        try {
            ios.contents(Stream.ERR);
            fail("did not detect attempt to access content from outside test");
        } catch(IllegalStateException e) {
            assertEquals("Attempting to access stream contents outside the test", e.getMessage());
        }
    }

    @Test
    public void testApply_NotIntercepting() throws Throwable {
        final InterceptOutputStreams ios = new InterceptOutputStreams(Stream.OUT);

        Statement statement = new Statement() {
            @Override
            public void evaluate() throws Throwable {
                try {
                    ios.contents(Stream.ERR);
                    fail("did not detect attempt to access content from unintercepted stream");
                } catch (IllegalStateException e) {
                    assertEquals("Not intercepting ERR output, try:\n"
                            + "    new " + InterceptOutputStreams.class.getSimpleName()
                            + "(OUT, ERR)", e.getMessage());
                }
            }
        };

        statement = ios.apply(statement, Description.EMPTY);
        statement.evaluate();
    }

    @Test
    public void testApply_Nesting() throws Throwable {
        InterceptOutputStreams ios = new InterceptOutputStreams(Stream.OUT, Stream.ERR);

        Statement statement = new Statement() {
            @Override
            public void evaluate() throws Throwable {
                System.out.println("Inner OUT");
                System.err.println("Inner ERR");
                throw new UnsupportedOperationException();
            }
        };

        statement = ios.apply(statement, Description.EMPTY);
        try {
            System.out.println("Outer before OUT");
            System.err.println("Outer before ERR");
            statement.evaluate();
            fail("did not propagate exception");
        } catch (UnsupportedOperationException e) {
            System.out.println("Outer after OUT");
            System.err.println("Outer after ERR");
        }

        assertEquals(""
                + "Before Tests OUT\n"
                + "Outer before OUT\n"
                + "Outer after OUT\n", iosRule.contents(Stream.OUT));
        assertEquals(""
                + "Before Tests ERR\n"
                + "Outer before ERR\n"
                + "Outer after ERR\n", iosRule.contents(Stream.ERR));
    }
}
