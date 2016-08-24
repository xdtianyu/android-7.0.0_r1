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
package com.android.emergency.edit;

import android.app.Instrumentation;
import android.content.Intent;
import android.content.IntentFilter;
import android.preference.Preference;
import android.provider.ContactsContract;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.MediumTest;

import com.android.emergency.PreferenceKeys;

/**
 * Tests for {@link EditEmergencyContactsFragment}.
 */
@MediumTest
public class EditEmergencyContactsFragmentTest
        extends ActivityInstrumentationTestCase2<EditInfoActivity> {
    private EditEmergencyContactsFragment mEditEmergencyContactsFragment;
    private Preference mAddContactPreference;

    public EditEmergencyContactsFragmentTest() {
        super(EditInfoActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mEditEmergencyContactsFragment = (EditEmergencyContactsFragment)
                getActivity().getFragments().get(1).second;
        mAddContactPreference =
                mEditEmergencyContactsFragment.findPreference(PreferenceKeys.KEY_ADD_CONTACT);
    }

    public void testAddContactPreference() throws Throwable {
        assertNotNull(mAddContactPreference);

        IntentFilter intentFilter = new IntentFilter(Intent.ACTION_PICK);
        intentFilter.addDataType(ContactsContract.CommonDataKinds.Phone.CONTENT_TYPE);

        Instrumentation.ActivityMonitor activityMonitor =
                getInstrumentation().addMonitor(intentFilter, null, true /* block */);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mAddContactPreference
                        .getOnPreferenceClickListener().onPreferenceClick(mAddContactPreference);
            }
        });

        assertEquals(true, getInstrumentation().checkMonitorHit(activityMonitor, 1 /* minHits */));
    }
}
