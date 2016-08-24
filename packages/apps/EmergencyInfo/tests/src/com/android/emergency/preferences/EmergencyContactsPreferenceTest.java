/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.emergency.preferences;

import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.DialogInterface;
import android.net.Uri;
import android.preference.PreferenceManager;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.LargeTest;
import android.view.View;

import com.android.emergency.ContactTestUtils;
import com.android.emergency.PreferenceKeys;
import com.android.emergency.R;
import com.android.emergency.edit.EditEmergencyContactsFragment;
import com.android.emergency.edit.EditInfoActivity;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for {@link EmergencyContactsPreference}.
 */
@LargeTest
public class EmergencyContactsPreferenceTest
        extends ActivityInstrumentationTestCase2<EditInfoActivity> {
    private EmergencyContactsPreference mEmergencyContactsPreference;
    private EditEmergencyContactsFragment mEditEmergencyContactsFragment;
    private ContentResolver mContentResolver;

    public EmergencyContactsPreferenceTest() {
        super(EditInfoActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mEditEmergencyContactsFragment =
                (EditEmergencyContactsFragment) getActivity().getFragments().get(1).second;
        mEmergencyContactsPreference =
                (EmergencyContactsPreference) mEditEmergencyContactsFragment
                        .findPreference(PreferenceKeys.KEY_EMERGENCY_CONTACTS);
        try {
            runTestOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mEmergencyContactsPreference.setEmergencyContacts(new ArrayList<Uri>());
                }
            });
        } catch (Throwable throwable) {
            fail("Should not fail" + throwable);
        }

        mContentResolver = getActivity().getContentResolver();
    }

    @Override
    protected void tearDown() throws Exception {
        PreferenceManager.getDefaultSharedPreferences(getActivity()).edit().clear().commit();
        super.tearDown();
    }

    public void testEmptyState() {
        assertNotNull(mEmergencyContactsPreference);
        assertTrue(mEmergencyContactsPreference.isPersistent());
        assertTrue(mEmergencyContactsPreference.isNotSet());
        assertTrue(mEmergencyContactsPreference.getEmergencyContacts().isEmpty());
        assertEquals(0, mEmergencyContactsPreference.getPreferenceCount());
    }

    public void testAddAndRemoveEmergencyContact() throws Throwable {
        final String name = "Jane";
        final String phoneNumber = "456";

        final Uri emergencyContactUri =
                ContactTestUtils.createContact(mContentResolver, name, phoneNumber);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mEmergencyContactsPreference.addNewEmergencyContact(emergencyContactUri);
            }
        });

        assertEquals(1, mEmergencyContactsPreference.getEmergencyContacts().size());
        assertEquals(1, mEmergencyContactsPreference.getPreferenceCount());
        ContactPreference contactPreference = (ContactPreference)
                mEmergencyContactsPreference.getPreference(0);

        assertEquals(emergencyContactUri, contactPreference.getContactUri());
        assertEquals(name, contactPreference.getTitle());
        assertTrue(((String) contactPreference.getSummary()).contains(phoneNumber));

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mEmergencyContactsPreference.onRemoveContactPreference(
                        (ContactPreference) mEmergencyContactsPreference.getPreference(0));
            }
        });

        assertEquals(0, mEmergencyContactsPreference.getEmergencyContacts().size());
        assertEquals(0, mEmergencyContactsPreference.getPreferenceCount());

        // Clean up the inserted contact
        assertTrue(ContactTestUtils.deleteContact(mContentResolver, name, phoneNumber));
    }

    public void testReloadFromPreference() throws Throwable {
        final String nameJane = "Jane";
        final String phoneNumberJane = "456";
        final Uri emergencyContactJane = ContactTestUtils
                .createContact(mContentResolver, nameJane, phoneNumberJane);

        final String nameJohn = "John";
        final String phoneNumberJohn = "123";
        final Uri emergencyContactJohn = ContactTestUtils
                .createContact(mContentResolver, nameJohn, phoneNumberJohn);

        final List<Uri> emergencyContacts = new ArrayList<>();
        emergencyContacts.add(emergencyContactJane);
        emergencyContacts.add(emergencyContactJohn);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mEmergencyContactsPreference.setEmergencyContacts(emergencyContacts);
            }
        });

        assertEquals(2, mEmergencyContactsPreference.getEmergencyContacts().size());
        assertEquals(2, mEmergencyContactsPreference.getPreferenceCount());

        // Delete Jane from other app (e.g. contacts)
        assertTrue(ContactTestUtils
                .deleteContact(mContentResolver, nameJane, phoneNumberJane));
        getInstrumentation().waitForIdleSync();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mEmergencyContactsPreference.reloadFromPreference();
            }
        });

        getInstrumentation().waitForIdleSync();

        // Assert the only remaining contact is John
        assertEquals(1, mEmergencyContactsPreference.getEmergencyContacts().size());
        assertEquals(1, mEmergencyContactsPreference.getPreferenceCount());
        ContactPreference contactPreference = (ContactPreference)
                mEmergencyContactsPreference.getPreference(0);
        assertEquals(emergencyContactJohn, contactPreference.getContactUri());

        // Clean up the inserted contact
        assertTrue(ContactTestUtils
                .deleteContact(mContentResolver, nameJohn, phoneNumberJohn));
    }

    public void testWidgetClick_positiveButton() throws Throwable {
        final String name = "Jane";
        final String phoneNumber = "456";

        final Uri emergencyContactUri =
                ContactTestUtils.createContact(mContentResolver, name, phoneNumber);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mEmergencyContactsPreference.addNewEmergencyContact(emergencyContactUri);
            }
        });

        assertEquals(1, mEmergencyContactsPreference.getEmergencyContacts().size());
        assertEquals(1, mEmergencyContactsPreference.getPreferenceCount());
        ContactPreference contactPreference = (ContactPreference)
                mEmergencyContactsPreference.getPreference(0);

        View contactPreferenceView = contactPreference.getView(null, null);
        assertNotNull(contactPreferenceView);
        final View deleteContactWidget = contactPreferenceView.findViewById(R.id.delete_contact);
        assertEquals(View.VISIBLE, deleteContactWidget.getVisibility());

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                deleteContactWidget.performClick();
            }
        });

        getInstrumentation().waitForIdleSync();
        final AlertDialog removeContactDialog = contactPreference.getRemoveContactDialog();
        assertTrue(removeContactDialog.isShowing());

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                removeContactDialog.getButton(DialogInterface.BUTTON_POSITIVE).performClick();
            }
        });
        getInstrumentation().waitForIdleSync();

        assertEquals(0, mEmergencyContactsPreference.getEmergencyContacts().size());
        assertEquals(0, mEmergencyContactsPreference.getPreferenceCount());

        // Clean up the inserted contact
        assertTrue(ContactTestUtils.deleteContact(mContentResolver, name, phoneNumber));
    }

    public void testWidgetClick_negativeButton() throws Throwable {
        final String name = "Jane";
        final String phoneNumber = "456";

        final Uri emergencyContactUri =
                ContactTestUtils.createContact(mContentResolver, name, phoneNumber);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mEmergencyContactsPreference.addNewEmergencyContact(emergencyContactUri);
            }
        });

        assertEquals(1, mEmergencyContactsPreference.getEmergencyContacts().size());
        assertEquals(1, mEmergencyContactsPreference.getPreferenceCount());
        ContactPreference contactPreference = (ContactPreference)
                mEmergencyContactsPreference.getPreference(0);

        View contactPreferenceView = contactPreference.getView(null, null);
        assertNotNull(contactPreferenceView);
        final View deleteContactWidget = contactPreferenceView.findViewById(R.id.delete_contact);
        assertEquals(View.VISIBLE, deleteContactWidget.getVisibility());

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                deleteContactWidget.performClick();
            }
        });
        getInstrumentation().waitForIdleSync();

        getInstrumentation().waitForIdleSync();
        final AlertDialog removeContactDialog = contactPreference.getRemoveContactDialog();
        assertTrue(removeContactDialog.isShowing());

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                removeContactDialog.getButton(DialogInterface.BUTTON_NEGATIVE).performClick();
            }
        });

        assertEquals(1, mEmergencyContactsPreference.getEmergencyContacts().size());
        assertEquals(1, mEmergencyContactsPreference.getPreferenceCount());

        // Clean up the inserted contact
        assertTrue(ContactTestUtils.deleteContact(mContentResolver, name, phoneNumber));
    }
}