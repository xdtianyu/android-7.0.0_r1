/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.cts.verifier.bluetooth;

import java.util.Date;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattServerCallback;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.ParcelUuid;
import android.util.Log;
import android.widget.Toast;

public class BleServerService extends Service {

    public static final boolean DEBUG = true;
    public static final String TAG = "BleServerService";

    public static final int COMMAND_ADD_SERVICE = 0;
    public static final int COMMAND_WRITE_CHARACTERISTIC = 1;
    public static final int COMMAND_WRITE_DESCRIPTOR = 2;

    public static final String BLE_SERVER_CONNECTED =
            "com.android.cts.verifier.bluetooth.BLE_SERVER_CONNECTED";
    public static final String BLE_SERVER_DISCONNECTED =
            "com.android.cts.verifier.bluetooth.BLE_SERVER_DISCONNECTED";
    public static final String BLE_SERVICE_ADDED =
            "com.android.cts.verifier.bluetooth.BLE_SERVICE_ADDED";
    public static final String BLE_CHARACTERISTIC_READ_REQUEST =
            "com.android.cts.verifier.bluetooth.BLE_CHARACTERISTIC_READ_REQUEST";
    public static final String BLE_CHARACTERISTIC_WRITE_REQUEST =
            "com.android.cts.verifier.bluetooth.BLE_CHARACTERISTIC_WRITE_REQUEST";
    public static final String BLE_DESCRIPTOR_READ_REQUEST =
            "com.android.cts.verifier.bluetooth.BLE_DESCRIPTOR_READ_REQUEST";
    public static final String BLE_DESCRIPTOR_WRITE_REQUEST =
            "com.android.cts.verifier.bluetooth.BLE_DESCRIPTOR_WRITE_REQUEST";
    public static final String BLE_EXECUTE_WRITE =
            "com.android.cts.verifier.bluetooth.BLE_EXECUTE_WRITE";
    public static final String BLE_OPEN_FAIL =
            "com.android.cts.verifier.bluetooth.BLE_OPEN_FAIL";

    private static final UUID SERVICE_UUID =
            UUID.fromString("00009999-0000-1000-8000-00805f9b34fb");
    private static final UUID CHARACTERISTIC_UUID =
            UUID.fromString("00009998-0000-1000-8000-00805f9b34fb");
    private static final UUID UPDATE_CHARACTERISTIC_UUID =
            UUID.fromString("00009997-0000-1000-8000-00805f9b34fb");
    private static final UUID DESCRIPTOR_UUID =
            UUID.fromString("00009996-0000-1000-8000-00805f9b34fb");
    public static final UUID ADV_SERVICE_UUID=
            UUID.fromString("00003333-0000-1000-8000-00805f9b34fb");

    private BluetoothManager mBluetoothManager;
    private BluetoothGattServer mGattServer;
    private BluetoothGattService mService;
    private BluetoothDevice mDevice;
    private Timer mNotificationTimer;
    private Handler mHandler;
    private String mReliableWriteValue;
    private BluetoothLeAdvertiser mAdvertiser;

