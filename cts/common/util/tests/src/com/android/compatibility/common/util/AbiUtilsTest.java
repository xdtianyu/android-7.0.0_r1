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
 * limitations under the License
 */

package com.android.compatibility.common.util;

import junit.framework.TestCase;

/**
 * Unit tests for {@link AbiUtils}
 */
public class AbiUtilsTest extends TestCase {

    private static final String MODULE_NAME = "ModuleName";
    private static final String ABI_NAME = "mips64";
    private static final String ABI_FLAG = "--abi mips64 ";
    private static final String ABI_ID = "mips64 ModuleName";

    public void testCreateAbiFlag() {
        String flag = AbiUtils.createAbiFlag(ABI_NAME);
        assertEquals("Incorrect flag created", ABI_FLAG, flag);
    }

    public void testCreateId() {
        String id = AbiUtils.createId(ABI_NAME, MODULE_NAME);
        assertEquals("Incorrect id created", ABI_ID, id);
    }

    public void testParseId() {
        String[] parts = AbiUtils.parseId(ABI_ID);
        assertEquals("Wrong size array", 2, parts.length);
        assertEquals("Wrong abi name", ABI_NAME, parts[0]);
        assertEquals("Wrong module name", MODULE_NAME, parts[1]);
    }

    public void testParseName() {
        String name = AbiUtils.parseTestName(ABI_ID);
        assertEquals("Incorrect module name", MODULE_NAME, name);
    }

    public void testParseAbi() {
        String abi = AbiUtils.parseAbi(ABI_ID);
        assertEquals("Incorrect abi name", ABI_NAME, abi);
    }

}
