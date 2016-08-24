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
 * limitations under the License
 */

package com.android.phone.vvm.omtp.sync;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.provider.VoicemailContract;
import android.util.Log;

public class OmtpVvmSyncReceiver extends BroadcastReceiver {

    private static final String TAG = "OmtpVvmSyncReceiver";

    @Override
    public void onReceive(final Context context, Intent intent) {
        if (VoicemailContract.ACTION_SYNC_VOICEMAIL.equals(intent.getAction())) {
            Log.v(TAG, "Sync intent received");
            Intent syncIntent = OmtpVvmSyncService
                    .getSyncIntent(context, OmtpVvmSyncService.SYNC_FULL_SYNC, null, true);
            intent.putExtra(OmtpVvmSyncService.EXTRA_IS_MANUAL_SYNC, true);
            context.startService(syncIntent);
        }
    }
}
