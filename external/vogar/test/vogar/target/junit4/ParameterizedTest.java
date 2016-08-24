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

import static org.junit.Assert.*;

import java.util.Arrays;
import java.util.Collection;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public class ParameterizedTest {
    private final int field1;
    private final int field2;

    public ParameterizedTest(int field1, int field2) {
        this.field1 = field1;
        this.field2 = field2;
    }

    @Parameters
    public static Collection<Object[]> data() {
      Object[][] data = new Object[][] { { 5, 10 } };
      return Arrays.asList(data);
    }

    @Test
    public void params() {
        assertEquals(5, field1);
        assertEquals(10, field2);
    }
}
