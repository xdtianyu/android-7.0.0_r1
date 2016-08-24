/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.android.cellbroadcastreceiver;

import android.content.Context;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.os.UserManager;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

/**
 * Settings activity for the cell broadcast receiver.
 */
public class CellBroadcastSettings extends PreferenceActivity {

    private static final String TAG = "CellBroadcastSettings";

    // Preference key for whether to enable emergency notifications (default enabled).
    public static final String KEY_ENABLE_EMERGENCY_ALERTS = "enable_emergency_alerts";

    // Duration of alert sound (in seconds).
    public static final String KEY_ALERT_SOUND_DURATION = "alert_sound_duration";

    // Default alert duration (in seconds).
    public static final String ALERT_SOUND_DEFAULT_DURATION = "4";

    // Enable vibration on alert (unless master volume is silent).
    public static final String KEY_ENABLE_ALERT_VIBRATE = "enable_alert_vibrate";

    // Speak contents of alert after playing the alert sound.
    public static final String KEY_ENABLE_ALERT_SPEECH = "enable_alert_speech";

    // Preference category for emergency alert and CMAS settings.
    public static final String KEY_CATEGORY_ALERT_SETTINGS = "category_alert_settings";

    // Preference category for ETWS related settings.
    public static final String KEY_CATEGORY_ETWS_SETTINGS = "category_etws_settings";

    // Whether to display CMAS extreme threat notifications (default is enabled).
    public static final String KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS =
            "enable_cmas_extreme_threat_alerts";

    // Whether to display CMAS severe threat notifications (default is enabled).
    public static final String KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS =
            "enable_cmas_severe_threat_alerts";

    // Whether to display CMAS amber alert messages (default is enabled).
    public static final String KEY_ENABLE_CMAS_AMBER_ALERTS = "enable_cmas_amber_alerts";

    // Preference category for development settings (enabled by settings developer options toggle).
    public static final String KEY_CATEGORY_DEV_SETTINGS = "category_dev_settings";

    // Whether to display ETWS test messages (default is disabled).
    public static final String KEY_ENABLE_ETWS_TEST_ALERTS = "enable_etws_test_alerts";

    // Whether to display CMAS monthly test messages (default is disabled).
    public static final String KEY_ENABLE_CMAS_TEST_ALERTS = "enable_cmas_test_alerts";

    // Preference category for Brazil specific settings.
    public static final String KEY_CATEGORY_BRAZIL_SETTINGS = "category_brazil_settings";

    // Preference key for whether to enable channel 50 notifications
    // Enabled by default for phones sold in Brazil, otherwise this setting may be hidden.
    public static final String KEY_ENABLE_CHANNEL_50_ALERTS = "enable_channel_50_alerts";

    // Preference key for initial opt-in/opt-out dialog.
    public static final String KEY_SHOW_CMAS_OPT_OUT_DIALOG = "show_cmas_opt_out_dialog";

    // Alert reminder interval ("once" = single 2 minute reminder).
    public static final String KEY_ALERT_REMINDER_INTERVAL = "alert_reminder_interval";

    // Default reminder interval.
    public static final String ALERT_REMINDER_INTERVAL = "0";

    // First time use
    public static final String KEY_FIRST_TIME = "first_time";

    // Brazil country code
    private static final String COUNTRY_BRAZIL = "br";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        UserManager userManager = (UserManager) getSystemService(Context.USER_SERVICE);
        if (userManager.hasUserRestriction(UserManager.DISALLOW_CONFIG_CELL_BROADCASTS)) {
            setContentView(R.layout.cell_broadcast_disallowed_preference_screen);
            return;
        }

