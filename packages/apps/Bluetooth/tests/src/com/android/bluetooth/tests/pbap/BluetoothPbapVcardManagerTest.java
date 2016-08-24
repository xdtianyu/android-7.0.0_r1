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

import com.android.bluetooth.pbap.BluetoothPbapObexServer;
import com.android.bluetooth.pbap.BluetoothPbapVcardManager;
import com.android.bluetooth.tests.mock.BluetoothMockContext;
import com.android.bluetooth.tests.mock.SimpleMockContentProvider;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.provider.ContactsContract;
import android.provider.ContactsContract.PhoneLookup;
import android.test.AndroidTestCase;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.text.TextUtils;


import java.util.ArrayList;

public class BluetoothPbapVcardManagerTest extends AndroidTestCase {

    public void testGetContactNamesByNumberWithEmptyPhoneNumber() {
        getContactNamesByNumberInternal("");
    }

    public void testGetContactNamesByNumberWithPhoneNumber() {
        getContactNamesByNumberInternal("111-111-111");
    }

    private void getContactNamesByNumberInternal(String phoneNumber) {
        String[] columnNames;
        if (TextUtils.isEmpty(phoneNumber)) {
            columnNames = new String[]{Phone.CONTACT_ID, Phone.DISPLAY_NAME};
        } else {
            columnNames = new String[]{PhoneLookup._ID, PhoneLookup.DISPLAY_NAME};
        }

        MatrixCursor mc = new MatrixCursor(columnNames);
        mc.addRow(new Object[]{1L, "A"});
        mc.addRow(new Object[]{1L, "A (1)"});
        mc.addRow(new Object[]{2L, "B"});
        mc.addRow(new Object[]{2L, "B (1)"});
        mc.addRow(new Object[]{3L, "C"});
        mc.addRow(new Object[]{3L, "C (1)"});
        mc.addRow(new Object[]{3L, "C (2)"});
        mc.addRow(new Object[]{4L, "D"});
        BluetoothPbapVcardManager manager = createBluetoothPbapVcardManager(mc);
        ArrayList<String> nameList = manager.getContactNamesByNumber(phoneNumber);

        // If there are multiple display name per id, first one is picked.
        assertEquals("A,1", nameList.get(0));
        assertEquals("B,2", nameList.get(1));
        assertEquals("C,3", nameList.get(2));
        assertEquals("D,4", nameList.get(3));
    }

    public void testGetDistinctContactIdSize() {
        MatrixCursor mc = new MatrixCursor(new String[]{ContactsContract.Data.CONTACT_ID});
        mc.addRow(new String[]{"1"});
        mc.addRow(new String[]{"1"});
        mc.addRow(new String[]{"2"});
        mc.addRow(new String[]{"2"});
        mc.addRow(new String[]{"3"});
        mc.addRow(new String[]{"3"});
        mc.addRow(new String[]{"3"});
        mc.addRow(new String[]{"4"});
        mc.addRow(new String[]{"5"});
        BluetoothPbapVcardManager manager = createBluetoothPbapVcardManager(mc);
        int size = manager.getContactsSize();

        assertEquals(5 + 1, size);  // +1 becoz of always has the 0.vcf
    }

    public void testGetPhonebookNameListOrderByIndex() {
        MatrixCursor mc = new MatrixCursor(
                new String[]{ContactsContract.CommonDataKinds.Phone.CONTACT_ID,
                        ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME});
        // test name duplication.
        mc.addRow(new Object[]{1L, "A"});
        mc.addRow(new Object[]{1L, "A (1)"});
        mc.addRow(new Object[]{2L, "B"});
        mc.addRow(new Object[]{2L, "B (1)"});
        mc.addRow(new Object[]{3L, "C"});
        mc.addRow(new Object[]{3L, "C (1)"});
        mc.addRow(new Object[]{3L, "C (2)"});
        mc.addRow(new Object[]{4L, "D"});
        // test default name.
        mc.addRow(new Object[]{5L, null});
        BluetoothPbapVcardManager manager = createBluetoothPbapVcardManager(mc);
        ArrayList<String> nameList = manager
                .getPhonebookNameList(BluetoothPbapObexServer.ORDER_BY_INDEXED);

        // Skip the first one which is supposed to be owner name.
        assertEquals("A,1", nameList.get(1));
        assertEquals("B,2", nameList.get(2));
        assertEquals("C,3", nameList.get(3));
        assertEquals("D,4", nameList.get(4));
        assertEquals(getContext().getString(android.R.string.unknownName) + ",5", nameList.get(5));
    }

    public void testGetPhonebookNameListOrderByAlphabetical() {
        MatrixCursor mc = new MatrixCursor(
                new String[]{ContactsContract.CommonDataKinds.Phone.CONTACT_ID,
                        ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME});
        // test sorting order.
        mc.addRow(new Object[]{1L, "D"});
        mc.addRow(new Object[]{1L, "D (1)"});
        mc.addRow(new Object[]{2L, "C"});
        mc.addRow(new Object[]{2L, "C (1)"});
        mc.addRow(new Object[]{3L, "A"});
        mc.addRow(new Object[]{3L, "A (1)"});
        mc.addRow(new Object[]{3L, "A (2)"});
        mc.addRow(new Object[]{4L, "B"});
        BluetoothPbapVcardManager manager = createBluetoothPbapVcardManager(mc);
        ArrayList<String> nameList = manager
                .getPhonebookNameList(BluetoothPbapObexServer.ORDER_BY_ALPHABETICAL);

        // Skip the first one which is supposed to be owner name.
        assertEquals("A,3", nameList.get(1));
        assertEquals("B,4", nameList.get(2));
        assertEquals("C,2", nameList.get(3));
        assertEquals("D,1", nameList.get(4));
    }

    private BluetoothPbapVcardManager createBluetoothPbapVcardManager(Cursor result) {
        MockContentProvider contentProvider = new SimpleMockContentProvider(result);
        MockContentResolver contentResolver = new MockContentResolver();
        contentResolver.addProvider(ContactsContract.AUTHORITY, contentProvider);
        BluetoothMockContext mockContext = new BluetoothMockContext(contentResolver, getContext());
        return new BluetoothPbapVcardManager(mockContext);
    }
}
