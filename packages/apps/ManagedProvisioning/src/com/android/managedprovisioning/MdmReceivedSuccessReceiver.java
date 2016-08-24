/*
 * Copyright 2015, The Android Open Source Project
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

package com.android.managedprovisioning;

import static android.app.admin.DevicePolicyManager.ACTION_MANAGED_PROFILE_PROVISIONED;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE;

import android.accounts.Account;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;

import com.android.managedprovisioning.common.Utils;

/**
 * Class that acts as the final receiver of the intent ACTION_PROFILE_PROVISIONING_COMPLETE
 * which is broadcasted using
 * {@link Context#sendOrderedBroadcast(Intent, String, BroadcastReceiver, android.os.Handler, int, String, Bundle)}
 * after profile owner or device owner provisioning is completed.
 */
public class MdmReceivedSuccessReceiver extends BroadcastReceiver {

    private final Account mMigratedAccount;
    private final String mMdmPackageName;

    private final Utils mUtils = new Utils();

    public MdmReceivedSuccessReceiver(Account migratedAccount, String mdmPackageName) {
        mMigratedAccount = migratedAccount;
        mMdmPackageName = mdmPackageName;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        ProvisionLogger.logd("ACTION_PROFILE_PROVISIONING_COMPLETE broadcast received by mdm");

        final Intent primaryProfileSuccessIntent = new Intent(ACTION_MANAGED_PROFILE_PROVISIONED);
        primaryProfileSuccessIntent.setPackage(mMdmPackageName);
        primaryProfileSuccessIntent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES |
                Intent.FLAG_RECEIVER_FOREGROUND);

        // Now cleanup the primary profile if necessary
        if (mMigratedAccount != null) {
            primaryProfileSuccessIntent.putExtra(EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE,
                    mMigratedAccount);
            finishAccountMigration(context, primaryProfileSuccessIntent);
            // Note that we currently do not check if account migration worked
        } else {
            context.sendBroadcast(primaryProfileSuccessIntent);
        }
    }

    private void finishAccountMigration(final Context context,
            final Intent primaryProfileSuccessIntent) {
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                mUtils.removeAccount(context, mMigratedAccount);
                context.sendBroadcast(primaryProfileSuccessIntent);
                return null;
            }
        }.execute();
    }
}
