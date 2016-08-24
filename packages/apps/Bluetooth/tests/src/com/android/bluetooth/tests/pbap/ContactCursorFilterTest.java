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

package com.android.bluetooth.tests.pbap;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.provider.ContactsContract;
import android.test.AndroidTestCase;

import com.android.bluetooth.pbap.BluetoothPbapVcardManager;

public class ContactCursorFilterTest extends AndroidTestCase {

    public void testFilterByRangeWithoutDup() {
        MatrixCursor mc = new MatrixCursor(new String[]{
                ContactsContract.CommonDataKinds.Phone.CONTACT_ID,
                ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME});
        mc.addRow(new Object[]{1L, "Test"});
        mc.addRow(new Object[]{2L, "Test"});
        mc.addRow(new Object[]{3L, "Test"});

        Cursor cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByRange(mc, 1, 2);
        assertEquals(2, cursor.getCount());
        assertEquals(1L, getContactsIdFromCursor(cursor, 0));
        assertEquals(2L, getContactsIdFromCursor(cursor, 1));
        cursor.close();

        mc.moveToPosition(-1);
        cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByRange(mc, 1, 3);
        assertEquals(3, cursor.getCount());
        assertEquals(1L, getContactsIdFromCursor(cursor, 0));
        assertEquals(2L, getContactsIdFromCursor(cursor, 1));
        assertEquals(3L, getContactsIdFromCursor(cursor, 2));
        cursor.close();

        mc.moveToPosition(-1);
        cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByRange(mc, 2, 3);
        assertEquals(2, cursor.getCount());
        assertEquals(2L, getContactsIdFromCursor(cursor, 0));
        assertEquals(3L, getContactsIdFromCursor(cursor, 1));
        cursor.close();

        mc.moveToPosition(-1);
        cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByRange(mc, 3, 3);
        assertEquals(1, cursor.getCount());
        assertEquals(3L, getContactsIdFromCursor(cursor, 0));
        cursor.close();
    }


    public void testFilterByRangeWithDup() {
        MatrixCursor mc = new MatrixCursor(new String[]{ContactsContract.CommonDataKinds.Phone
                .CONTACT_ID, ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME});
        mc.addRow(new Object[]{1L, "Test"});
        mc.addRow(new Object[]{1L, "Test"});
        mc.addRow(new Object[]{2L, "Test"});
        mc.addRow(new Object[]{2L, "Test"});
        mc.addRow(new Object[]{3L, "Test"});

        Cursor cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByRange(mc, 1, 2);
        assertEquals(2, cursor.getCount());
        assertEquals(1L, getContactsIdFromCursor(cursor, 0));
        assertEquals(2L, getContactsIdFromCursor(cursor, 1));
        cursor.close();

        mc.moveToPosition(-1);
        cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByRange(mc, 1, 3);
        assertEquals(3, cursor.getCount());
        assertEquals(1L, getContactsIdFromCursor(cursor, 0));
        assertEquals(2L, getContactsIdFromCursor(cursor, 1));
        assertEquals(3L, getContactsIdFromCursor(cursor, 2));
        cursor.close();

        mc.moveToPosition(-1);
        cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByRange(mc, 2, 3);
        assertEquals(2, cursor.getCount());
        assertEquals(2L, getContactsIdFromCursor(cursor, 0));
        assertEquals(3L, getContactsIdFromCursor(cursor, 1));
        cursor.close();

        mc.moveToPosition(-1);
        cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByRange(mc, 3, 3);
        assertEquals(1, cursor.getCount());
        assertEquals(3L, getContactsIdFromCursor(cursor, 0));
        cursor.close();
    }

    public void testFilterByOffsetWithoutDup() {
        MatrixCursor mc = new MatrixCursor(new String[]{ContactsContract.CommonDataKinds.Phone
                .CONTACT_ID, ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME});
        mc.addRow(new Object[]{1L, "Test"});
        mc.addRow(new Object[]{2L, "Test"});
        mc.addRow(new Object[]{3L, "Test"});

        Cursor cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByOffset(mc, 1);
        assertEquals(1, cursor.getCount());
        assertEquals(1L, getContactsIdFromCursor(cursor, 0));
        cursor.close();

        mc.moveToPosition(-1);
        cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByOffset(mc, 2);
        assertEquals(1, cursor.getCount());
        assertEquals(2L, getContactsIdFromCursor(cursor, 0));
        cursor.close();

        mc.moveToPosition(-1);
        cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByOffset(mc, 3);
        assertEquals(1, cursor.getCount());
        assertEquals(3L, getContactsIdFromCursor(cursor, 0));
        cursor.close();
    }

    public void testFilterByOffsetWithDup() {
        MatrixCursor mc = new MatrixCursor(new String[]{ContactsContract.CommonDataKinds.Phone
                .CONTACT_ID, ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME});
        mc.addRow(new Object[]{1L, "Test"});
        mc.addRow(new Object[]{1L, "Test"});
        mc.addRow(new Object[]{2L, "Test"});
        mc.addRow(new Object[]{2L, "Test"});
        mc.addRow(new Object[]{3L, "Test"});

        Cursor cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByOffset(mc, 1);
        assertEquals(1, cursor.getCount());
        assertEquals(1L, getContactsIdFromCursor(cursor, 0));
        cursor.close();

        mc.moveToPosition(-1);
        cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByOffset(mc, 2);
        assertEquals(1, cursor.getCount());
        assertEquals(2L, getContactsIdFromCursor(cursor, 0));
        cursor.close();

        mc.moveToPosition(-1);
        cursor = BluetoothPbapVcardManager.ContactCursorFilter.filterByOffset(mc, 3);
        assertEquals(1, cursor.getCount());
        assertEquals(3L, getContactsIdFromCursor(cursor, 0));
        cursor.close();
        mc.moveToFirst();

    }

    private long getContactsIdFromCursor(Cursor cursor, int offset) {
        int index = cursor.getColumnIndex(ContactsContract.CommonDataKinds.Phone.CONTACT_ID);
        cursor.moveToPosition(offset);
        return cursor.getLong(index);
    }
}
