package com.android.tv.settings.accessories;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.support.v4.content.LocalBroadcastManager;
import android.text.TextUtils;

import com.android.tv.settings.MainSettings;

import java.util.HashSet;
import java.util.Set;

public class BluetoothConnectionsManager extends BroadcastReceiver {
    private static final String PREFS_NAME = "bt-connected-devs";
    private static final String KEY_CONNECTED_SET = "conencted-set";
    private static final String KEY_BT_STATE = "bt-state";

    public static final String ACTION_BLUETOOTH_UPDATE =
            "BluetoothConnectionsManager.BLUETOOTH_UPDATE";

    public void onReceive(Context context, Intent intent) {
        onConnectionChanged(context, intent);
    }

    public static void onConnectionChanged(Context context, Intent intent) {
        final String action = intent.getAction();

        final SharedPreferences prefs =
                context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        Set<String> connected = prefs.getStringSet(KEY_CONNECTED_SET, new HashSet<String>());
        int btState = prefs.getInt(KEY_BT_STATE, BluetoothAdapter.STATE_OFF);

        boolean listChanged = false;
        boolean btStateChanged = false;

        if (TextUtils.equals(action, BluetoothAdapter.ACTION_STATE_CHANGED)) {
            final int newBtState = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE,
                    BluetoothAdapter.ERROR);

            if (btState != newBtState) {
                // if BT was just turned off, we can't be connected to any devices.
                // if BT was just turned on, we haven't had the time to connect to any devices yet.
                if (newBtState == BluetoothAdapter.STATE_ON ||
                        newBtState == BluetoothAdapter.STATE_OFF) {
                    listChanged = true;
                    connected.clear();
                }

                btStateChanged = true;
                btState = newBtState;
            }
        } else if (intent.getExtras() != null) {
            BluetoothDevice device = intent.getExtras().getParcelable(BluetoothDevice.EXTRA_DEVICE);
            if (device != null) {
                if (TextUtils.equals(action, BluetoothDevice.ACTION_ACL_CONNECTED)) {
                    listChanged = connected.add(device.getAddress());
                } else if (TextUtils.equals(action, BluetoothDevice.ACTION_ACL_DISCONNECTED)) {
                    listChanged = connected.remove(device.getAddress());
                }
            }
        }

        if (btStateChanged || listChanged) {
            SharedPreferences.Editor editor = prefs.edit();

            if (btStateChanged) {
                editor.putInt(KEY_BT_STATE, btState);
            }
            if (listChanged) {
                editor.putStringSet(KEY_CONNECTED_SET, connected);
            }

            editor.apply();
        }

        if (listChanged) {
            LocalBroadcastManager.getInstance(context)
                    .sendBroadcast(new Intent(ACTION_BLUETOOTH_UPDATE));
        }
    }

    public static Set<String> getConnectedSet(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        return prefs.getStringSet(KEY_CONNECTED_SET, new HashSet<String>());
    }
}
