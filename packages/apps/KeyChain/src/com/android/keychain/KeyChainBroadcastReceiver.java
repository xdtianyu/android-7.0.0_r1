// Copyright 2011 Google Inc. All Rights Reserved.
package com.android.keychain;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class KeyChainBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(final Context context, final Intent intent) {
        intent.setClass(context, KeyChainService.class);
        context.startService(intent);
    }
}
