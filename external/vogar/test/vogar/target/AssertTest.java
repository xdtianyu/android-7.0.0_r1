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

package vogar.target;

import junit.framework.AssertionFailedError;
import junit.framework.TestCase;

public final class AssertTest extends TestCase {
    
    public void testAssertEquals() {
        Object o = new Object();
        assertEquals(o, o);
        
        boolean success = false;
        try {
            assertEquals(o, new Object());
            // can't fail(), that throws AssertionFailedError
        } catch (AssertionFailedError expected) {
            success = true;
        }
        assertTrue(success);
    }

    public void testAssertEqualsNan() {
        assertEquals(Double.NaN, Double.NaN, 0.0);
        
        boolean success = false;
        try {
            assertEquals(5, Double.NaN, 0.0);
            // can't fail(), that throws AssertionFailedError
        } catch (AssertionFailedError expected) {
            success = true;
        }
        assertTrue(success);
    }

    public void testAssertEqualsWithDelta() {
        assertEquals(0.0, 5.0, 5.0);
        
        boolean success = false;
        try {
            assertEquals(0.0, 6.0, 5.0);
            // can't fail(), that throws AssertionFailedError
        } catch (AssertionFailedError expected) {
            success = true;
        }
        assertTrue(success);
    }

    public void testAssertEqualsWithNaNDelta() {
        assertEquals(0.0, 0.0, Double.NaN);

        boolean success = false;
        try {
            assertEquals(0.0, 6.0, Double.NaN);
            // can't fail(), that throws AssertionFailedError
        } catch (AssertionFailedError expected) {
            success = true;
        }
        assertTrue(success);
    }

    public void testAssertSame() {
        assertSame(this, this);

        boolean success = false;
        try {
            assertSame(new Object(), new Object());
            // can't fail(), that throws AssertionFailedError
        } catch (AssertionFailedError expected) {
            success = true;
        }
        assertTrue(success);
    }

    public void testAssertNotSame() {
        assertNotSame(new Object(), new Object());

        boolean success = false;
        try {
            assertNotSame(this, this);
            // can't fail(), that throws AssertionFailedError
        } catch (AssertionFailedError expected) {
            success = true;
        }
        assertTrue(success);
    }

    public void testAssertNull() {
        assertNull(null);

        boolean success = false;
        try {
            assertNull(this);
            // can't fail(), that throws AssertionFailedError
        } catch (AssertionFailedError expected) {
            success = true;
        }
        assertTrue(success);
    }

    public void testAssertNotNull() {
        assertNotNull(new Object());

        boolean success = false;
        try {
            assertNotNull(null);
            // can't fail(), that throws AssertionFailedError
        } catch (AssertionFailedError expected) {
            success = true;
        }
        assertTrue(success);
    }
}
