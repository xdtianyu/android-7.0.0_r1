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

import android.preference.PreferenceManager;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.MediumTest;

import com.android.emergency.PreferenceKeys;
import com.android.emergency.R;
import com.android.emergency.edit.EditEmergencyInfoFragment;
import com.android.emergency.edit.EditInfoActivity;

/**
 * Tests for {@link EmergencyEditTextPreference}.
 */
@MediumTest
public class EmergencyEditTextPreferenceTest
        extends ActivityInstrumentationTestCase2<EditInfoActivity> {
    private EmergencyEditTextPreference mPreference;
    private EditEmergencyInfoFragment mEditInfoFragment;

    public EmergencyEditTextPreferenceTest() {
        super(EditInfoActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mEditInfoFragment = (EditEmergencyInfoFragment) getActivity().getFragments().get(0).second;
        mPreference = (EmergencyEditTextPreference)
                mEditInfoFragment.findPreference(PreferenceKeys.KEY_MEDICAL_CONDITIONS);
        try {
            runTestOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mPreference.setText("");
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
        String summary = (String) mPreference.getSummary();
        String summaryExp =
                getActivity().getResources().getString(R.string.unknown_medical_conditions);
        assertEquals(summaryExp, summary);
    }

    public void testTitle() {
        String title = (String) mPreference.getTitle();
        String titleExp =
                getActivity().getResources().getString(R.string.medical_conditions);
        assertEquals(titleExp, title);
    }

    public void testProperties() {
        assertNotNull(mPreference);
        assertEquals(PreferenceKeys.KEY_MEDICAL_CONDITIONS, mPreference.getKey());
        assertTrue(mPreference.isEnabled());
        assertTrue(mPreference.isPersistent());
        assertTrue(mPreference.isSelectable());
        assertTrue(mPreference.isNotSet());
        assertEquals("", mPreference.getText());
    }

    public void testReloadFromPreference() throws Throwable {
        String medicalConditions = "Asthma";
        mEditInfoFragment.getPreferenceManager().getSharedPreferences().edit()
                .putString(mPreference.getKey(), medicalConditions).commit();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mPreference.reloadFromPreference();
            }
        });
        assertEquals(medicalConditions, mPreference.getText());
        assertFalse(mPreference.isNotSet());
    }

    public void testSetText() throws Throwable {
        final String medicalConditions = "Asthma";
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mPreference.setText(medicalConditions);
            }
        });

        assertEquals(medicalConditions, mPreference.getText());
        assertEquals(medicalConditions, mPreference.getSummary());
    }
}