        // Display the fragment as the main content.
        getFragmentManager().beginTransaction().replace(android.R.id.content,
                new CellBroadcastSettingsFragment()).commit();
    }

    /**
     * New fragment-style implementation of preferences.
     */
    public static class CellBroadcastSettingsFragment extends PreferenceFragment {

        private CheckBoxPreference mExtremeCheckBox;
        private CheckBoxPreference mSevereCheckBox;
        private CheckBoxPreference mAmberCheckBox;
        private CheckBoxPreference mEmergencyCheckBox;
        private ListPreference mAlertDuration;
        private ListPreference mReminderInterval;
        private CheckBoxPreference mSpeechCheckBox;
        private CheckBoxPreference mEtwsTestCheckBox;
        private CheckBoxPreference mChannel50CheckBox;
        private CheckBoxPreference mCmasTestCheckBox;
        private PreferenceCategory mAlertCategory;
        private PreferenceCategory mETWSSettingCategory;

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            // Load the preferences from an XML resource
            addPreferencesFromResource(R.xml.preferences);

            PreferenceScreen preferenceScreen = getPreferenceScreen();

            mExtremeCheckBox = (CheckBoxPreference)
                    findPreference(KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS);
            mSevereCheckBox = (CheckBoxPreference)
                    findPreference(KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS);
            mAmberCheckBox = (CheckBoxPreference)
                    findPreference(KEY_ENABLE_CMAS_AMBER_ALERTS);
            mEmergencyCheckBox = (CheckBoxPreference)
                    findPreference(KEY_ENABLE_EMERGENCY_ALERTS);
            mAlertDuration = (ListPreference)
                    findPreference(KEY_ALERT_SOUND_DURATION);
            mReminderInterval = (ListPreference)
                    findPreference(KEY_ALERT_REMINDER_INTERVAL);
            mSpeechCheckBox = (CheckBoxPreference)
                    findPreference(KEY_ENABLE_ALERT_SPEECH);
            mEtwsTestCheckBox = (CheckBoxPreference)
                    findPreference(KEY_ENABLE_ETWS_TEST_ALERTS);
            mChannel50CheckBox = (CheckBoxPreference)
                    findPreference(KEY_ENABLE_CHANNEL_50_ALERTS);
            mCmasTestCheckBox = (CheckBoxPreference)
                    findPreference(KEY_ENABLE_CMAS_TEST_ALERTS);
            mAlertCategory = (PreferenceCategory)
                    findPreference(KEY_CATEGORY_ALERT_SETTINGS);
            mETWSSettingCategory = (PreferenceCategory)
                    findPreference(KEY_CATEGORY_ETWS_SETTINGS);

            // Handler for settings that require us to reconfigure enabled channels in radio
            Preference.OnPreferenceChangeListener startConfigServiceListener =
                    new Preference.OnPreferenceChangeListener() {
                        @Override
                        public boolean onPreferenceChange(Preference pref, Object newValue) {
                            CellBroadcastReceiver.startConfigService(pref.getContext());

                            if (pref.getKey().equals(KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS)) {
                                boolean isExtremeAlertChecked = (Boolean)newValue;
                                if (mSevereCheckBox != null) {
                                    mSevereCheckBox.setEnabled(isExtremeAlertChecked);
                                    mSevereCheckBox.setChecked(false);
                                }
                            }

                            return true;
                        }
                    };

            // Show extra settings when developer options is enabled in settings.
            boolean enableDevSettings = Settings.Global.getInt(getActivity().getContentResolver(),
                    Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, 0) != 0;

            Resources res = getResources();
            boolean showEtwsSettings = res.getBoolean(R.bool.show_etws_settings);

            // alert reminder interval
            mReminderInterval.setSummary(mReminderInterval.getEntry());
            mReminderInterval.setOnPreferenceChangeListener(
                    new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference pref, Object newValue) {
                    final ListPreference listPref = (ListPreference) pref;
                    final int idx = listPref.findIndexOfValue((String) newValue);
                    listPref.setSummary(listPref.getEntries()[idx]);
                    return true;
                }
            });

            boolean forceDisableEtwsCmasTest =
                    isEtwsCmasTestMessageForcedDisabled(getActivity());

            boolean emergencyAlertOnOffOptionEnabled =
                    isEmergencyAlertOnOffOptionEnabled(getActivity());

            if (enableDevSettings || showEtwsSettings || emergencyAlertOnOffOptionEnabled) {
                // enable/disable all alerts except CMAS presidential alerts.
                if (mEmergencyCheckBox != null) {
                    mEmergencyCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
                }
            } else {
                mAlertCategory.removePreference(mEmergencyCheckBox);
            }

            // Show alert settings and ETWS categories for ETWS builds and developer mode.
            if (enableDevSettings || showEtwsSettings) {
                // alert sound duration
                ListPreference duration = mAlertDuration;
                duration.setSummary(duration.getEntry());
                duration.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                    @Override
                    public boolean onPreferenceChange(Preference pref, Object newValue) {
                        final ListPreference listPref = (ListPreference) pref;
                        final int idx = listPref.findIndexOfValue((String) newValue);
                        listPref.setSummary(listPref.getEntries()[idx]);
                        return true;
                    }
                });

                if (forceDisableEtwsCmasTest) {
                    // Remove ETWS test preference.
                    preferenceScreen.removePreference(mETWSSettingCategory);

                    PreferenceCategory devSettingCategory =
                            (PreferenceCategory) findPreference(KEY_CATEGORY_DEV_SETTINGS);

                    // Remove CMAS test preference.
                    if (devSettingCategory != null) {
                        devSettingCategory.removePreference(mCmasTestCheckBox);
                    }
                }
            } else {
                mAlertCategory.removePreference(mAlertDuration);
                mAlertCategory.removePreference(mSpeechCheckBox);
                // Remove ETWS test preference category.
                preferenceScreen.removePreference(mETWSSettingCategory);
            }

            if (!res.getBoolean(R.bool.show_cmas_settings)) {
                // Remove CMAS preference items in emergency alert category.
                mAlertCategory.removePreference(mExtremeCheckBox);
                mAlertCategory.removePreference(mSevereCheckBox);
                mAlertCategory.removePreference(mAmberCheckBox);
            }

            TelephonyManager tm = (TelephonyManager) getActivity().getSystemService(
                    Context.TELEPHONY_SERVICE);

            int subId = SubscriptionManager.getDefaultSmsSubscriptionId();
            if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
                subId = SubscriptionManager.getDefaultSubscriptionId();
            }

            boolean enableChannel50Support = res.getBoolean(R.bool.show_brazil_settings) ||
                    COUNTRY_BRAZIL.equals(tm.getSimCountryIso(subId));

            if (!enableChannel50Support) {
                preferenceScreen.removePreference(findPreference(KEY_CATEGORY_BRAZIL_SETTINGS));
            }
            if (!enableDevSettings) {
                preferenceScreen.removePreference(findPreference(KEY_CATEGORY_DEV_SETTINGS));
            }

            if (mChannel50CheckBox != null) {
                mChannel50CheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mEtwsTestCheckBox != null) {
                mEtwsTestCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mExtremeCheckBox != null) {
                mExtremeCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mSevereCheckBox != null) {
                mSevereCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
                if (mExtremeCheckBox != null) {
                    mSevereCheckBox.setEnabled(mExtremeCheckBox.isChecked());
                }
            }
            if (mAmberCheckBox != null) {
                mAmberCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mCmasTestCheckBox != null) {
                mCmasTestCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
        }
    }

    // Check if ETWS/CMAS test message is forced disabled on the device.
    public static boolean isEtwsCmasTestMessageForcedDisabled(Context context) {

        if (context == null) {
            return false;
        }

        int subId = SubscriptionManager.getDefaultSmsSubscriptionId();
        if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            subId = SubscriptionManager.getDefaultSubscriptionId();
            if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID)
                return false;
        }

        CarrierConfigManager configManager =
                (CarrierConfigManager) context.getSystemService(Context.CARRIER_CONFIG_SERVICE);

        if (configManager != null) {
            PersistableBundle carrierConfig =
                    configManager.getConfigForSubId(subId);

            if (carrierConfig != null) {
                return carrierConfig.getBoolean(
                        CarrierConfigManager.KEY_CARRIER_FORCE_DISABLE_ETWS_CMAS_TEST_BOOL);
            }
        }

        return false;
    }

    // Check if "Turn on Notifications" option should be always displayed regardless of developer
    // options turned on or not.
    public static boolean isEmergencyAlertOnOffOptionEnabled(Context context) {

        if (context == null) {
            return false;
        }

        int subId = SubscriptionManager.getDefaultSmsSubscriptionId();
        if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            subId = SubscriptionManager.getDefaultSubscriptionId();
            if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID)
                return false;
        }

        CarrierConfigManager configManager =
                (CarrierConfigManager) context.getSystemService(Context.CARRIER_CONFIG_SERVICE);

        if (configManager != null) {
            PersistableBundle carrierConfig =
                    configManager.getConfigForSubId(subId);

            if (carrierConfig != null) {
                return carrierConfig.getBoolean(
                    CarrierConfigManager.KEY_ALWAYS_SHOW_EMERGENCY_ALERT_ONOFF_BOOL);
            }
        }

        return false;
    }
}
