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

package com.android.cts.verifier.managedprovisioning;

import android.content.Context;
import android.content.Intent;
import android.os.UserManager;
import android.provider.Settings;
import android.util.ArrayMap;

import java.util.ArrayList;

import com.android.cts.verifier.R;

public class UserRestrictions {
    private static final String[] RESTRICTION_IDS = new String[] {
        UserManager.DISALLOW_ADD_USER,
        UserManager.DISALLOW_ADJUST_VOLUME,
        UserManager.DISALLOW_APPS_CONTROL,
        UserManager.DISALLOW_CONFIG_CELL_BROADCASTS,
        UserManager.DISALLOW_CONFIG_CREDENTIALS,
        UserManager.DISALLOW_CONFIG_MOBILE_NETWORKS,
        UserManager.DISALLOW_CONFIG_TETHERING,
        UserManager.DISALLOW_CONFIG_WIFI,
        UserManager.DISALLOW_DEBUGGING_FEATURES,
        UserManager.DISALLOW_FACTORY_RESET,
        UserManager.DISALLOW_FUN,
        UserManager.DISALLOW_INSTALL_UNKNOWN_SOURCES,
        UserManager.DISALLOW_MODIFY_ACCOUNTS,
        UserManager.DISALLOW_NETWORK_RESET,
        UserManager.DISALLOW_OUTGOING_BEAM,
        UserManager.DISALLOW_REMOVE_USER,
        UserManager.DISALLOW_SHARE_LOCATION,
        UserManager.DISALLOW_UNINSTALL_APPS
    };

    private static final ArrayMap<String, UserRestrictionItem> USER_RESTRICTION_ITEMS;
    static {
        final int[] restrictionLabels = new int[] {
            R.string.disallow_add_user,
            R.string.disallow_adjust_volume,
            R.string.disallow_apps_control,
            R.string.disallow_config_cell_broadcasts,
            R.string.disallow_config_credentials,
            R.string.disallow_config_mobile_networks,
            R.string.disallow_config_tethering,
            R.string.disallow_config_wifi,
            R.string.disallow_debugging_features,
            R.string.disallow_factory_reset,
            R.string.disallow_fun,
            R.string.disallow_install_unknown_sources,
            R.string.disallow_modify_accounts,
            R.string.disallow_network_reset,
            R.string.disallow_outgoing_beam,
            R.string.disallow_remove_user,
            R.string.disallow_share_location,
            R.string.disallow_uninstall_apps
        };

        final int[] restrictionActions = new int[] {
            R.string.disallow_add_user_action,
            R.string.disallow_adjust_volume_action,
            R.string.disallow_apps_control_action,
            R.string.disallow_config_cell_broadcasts_action,
            R.string.disallow_config_credentials_action,
            R.string.disallow_config_mobile_networks_action,
            R.string.disallow_config_tethering_action,
            R.string.disallow_config_wifi_action,
            R.string.disallow_debugging_features_action,
            R.string.disallow_factory_reset_action,
            R.string.disallow_fun_action,
            R.string.disallow_install_unknown_sources_action,
            R.string.disallow_modify_accounts_action,
            R.string.disallow_network_reset_action,
            R.string.disallow_outgoing_beam_action,
            R.string.disallow_remove_user_action,
            R.string.disallow_share_location_action,
            R.string.disallow_uninstall_apps_action
        };

        final String[] settingsIntentActions = new String[] {
            Settings.ACTION_SETTINGS,
            Settings.ACTION_SOUND_SETTINGS,
            Settings.ACTION_APPLICATION_SETTINGS,
            Settings.ACTION_SOUND_SETTINGS,
            Settings.ACTION_SECURITY_SETTINGS,
            Settings.ACTION_WIRELESS_SETTINGS,
            Settings.ACTION_WIRELESS_SETTINGS,
            Settings.ACTION_WIFI_SETTINGS,
            Settings.ACTION_DEVICE_INFO_SETTINGS,
            Settings.ACTION_PRIVACY_SETTINGS,
            Settings.ACTION_DEVICE_INFO_SETTINGS,
            Settings.ACTION_SECURITY_SETTINGS,
            Settings.ACTION_SYNC_SETTINGS,
            Settings.ACTION_PRIVACY_SETTINGS,
            Settings.ACTION_WIRELESS_SETTINGS,
            Settings.ACTION_SETTINGS,
            Settings.ACTION_LOCATION_SOURCE_SETTINGS,
            Settings.ACTION_APPLICATION_SETTINGS,
        };

        if (RESTRICTION_IDS.length != restrictionLabels.length
                || RESTRICTION_IDS.length != restrictionActions.length
                || RESTRICTION_IDS.length != settingsIntentActions.length) {
            throw new AssertionError("Number of items in restrictionIds, restrictionLabels, "
                    + "restrictionActions, and settingsIntentActions do not match");
        }
        USER_RESTRICTION_ITEMS = new ArrayMap<>(RESTRICTION_IDS.length);
        for (int i = 0; i < RESTRICTION_IDS.length; ++i) {
            USER_RESTRICTION_ITEMS.put(RESTRICTION_IDS[i], new UserRestrictionItem(
                    restrictionLabels[i],
                    restrictionActions[i],
                    settingsIntentActions[i]));
        }
    }

