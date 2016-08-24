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

import android.app.Activity;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.UserManager;
import android.util.Log;

import com.android.cts.verifier.R;
import com.android.cts.verifier.managedprovisioning.Utils;

import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

public class CommandReceiverActivity extends Activity {
    private static final String TAG = "CommandReceiverActivity";

    public static final String ACTION_EXECUTE_COMMAND =
            "com.android.cts.verifier.managedprovisioning.action.EXECUTE_COMMAND";
    public static final String EXTRA_COMMAND =
            "com.android.cts.verifier.managedprovisioning.extra.COMMAND";

    public static final String COMMAND_SET_USER_RESTRICTION = "set-user_restriction";
    public static final String COMMAND_DISALLOW_KEYGUARD_UNREDACTED_NOTIFICATIONS =
            "disallow-keyguard-unredacted-notifications";
    public static final String COMMAND_SET_AUTO_TIME_REQUIRED = "set-auto-time-required";
    public static final String COMMAND_SET_GLOBAL_SETTING =
            "set-global-setting";
    public static final String COMMAND_SET_MAXIMUM_TO_LOCK = "set-maximum-time-to-lock";
    public static final String COMMAND_SET_PASSWORD_QUALITY = "set-password-quality";
    public static final String COMMAND_SET_KEYGUARD_DISABLED = "set-keyguard-disabled";
    public static final String COMMAND_SET_LOCK_SCREEN_INFO = "set-lock-screen-info";
    public static final String COMMAND_SET_STATUSBAR_DISABLED = "set-statusbar-disabled";
    public static final String COMMAND_ALLOW_ONLY_SYSTEM_INPUT_METHODS =
            "allow-only-system-input-methods";
    public static final String COMMAND_ALLOW_ONLY_SYSTEM_ACCESSIBILITY_SERVICES =
            "allow-only-system-accessibility-services";
    public static final String COMMAND_DEVICE_OWNER_CLEAR_POLICIES = "do-clear-policies";
    public static final String COMMAND_PROFILE_OWNER_CLEAR_POLICIES = "po-clear-policies";
    public static final String COMMAND_REMOVE_DEVICE_OWNER = "remove-device-owner";
    public static final String COMMAND_REQUEST_BUGREPORT = "request-bugreport";
    public static final String COMMAND_SET_USER_ICON = "set-user-icon";

    public static final String EXTRA_USER_RESTRICTION =
            "com.android.cts.verifier.managedprovisioning.extra.USER_RESTRICTION";
    public static final String EXTRA_SETTING =
            "com.android.cts.verifier.managedprovisioning.extra.SETTING";
    // This extra can be used along with a command extra to set policy to
    // specify if that policy is enforced or not.
    public static final String EXTRA_ENFORCED =
            "com.android.cts.verifier.managedprovisioning.extra.ENFORCED";
    public static final String EXTRA_VALUE =
            "com.android.cts.verifier.managedprovisioning.extra.VALUE";

