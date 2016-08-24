/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.permission.cts;

import android.content.ContentValues;
import android.provider.CallLog;
import android.provider.Contacts;
import android.provider.Settings;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.MediumTest;

/**
 * Tests Permissions related to reading from and writing to providers
 */
@MediumTest
public class ProviderPermissionTest extends AndroidTestCase {

    /**
     * Verify that read and write to contact requires permissions.
     * <p>Tests Permission:
     *   {@link android.Manifest.permission#READ_CONTACTS}
     */
    public void testReadContacts() {
        assertReadingContentUriRequiresPermission(Contacts.People.CONTENT_URI,
                android.Manifest.permission.READ_CONTACTS);
    }

    /**
     * Verify that write to contact requires permissions.
     * <p>Tests Permission:
     *   {@link android.Manifest.permission#WRITE_CONTACTS}
     */
    public void testWriteContacts() {
        assertWritingContentUriRequiresPermission(Contacts.People.CONTENT_URI,
                android.Manifest.permission.WRITE_CONTACTS);
    }

    /**
     * Verify that reading call logs requires permissions.
     * <p>Tests Permission:
     *   {@link android.Manifest.permission#READ_CALL_LOG}
     */
    public void testReadCallLog() {
        assertReadingContentUriRequiresPermission(CallLog.CONTENT_URI,
                android.Manifest.permission.READ_CALL_LOG);
    }

    /**
     * Verify that writing call logs requires permissions.
     * <p>Tests Permission:
     *   {@link android.Manifest.permission#WRITE_CALL_LOG}
     */
    public void testWriteCallLog() {
        assertWritingContentUriRequiresPermission(CallLog.CONTENT_URI,
                android.Manifest.permission.WRITE_CALL_LOG);
    }

    /**
     * Verify that write to settings requires permissions.
     * <p>Tests Permission:
     *   {@link android.Manifest.permission#WRITE_SETTINGS}
     */
    public void testWriteSettings() {
        final String permission = android.Manifest.permission.WRITE_SETTINGS;
        ContentValues value = new ContentValues();
        value.put(Settings.System.NAME, "name");
        value.put(Settings.System.VALUE, "value_insert");

        try {
            getContext().getContentResolver().insert(Settings.System.CONTENT_URI, value);
            fail("expected SecurityException requiring " + permission);
        } catch (SecurityException expected) {
            assertNotNull("security exception's error message.", expected.getMessage());
            assertTrue("error message should contain \"" + permission + "\". Got: \""
                    + expected.getMessage() + "\".",
                    expected.getMessage().contains(permission));
        }
    }
}
