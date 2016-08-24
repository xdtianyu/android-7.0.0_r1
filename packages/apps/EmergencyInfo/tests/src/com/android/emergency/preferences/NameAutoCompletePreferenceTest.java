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
import android.content.DialogInterface;
import android.preference.PreferenceManager;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.LargeTest;
import android.widget.AutoCompleteTextView;

import com.android.emergency.PreferenceKeys;
import com.android.emergency.R;
import com.android.emergency.edit.EditEmergencyInfoFragment;
import com.android.emergency.edit.EditInfoActivity;

/**
 * Tests for {@link NameAutoCompletePreference}.
 */
@LargeTest
public class NameAutoCompletePreferenceTest
        extends ActivityInstrumentationTestCase2<EditInfoActivity> {
    private NameAutoCompletePreference mNameAutoCompletePreference;
    private EditEmergencyInfoFragment mEditInfoFragment;

    public NameAutoCompletePreferenceTest() {
        super(EditInfoActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mEditInfoFragment = (EditEmergencyInfoFragment) getActivity().getFragments().get(0).second;
        mNameAutoCompletePreference = (NameAutoCompletePreference)
                mEditInfoFragment.findPreference(PreferenceKeys.KEY_NAME);
        try {
            runTestOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mNameAutoCompletePreference.setText("");
                }
            });
        } catch (Throwable throwable) {
            fail("Should not throw exception");
        }
    }

    @Override
    protected void tearDown() throws Exception {
        PreferenceManager.getDefaultSharedPreferences(getActivity()).edit().clear().commit();
        super.tearDown();
    }

    public void testSummary() {
        String summary = (String) mNameAutoCompletePreference.getSummary();
        String summaryExp =
                getActivity().getResources().getString(R.string.unknown_name);
        assertEquals(summaryExp, summary);
    }

    public void testTitle() {
        String title = (String) mNameAutoCompletePreference.getTitle();
        String titleExp =
                getActivity().getResources().getString(R.string.name);
        assertEquals(titleExp, title);
    }

    public void testProperties() {
        assertNotNull(mNameAutoCompletePreference);
        assertEquals(PreferenceKeys.KEY_NAME, mNameAutoCompletePreference.getKey());
        assertTrue(mNameAutoCompletePreference.isEnabled());
        assertTrue(mNameAutoCompletePreference.isPersistent());
        assertTrue(mNameAutoCompletePreference.isSelectable());
        assertTrue(mNameAutoCompletePreference.isNotSet());
        assertEquals("", mNameAutoCompletePreference.getText());
    }

    public void testReloadFromPreference() throws Throwable {
        String name = "John";
        mEditInfoFragment.getPreferenceManager().getSharedPreferences().edit()
                .putString(mNameAutoCompletePreference.getKey(), name).commit();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mNameAutoCompletePreference.reloadFromPreference();
            }
        });
        assertEquals(name, mNameAutoCompletePreference.getText());
        assertFalse(mNameAutoCompletePreference.isNotSet());
    }

    public void testSetText() throws Throwable {
        final String name = "John";
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mNameAutoCompletePreference.setText(name);
            }
        });

        assertEquals(name, mNameAutoCompletePreference.getText());
        assertEquals(name, mNameAutoCompletePreference.getSummary());
    }

    public void testGetAutoCompleteTextView() {
        AutoCompleteTextView autoCompleteTextView =
                mNameAutoCompletePreference.getAutoCompleteTextView();
        assertNotNull(autoCompleteTextView);
    }

    public void testDialogShowAndDismiss_positiveButton() throws Throwable {
        assertNull(mNameAutoCompletePreference.getDialog());
        assertNotNull(mNameAutoCompletePreference);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mNameAutoCompletePreference.onClick();
            }
        });
        final AlertDialog dialog = (AlertDialog) mNameAutoCompletePreference.getDialog();
        assertTrue(dialog.isShowing());
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                dialog.getButton(DialogInterface.BUTTON_POSITIVE).performClick();
            }
        });
        getInstrumentation().waitForIdleSync();
        assertFalse(dialog.isShowing());
    }

    public void testDialogShowAndDismiss_negativeButton() throws Throwable {
        assertNull(mNameAutoCompletePreference.getDialog());
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mNameAutoCompletePreference.onClick();
            }
        });
        final AlertDialog dialog = (AlertDialog) mNameAutoCompletePreference.getDialog();
        assertTrue(dialog.isShowing());
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                dialog.getButton(DialogInterface.BUTTON_NEGATIVE).performClick();
            }
        });
        getInstrumentation().waitForIdleSync();
        assertFalse(dialog.isShowing());
    }
}
