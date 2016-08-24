package com.android.bluetooth.tests;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.BluetoothUuid;
import android.test.AndroidTestCase;
import android.util.Log;

import java.io.IOException;

public class SecurityTest extends AndroidTestCase {
    static final String TAG = "SecurityTest";

    public void connectSapNoSec() {
        BluetoothAdapter bt = BluetoothAdapter.getDefaultAdapter();
        if (bt == null) {
            Log.e(TAG,"No Bluetooth Device!");
            assertTrue(false);
        }

        BluetoothTestUtils.enableBt(bt);
        Log.i(TAG,"BT Enabled");
        BluetoothDevice serverDevice = bt.getRemoteDevice(ObexTest.SERVER_ADDRESS);
        Log.i(TAG,"ServerDevice: " + serverDevice);

        try {
            BluetoothSocket socket =
                    serverDevice.createInsecureRfcommSocketToServiceRecord(BluetoothUuid.SAP.getUuid());
            Log.i(TAG,"createInsecureRfcommSocketToServiceRecord() - waiting for connect...");
            socket.connect();
            Log.i(TAG,"Connected!");
            Thread.sleep(5000);
            Log.i(TAG,"Closing...");
            socket.close();
            Log.i(TAG,"Closed!");

        } catch (InterruptedException e) {
            Log.w(TAG, "Sleep interrupted", e);
            fail();

        }  catch (IOException e) {
            Log.e(TAG, "Failed to create connection", e);
            fail();
        }
        Log.i(TAG, "\n\n\nTest done - please fetch logs within 30 seconds...\n\n\n");
        try {
            Thread.sleep(30000);
        } catch (InterruptedException e) {}
    }
}