    private ComponentName mAdmin;
    private DevicePolicyManager mDpm;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Intent intent = getIntent();
        try {
            mDpm = (DevicePolicyManager) getSystemService(
                    Context.DEVICE_POLICY_SERVICE);
            mAdmin = DeviceAdminTestReceiver.getReceiverComponentName();
            Log.i(TAG, "Command: " + intent);

            final String command = getIntent().getStringExtra(EXTRA_COMMAND);
            switch (command) {
                case COMMAND_SET_USER_RESTRICTION: {
                    String restrictionKey = intent.getStringExtra(EXTRA_USER_RESTRICTION);
                    boolean enforced = intent.getBooleanExtra(EXTRA_ENFORCED, false);
                    if (enforced) {
                        mDpm.addUserRestriction(mAdmin, restrictionKey);
                    } else {
                        mDpm.clearUserRestriction(mAdmin, restrictionKey);
                    }
                } break;
                case COMMAND_DISALLOW_KEYGUARD_UNREDACTED_NOTIFICATIONS: {
                    mDpm.setKeyguardDisabledFeatures(mAdmin,
                            DevicePolicyManager.KEYGUARD_DISABLE_UNREDACTED_NOTIFICATIONS);
                } break;
                case COMMAND_SET_AUTO_TIME_REQUIRED: {
                    mDpm.setAutoTimeRequired(mAdmin,
                            intent.getBooleanExtra(EXTRA_ENFORCED, false));
                }
                case COMMAND_SET_LOCK_SCREEN_INFO: {
                    mDpm.setDeviceOwnerLockScreenInfo(mAdmin, intent.getStringExtra(EXTRA_VALUE));
                }
                case COMMAND_SET_MAXIMUM_TO_LOCK: {
                    final long timeInSeconds = Long.parseLong(intent.getStringExtra(EXTRA_VALUE));
                    mDpm.setMaximumTimeToLock(mAdmin,
                            TimeUnit.SECONDS.toMillis(timeInSeconds) /* in milliseconds */);
                } break;
                case COMMAND_SET_PASSWORD_QUALITY: {
                    int quality = intent.getIntExtra(EXTRA_VALUE, 0);
                    mDpm.setPasswordQuality(mAdmin, quality);
                } break;
                case COMMAND_SET_KEYGUARD_DISABLED: {
                    boolean enforced = intent.getBooleanExtra(EXTRA_ENFORCED, false);
                    if (enforced) {
                        mDpm.resetPassword(null, 0);
                    }
                    mDpm.setKeyguardDisabled(mAdmin, enforced);
                } break;
                case COMMAND_SET_STATUSBAR_DISABLED: {
                    boolean enforced = intent.getBooleanExtra(EXTRA_ENFORCED, false);
                    mDpm.setStatusBarDisabled(mAdmin, enforced);
                } break;
                case COMMAND_ALLOW_ONLY_SYSTEM_INPUT_METHODS: {
                    boolean enforced = intent.getBooleanExtra(EXTRA_ENFORCED, false);
                    mDpm.setPermittedInputMethods(mAdmin, enforced ? new ArrayList() : null);
                } break;
                case COMMAND_ALLOW_ONLY_SYSTEM_ACCESSIBILITY_SERVICES: {
                    boolean enforced = intent.getBooleanExtra(EXTRA_ENFORCED, false);
                    mDpm.setPermittedAccessibilityServices(mAdmin,
                            enforced ? new ArrayList() : null);
                } break;
                case COMMAND_SET_GLOBAL_SETTING: {
                    final String setting = intent.getStringExtra(EXTRA_SETTING);
                    final String value = intent.getStringExtra(EXTRA_VALUE);
                    mDpm.setGlobalSetting(mAdmin, setting, value);
                } break;
                case COMMAND_REMOVE_DEVICE_OWNER: {
                    if (!mDpm.isDeviceOwnerApp(getPackageName())) {
                        return;
                    }
                    clearAllPolicies();
                    mDpm.clearDeviceOwnerApp(getPackageName());
                } break;
                case COMMAND_REQUEST_BUGREPORT: {
                    if (!mDpm.isDeviceOwnerApp(getPackageName())) {
                        return;
                    }
                    final boolean bugreportStarted = mDpm.requestBugreport(mAdmin);
                    if (!bugreportStarted) {
                        Utils.showBugreportNotification(this, getString(
                                R.string.bugreport_already_in_progress),
                                Utils.BUGREPORT_NOTIFICATION_ID);
                    }
                } break;
                case COMMAND_DEVICE_OWNER_CLEAR_POLICIES: {
                    if (!mDpm.isDeviceOwnerApp(getPackageName())) {
                        return;
                    }
                    clearAllPolicies();
                } break;
                case COMMAND_PROFILE_OWNER_CLEAR_POLICIES: {
                    if (!mDpm.isProfileOwnerApp(getPackageName())) {
                        return;
                    }
                    clearProfileOwnerRelatedPolicies();
                } break;
                case COMMAND_SET_USER_ICON: {
                    if (!mDpm.isDeviceOwnerApp(getPackageName())) {
                        return;
                    }
                    mDpm.setUserIcon(mAdmin, BitmapFactory.decodeResource(getResources(),
                            com.android.cts.verifier.R.drawable.icon));
                } break;
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to execute command: " + intent, e);
        } finally {
            finish();
        }
    }

    private void clearAllPolicies() {
        clearProfileOwnerRelatedPolicies();

        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_ADD_USER);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_ADJUST_VOLUME);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_CONFIG_BLUETOOTH);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_CONFIG_CELL_BROADCASTS);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_CONFIG_MOBILE_NETWORKS);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_CONFIG_TETHERING);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_CONFIG_VPN);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_CONFIG_WIFI);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_DATA_ROAMING);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_DEBUGGING_FEATURES);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_FACTORY_RESET);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_FUN);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_INSTALL_UNKNOWN_SOURCES);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_NETWORK_RESET);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_OUTGOING_BEAM);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_REMOVE_USER);

        mDpm.setDeviceOwnerLockScreenInfo(mAdmin, null);
        mDpm.setKeyguardDisabled(mAdmin, false);
        mDpm.setAutoTimeRequired(mAdmin, false);
        mDpm.setStatusBarDisabled(mAdmin, false);
    }

    private void clearProfileOwnerRelatedPolicies() {
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_APPS_CONTROL);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_CONFIG_CREDENTIALS);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_MODIFY_ACCOUNTS);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_SHARE_LOCATION);
        mDpm.clearUserRestriction(mAdmin, UserManager.DISALLOW_UNINSTALL_APPS);

        mDpm.setKeyguardDisabledFeatures(mAdmin, 0);
        mDpm.setPasswordQuality(mAdmin, 0);
        mDpm.setMaximumTimeToLock(mAdmin, 0);
        mDpm.setPermittedAccessibilityServices(mAdmin, null);
        mDpm.setPermittedInputMethods(mAdmin, null);
    }
}
