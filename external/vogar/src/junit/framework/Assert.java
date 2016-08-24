/*
 * Copyright (C) 2011 The Android Open Source Project
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

// Some parts of this code is Common Public License - v 1.0
// See NOTICE file in junit project.

package junit.framework;

public class Assert {
    protected Assert() {}

    public static void assertEquals(String message, boolean expected, boolean actual) {
        if (actual != expected) {
            fail(message, "expected " + expected + " but was " + actual);
        }
    }
    
    public static void assertEquals(boolean expected, boolean actual) {
        assertEquals("", expected, actual);
    }

    public static void assertEquals(String message, byte expected, byte actual) {
        if (actual != expected) {
            fail(message, "expected " + expected + " but was " + actual);
        }
    }

    public static void assertEquals(byte expected, byte actual) {
        assertEquals("", expected, actual);
    }

    public static void assertEquals(String message, short expected, short actual) {
        if (actual != expected) {
            fail(message, "expected " + expected + " but was " + actual);
        }
    }

    public static void assertEquals(short expected, short actual) {
        assertEquals("", expected, actual);
    }

    public static void assertEquals(String message, int expected, int actual) {
        if (actual != expected) {
            fail(message, "expected " + expected + " but was " + actual);
        }
    }

    public static void assertEquals(int expected, int actual) {
        assertEquals("", expected, actual);
    }

    public static void assertEquals(String message, long expected, long actual) {
        if (actual != expected) {
            fail(message, "expected " + expected + " but was " + actual);
        }
    }
    
    public static void assertEquals(long expected, long actual) {
        assertEquals("", expected, actual);
    }
    
    public static void assertEquals(String message, char expected, char actual) {
        if (actual != expected) {
            fail(message, "expected " + expected + " but was " + actual);
        }
    }
    
    public static void assertEquals(char expected, char actual) {
        assertEquals("", expected, actual);
    }
    
    public static void assertEquals(String message, String expected, String actual) {
        assertEquals(message, (Object) expected, (Object) actual);
    }
    
    public static void assertEquals(String expected, String actual) {
        assertEquals("", (Object) expected, (Object) actual);
    }
    
    public static void assertEquals(String message, Object expected, Object actual) {
        if (expected == null && actual == null) {
            return;
        }
        if (expected != null && expected.equals(actual)) {
            return;
        }
        fail(message, "expected " + expected + " but was " + actual);
    }
    
    public static void assertEquals(Object expected, Object actual) {
        assertEquals("", expected, actual);
    }
    
    // assertEquals with delta
    static public void assertEquals(String message, double expected, double actual, double delta) {
        if (Double.compare(expected, actual) == 0) {
            return;
        }
        if (!(Math.abs(expected-actual) <= delta)) {
            fail(message, "expected " + expected + " but was " + actual + "; delta=" + delta);
        }
    } 
    
    public static void assertEquals(double expected, double actual, double delta) {
        assertEquals("", expected, actual, delta);
    }

    static public void assertEquals(String message, float expected, float actual, float delta) {
        if (Float.compare(expected, actual) == 0) {
            return;
        }
        if (!(Math.abs(expected - actual) <= delta)) {
            fail(message, "expected " + expected + " but was " + actual + "; delta=" + delta);
        }
    }

    public static void assertEquals(float expected, float actual, float delta) {
        assertEquals("", expected, actual, delta);
    }
    
    // other asserts

    public static void assertTrue(String message, boolean condition) {
        if (!condition) {
            throw new AssertionFailedError(message);
        }
    }

    public static void assertTrue(boolean condition) {
        assertTrue("", condition);
    }

    public static void assertFalse(String message, boolean condition) {
        if (condition) {
            throw new AssertionFailedError(message);
        }
    }
    
    public static void assertFalse(boolean condition) {
        assertFalse("", condition);
    }

    public static void assertNull(String message, Object reference) {
        if (reference != null) {
            throw new AssertionFailedError(message);
        }
    }
    
    public static void assertNull(Object reference) {
        assertNull("", reference);
    }

    public static void assertNotNull(String message, Object reference) {
        if (reference == null) {
            throw new AssertionFailedError(message);
        }
    }
    
    public static void assertNotNull(Object reference) {
        assertNotNull("", reference);
    }

    public static void assertSame(String message, Object expected, Object actual) {
        if (expected != actual) {
            fail(message, "expected same " + expected + ", " + actual);
        }
    }

    public static void assertSame(Object expected, Object actual) {
        assertSame("", expected, actual);
    }

    public static void assertNotSame(String message, Object expected, Object actual) {
        if (expected == actual) {
            fail(message, "expected not same " + expected + ", " + actual);
        }
    }

    public static void assertNotSame(Object expected, Object actual) {
        assertNotSame("", expected, actual);
    }
    
    // fail

    public static void fail(String message) {
        throw new AssertionFailedError(message);
    }

    public static void fail() {
        throw new AssertionFailedError();
    }

    protected static void fail(String message, String detail) {
        if (message == null || message.isEmpty()) {
            throw new AssertionFailedError(detail);
        } else {
            throw new AssertionFailedError(message + ": " + detail);
        }
    }
}
