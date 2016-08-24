/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.phone;

import com.android.ims.ImsManager;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.TelephonyProperties;
import com.android.settingslib.RestrictedLockUtils;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import android.app.ActionBar;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.UserManager;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.telephony.CarrierConfigManager;
import android.telephony.PhoneStateListener;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.TabHost.OnTabChangeListener;
import android.widget.TabHost.TabContentFactory;
import android.widget.TabHost.TabSpec;
import android.widget.TabHost;

/**
 * "Mobile network settings" screen.  This preference screen lets you
 * enable/disable mobile data, and control data roaming and other
 * network-specific mobile data features.  It's used on non-voice-capable
 * tablets as well as regular phone devices.
 *
 * Note that this PreferenceActivity is part of the phone app, even though
 * you reach it from the "Wireless & Networks" section of the main
 * Settings app.  It's not part of the "Call settings" hierarchy that's
 * available from the Phone app (see CallFeaturesSetting for that.)
 */
public class MobileNetworkSettings extends PreferenceActivity
        implements DialogInterface.OnClickListener,
        DialogInterface.OnDismissListener, Preference.OnPreferenceChangeListener{

    // debug data
    private static final String LOG_TAG = "NetworkSettings";
    private static final boolean DBG = true;
    public static final int REQUEST_CODE_EXIT_ECM = 17;

    // Number of active Subscriptions to show tabs
    private static final int TAB_THRESHOLD = 2;

    //String keys for preference lookup
    private static final String BUTTON_PREFERED_NETWORK_MODE = "preferred_network_mode_key";
    private static final String BUTTON_ROAMING_KEY = "button_roaming_key";
    private static final String BUTTON_CDMA_LTE_DATA_SERVICE_KEY = "cdma_lte_data_service_key";
    private static final String BUTTON_ENABLED_NETWORKS_KEY = "enabled_networks_key";
    private static final String BUTTON_4G_LTE_KEY = "enhanced_4g_lte";
    private static final String BUTTON_CELL_BROADCAST_SETTINGS = "cell_broadcast_settings";
    private static final String BUTTON_APN_EXPAND_KEY = "button_apn_key";
    private static final String BUTTON_OPERATOR_SELECTION_EXPAND_KEY = "button_carrier_sel_key";
    private static final String BUTTON_CARRIER_SETTINGS_KEY = "carrier_settings_key";
    private static final String BUTTON_CDMA_SYSTEM_SELECT_KEY = "cdma_system_select_key";

    static final int preferredNetworkMode = Phone.PREFERRED_NT_MODE;

    //Information about logical "up" Activity
    private static final String UP_ACTIVITY_PACKAGE = "com.android.settings";
    private static final String UP_ACTIVITY_CLASS =
            "com.android.settings.Settings$WirelessSettingsActivity";

    private SubscriptionManager mSubscriptionManager;

    //UI objects
    private ListPreference mButtonPreferredNetworkMode;
    private ListPreference mButtonEnabledNetworks;
    private RestrictedSwitchPreference mButtonDataRoam;
    private SwitchPreference mButton4glte;
    private Preference mLteDataServicePref;

    private static final String iface = "rmnet0"; //TODO: this will go away
    private List<SubscriptionInfo> mActiveSubInfos;

    private UserManager mUm;
    private Phone mPhone;
    private MyHandler mHandler;
    private boolean mOkClicked;

    // We assume the the value returned by mTabHost.getCurrentTab() == slotId
    private TabHost mTabHost;

    //GsmUmts options and Cdma options
    GsmUmtsOptions mGsmUmtsOptions;
    CdmaOptions mCdmaOptions;

    private Preference mClickedPreference;
    private boolean mShow4GForLTE;
    private boolean mIsGlobalCdma;
    private boolean mUnavailable;

    private final PhoneStateListener mPhoneStateListener = new PhoneStateListener() {
        /*
         * Enable/disable the 'Enhanced 4G LTE Mode' when in/out of a call
         * and depending on TTY mode and TTY support over VoLTE.
         * @see android.telephony.PhoneStateListener#onCallStateChanged(int,
         * java.lang.String)
         */
        @Override
        public void onCallStateChanged(int state, String incomingNumber) {
            if (DBG) log("PhoneStateListener.onCallStateChanged: state=" + state);
            boolean enabled = (state == TelephonyManager.CALL_STATE_IDLE) &&
                    ImsManager.isNonTtyOrTtyOnVolteEnabled(getApplicationContext());
            Preference pref = getPreferenceScreen().findPreference(BUTTON_4G_LTE_KEY);
            if (pref != null) pref.setEnabled(enabled && hasActiveSubscriptions());
        }
    };

    private final BroadcastReceiver mPhoneChangeReceiver = new PhoneChangeReceiver();

    private class PhoneChangeReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (DBG) log("onReceive:");
            // When the radio changes (ex: CDMA->GSM), refresh all options.
            mGsmUmtsOptions = null;
            mCdmaOptions = null;
            updateBody();
        }
    }

    //This is a method implemented for DialogInterface.OnClickListener.
    //  Used to dismiss the dialogs when they come up.
    public void onClick(DialogInterface dialog, int which) {
        if (which == DialogInterface.BUTTON_POSITIVE) {
            mPhone.setDataRoamingEnabled(true);
            mOkClicked = true;
        } else {
            // Reset the toggle
            mButtonDataRoam.setChecked(false);
        }
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        // Assuming that onClick gets called first
        mButtonDataRoam.setChecked(mOkClicked);
    }

    /**
     * Invoked on each preference click in this hierarchy, overrides
     * PreferenceActivity's implementation.  Used to make sure we track the
     * preference click events.
     */
    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        /** TODO: Refactor and get rid of the if's using subclasses */
        final int phoneSubId = mPhone.getSubId();
        if (preference.getKey().equals(BUTTON_4G_LTE_KEY)) {
            return true;
        } else if (mGsmUmtsOptions != null &&
                mGsmUmtsOptions.preferenceTreeClick(preference) == true) {
            return true;
        } else if (mCdmaOptions != null &&
                   mCdmaOptions.preferenceTreeClick(preference) == true) {
            if (Boolean.parseBoolean(
                    SystemProperties.get(TelephonyProperties.PROPERTY_INECM_MODE))) {

                mClickedPreference = preference;

                // In ECM mode launch ECM app dialog
                startActivityForResult(
                    new Intent(TelephonyIntents.ACTION_SHOW_NOTICE_ECM_BLOCK_OTHERS, null),
                    REQUEST_CODE_EXIT_ECM);
            }
            return true;
        } else if (preference == mButtonPreferredNetworkMode) {
            //displays the value taken from the Settings.System
            int settingsNetworkMode = android.provider.Settings.Global.getInt(mPhone.getContext().
                    getContentResolver(),
                    android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                    preferredNetworkMode);
            mButtonPreferredNetworkMode.setValue(Integer.toString(settingsNetworkMode));
            return true;
        } else if (preference == mLteDataServicePref) {
            String tmpl = android.provider.Settings.Global.getString(getContentResolver(),
                        android.provider.Settings.Global.SETUP_PREPAID_DATA_SERVICE_URL);
            if (!TextUtils.isEmpty(tmpl)) {
                TelephonyManager tm = (TelephonyManager) getSystemService(
                        Context.TELEPHONY_SERVICE);
                String imsi = tm.getSubscriberId();
                if (imsi == null) {
                    imsi = "";
                }
                final String url = TextUtils.isEmpty(tmpl) ? null
                        : TextUtils.expandTemplate(tmpl, imsi).toString();
                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
                startActivity(intent);
            } else {
                android.util.Log.e(LOG_TAG, "Missing SETUP_PREPAID_DATA_SERVICE_URL");
            }
            return true;
        }  else if (preference == mButtonEnabledNetworks) {
            int settingsNetworkMode = android.provider.Settings.Global.getInt(mPhone.getContext().
                            getContentResolver(),
                    android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                    preferredNetworkMode);
            mButtonEnabledNetworks.setValue(Integer.toString(settingsNetworkMode));
            return true;
        } else if (preference == mButtonDataRoam) {
            // Do not disable the preference screen if the user clicks Data roaming.
            return true;
        } else {
            // if the button is anything but the simple toggle preference,
            // we'll need to disable all preferences to reject all click
            // events until the sub-activity's UI comes up.
            preferenceScreen.setEnabled(false);
            // Let the intents be launched by the Preference manager
            return false;
        }
    }

    private final SubscriptionManager.OnSubscriptionsChangedListener mOnSubscriptionsChangeListener
            = new SubscriptionManager.OnSubscriptionsChangedListener() {
        @Override
        public void onSubscriptionsChanged() {
            if (DBG) log("onSubscriptionsChanged:");
            initializeSubscriptions();
        }
    };

    private void initializeSubscriptions() {
        if (isDestroyed()) { // Process preferences in activity only if its not destroyed
            return;
        }
        int currentTab = 0;
        if (DBG) log("initializeSubscriptions:+");

        // Before updating the the active subscription list check
        // if tab updating is needed as the list is changing.
        List<SubscriptionInfo> sil = mSubscriptionManager.getActiveSubscriptionInfoList();
        TabState state = isUpdateTabsNeeded(sil);

        // Update to the active subscription list
        mActiveSubInfos.clear();
        if (sil != null) {
            mActiveSubInfos.addAll(sil);
            // If there is only 1 sim then currenTab should represent slot no. of the sim.
            if (sil.size() == 1) {
                currentTab = sil.get(0).getSimSlotIndex();
            }
        }

        switch (state) {
            case UPDATE: {
                if (DBG) log("initializeSubscriptions: UPDATE");
                currentTab = mTabHost != null ? mTabHost.getCurrentTab() : 0;

                setContentView(com.android.internal.R.layout.common_tab_settings);

                mTabHost = (TabHost) findViewById(android.R.id.tabhost);
                mTabHost.setup();

                // Update the tabName. Since the mActiveSubInfos are in slot order
                // we can iterate though the tabs and subscription info in one loop. But
                // we need to handle the case where a slot may be empty.

                Iterator<SubscriptionInfo> siIterator = mActiveSubInfos.listIterator();
                SubscriptionInfo si = siIterator.hasNext() ? siIterator.next() : null;
                for (int simSlotIndex = 0; simSlotIndex  < mActiveSubInfos.size(); simSlotIndex++) {
                    String tabName;
                    if (si != null && si.getSimSlotIndex() == simSlotIndex) {
                        // Slot is not empty and we match
                        tabName = String.valueOf(si.getDisplayName());
                        si = siIterator.hasNext() ? siIterator.next() : null;
                    } else {
                        // Slot is empty, set name to unknown
                        tabName = getResources().getString(R.string.unknown);
                    }
                    if (DBG) {
                        log("initializeSubscriptions: tab=" + simSlotIndex + " name=" + tabName);
                    }

                    mTabHost.addTab(buildTabSpec(String.valueOf(simSlotIndex), tabName));
                }

                mTabHost.setOnTabChangedListener(mTabListener);
                mTabHost.setCurrentTab(currentTab);
                break;
            }
            case NO_TABS: {
                if (DBG) log("initializeSubscriptions: NO_TABS");

                if (mTabHost != null) {
                    mTabHost.clearAllTabs();
                    mTabHost = null;
                }
                setContentView(com.android.internal.R.layout.common_tab_settings);
                break;
            }
            case DO_NOTHING: {
                if (DBG) log("initializeSubscriptions: DO_NOTHING");
                if (mTabHost != null) {
                    currentTab = mTabHost.getCurrentTab();
                }
                break;
            }
        }
        updatePhone(currentTab);
        updateBody();
        if (DBG) log("initializeSubscriptions:-");
    }

    private enum TabState {
        NO_TABS, UPDATE, DO_NOTHING
    }
    private TabState isUpdateTabsNeeded(List<SubscriptionInfo> newSil) {
        TabState state = TabState.DO_NOTHING;
        if (newSil == null) {
            if (mActiveSubInfos.size() >= TAB_THRESHOLD) {
                if (DBG) log("isUpdateTabsNeeded: NO_TABS, size unknown and was tabbed");
                state = TabState.NO_TABS;
            }
        } else if (newSil.size() < TAB_THRESHOLD && mActiveSubInfos.size() >= TAB_THRESHOLD) {
            if (DBG) log("isUpdateTabsNeeded: NO_TABS, size went to small");
            state = TabState.NO_TABS;
        } else if (newSil.size() >= TAB_THRESHOLD && mActiveSubInfos.size() < TAB_THRESHOLD) {
            if (DBG) log("isUpdateTabsNeeded: UPDATE, size changed");
            state = TabState.UPDATE;
        } else if (newSil.size() >= TAB_THRESHOLD) {
            Iterator<SubscriptionInfo> siIterator = mActiveSubInfos.iterator();
            for(SubscriptionInfo newSi : newSil) {
                SubscriptionInfo curSi = siIterator.next();
                if (!newSi.getDisplayName().equals(curSi.getDisplayName())) {
                    if (DBG) log("isUpdateTabsNeeded: UPDATE, new name=" + newSi.getDisplayName());
                    state = TabState.UPDATE;
                    break;
                }
            }
        }
        if (DBG) {
            log("isUpdateTabsNeeded:- " + state
                + " newSil.size()=" + ((newSil != null) ? newSil.size() : 0)
                + " mActiveSubInfos.size()=" + mActiveSubInfos.size());
        }
        return state;
    }

    private OnTabChangeListener mTabListener = new OnTabChangeListener() {
        @Override
        public void onTabChanged(String tabId) {
            if (DBG) log("onTabChanged:");
            // The User has changed tab; update the body.
            updatePhone(Integer.parseInt(tabId));
            updateBody();
        }
    };

    private void updatePhone(int slotId) {
        final SubscriptionInfo sir = mSubscriptionManager
                .getActiveSubscriptionInfoForSimSlotIndex(slotId);
        if (sir != null) {
            mPhone = PhoneFactory.getPhone(
                    SubscriptionManager.getPhoneId(sir.getSubscriptionId()));
        }
        if (mPhone == null) {
            // Do the best we can
            mPhone = PhoneGlobals.getPhone();
        }
        if (DBG) log("updatePhone:- slotId=" + slotId + " sir=" + sir);
    }

    private TabContentFactory mEmptyTabContent = new TabContentFactory() {
        @Override
        public View createTabContent(String tag) {
            return new View(mTabHost.getContext());
        }
    };

    private TabSpec buildTabSpec(String tag, String title) {
        return mTabHost.newTabSpec(tag).setIndicator(title).setContent(
                mEmptyTabContent);
    }

    @Override
    protected void onCreate(Bundle icicle) {
        if (DBG) log("onCreate:+");
        setTheme(R.style.Theme_Material_Settings);
        super.onCreate(icicle);

        mHandler = new MyHandler();
        mUm = (UserManager) getSystemService(Context.USER_SERVICE);
        mSubscriptionManager = SubscriptionManager.from(this);

        if (mUm.hasUserRestriction(UserManager.DISALLOW_CONFIG_MOBILE_NETWORKS)) {
            mUnavailable = true;
            setContentView(R.layout.telephony_disallowed_preference_screen);
            return;
        }

        addPreferencesFromResource(R.xml.network_setting);

        mButton4glte = (SwitchPreference)findPreference(BUTTON_4G_LTE_KEY);
        mButton4glte.setOnPreferenceChangeListener(this);

        try {
            Context con = createPackageContext("com.android.systemui", 0);
            int id = con.getResources().getIdentifier("config_show4GForLTE",
                    "bool", "com.android.systemui");
            mShow4GForLTE = con.getResources().getBoolean(id);
        } catch (NameNotFoundException e) {
            loge("NameNotFoundException for show4GFotLTE");
            mShow4GForLTE = false;
        }

        //get UI object references
        PreferenceScreen prefSet = getPreferenceScreen();

        mButtonDataRoam = (RestrictedSwitchPreference) prefSet.findPreference(BUTTON_ROAMING_KEY);
        mButtonPreferredNetworkMode = (ListPreference) prefSet.findPreference(
                BUTTON_PREFERED_NETWORK_MODE);
        mButtonEnabledNetworks = (ListPreference) prefSet.findPreference(
                BUTTON_ENABLED_NETWORKS_KEY);
        mButtonDataRoam.setOnPreferenceChangeListener(this);

        mLteDataServicePref = prefSet.findPreference(BUTTON_CDMA_LTE_DATA_SERVICE_KEY);

        // Initialize mActiveSubInfo
        int max = mSubscriptionManager.getActiveSubscriptionInfoCountMax();
        mActiveSubInfos = new ArrayList<SubscriptionInfo>(max);

        initializeSubscriptions();

        IntentFilter intentFilter = new IntentFilter(
                TelephonyIntents.ACTION_RADIO_TECHNOLOGY_CHANGED);
        registerReceiver(mPhoneChangeReceiver, intentFilter);
        if (DBG) log("onCreate:-");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mPhoneChangeReceiver);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (DBG) log("onResume:+");

        if (mUnavailable) {
            if (DBG) log("onResume:- ignore mUnavailable == false");
            return;
        }

        // upon resumption from the sub-activity, make sure we re-enable the
        // preferences.
        getPreferenceScreen().setEnabled(true);

        // Set UI state in onResume because a user could go home, launch some
        // app to change this setting's backend, and re-launch this settings app
        // and the UI state would be inconsistent with actual state
        mButtonDataRoam.setChecked(mPhone.getDataRoamingEnabled());

        if (getPreferenceScreen().findPreference(BUTTON_PREFERED_NETWORK_MODE) != null
                || getPreferenceScreen().findPreference(BUTTON_ENABLED_NETWORKS_KEY) != null)  {
            updatePreferredNetworkUIFromDb();
        }

        if (ImsManager.isVolteEnabledByPlatform(this)
                && ImsManager.isVolteProvisionedOnDevice(this)) {
            TelephonyManager tm = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
            tm.listen(mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
        }

        // NOTE: Buttons will be enabled/disabled in mPhoneStateListener
        boolean enh4glteMode = ImsManager.isEnhanced4gLteModeSettingEnabledByUser(this)
                && ImsManager.isNonTtyOrTtyOnVolteEnabled(this);
        mButton4glte.setChecked(enh4glteMode);

        mSubscriptionManager.addOnSubscriptionsChangedListener(mOnSubscriptionsChangeListener);

        if (DBG) log("onResume:-");

    }

    private boolean hasActiveSubscriptions() {
        return mActiveSubInfos.size() > 0;
    }

    private void updateBody() {
        final Context context = getApplicationContext();
        PreferenceScreen prefSet = getPreferenceScreen();
        boolean isLteOnCdma = mPhone.getLteOnCdmaMode() == PhoneConstants.LTE_ON_CDMA_TRUE;
        final int phoneSubId = mPhone.getSubId();

        if (DBG) {
            log("updateBody: isLteOnCdma=" + isLteOnCdma + " phoneSubId=" + phoneSubId);
        }

        if (prefSet != null) {
            prefSet.removeAll();
            prefSet.addPreference(mButtonDataRoam);
            prefSet.addPreference(mButtonPreferredNetworkMode);
            prefSet.addPreference(mButtonEnabledNetworks);
            prefSet.addPreference(mButton4glte);
        }

        int settingsNetworkMode = android.provider.Settings.Global.getInt(
                mPhone.getContext().getContentResolver(),
                android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                preferredNetworkMode);

        PersistableBundle carrierConfig =
                PhoneGlobals.getInstance().getCarrierConfigForSubId(mPhone.getSubId());
        mIsGlobalCdma = isLteOnCdma
                && carrierConfig.getBoolean(CarrierConfigManager.KEY_SHOW_CDMA_CHOICES_BOOL);
        if (carrierConfig.getBoolean(CarrierConfigManager.KEY_HIDE_CARRIER_NETWORK_SETTINGS_BOOL)) {
            prefSet.removePreference(mButtonPreferredNetworkMode);
            prefSet.removePreference(mButtonEnabledNetworks);
            prefSet.removePreference(mLteDataServicePref);
        } else if (carrierConfig.getBoolean(CarrierConfigManager
                    .KEY_HIDE_PREFERRED_NETWORK_TYPE_BOOL)
                && !mPhone.getServiceState().getRoaming()) {
            prefSet.removePreference(mButtonPreferredNetworkMode);
            prefSet.removePreference(mButtonEnabledNetworks);

            final int phoneType = mPhone.getPhoneType();
            if (phoneType == PhoneConstants.PHONE_TYPE_CDMA) {
                mCdmaOptions = new CdmaOptions(this, prefSet, mPhone);
                // In World mode force a refresh of GSM Options.
                if (isWorldMode()) {
                    mGsmUmtsOptions = null;
                }
            } else if (phoneType == PhoneConstants.PHONE_TYPE_GSM) {
                mGsmUmtsOptions = new GsmUmtsOptions(this, prefSet, phoneSubId);
            } else {
                throw new IllegalStateException("Unexpected phone type: " + phoneType);
            }
            // Since pref is being hidden from user, set network mode to default
            // in case it is currently something else. That is possible if user
            // changed the setting while roaming and is now back to home network.
            settingsNetworkMode = preferredNetworkMode;
        } else if (carrierConfig.getBoolean(CarrierConfigManager.KEY_WORLD_PHONE_BOOL) == true) {
            prefSet.removePreference(mButtonEnabledNetworks);
            // set the listener for the mButtonPreferredNetworkMode list preference so we can issue
            // change Preferred Network Mode.
            mButtonPreferredNetworkMode.setOnPreferenceChangeListener(this);

            mCdmaOptions = new CdmaOptions(this, prefSet, mPhone);
            mGsmUmtsOptions = new GsmUmtsOptions(this, prefSet, phoneSubId);
        } else {
            prefSet.removePreference(mButtonPreferredNetworkMode);
            final int phoneType = mPhone.getPhoneType();
            if (phoneType == PhoneConstants.PHONE_TYPE_CDMA) {
                int lteForced = android.provider.Settings.Global.getInt(
                        mPhone.getContext().getContentResolver(),
                        android.provider.Settings.Global.LTE_SERVICE_FORCED + mPhone.getSubId(),
                        0);

                if (isLteOnCdma) {
                    if (lteForced == 0) {
                        mButtonEnabledNetworks.setEntries(
                                R.array.enabled_networks_cdma_choices);
                        mButtonEnabledNetworks.setEntryValues(
                                R.array.enabled_networks_cdma_values);
                    } else {
                        switch (settingsNetworkMode) {
                            case Phone.NT_MODE_CDMA:
                            case Phone.NT_MODE_CDMA_NO_EVDO:
                            case Phone.NT_MODE_EVDO_NO_CDMA:
                                mButtonEnabledNetworks.setEntries(
                                        R.array.enabled_networks_cdma_no_lte_choices);
                                mButtonEnabledNetworks.setEntryValues(
                                        R.array.enabled_networks_cdma_no_lte_values);
                                break;
                            case Phone.NT_MODE_GLOBAL:
                            case Phone.NT_MODE_LTE_CDMA_AND_EVDO:
                            case Phone.NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA:
                            case Phone.NT_MODE_LTE_ONLY:
                                mButtonEnabledNetworks.setEntries(
                                        R.array.enabled_networks_cdma_only_lte_choices);
                                mButtonEnabledNetworks.setEntryValues(
                                        R.array.enabled_networks_cdma_only_lte_values);
                                break;
                            default:
                                mButtonEnabledNetworks.setEntries(
                                        R.array.enabled_networks_cdma_choices);
                                mButtonEnabledNetworks.setEntryValues(
                                        R.array.enabled_networks_cdma_values);
                                break;
                        }
                    }
                }
                mCdmaOptions = new CdmaOptions(this, prefSet, mPhone);

                // In World mode force a refresh of GSM Options.
                if (isWorldMode()) {
                    mGsmUmtsOptions = null;
                }
            } else if (phoneType == PhoneConstants.PHONE_TYPE_GSM) {
                if (isSupportTdscdma()) {
                    mButtonEnabledNetworks.setEntries(
                            R.array.enabled_networks_tdscdma_choices);
                    mButtonEnabledNetworks.setEntryValues(
                            R.array.enabled_networks_tdscdma_values);
                } else if (!carrierConfig.getBoolean(CarrierConfigManager.KEY_PREFER_2G_BOOL)
                        && !getResources().getBoolean(R.bool.config_enabled_lte)) {
                    mButtonEnabledNetworks.setEntries(
                            R.array.enabled_networks_except_gsm_lte_choices);
                    mButtonEnabledNetworks.setEntryValues(
                            R.array.enabled_networks_except_gsm_lte_values);
                } else if (!carrierConfig.getBoolean(CarrierConfigManager.KEY_PREFER_2G_BOOL)) {
                    int select = (mShow4GForLTE == true) ?
                            R.array.enabled_networks_except_gsm_4g_choices
                            : R.array.enabled_networks_except_gsm_choices;
                    mButtonEnabledNetworks.setEntries(select);
                    mButtonEnabledNetworks.setEntryValues(
                            R.array.enabled_networks_except_gsm_values);
                } else if (!getResources().getBoolean(R.bool.config_enabled_lte)) {
                    mButtonEnabledNetworks.setEntries(
                            R.array.enabled_networks_except_lte_choices);
                    mButtonEnabledNetworks.setEntryValues(
                            R.array.enabled_networks_except_lte_values);
                } else if (mIsGlobalCdma) {
                    mButtonEnabledNetworks.setEntries(
                            R.array.enabled_networks_cdma_choices);
                    mButtonEnabledNetworks.setEntryValues(
                            R.array.enabled_networks_cdma_values);
                } else {
                    int select = (mShow4GForLTE == true) ? R.array.enabled_networks_4g_choices
                            : R.array.enabled_networks_choices;
                    mButtonEnabledNetworks.setEntries(select);
                    mButtonEnabledNetworks.setEntryValues(
                            R.array.enabled_networks_values);
                }
                mGsmUmtsOptions = new GsmUmtsOptions(this, prefSet, phoneSubId);
            } else {
                throw new IllegalStateException("Unexpected phone type: " + phoneType);
            }
            if (isWorldMode()) {
                mButtonEnabledNetworks.setEntries(
                        R.array.preferred_network_mode_choices_world_mode);
                mButtonEnabledNetworks.setEntryValues(
                        R.array.preferred_network_mode_values_world_mode);
            }
            mButtonEnabledNetworks.setOnPreferenceChangeListener(this);
            if (DBG) log("settingsNetworkMode: " + settingsNetworkMode);
        }

        final boolean missingDataServiceUrl = TextUtils.isEmpty(
                android.provider.Settings.Global.getString(getContentResolver(),
                        android.provider.Settings.Global.SETUP_PREPAID_DATA_SERVICE_URL));
        if (!isLteOnCdma || missingDataServiceUrl) {
            prefSet.removePreference(mLteDataServicePref);
        } else {
            android.util.Log.d(LOG_TAG, "keep ltePref");
        }

        // Enable enhanced 4G LTE mode settings depending on whether exists on platform
        if (!(ImsManager.isVolteEnabledByPlatform(this)
                && ImsManager.isVolteProvisionedOnDevice(this))) {
            Preference pref = prefSet.findPreference(BUTTON_4G_LTE_KEY);
            if (pref != null) {
                prefSet.removePreference(pref);
            }
        }

        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            // android.R.id.home will be triggered in onOptionsItemSelected()
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        // Enable link to CMAS app settings depending on the value in config.xml.
        final boolean isCellBroadcastAppLinkEnabled = this.getResources().getBoolean(
                com.android.internal.R.bool.config_cellBroadcastAppLinks);
        if (!mUm.isAdminUser() || !isCellBroadcastAppLinkEnabled
                || mUm.hasUserRestriction(UserManager.DISALLOW_CONFIG_CELL_BROADCASTS)) {
            PreferenceScreen root = getPreferenceScreen();
            Preference ps = findPreference(BUTTON_CELL_BROADCAST_SETTINGS);
            if (ps != null) {
                root.removePreference(ps);
            }
        }

        // Get the networkMode from Settings.System and displays it
        mButtonDataRoam.setChecked(mPhone.getDataRoamingEnabled());
        mButtonEnabledNetworks.setValue(Integer.toString(settingsNetworkMode));
        mButtonPreferredNetworkMode.setValue(Integer.toString(settingsNetworkMode));
        UpdatePreferredNetworkModeSummary(settingsNetworkMode);
        UpdateEnabledNetworksValueAndSummary(settingsNetworkMode);
        // Display preferred network type based on what modem returns b/18676277
        mPhone.setPreferredNetworkType(settingsNetworkMode, mHandler
                .obtainMessage(MyHandler.MESSAGE_SET_PREFERRED_NETWORK_TYPE));

        /**
         * Enable/disable depending upon if there are any active subscriptions.
         *
         * I've decided to put this enable/disable code at the bottom as the
         * code above works even when there are no active subscriptions, thus
         * putting it afterwards is a smaller change. This can be refined later,
         * but you do need to remember that this all needs to work when subscriptions
         * change dynamically such as when hot swapping sims.
         */
        boolean hasActiveSubscriptions = hasActiveSubscriptions();
        TelephonyManager tm = (TelephonyManager) getSystemService(
                Context.TELEPHONY_SERVICE);
        boolean canChange4glte = (tm.getCallState() == TelephonyManager.CALL_STATE_IDLE) &&
                ImsManager.isNonTtyOrTtyOnVolteEnabled(getApplicationContext()) &&
                carrierConfig.getBoolean(CarrierConfigManager.KEY_EDITABLE_ENHANCED_4G_LTE_BOOL);
        mButtonDataRoam.setDisabledByAdmin(false);
        mButtonDataRoam.setEnabled(hasActiveSubscriptions);
        if (mButtonDataRoam.isEnabled()) {
            if (RestrictedLockUtils.hasBaseUserRestriction(context,
                    UserManager.DISALLOW_DATA_ROAMING, UserHandle.myUserId())) {
                mButtonDataRoam.setEnabled(false);
            } else {
                mButtonDataRoam.checkRestrictionAndSetDisabled(UserManager.DISALLOW_DATA_ROAMING);
            }
        }
        mButtonPreferredNetworkMode.setEnabled(hasActiveSubscriptions);
        mButtonEnabledNetworks.setEnabled(hasActiveSubscriptions);
        mButton4glte.setEnabled(hasActiveSubscriptions && canChange4glte);
        mLteDataServicePref.setEnabled(hasActiveSubscriptions);
        Preference ps;
        PreferenceScreen root = getPreferenceScreen();
        ps = findPreference(BUTTON_CELL_BROADCAST_SETTINGS);
        if (ps != null) {
            ps.setEnabled(hasActiveSubscriptions);
        }
        ps = findPreference(BUTTON_APN_EXPAND_KEY);
        if (ps != null) {
            ps.setEnabled(hasActiveSubscriptions);
        }
        ps = findPreference(BUTTON_OPERATOR_SELECTION_EXPAND_KEY);
        if (ps != null) {
            ps.setEnabled(hasActiveSubscriptions);
        }
        ps = findPreference(BUTTON_CARRIER_SETTINGS_KEY);
        if (ps != null) {
            ps.setEnabled(hasActiveSubscriptions);
        }
        ps = findPreference(BUTTON_CDMA_SYSTEM_SELECT_KEY);
        if (ps != null) {
            ps.setEnabled(hasActiveSubscriptions);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (DBG) log("onPause:+");

        if (ImsManager.isVolteEnabledByPlatform(this)
                && ImsManager.isVolteProvisionedOnDevice(this)) {
            TelephonyManager tm = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
            tm.listen(mPhoneStateListener, PhoneStateListener.LISTEN_NONE);
        }

        mSubscriptionManager
            .removeOnSubscriptionsChangedListener(mOnSubscriptionsChangeListener);
        if (DBG) log("onPause:-");
    }

    /**
     * Implemented to support onPreferenceChangeListener to look for preference
     * changes specifically on CLIR.
     *
     * @param preference is the preference to be changed, should be mButtonCLIR.
     * @param objValue should be the value of the selection, NOT its localized
     * display value.
     */
    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final int phoneSubId = mPhone.getSubId();
        if (preference == mButtonPreferredNetworkMode) {
            //NOTE onPreferenceChange seems to be called even if there is no change
            //Check if the button value is changed from the System.Setting
            mButtonPreferredNetworkMode.setValue((String) objValue);
            int buttonNetworkMode;
            buttonNetworkMode = Integer.parseInt((String) objValue);
            int settingsNetworkMode = android.provider.Settings.Global.getInt(
                    mPhone.getContext().getContentResolver(),
                    android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                    preferredNetworkMode);
            if (buttonNetworkMode != settingsNetworkMode) {
                int modemNetworkMode;
                // if new mode is invalid ignore it
                switch (buttonNetworkMode) {
                    case Phone.NT_MODE_WCDMA_PREF:
                    case Phone.NT_MODE_GSM_ONLY:
                    case Phone.NT_MODE_WCDMA_ONLY:
                    case Phone.NT_MODE_GSM_UMTS:
                    case Phone.NT_MODE_CDMA:
                    case Phone.NT_MODE_CDMA_NO_EVDO:
                    case Phone.NT_MODE_EVDO_NO_CDMA:
                    case Phone.NT_MODE_GLOBAL:
                    case Phone.NT_MODE_LTE_CDMA_AND_EVDO:
                    case Phone.NT_MODE_LTE_GSM_WCDMA:
                    case Phone.NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA:
                    case Phone.NT_MODE_LTE_ONLY:
                    case Phone.NT_MODE_LTE_WCDMA:
                    case Phone.NT_MODE_TDSCDMA_ONLY:
                    case Phone.NT_MODE_TDSCDMA_WCDMA:
                    case Phone.NT_MODE_LTE_TDSCDMA:
                    case Phone.NT_MODE_TDSCDMA_GSM:
                    case Phone.NT_MODE_LTE_TDSCDMA_GSM:
                    case Phone.NT_MODE_TDSCDMA_GSM_WCDMA:
                    case Phone.NT_MODE_LTE_TDSCDMA_WCDMA:
                    case Phone.NT_MODE_LTE_TDSCDMA_GSM_WCDMA:
                    case Phone.NT_MODE_TDSCDMA_CDMA_EVDO_GSM_WCDMA:
                    case Phone.NT_MODE_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA:
                        // This is one of the modes we recognize
                        modemNetworkMode = buttonNetworkMode;
                        break;
                    default:
                        loge("Invalid Network Mode (" + buttonNetworkMode + ") chosen. Ignore.");
                        return true;
                }

                android.provider.Settings.Global.putInt(mPhone.getContext().getContentResolver(),
                        android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                        buttonNetworkMode );
                //Set the modem network mode
                mPhone.setPreferredNetworkType(modemNetworkMode, mHandler
                        .obtainMessage(MyHandler.MESSAGE_SET_PREFERRED_NETWORK_TYPE));
            }
        } else if (preference == mButtonEnabledNetworks) {
            mButtonEnabledNetworks.setValue((String) objValue);
            int buttonNetworkMode;
            buttonNetworkMode = Integer.parseInt((String) objValue);
            if (DBG) log("buttonNetworkMode: " + buttonNetworkMode);
            int settingsNetworkMode = android.provider.Settings.Global.getInt(
                    mPhone.getContext().getContentResolver(),
                    android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                    preferredNetworkMode);
            if (buttonNetworkMode != settingsNetworkMode) {
                int modemNetworkMode;
                // if new mode is invalid ignore it
                switch (buttonNetworkMode) {
                    case Phone.NT_MODE_WCDMA_PREF:
                    case Phone.NT_MODE_GSM_ONLY:
                    case Phone.NT_MODE_LTE_GSM_WCDMA:
                    case Phone.NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA:
                    case Phone.NT_MODE_CDMA:
                    case Phone.NT_MODE_CDMA_NO_EVDO:
                    case Phone.NT_MODE_LTE_CDMA_AND_EVDO:
                    case Phone.NT_MODE_TDSCDMA_ONLY:
                    case Phone.NT_MODE_TDSCDMA_WCDMA:
                    case Phone.NT_MODE_LTE_TDSCDMA:
                    case Phone.NT_MODE_TDSCDMA_GSM:
                    case Phone.NT_MODE_LTE_TDSCDMA_GSM:
                    case Phone.NT_MODE_TDSCDMA_GSM_WCDMA:
                    case Phone.NT_MODE_LTE_TDSCDMA_WCDMA:
                    case Phone.NT_MODE_LTE_TDSCDMA_GSM_WCDMA:
                    case Phone.NT_MODE_TDSCDMA_CDMA_EVDO_GSM_WCDMA:
                    case Phone.NT_MODE_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA:
                        // This is one of the modes we recognize
                        modemNetworkMode = buttonNetworkMode;
                        break;
                    default:
                        loge("Invalid Network Mode (" + buttonNetworkMode + ") chosen. Ignore.");
                        return true;
                }

                UpdateEnabledNetworksValueAndSummary(buttonNetworkMode);

                android.provider.Settings.Global.putInt(mPhone.getContext().getContentResolver(),
                        android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                        buttonNetworkMode );
                //Set the modem network mode
                mPhone.setPreferredNetworkType(modemNetworkMode, mHandler
                        .obtainMessage(MyHandler.MESSAGE_SET_PREFERRED_NETWORK_TYPE));
            }
        } else if (preference == mButton4glte) {
            SwitchPreference enhanced4gModePref = (SwitchPreference) preference;
            boolean enhanced4gMode = !enhanced4gModePref.isChecked();
            enhanced4gModePref.setChecked(enhanced4gMode);
            ImsManager.setEnhanced4gLteModeSetting(this, enhanced4gModePref.isChecked());
        } else if (preference == mButtonDataRoam) {
            if (DBG) log("onPreferenceTreeClick: preference == mButtonDataRoam.");

            //normally called on the toggle click
            if (!mButtonDataRoam.isChecked()) {
                // First confirm with a warning dialog about charges
                mOkClicked = false;
                new AlertDialog.Builder(this).setMessage(
                        getResources().getString(R.string.roaming_warning))
                        .setTitle(R.string.roaming_alert_title)
                        .setIconAttribute(android.R.attr.alertDialogIcon)
                        .setPositiveButton(android.R.string.yes, this)
                        .setNegativeButton(android.R.string.no, this)
                        .show()
                        .setOnDismissListener(this);
            } else {
                mPhone.setDataRoamingEnabled(false);
            }
            return true;
        }

        updateBody();
        // always let the preference setting proceed.
        return true;
    }

    private class MyHandler extends Handler {

        static final int MESSAGE_SET_PREFERRED_NETWORK_TYPE = 0;

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_SET_PREFERRED_NETWORK_TYPE:
                    handleSetPreferredNetworkTypeResponse(msg);
                    break;
            }
        }

        private void handleSetPreferredNetworkTypeResponse(Message msg) {
            if (isDestroyed()) { // Access preferences of activity only if it is not destroyed
                return;
            }

            AsyncResult ar = (AsyncResult) msg.obj;
            final int phoneSubId = mPhone.getSubId();

            if (ar.exception == null) {
                int networkMode;
                if (getPreferenceScreen().findPreference(BUTTON_PREFERED_NETWORK_MODE) != null)  {
                    networkMode =  Integer.parseInt(mButtonPreferredNetworkMode.getValue());
                    android.provider.Settings.Global.putInt(mPhone.getContext().getContentResolver(),
                            android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                            networkMode );
                }
                if (getPreferenceScreen().findPreference(BUTTON_ENABLED_NETWORKS_KEY) != null)  {
                    networkMode = Integer.parseInt(mButtonEnabledNetworks.getValue());
                    android.provider.Settings.Global.putInt(mPhone.getContext().getContentResolver(),
                            android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                            networkMode );
                }
            } else {
                if (DBG) {
                    log("handleSetPreferredNetworkTypeResponse: exception in setting network mode.");
                }
                updatePreferredNetworkUIFromDb();
            }
        }
    }

    private void updatePreferredNetworkUIFromDb() {
        final int phoneSubId = mPhone.getSubId();

        int settingsNetworkMode = android.provider.Settings.Global.getInt(
                mPhone.getContext().getContentResolver(),
                android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                preferredNetworkMode);

        if (DBG) {
            log("updatePreferredNetworkUIFromDb: settingsNetworkMode = " +
                    settingsNetworkMode);
        }

        UpdatePreferredNetworkModeSummary(settingsNetworkMode);
        UpdateEnabledNetworksValueAndSummary(settingsNetworkMode);
        // changes the mButtonPreferredNetworkMode accordingly to settingsNetworkMode
        mButtonPreferredNetworkMode.setValue(Integer.toString(settingsNetworkMode));
    }

    private void UpdatePreferredNetworkModeSummary(int NetworkMode) {
        switch(NetworkMode) {
            case Phone.NT_MODE_TDSCDMA_GSM_WCDMA:
            case Phone.NT_MODE_TDSCDMA_GSM:
            case Phone.NT_MODE_WCDMA_PREF:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_wcdma_perf_summary);
                break;
            case Phone.NT_MODE_GSM_ONLY:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_gsm_only_summary);
                break;
            case Phone.NT_MODE_TDSCDMA_WCDMA:
            case Phone.NT_MODE_WCDMA_ONLY:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_wcdma_only_summary);
                break;
            case Phone.NT_MODE_GSM_UMTS:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_gsm_wcdma_summary);
                break;
            case Phone.NT_MODE_CDMA:
                switch (mPhone.getLteOnCdmaMode()) {
                    case PhoneConstants.LTE_ON_CDMA_TRUE:
                        mButtonPreferredNetworkMode.setSummary(
                            R.string.preferred_network_mode_cdma_summary);
                    break;
                    case PhoneConstants.LTE_ON_CDMA_FALSE:
                    default:
                        mButtonPreferredNetworkMode.setSummary(
                            R.string.preferred_network_mode_cdma_evdo_summary);
                        break;
                }
                break;
            case Phone.NT_MODE_CDMA_NO_EVDO:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_cdma_only_summary);
                break;
            case Phone.NT_MODE_EVDO_NO_CDMA:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_evdo_only_summary);
                break;
            case Phone.NT_MODE_LTE_TDSCDMA:
            case Phone.NT_MODE_LTE_ONLY:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_lte_summary);
                break;
            case Phone.NT_MODE_LTE_TDSCDMA_GSM:
            case Phone.NT_MODE_LTE_TDSCDMA_GSM_WCDMA:
            case Phone.NT_MODE_LTE_GSM_WCDMA:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_lte_gsm_wcdma_summary);
                break;
            case Phone.NT_MODE_LTE_CDMA_AND_EVDO:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_lte_cdma_evdo_summary);
                break;
            case Phone.NT_MODE_TDSCDMA_ONLY:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_tdscdma_summary);
                break;
            case Phone.NT_MODE_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA:
            case Phone.NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA:
                if (mPhone.getPhoneType() == PhoneConstants.PHONE_TYPE_CDMA ||
                        mIsGlobalCdma ||
                        isWorldMode()) {
                    mButtonPreferredNetworkMode.setSummary(
                            R.string.preferred_network_mode_global_summary);
                } else {
                    mButtonPreferredNetworkMode.setSummary(
                            R.string.preferred_network_mode_lte_summary);
                }
                break;
            case Phone.NT_MODE_TDSCDMA_CDMA_EVDO_GSM_WCDMA:
            case Phone.NT_MODE_GLOBAL:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_cdma_evdo_gsm_wcdma_summary);
                break;
            case Phone.NT_MODE_LTE_TDSCDMA_WCDMA:
            case Phone.NT_MODE_LTE_WCDMA:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_lte_wcdma_summary);
                break;
            default:
                mButtonPreferredNetworkMode.setSummary(
                        R.string.preferred_network_mode_global_summary);
        }
    }

    private void UpdateEnabledNetworksValueAndSummary(int NetworkMode) {
        switch (NetworkMode) {
            case Phone.NT_MODE_TDSCDMA_WCDMA:
            case Phone.NT_MODE_TDSCDMA_GSM_WCDMA:
            case Phone.NT_MODE_TDSCDMA_GSM:
                mButtonEnabledNetworks.setValue(
                        Integer.toString(Phone.NT_MODE_TDSCDMA_GSM_WCDMA));
                mButtonEnabledNetworks.setSummary(R.string.network_3G);
                break;
            case Phone.NT_MODE_WCDMA_ONLY:
            case Phone.NT_MODE_GSM_UMTS:
            case Phone.NT_MODE_WCDMA_PREF:
                if (!mIsGlobalCdma) {
                    mButtonEnabledNetworks.setValue(
                            Integer.toString(Phone.NT_MODE_WCDMA_PREF));
                    mButtonEnabledNetworks.setSummary(R.string.network_3G);
                } else {
                    mButtonEnabledNetworks.setValue(
                            Integer.toString(Phone.NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA));
                    mButtonEnabledNetworks.setSummary(R.string.network_global);
                }
                break;
            case Phone.NT_MODE_GSM_ONLY:
                if (!mIsGlobalCdma) {
                    mButtonEnabledNetworks.setValue(
                            Integer.toString(Phone.NT_MODE_GSM_ONLY));
                    mButtonEnabledNetworks.setSummary(R.string.network_2G);
                } else {
                    mButtonEnabledNetworks.setValue(
                            Integer.toString(Phone.NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA));
                    mButtonEnabledNetworks.setSummary(R.string.network_global);
                }
                break;
            case Phone.NT_MODE_LTE_GSM_WCDMA:
                if (isWorldMode()) {
                    mButtonEnabledNetworks.setSummary(
                            R.string.preferred_network_mode_lte_gsm_umts_summary);
                    controlCdmaOptions(false);
                    controlGsmOptions(true);
                    break;
                }
            case Phone.NT_MODE_LTE_ONLY:
            case Phone.NT_MODE_LTE_WCDMA:
                if (!mIsGlobalCdma) {
                    mButtonEnabledNetworks.setValue(
                            Integer.toString(Phone.NT_MODE_LTE_GSM_WCDMA));
                    mButtonEnabledNetworks.setSummary((mShow4GForLTE == true)
                            ? R.string.network_4G : R.string.network_lte);
                } else {
                    mButtonEnabledNetworks.setValue(
                            Integer.toString(Phone.NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA));
                    mButtonEnabledNetworks.setSummary(R.string.network_global);
                }
                break;
            case Phone.NT_MODE_LTE_CDMA_AND_EVDO:
                if (isWorldMode()) {
                    mButtonEnabledNetworks.setSummary(
                            R.string.preferred_network_mode_lte_cdma_summary);
                    controlCdmaOptions(true);
                    controlGsmOptions(false);
                } else {
                    mButtonEnabledNetworks.setValue(
                            Integer.toString(Phone.NT_MODE_LTE_CDMA_AND_EVDO));
                    mButtonEnabledNetworks.setSummary(R.string.network_lte);
                }
                break;
            case Phone.NT_MODE_TDSCDMA_CDMA_EVDO_GSM_WCDMA:
                mButtonEnabledNetworks.setValue(
                        Integer.toString(Phone.NT_MODE_TDSCDMA_CDMA_EVDO_GSM_WCDMA));
                mButtonEnabledNetworks.setSummary(R.string.network_3G);
                break;
            case Phone.NT_MODE_CDMA:
            case Phone.NT_MODE_EVDO_NO_CDMA:
            case Phone.NT_MODE_GLOBAL:
                mButtonEnabledNetworks.setValue(
                        Integer.toString(Phone.NT_MODE_CDMA));
                mButtonEnabledNetworks.setSummary(R.string.network_3G);
                break;
            case Phone.NT_MODE_CDMA_NO_EVDO:
                mButtonEnabledNetworks.setValue(
                        Integer.toString(Phone.NT_MODE_CDMA_NO_EVDO));
                mButtonEnabledNetworks.setSummary(R.string.network_1x);
                break;
            case Phone.NT_MODE_TDSCDMA_ONLY:
                mButtonEnabledNetworks.setValue(
                        Integer.toString(Phone.NT_MODE_TDSCDMA_ONLY));
                mButtonEnabledNetworks.setSummary(R.string.network_3G);
                break;
            case Phone.NT_MODE_LTE_TDSCDMA_GSM:
            case Phone.NT_MODE_LTE_TDSCDMA_GSM_WCDMA:
            case Phone.NT_MODE_LTE_TDSCDMA:
            case Phone.NT_MODE_LTE_TDSCDMA_WCDMA:
            case Phone.NT_MODE_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA:
            case Phone.NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA:
                if (isSupportTdscdma()) {
                    mButtonEnabledNetworks.setValue(
                            Integer.toString(Phone.NT_MODE_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA));
                    mButtonEnabledNetworks.setSummary(R.string.network_lte);
                } else {
                    if (isWorldMode()) {
                        controlCdmaOptions(true);
                        controlGsmOptions(false);
                    }
                    mButtonEnabledNetworks.setValue(
                            Integer.toString(Phone.NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA));
                    if (mPhone.getPhoneType() == PhoneConstants.PHONE_TYPE_CDMA ||
                            mIsGlobalCdma ||
                            isWorldMode()) {
                        mButtonEnabledNetworks.setSummary(R.string.network_global);
                    } else {
                        mButtonEnabledNetworks.setSummary((mShow4GForLTE == true)
                                ? R.string.network_4G : R.string.network_lte);
                    }
                }
                break;
            default:
                String errMsg = "Invalid Network Mode (" + NetworkMode + "). Ignore.";
                loge(errMsg);
                mButtonEnabledNetworks.setSummary(errMsg);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch(requestCode) {
        case REQUEST_CODE_EXIT_ECM:
            Boolean isChoiceYes =
                data.getBooleanExtra(EmergencyCallbackModeExitDialog.EXTRA_EXIT_ECM_RESULT, false);
            if (isChoiceYes) {
                // If the phone exits from ECM mode, show the CDMA Options
                mCdmaOptions.showDialog(mClickedPreference);
            } else {
                // do nothing
            }
            break;

        default:
            break;
        }
    }

    private static void log(String msg) {
        Log.d(LOG_TAG, msg);
    }

    private static void loge(String msg) {
        Log.e(LOG_TAG, msg);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        final int itemId = item.getItemId();
        if (itemId == android.R.id.home) {  // See ActionBar#setDisplayHomeAsUpEnabled()
            // Commenting out "logical up" capability. This is a workaround for issue 5278083.
            //
            // Settings app may not launch this activity via UP_ACTIVITY_CLASS but the other
            // Activity that looks exactly same as UP_ACTIVITY_CLASS ("SubSettings" Activity).
            // At that moment, this Activity launches UP_ACTIVITY_CLASS on top of the Activity.
            // which confuses users.
            // TODO: introduce better mechanism for "up" capability here.
            /*Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.setClassName(UP_ACTIVITY_PACKAGE, UP_ACTIVITY_CLASS);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            startActivity(intent);*/
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private boolean isWorldMode() {
        boolean worldModeOn = false;
        final TelephonyManager tm = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        final String configString = getResources().getString(R.string.config_world_mode);

        if (!TextUtils.isEmpty(configString)) {
            String[] configArray = configString.split(";");
            // Check if we have World mode configuration set to True only or config is set to True
            // and SIM GID value is also set and matches to the current SIM GID.
            if (configArray != null &&
                   ((configArray.length == 1 && configArray[0].equalsIgnoreCase("true")) ||
                       (configArray.length == 2 && !TextUtils.isEmpty(configArray[1]) &&
                           tm != null && configArray[1].equalsIgnoreCase(tm.getGroupIdLevel1())))) {
                               worldModeOn = true;
            }
        }

        if (DBG) {
            log("isWorldMode=" + worldModeOn);
        }

        return worldModeOn;
    }

    private void controlGsmOptions(boolean enable) {
        PreferenceScreen prefSet = getPreferenceScreen();
        if (prefSet == null) {
            return;
        }

        if (mGsmUmtsOptions == null) {
            mGsmUmtsOptions = new GsmUmtsOptions(this, prefSet, mPhone.getSubId());
        }
        PreferenceScreen apnExpand =
                (PreferenceScreen) prefSet.findPreference(BUTTON_APN_EXPAND_KEY);
        PreferenceScreen operatorSelectionExpand =
                (PreferenceScreen) prefSet.findPreference(BUTTON_OPERATOR_SELECTION_EXPAND_KEY);
        PreferenceScreen carrierSettings =
                (PreferenceScreen) prefSet.findPreference(BUTTON_CARRIER_SETTINGS_KEY);
        if (apnExpand != null) {
            apnExpand.setEnabled(isWorldMode() || enable);
        }
        if (operatorSelectionExpand != null) {
            if (enable) {
                operatorSelectionExpand.setEnabled(true);
            } else {
                prefSet.removePreference(operatorSelectionExpand);
            }
        }
        if (carrierSettings != null) {
            prefSet.removePreference(carrierSettings);
        }
    }

    private void controlCdmaOptions(boolean enable) {
        PreferenceScreen prefSet = getPreferenceScreen();
        if (prefSet == null) {
            return;
        }
        if (enable && mCdmaOptions == null) {
            mCdmaOptions = new CdmaOptions(this, prefSet, mPhone);
        }
        CdmaSystemSelectListPreference systemSelect =
                (CdmaSystemSelectListPreference)prefSet.findPreference
                        (BUTTON_CDMA_SYSTEM_SELECT_KEY);
        if (systemSelect != null) {
            systemSelect.setEnabled(enable);
        }
    }

    private boolean isSupportTdscdma() {
        if (getResources().getBoolean(R.bool.config_support_tdscdma)) {
            return true;
        }

        String operatorNumeric = mPhone.getServiceState().getOperatorNumeric();
        String[] numericArray = getResources().getStringArray(
                R.array.config_support_tdscdma_roaming_on_networks);
        if (numericArray.length == 0 || operatorNumeric == null) {
            return false;
        }
        for (String numeric : numericArray) {
            if (operatorNumeric.equals(numeric)) {
                return true;
            }
        }
        return false;
    }
}