    private static final ArrayList<String> ALSO_VALID_FOR_PO =
            new ArrayList<String>();
    static {
        ALSO_VALID_FOR_PO.add(UserManager.DISALLOW_APPS_CONTROL);
        ALSO_VALID_FOR_PO.add(UserManager.DISALLOW_UNINSTALL_APPS);
        ALSO_VALID_FOR_PO.add(UserManager.DISALLOW_MODIFY_ACCOUNTS);
        ALSO_VALID_FOR_PO.add(UserManager.DISALLOW_SHARE_LOCATION);
    }

    public static String getRestrictionLabel(Context context, String restriction) {
        final UserRestrictionItem item = findRestrictionItem(restriction);
        return context.getString(item.label);
    }

    public static String getUserAction(Context context, String restriction) {
        final UserRestrictionItem item = findRestrictionItem(restriction);
        return context.getString(item.userAction);
    }

    private static UserRestrictionItem findRestrictionItem(String restriction) {
        final UserRestrictionItem item = USER_RESTRICTION_ITEMS.get(restriction);
        if (item == null) {
            throw new IllegalArgumentException("Unknown restriction: " + restriction);
        }
        return item;
    }

    public static boolean isValidForPO(String restriction) {
        return ALSO_VALID_FOR_PO.contains(restriction);
    }

    public static String[] getUserRestrictions() {
        return RESTRICTION_IDS;
    }

    public static Intent getUserRestrictionTestIntent(Context context, String restriction) {
        final UserRestrictionItem item = USER_RESTRICTION_ITEMS.get(restriction);
        return new Intent(PolicyTransparencyTestActivity.ACTION_SHOW_POLICY_TRANSPARENCY_TEST)
                .putExtra(PolicyTransparencyTestActivity.EXTRA_TEST,
                        PolicyTransparencyTestActivity.TEST_CHECK_USER_RESTRICTION)
                .putExtra(CommandReceiverActivity.EXTRA_USER_RESTRICTION, restriction)
                .putExtra(PolicyTransparencyTestActivity.EXTRA_TITLE, context.getString(item.label))
                .putExtra(PolicyTransparencyTestActivity.EXTRA_SETTINGS_INTENT_ACTION,
                        item.intentAction);
    }

    private static class UserRestrictionItem {
        final int label;
        final int userAction;
        final String intentAction;
        public UserRestrictionItem(int label, int userAction, String intentAction) {
            this.label = label;
            this.userAction = userAction;
            this.intentAction = intentAction;
        }
    }
}