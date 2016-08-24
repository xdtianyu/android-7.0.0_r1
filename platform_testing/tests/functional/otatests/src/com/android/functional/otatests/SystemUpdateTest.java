package com.android.functional.otatests;

/**
 * A basic test case to assert that the system was updated to the expected version.
 */
public class SystemUpdateTest extends VersionCheckingTest {

    public void testIsUpdated() throws Exception {
        assertUpdated();
    }
}
