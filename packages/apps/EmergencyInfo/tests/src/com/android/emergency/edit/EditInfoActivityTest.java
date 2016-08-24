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

import android.app.AlertDialog;
import android.app.Fragment;
import android.content.Context;
import android.content.DialogInterface;
import android.net.Uri;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Pair;

import com.android.emergency.ContactTestUtils;
import com.android.emergency.PreferenceKeys;
import com.android.emergency.R;
import com.android.emergency.preferences.EmergencyContactsPreference;
import com.android.emergency.preferences.EmergencyEditTextPreference;
import com.android.emergency.preferences.EmergencyListPreference;
import com.android.emergency.preferences.NameAutoCompletePreference;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for {@link EditInfoActivity}.
 */
@LargeTest
public class EditInfoActivityTest extends ActivityInstrumentationTestCase2<EditInfoActivity> {
    private ArrayList<Pair<String, Fragment>> mFragments;
    private EditEmergencyInfoFragment mEditEmergencyInfoFragment;
    private EditEmergencyContactsFragment mEditEmergencyContactsFragment;
    private PowerManager.WakeLock mKeepScreenOnWakeLock;

    public EditInfoActivityTest() {
        super(EditInfoActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        PreferenceManager.getDefaultSharedPreferences(getActivity()).edit().clear().commit();
        forceScreenOn();

        mFragments = getActivity().getFragments();
        mEditEmergencyInfoFragment = (EditEmergencyInfoFragment) mFragments.get(0).second;
        mEditEmergencyContactsFragment = (EditEmergencyContactsFragment) mFragments.get(1).second;
    }

    @Override
    protected void tearDown() throws Exception {
        PreferenceManager.getDefaultSharedPreferences(getActivity()).edit().clear().commit();
        releaseScreenOn();
        super.tearDown();
    }

    public void testTwoFragments() {
        assertEquals(2, mFragments.size());
    }

    public void testInitialState() {
        for (String key : PreferenceKeys.KEYS_EDIT_EMERGENCY_INFO) {
            assertNotNull(mEditEmergencyInfoFragment.findPreference(key));
        }
        EmergencyContactsPreference emergencyContactsPreference =
                (EmergencyContactsPreference) mEditEmergencyContactsFragment
                        .findPreference(PreferenceKeys.KEY_EMERGENCY_CONTACTS);
        assertNotNull(emergencyContactsPreference);
        assertEquals(0, emergencyContactsPreference.getPreferenceCount());
    }

    public void testClearAllPreferences () throws Throwable {
        EditInfoActivity editInfoActivity = getActivity();
        final NameAutoCompletePreference namePreference =
                (NameAutoCompletePreference) mEditEmergencyInfoFragment
                        .findPreference(PreferenceKeys.KEY_NAME);
        final EmergencyEditTextPreference addressPreference =
                (EmergencyEditTextPreference) mEditEmergencyInfoFragment
                        .findPreference(PreferenceKeys.KEY_ADDRESS);
        final EmergencyListPreference bloodTypePreference =
                (EmergencyListPreference) mEditEmergencyInfoFragment
                        .findPreference(PreferenceKeys.KEY_BLOOD_TYPE);
        final EmergencyEditTextPreference allergiesPreference =
                (EmergencyEditTextPreference) mEditEmergencyInfoFragment
                        .findPreference(PreferenceKeys.KEY_ALLERGIES);
        final EmergencyEditTextPreference medicationsPreference =
                (EmergencyEditTextPreference) mEditEmergencyInfoFragment
                        .findPreference(PreferenceKeys.KEY_MEDICATIONS);
        final EmergencyEditTextPreference medicalConditionsPreference =
                (EmergencyEditTextPreference) mEditEmergencyInfoFragment
                        .findPreference(PreferenceKeys.KEY_MEDICAL_CONDITIONS);
        final EmergencyListPreference organDonorPreference =
                (EmergencyListPreference) mEditEmergencyInfoFragment
                        .findPreference(PreferenceKeys.KEY_ORGAN_DONOR);

        final EmergencyContactsPreference emergencyContactsPreference =
                (EmergencyContactsPreference) mEditEmergencyContactsFragment
                        .findPreference(PreferenceKeys.KEY_EMERGENCY_CONTACTS);
        final Uri contactUri = ContactTestUtils
                .createContact(editInfoActivity.getContentResolver(), "Michael", "789");
        final List<Uri> emergencyContacts = new ArrayList<>();
        emergencyContacts.add(contactUri);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                namePreference.setText("John");
                addressPreference.setText("Home");
                bloodTypePreference.setValue("A+");
                allergiesPreference.setText("Peanuts");
                medicationsPreference.setText("Aspirin");
                medicalConditionsPreference.setText("Asthma");
                organDonorPreference.setValue("Yes");
                emergencyContactsPreference.setEmergencyContacts(emergencyContacts);
            }
        });

        String unknownName = editInfoActivity.getResources().getString(R.string.unknown_name);
        String unknownAddress = getActivity().getResources().getString(R.string.unknown_address);
        String unknownBloodType =
                editInfoActivity.getResources().getString(R.string.unknown_blood_type);
        String unknownAllergies =
                editInfoActivity.getResources().getString(R.string.unknown_allergies);
        String unknownMedications =
                editInfoActivity.getResources().getString(R.string.unknown_medications);
        String unknownMedicalConditions =
                editInfoActivity.getResources().getString(R.string.unknown_medical_conditions);
        String unknownOrganDonor =
                editInfoActivity.getResources().getString(R.string.unknown_organ_donor);

        assertNotSame(unknownName, namePreference.getSummary());
        assertNotSame(unknownAddress, addressPreference.getSummary());
        assertNotSame(unknownBloodType, bloodTypePreference.getSummary());
        assertNotSame(unknownAllergies, allergiesPreference.getSummary());
        assertNotSame(unknownMedications, medicationsPreference.getSummary());
        assertNotSame(unknownMedicalConditions, medicalConditionsPreference.getSummary());
        assertNotSame(unknownOrganDonor, organDonorPreference.getSummary());
        assertEquals(1, emergencyContactsPreference.getEmergencyContacts().size());
        assertEquals(1, emergencyContactsPreference.getPreferenceCount());


        EditInfoActivity.ClearAllDialogFragment clearAllDialogFragment =
                (EditInfoActivity.ClearAllDialogFragment) editInfoActivity.getFragmentManager()
                        .findFragmentByTag(EditInfoActivity.TAG_CLEAR_ALL_DIALOG);
        assertNull(clearAllDialogFragment);
        getInstrumentation().invokeMenuActionSync(editInfoActivity, R.id.action_clear_all,
                0 /* flags */);
        getInstrumentation().waitForIdleSync();
        final EditInfoActivity.ClearAllDialogFragment clearAllDialogFragmentAfterwards =
                (EditInfoActivity.ClearAllDialogFragment) editInfoActivity.getFragmentManager()
                        .findFragmentByTag(EditInfoActivity.TAG_CLEAR_ALL_DIALOG);

        assertTrue(clearAllDialogFragmentAfterwards.getDialog().isShowing());

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                ((AlertDialog) clearAllDialogFragmentAfterwards.getDialog())
                        .getButton(DialogInterface.BUTTON_POSITIVE)
                        .performClick();
            }
        });
        getInstrumentation().waitForIdleSync();

        assertEquals(unknownName, namePreference.getSummary());
        assertEquals(unknownAddress, addressPreference.getSummary());
        assertEquals(unknownBloodType, bloodTypePreference.getSummary().toString());
        assertEquals(unknownAllergies, allergiesPreference.getSummary());
        assertEquals(unknownMedications, medicationsPreference.getSummary());
        assertEquals(unknownMedicalConditions, medicalConditionsPreference.getSummary());
        assertEquals(unknownOrganDonor, organDonorPreference.getSummary());
        assertEquals(0, emergencyContactsPreference.getEmergencyContacts().size());
        assertEquals(0, emergencyContactsPreference.getPreferenceCount());

        assertTrue(ContactTestUtils
                .deleteContact(getActivity().getContentResolver(), "Michael", "789"));
    }

    public void testWarningDialog_onPauseAndResume() throws Throwable {
        final EditInfoActivity.WarningDialogFragment dialog =
                (EditInfoActivity.WarningDialogFragment) getActivity().getFragmentManager()
                        .findFragmentByTag(EditInfoActivity.TAG_WARNING_DIALOG);
        assertTrue(dialog.getDialog().isShowing());

        onPause();
        onResume();

        final EditInfoActivity.WarningDialogFragment dialogAfterOnResume =
                (EditInfoActivity.WarningDialogFragment) getActivity().getFragmentManager()
                        .findFragmentByTag(EditInfoActivity.TAG_WARNING_DIALOG);
        assertTrue(dialogAfterOnResume.getDialog().isShowing());
    }

    public void testWarningDialog_negativeButton() throws Throwable {
        EditInfoActivity activity = getActivity();
        final EditInfoActivity.WarningDialogFragment dialogFragment =
                (EditInfoActivity.WarningDialogFragment) activity.getFragmentManager()
                        .findFragmentByTag(EditInfoActivity.TAG_WARNING_DIALOG);
        assertNotNull(dialogFragment.getActivity());
        assertTrue(dialogFragment.getDialog().isShowing());
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                ((AlertDialog) dialogFragment.getDialog())
                        .getButton(DialogInterface.BUTTON_NEGATIVE)
                        .performClick();
            }
        });
        getInstrumentation().waitForIdleSync();

        assertNull(dialogFragment.getDialog());
    }

    public void testWarningDialog_positiveButton() throws Throwable {
        EditInfoActivity activity = getActivity();
        final EditInfoActivity.WarningDialogFragment dialogFragment =
                (EditInfoActivity.WarningDialogFragment) activity.getFragmentManager()
                        .findFragmentByTag(EditInfoActivity.TAG_WARNING_DIALOG);
        assertTrue(dialogFragment.getDialog().isShowing());

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                ((AlertDialog) dialogFragment.getDialog())
                        .getButton(DialogInterface.BUTTON_POSITIVE)
                        .performClick();
            }
        });
        getInstrumentation().waitForIdleSync();
        assertNull(dialogFragment.getDialog());

        onPause();
        onResume();

        EditInfoActivity.WarningDialogFragment dialogAfterOnResume =
                (EditInfoActivity.WarningDialogFragment) getActivity().getFragmentManager()
                        .findFragmentByTag(EditInfoActivity.TAG_WARNING_DIALOG);
        assertNull(dialogAfterOnResume);
    }

    public void testWarningDialogTimer_overOneDayAgo() throws Throwable {
        EditInfoActivity activity = getActivity();
        final EditInfoActivity.WarningDialogFragment dialogFragment =
                (EditInfoActivity.WarningDialogFragment) activity.getFragmentManager()
                        .findFragmentByTag(EditInfoActivity.TAG_WARNING_DIALOG);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                ((AlertDialog) dialogFragment.getDialog())
                        .getButton(DialogInterface.BUTTON_POSITIVE)
                        .performClick();
            }
        });
        getInstrumentation().waitForIdleSync();

        onPause();
        // Manually make the last consent be a bit over a day ago
        long overOneDayAgoMs = System.currentTimeMillis() - EditInfoActivity.ONE_DAY_MS - 60_000;
        PreferenceManager.getDefaultSharedPreferences(activity).edit()
                .putLong(EditInfoActivity.KEY_LAST_CONSENT_TIME_MS,
                        overOneDayAgoMs).commit();
        onResume();

        EditInfoActivity.WarningDialogFragment dialogAfterOnResume =
                (EditInfoActivity.WarningDialogFragment) getActivity().getFragmentManager()
                        .findFragmentByTag(EditInfoActivity.TAG_WARNING_DIALOG);
        assertTrue(dialogAfterOnResume.getDialog().isShowing());
    }

    private void onPause() throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                getInstrumentation().callActivityOnPause(getActivity());
            }
        });
    }

    private void onResume() throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                getInstrumentation().callActivityOnResume(getActivity());
            }
        });
        getInstrumentation().waitForIdleSync();
    }

    private void forceScreenOn() {
        int levelAndFlags = PowerManager.FULL_WAKE_LOCK
                | PowerManager.ON_AFTER_RELEASE
                | PowerManager.ACQUIRE_CAUSES_WAKEUP;
        PowerManager powerManager =
                (PowerManager) getActivity().getSystemService(Context.POWER_SERVICE);
        mKeepScreenOnWakeLock = powerManager.newWakeLock(levelAndFlags, "EditEmergencyInfo");
        mKeepScreenOnWakeLock.setReferenceCounted(false);
        mKeepScreenOnWakeLock.acquire();
    }

    private void releaseScreenOn() {
        mKeepScreenOnWakeLock.release();
    }
}