    @Override
    public void onCreate() {
        super.onCreate();

        mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mAdvertiser = mBluetoothManager.getAdapter().getBluetoothLeAdvertiser();
        mGattServer = mBluetoothManager.openGattServer(this, mCallbacks);
        mService = createService();
        if (mGattServer != null) {
            mGattServer.addService(mService);
        }
        mDevice = null;
        mReliableWriteValue = null;

        mHandler = new Handler();
        if (mGattServer == null) {
            notifyOpenFail();
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        startAdvertise();
        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        stopAdvertise();
        if (mGattServer == null) {
           return;
        }
        if (mDevice != null) mGattServer.cancelConnection(mDevice);
        mGattServer.close();
    }

    private void writeCharacteristic(String writeValue) {
        BluetoothGattCharacteristic characteristic = getCharacteristic(CHARACTERISTIC_UUID);
        if (characteristic != null) return;
        characteristic.setValue(writeValue);
    }

    private void writeDescriptor(String writeValue) {
        BluetoothGattDescriptor descriptor = getDescriptor();
        if (descriptor == null) return;
        descriptor.setValue(writeValue.getBytes());
    }

    private void notifyOpenFail() {
        if (DEBUG) Log.d(TAG, "notifyOpenFail");
        Intent intent = new Intent(BLE_OPEN_FAIL);
        sendBroadcast(intent);
    }

    private void notifyConnected() {
        if (DEBUG) Log.d(TAG, "notifyConnected");
        Intent intent = new Intent(BLE_SERVER_CONNECTED);
        sendBroadcast(intent);
    }

    private void notifyDisconnected() {
        if (DEBUG) Log.d(TAG, "notifyDisconnected");
        Intent intent = new Intent(BLE_SERVER_DISCONNECTED);
        sendBroadcast(intent);
    }

    private void notifyServiceAdded() {
        if (DEBUG) Log.d(TAG, "notifyServiceAdded");
        Intent intent = new Intent(BLE_SERVICE_ADDED);
        sendBroadcast(intent);
    }

    private void notifyCharacteristicReadRequest() {
        if (DEBUG) Log.d(TAG, "notifyCharacteristicReadRequest");
        Intent intent = new Intent(BLE_CHARACTERISTIC_READ_REQUEST);
        sendBroadcast(intent);
    }

    private void notifyCharacteristicWriteRequest() {
        if (DEBUG) Log.d(TAG, "notifyCharacteristicWriteRequest");
        Intent intent = new Intent(BLE_CHARACTERISTIC_WRITE_REQUEST);
        sendBroadcast(intent);
    }

    private void notifyDescriptorReadRequest() {
        if (DEBUG) Log.d(TAG, "notifyDescriptorReadRequest");
        Intent intent = new Intent(BLE_DESCRIPTOR_READ_REQUEST);
        sendBroadcast(intent);
    }

    private void notifyDescriptorWriteRequest() {
        if (DEBUG) Log.d(TAG, "notifyDescriptorWriteRequest");
        Intent intent = new Intent(BLE_DESCRIPTOR_WRITE_REQUEST);
        sendBroadcast(intent);
    }

    private void notifyExecuteWrite() {
        if (DEBUG) Log.d(TAG, "notifyExecuteWrite");
        Intent intent = new Intent(BLE_EXECUTE_WRITE);
        sendBroadcast(intent);
    }

    private BluetoothGattCharacteristic getCharacteristic(UUID uuid) {
        BluetoothGattCharacteristic characteristic =
                mService.getCharacteristic(uuid);
        if (characteristic == null) {
            showMessage("Characteristic not found");
            return null;
        }
        return characteristic;
    }

    private BluetoothGattDescriptor getDescriptor() {
        BluetoothGattCharacteristic characteristic = getCharacteristic(CHARACTERISTIC_UUID);
        if (characteristic == null) return null;

        BluetoothGattDescriptor descriptor = characteristic.getDescriptor(DESCRIPTOR_UUID);
        if (descriptor == null) {
            showMessage("Descriptor not found");
            return null;
        }
        return descriptor;
    }

    private BluetoothGattService createService() {
        BluetoothGattService service =
                new BluetoothGattService(SERVICE_UUID, BluetoothGattService.SERVICE_TYPE_PRIMARY);
        BluetoothGattCharacteristic characteristic =
                new BluetoothGattCharacteristic(CHARACTERISTIC_UUID, 0x0A, 0x11);
        BluetoothGattDescriptor descriptor = new BluetoothGattDescriptor(DESCRIPTOR_UUID, 0x11);
        characteristic.addDescriptor(descriptor);
        service.addCharacteristic(characteristic);

        BluetoothGattCharacteristic notiCharacteristic =
                new BluetoothGattCharacteristic(UPDATE_CHARACTERISTIC_UUID, 0x10, 0x00);
        service.addCharacteristic(notiCharacteristic);

        return service;
    }

    private void beginNotification() {
        TimerTask task = new TimerTask() {
            @Override
            public void run() {
                if (mGattServer == null) {
                    if (DEBUG) Log.d(TAG, "GattServer is null, return");
                    return;
                }
                BluetoothGattCharacteristic characteristic =
                        mService.getCharacteristic(UPDATE_CHARACTERISTIC_UUID);
                if (characteristic == null) return;

                String date = (new Date()).toString();
                characteristic.setValue(date);
                mGattServer.notifyCharacteristicChanged(mDevice, characteristic, false);
            }
        };
        mNotificationTimer = new Timer();
        mNotificationTimer.schedule(task, 0, 1000);
    }

    private void stopNotification() {
        if (mNotificationTimer == null) return;
        mNotificationTimer.cancel();
        mNotificationTimer = null;
    }

    private void showMessage(final String msg) {
        mHandler.post(new Runnable() {
            public void run() {
                Toast.makeText(BleServerService.this, msg, Toast.LENGTH_SHORT).show();
            }
        });
    }

    private final BluetoothGattServerCallback mCallbacks = new BluetoothGattServerCallback() {
        @Override
        public void onConnectionStateChange(BluetoothDevice device, int status, int newState) {
            if (DEBUG) Log.d(TAG, "onConnectionStateChange: newState=" + newState);
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    mDevice = device;
                    notifyConnected();
                    beginNotification();
                } else if (status == BluetoothProfile.STATE_DISCONNECTED) {
                    stopNotification();
                    notifyDisconnected();
                    mDevice = null;
                }
            }
        }

