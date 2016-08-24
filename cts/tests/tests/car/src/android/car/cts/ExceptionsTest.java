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
package android.car.cts;

import android.car.CarNotConnectedException;
import android.car.CarNotSupportedException;
import android.test.AndroidTestCase;

public class ExceptionsTest extends AndroidTestCase {
    private static final String MESSAGE = "Oops!";
    private static final Exception CAUSE = new RuntimeException();

    public void testCarNotConnectedException() {
        CarNotConnectedException exception = new CarNotConnectedException();
        assertNull(exception.getMessage());
        assertNull(exception.getCause());

        exception = new CarNotConnectedException(MESSAGE);
        assertEquals(MESSAGE, exception.getMessage());
        assertNull(exception.getCause());

        exception = new CarNotConnectedException(MESSAGE, CAUSE);
        assertEquals(MESSAGE, exception.getMessage());
        assertEquals(CAUSE, exception.getCause());

        exception = new CarNotConnectedException(CAUSE);
        assertEquals(CAUSE, exception.getCause());
    }

    public void testCarNotSupportedException() {
        CarNotSupportedException exception = new CarNotSupportedException();
        assertNull(exception.getMessage());
        assertNull(exception.getCause());

        exception = new CarNotSupportedException(MESSAGE);
        assertEquals(MESSAGE, exception.getMessage());
        assertNull(exception.getCause());

        exception = new CarNotSupportedException(MESSAGE, CAUSE);
        assertEquals(MESSAGE, exception.getMessage());
        assertEquals(CAUSE, exception.getCause());

        exception = new CarNotSupportedException(CAUSE);
        assertEquals(CAUSE, exception.getCause());
    }
}
