/*
 * Copyright (C) 2013 The Android Open Source Project
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

package vogar.target.junit4;

import org.junit.Test;

import static org.junit.Assert.*;

public class FailTest {
    @Test
    public void success() {
    }

    @Test
    public void failure() {
        fail("failed.");
    }

    @Test
    public void throwException() {
        throw new RuntimeException("exception");
    }

    @Test(expected = AwesomeException.class)
    public void throwExpectedException() throws Exception {
        throw new AwesomeException();
    }

    @Test(expected = AwesomeException.class)
    public void throwAnotherExpectedException() throws Exception {
        throw new EvenMoreAwesomeException();
    }

    private static class AwesomeException extends Exception {
    }
    
    private static class EvenMoreAwesomeException extends AwesomeException {
    }
}
