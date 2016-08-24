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
package com.android.cts.tradefed.testtype;

import com.android.cts.tradefed.UnitTests;

import junit.framework.TestCase;

/**
 * Unit tests for {@link GeeTest}.
 */
public class GeeTestTest extends TestCase {

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
    }

     /**
     * Test {@link GeeTestTest#getGTestFilters}
     * @throws DeviceNotAvailableException
     */
    public void testGetGTestFilters() {
        GeeTest test = new GeeTest("package_foo", "exe_foo");
        test.setPositiveFilters("a");
        test.setNegativeFilters("b");
        String actual = test.getGTestFilters();
        assertEquals("--gtest_filter=a:-b", actual);
    }

    /**
     * Test {@link GeeTestTest#getGTestFilters} with only positive filters
     * @throws DeviceNotAvailableException
     */
    public void testGetGTestFiltersPositiveOnly() {
        GeeTest test = new GeeTest("package_foo", "exe_foo");
        test.setPositiveFilters("a");
        String actual = test.getGTestFilters();
        assertEquals("--gtest_filter=a", actual);
    }

    /**
     * Test {@link GeeTestTest#getGTestFilters} with only negative filters
     * @throws DeviceNotAvailableException
     */
    public void testGetGTestFiltersNegativeOnly() {
        GeeTest test = new GeeTest("package_foo", "exe_foo");
        test.setNegativeFilters("b");
        String actual = test.getGTestFilters();
        assertEquals("--gtest_filter=-b", actual);
    }

    /**
     * Test {@link GeeTestTest#getGTestFilters} with empty filters
     * @throws DeviceNotAvailableException
     */
    public void testGetGTestFiltersWithNoFilters() {
        GeeTest test = new GeeTest("package_foo", "exe_foo");
        String actual = test.getGTestFilters();
        assertEquals("", actual);
    }
}
