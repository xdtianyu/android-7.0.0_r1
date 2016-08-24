/*
 * Copyright (c) 2008-2009, Motorola, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the name of the Motorola, Inc. nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package com.android.bluetooth.pbap;

import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class BluetoothPbapReceiver extends BroadcastReceiver {

    private static final String TAG = "BluetoothPbapReceiver";

    private static final boolean V = BluetoothPbapService.VERBOSE;

    @Override
    public void onReceive(Context context, Intent intent) {
        if (V) Log.v(TAG, "PbapReceiver onReceive ");

        Intent in = new Intent();
        in.putExtras(intent);
        in.setClass(context, BluetoothPbapService.class);
        String action = intent.getAction();
        in.putExtra("action", action);
        if (V) Log.v(TAG,"***********action = " + action);

        boolean startService = true;
        if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
            int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
            in.putExtra(BluetoothAdapter.EXTRA_STATE, state);
            if (V) Log.v(TAG,"***********state = " + state);
            if ((state == BluetoothAdapter.STATE_TURNING_ON)
                    || (state == BluetoothAdapter.STATE_OFF)) {
                //FIX: We turn on PBAP after BluetoothAdapter.STATE_ON,
                //but we turn off PBAP right after BluetoothAdapter.STATE_TURNING_OFF
                startService = false;
            }
        } else {
            // Don't forward intent unless device has bluetooth and bluetooth is enabled.
            BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
            if (adapter == null || !adapter.isEnabled()) {
                startService = false;
            }
        }
        if (startService) {
            if (V) Log.v(TAG,"***********Calling start service!!!! with action = " + in.getAction());
            context.startService(in);
        }
    }
}
