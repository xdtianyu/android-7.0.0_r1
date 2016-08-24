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
import android.test.suitebuilder.annotation.LargeTest;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.TtsSpan;

import com.android.emergency.PreferenceKeys;
import com.android.emergency.R;
import com.android.emergency.edit.EditEmergencyInfoFragment;
import com.android.emergency.edit.EditInfoActivity;

/**
 * Tests for {@link EmergencyListPreference}.
 */
@LargeTest
public class EmergencyListPreferenceTest
        extends ActivityInstrumentationTestCase2<EditInfoActivity> {
    private EmergencyListPreference mOrganDonorPreference;
    private EmergencyListPreference mBloodTypeListPreference;
    private EditEmergencyInfoFragment mEditInfoFragment;

    public EmergencyListPreferenceTest() {
        super(EditInfoActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mEditInfoFragment = (EditEmergencyInfoFragment) getActivity().getFragments().get(0).second;
        mOrganDonorPreference = (EmergencyListPreference)
                mEditInfoFragment.findPreference(PreferenceKeys.KEY_ORGAN_DONOR);
        mBloodTypeListPreference = (EmergencyListPreference)
                mEditInfoFragment.findPreference(PreferenceKeys.KEY_BLOOD_TYPE);
        try {
            runTestOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mOrganDonorPreference.setValue("");
                    mBloodTypeListPreference.setValue("");
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

    public void testSummary_organDonor() {
        String summary = (String) mOrganDonorPreference.getSummary();
        String summaryExp =
                getActivity().getResources().getString(R.string.unknown_organ_donor);
        assertEquals(summaryExp, summary);
    }

    public void testSummary_bloodType() {
        String summary = mBloodTypeListPreference.getSummary().toString();
        CharSequence summaryExp =
                getActivity().getResources().getString(R.string.unknown_blood_type);
        assertEquals(summaryExp, summary);
    }

    public void testTitle_organDonor() {
        String title = (String) mOrganDonorPreference.getTitle();
        String titleExp =
                getActivity().getResources().getString(R.string.organ_donor);
        assertEquals(titleExp, title);
    }

    public void testTitle_bloodType() {
        String title = (String) mBloodTypeListPreference.getTitle();
        String titleExp =
                getActivity().getResources().getString(R.string.blood_type);
        assertEquals(titleExp, title);
    }

    public void testProperties_organDonor() {
        assertNotNull(mOrganDonorPreference);
        assertEquals(PreferenceKeys.KEY_ORGAN_DONOR, mOrganDonorPreference.getKey());
        assertTrue(mOrganDonorPreference.isEnabled());
        assertTrue(mOrganDonorPreference.isPersistent());
        assertTrue(mOrganDonorPreference.isSelectable());
        assertTrue(mOrganDonorPreference.isNotSet());
        assertEquals("", mOrganDonorPreference.getValue());
        assertEquals(mOrganDonorPreference.getEntryValues().length,
                mOrganDonorPreference.getEntries().length);
        assertNull(mOrganDonorPreference.getContentDescriptions());
    }

    public void testProperties_bloodType() {
        assertNotNull(mBloodTypeListPreference);
        assertEquals(PreferenceKeys.KEY_BLOOD_TYPE, mBloodTypeListPreference.getKey());
        assertTrue(mBloodTypeListPreference.isEnabled());
        assertTrue(mBloodTypeListPreference.isPersistent());
        assertTrue(mBloodTypeListPreference.isSelectable());
        assertTrue(mBloodTypeListPreference.isNotSet());
        assertEquals("", mBloodTypeListPreference.getValue());
        assertEquals(mBloodTypeListPreference.getEntryValues().length,
                mBloodTypeListPreference.getEntries().length);
        assertEquals(mBloodTypeListPreference.getContentDescriptions().length,
                mBloodTypeListPreference.getEntries().length);
    }

    public void testReloadFromPreference() throws Throwable {
        mEditInfoFragment.getPreferenceManager().getSharedPreferences()
                .edit()
                .putString(mOrganDonorPreference.getKey(),
                        (String) mOrganDonorPreference.getEntryValues()[0])
                .commit();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mOrganDonorPreference.reloadFromPreference();
            }
        });
        assertEquals(mOrganDonorPreference.getEntryValues()[0], mOrganDonorPreference.getValue());
        assertFalse(mOrganDonorPreference.isNotSet());
    }

    public void testSetValue() throws Throwable {
        for (int i = 0; i < mOrganDonorPreference.getEntryValues().length; i++) {
            final int index = i;
            runTestOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mOrganDonorPreference.setValue((String)
                            mOrganDonorPreference.getEntryValues()[index]);
                }
            });

            assertEquals(mOrganDonorPreference.getEntryValues()[index],
                    mOrganDonorPreference.getValue());
            if (!TextUtils.isEmpty(mOrganDonorPreference.getEntryValues()[index])) {
                assertEquals(mOrganDonorPreference.getEntries()[index],
                        mOrganDonorPreference.getSummary());
            } else {
                assertEquals(getActivity().getResources().getString(R.string.unknown_organ_donor),
                        mOrganDonorPreference.getSummary());
            }
        }
    }

    public void testContentDescriptions() {
        for (int i = 0; i < mBloodTypeListPreference.getEntries().length; i++) {
            SpannableString entry = ((SpannableString) mBloodTypeListPreference.getEntries()[i]);
            TtsSpan[] span = entry.getSpans(0,
                    mBloodTypeListPreference.getContentDescriptions().length, TtsSpan.class);
            assertEquals(1, span.length);
            assertEquals(span[0].getArgs().get(TtsSpan.ARG_TEXT),
                    mBloodTypeListPreference.getContentDescriptions()[i]);
        }
    }
}