        @Override
        public void onServiceAdded(int status, BluetoothGattService service) {
            if (DEBUG) Log.d(TAG, "onServiceAdded()");
            if (status == BluetoothGatt.GATT_SUCCESS) notifyServiceAdded();
        }

        @Override
        public void onCharacteristicReadRequest(BluetoothDevice device, int requestId,
                int offset, BluetoothGattCharacteristic characteristic) {
            if (mGattServer == null) {
                if (DEBUG) Log.d(TAG, "GattServer is null, return");
                return;
            }
            if (DEBUG) Log.d(TAG, "onCharacteristicReadRequest()");

            notifyCharacteristicReadRequest();
            mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0,
                                     characteristic.getValue());
        }

        @Override
        public void onCharacteristicWriteRequest(BluetoothDevice device, int requestId,
                BluetoothGattCharacteristic characteristic,
                boolean preparedWrite, boolean responseNeeded,
                int offset, byte[] value) {
            if (mGattServer == null) {
                if (DEBUG) Log.d(TAG, "GattServer is null, return");
                return;
            }
            if (DEBUG) Log.d(TAG, "onCharacteristicWriteRequest: preparedWrite=" + preparedWrite);

            notifyCharacteristicWriteRequest();
            if (preparedWrite) mReliableWriteValue = new String(value);
            else characteristic.setValue(value);

            if (responseNeeded)
                mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0, null);
        }

        @Override
        public void onDescriptorReadRequest(BluetoothDevice device, int requestId,
                int offset, BluetoothGattDescriptor descriptor) {
            if (mGattServer == null) {
                if (DEBUG) Log.d(TAG, "GattServer is null, return");
                return;
            }
            if (DEBUG) Log.d(TAG, "onDescriptorReadRequest(): (descriptor == getDescriptor())="
                                  + (descriptor == getDescriptor()));

            notifyDescriptorReadRequest();
            mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0,
                                     descriptor.getValue());
        }

        @Override
        public void onDescriptorWriteRequest(BluetoothDevice device, int requestId,
                BluetoothGattDescriptor descriptor,
                boolean preparedWrite, boolean responseNeeded,
                int offset,  byte[] value) {
            if (mGattServer == null) {
                if (DEBUG) Log.d(TAG, "GattServer is null, return");
                return;
            }
            if (DEBUG) Log.d(TAG, "onDescriptorWriteRequest(): (descriptor == getDescriptor())="
                                  + (descriptor == getDescriptor()));

            notifyDescriptorWriteRequest();
            descriptor.setValue(value);
            if (responseNeeded)
                mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0, null);
        }

        @Override
        public void onExecuteWrite(BluetoothDevice device, int requestId, boolean execute) {
            if (mGattServer == null) {
                if (DEBUG) Log.d(TAG, "GattServer is null, return");
                return;
            }
            if (DEBUG) Log.d(TAG, "onExecuteWrite");
            if (execute) {
                notifyExecuteWrite();
                getCharacteristic(CHARACTERISTIC_UUID).setValue(mReliableWriteValue);
                mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0, null);
            }
        }
    };

    private void startAdvertise() {
        if (DEBUG) Log.d(TAG, "startAdvertise");
        AdvertiseData data = new AdvertiseData.Builder()
            .addServiceData(new ParcelUuid(ADV_SERVICE_UUID), new byte[]{1,2,3})
            .addServiceUuid(new ParcelUuid(ADV_SERVICE_UUID))
            .build();
        AdvertiseSettings setting = new AdvertiseSettings.Builder()
            .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
            .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_MEDIUM)
            .setConnectable(true)
            .build();
        mAdvertiser.startAdvertising(setting, data, mAdvertiseCallback);
    }

    private void stopAdvertise() {
        if (DEBUG) Log.d(TAG, "stopAdvertise");
        mAdvertiser.stopAdvertising(mAdvertiseCallback);
    }

    private final AdvertiseCallback mAdvertiseCallback = new AdvertiseCallback(){};
}

