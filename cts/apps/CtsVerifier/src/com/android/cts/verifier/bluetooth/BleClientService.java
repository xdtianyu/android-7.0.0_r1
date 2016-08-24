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

import java.util.Arrays;
import java.util.UUID;
import java.util.List;

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.ParcelUuid;
import android.util.Log;
import android.widget.Toast;

public class BleClientService extends Service {

    public static final boolean DEBUG = true;
    public static final String TAG = "BleClientService";

    public static final int COMMAND_CONNECT = 0;
    public static final int COMMAND_DISCONNECT = 1;
    public static final int COMMAND_DISCOVER_SERVICE = 2;
    public static final int COMMAND_READ_RSSI = 3;
    public static final int COMMAND_WRITE_CHARACTERISTIC = 4;
    public static final int COMMAND_READ_CHARACTERISTIC = 5;
    public static final int COMMAND_WRITE_DESCRIPTOR = 6;
    public static final int COMMAND_READ_DESCRIPTOR = 7;
    public static final int COMMAND_SET_NOTIFICATION = 8;
    public static final int COMMAND_BEGIN_WRITE = 9;
    public static final int COMMAND_EXECUTE_WRITE = 10;
    public static final int COMMAND_ABORT_RELIABLE = 11;
    public static final int COMMAND_SCAN_START = 12;
    public static final int COMMAND_SCAN_STOP = 13;

    public static final String BLE_BLUETOOTH_CONNECTED =
            "com.android.cts.verifier.bluetooth.BLE_BLUETOOTH_CONNECTED";
    public static final String BLE_BLUETOOTH_DISCONNECTED =
            "com.android.cts.verifier.bluetooth.BLE_BLUETOOTH_DISCONNECTED";
    public static final String BLE_SERVICES_DISCOVERED =
            "com.android.cts.verifier.bluetooth.BLE_SERVICES_DISCOVERED";
    public static final String BLE_CHARACTERISTIC_READ =
            "com.android.cts.verifier.bluetooth.BLE_CHARACTERISTIC_READ";
    public static final String BLE_CHARACTERISTIC_WRITE =
            "com.android.cts.verifier.bluetooth.BLE_CHARACTERISTIC_WRITE";
    public static final String BLE_CHARACTERISTIC_CHANGED =
            "com.android.cts.verifier.bluetooth.BLE_CHARACTERISTIC_CHANGED";
    public static final String BLE_DESCRIPTOR_READ =
            "com.android.cts.verifier.bluetooth.BLE_DESCRIPTOR_READ";
    public static final String BLE_DESCRIPTOR_WRITE =
            "com.android.cts.verifier.bluetooth.BLE_DESCRIPTOR_WRITE";
    public static final String BLE_RELIABLE_WRITE_COMPLETED =
            "com.android.cts.verifier.bluetooth.BLE_RELIABLE_WRITE_COMPLETED";
    public static final String BLE_READ_REMOTE_RSSI =
            "com.android.cts.verifier.bluetooth.BLE_READ_REMOTE_RSSI";

    public static final String EXTRA_COMMAND =
            "com.android.cts.verifier.bluetooth.EXTRA_COMMAND";
    public static final String EXTRA_WRITE_VALUE =
            "com.android.cts.verifier.bluetooth.EXTRA_WRITE_VALUE";
    public static final String EXTRA_BOOL =
            "com.android.cts.verifier.bluetooth.EXTRA_BOOL";
    public static final String EXTRA_CHARACTERISTIC_VALUE =
            "com.android.cts.verifier.bluetooth.EXTRA_CHARACTERISTIC_VALUE";
    public static final String EXTRA_DESCRIPTOR_VALUE =
            "com.android.cts.verifier.bluetooth.EXTRA_DESCRIPTOR_VALUE";
    public static final String EXTRA_RSSI_VALUE =
            "com.android.cts.verifier.bluetooth.EXTRA_RSSI_VALUE";
    public static final String EXTRA_ERROR_MESSAGE =
            "com.android.cts.verifier.bluetooth.EXTRA_ERROR_MESSAGE";

    private static final UUID SERVICE_UUID =
            UUID.fromString("00009999-0000-1000-8000-00805f9b34fb");
    private static final UUID CHARACTERISTIC_UUID =
            UUID.fromString("00009998-0000-1000-8000-00805f9b34fb");
    private static final UUID UPDATE_CHARACTERISTIC_UUID =
            UUID.fromString("00009997-0000-1000-8000-00805f9b34fb");
    private static final UUID DESCRIPTOR_UUID =
            UUID.fromString("00009996-0000-1000-8000-00805f9b34fb");

    private static final String WRITE_VALUE = "TEST";

    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothDevice mDevice;
    private BluetoothGatt mBluetoothGatt;
    private BluetoothLeScanner mScanner;
    private Handler mHandler;
    private Context mContext;

    @Override
    public void onCreate() {
        super.onCreate();

        mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = mBluetoothManager.getAdapter();
        mScanner = mBluetoothAdapter.getBluetoothLeScanner();
        mHandler = new Handler();
        mContext = this;
        startScan();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mBluetoothGatt.disconnect();
        mBluetoothGatt.close();
        mBluetoothGatt = null;
        stopScan();
    }

    private void writeCharacteristic(String writeValue) {
        BluetoothGattCharacteristic characteristic = getCharacteristic(CHARACTERISTIC_UUID);
        if (characteristic == null) return;
        characteristic.setValue(writeValue);
        mBluetoothGatt.writeCharacteristic(characteristic);
    }

    private void readCharacteristic() {
        BluetoothGattCharacteristic characteristic = getCharacteristic(CHARACTERISTIC_UUID);
        if (characteristic != null) mBluetoothGatt.readCharacteristic(characteristic);
    }

    private void writeDescriptor(String writeValue) {
        BluetoothGattDescriptor descriptor = getDescriptor();
        if (descriptor == null) return;
        descriptor.setValue(writeValue.getBytes());
        mBluetoothGatt.writeDescriptor(descriptor);
    }

    private void readDescriptor() {
        BluetoothGattDescriptor descriptor = getDescriptor();
        if (descriptor != null) mBluetoothGatt.readDescriptor(descriptor);
    }

    private void setNotification(boolean enable) {
        BluetoothGattCharacteristic characteristic = getCharacteristic(UPDATE_CHARACTERISTIC_UUID);
        if (characteristic != null)
            mBluetoothGatt.setCharacteristicNotification(characteristic, enable);
    }

    private void notifyError(String message) {
        showMessage(message);
    }

    private void notifyConnected() {
        showMessage("BLE connected");
        Intent intent = new Intent(BLE_BLUETOOTH_CONNECTED);
        sendBroadcast(intent);
    }

    private void notifyDisconnected() {
        showMessage("BLE disconnected");
        Intent intent = new Intent(BLE_BLUETOOTH_DISCONNECTED);
        sendBroadcast(intent);
    }

    private void notifyServicesDiscovered() {
        showMessage("Service discovered");
        Intent intent = new Intent(BLE_SERVICES_DISCOVERED);
        sendBroadcast(intent);
    }

    private void notifyCharacteristicRead(String value) {
        showMessage("Characteristic read: " + value);
        Intent intent = new Intent(BLE_CHARACTERISTIC_READ);
        intent.putExtra(EXTRA_CHARACTERISTIC_VALUE, value);
        sendBroadcast(intent);
    }

    private void notifyCharacteristicWrite(String value) {
        showMessage("Characteristic write: " + value);
        Intent intent = new Intent(BLE_CHARACTERISTIC_WRITE);
        sendBroadcast(intent);
    }

    private void notifyCharacteristicChanged(String value) {
        showMessage("Characteristic changed: " + value);
        Intent intent = new Intent(BLE_CHARACTERISTIC_CHANGED);
        intent.putExtra(EXTRA_CHARACTERISTIC_VALUE, value);
        sendBroadcast(intent);
    }

    private void notifyDescriptorRead(String value) {
        showMessage("Descriptor read: " + value);
        Intent intent = new Intent(BLE_DESCRIPTOR_READ);
        intent.putExtra(EXTRA_DESCRIPTOR_VALUE, value);
        sendBroadcast(intent);
    }

    private void notifyDescriptorWrite(String value) {
        showMessage("Descriptor write: " + value);
        Intent intent = new Intent(BLE_DESCRIPTOR_WRITE);
        sendBroadcast(intent);
    }

    private void notifyReliableWriteCompleted() {
        showMessage("Reliable write compelte");
        Intent intent = new Intent(BLE_RELIABLE_WRITE_COMPLETED);
        sendBroadcast(intent);
    }

    private void notifyReadRemoteRssi(int rssi) {
        showMessage("Remote rssi read: " + rssi);
        Intent intent = new Intent(BLE_READ_REMOTE_RSSI);
        intent.putExtra(EXTRA_RSSI_VALUE, rssi);
        sendBroadcast(intent);
    }

    private BluetoothGattService getService() {
        if (mBluetoothGatt == null) return null;

        BluetoothGattService service = mBluetoothGatt.getService(SERVICE_UUID);
        if (service == null) {
            showMessage("Service not found");
            return null;
        }
        return service;
    }

    private BluetoothGattCharacteristic getCharacteristic(UUID uuid) {
        BluetoothGattService service = getService();
        if (service == null) return null;

        BluetoothGattCharacteristic characteristic = service.getCharacteristic(uuid);
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

    private void showMessage(final String msg) {
        mHandler.post(new Runnable() {
            public void run() {
                Toast.makeText(BleClientService.this, msg, Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void sleep(int millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException e) {
            Log.e(TAG, "Error in thread sleep", e);
        }
    }

    private void reliableWrite() {
        mBluetoothGatt.beginReliableWrite();
        sleep(1000);
        writeCharacteristic(WRITE_VALUE);
        sleep(1000);
        if (!mBluetoothGatt.executeReliableWrite()) {
            Log.w(TAG, "reliable write failed");
        }
        sleep(1000);
        mBluetoothGatt.abortReliableWrite();
    }

    private final BluetoothGattCallback mGattCallbacks = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (DEBUG) Log.d(TAG, "onConnectionStateChange");
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    notifyConnected();
                    stopScan();
                    sleep(1000);
                    mBluetoothGatt.discoverServices();
                } else if (status == BluetoothProfile.STATE_DISCONNECTED) {
                    notifyDisconnected();
                }
            } else {
                showMessage("Failed to connect");
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (DEBUG) Log.d(TAG, "onServiceDiscovered");
            if ((status == BluetoothGatt.GATT_SUCCESS) &&
                (mBluetoothGatt.getService(SERVICE_UUID) != null)) {
                notifyServicesDiscovered();
                sleep(1000);
                writeCharacteristic(WRITE_VALUE);
            }
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt,
                                          BluetoothGattCharacteristic characteristic, int status) {
            String value = characteristic.getStringValue(0);
            if (DEBUG) Log.d(TAG, "onCharacteristicWrite: characteristic.val="
                    + value + " status=" + status);
            BluetoothGattCharacteristic mCharacteristic = getCharacteristic(CHARACTERISTIC_UUID);
            if ((status == BluetoothGatt.GATT_SUCCESS) &&
                (value.equals(mCharacteristic.getStringValue(0)))) {
                notifyCharacteristicWrite(value);
                sleep(1000);
                readCharacteristic();
            } else {
                notifyError("Failed to write characteristic: " + value);
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic, int status) {
            if (DEBUG) Log.d(TAG, "onCharacteristicRead");
            if ((status == BluetoothGatt.GATT_SUCCESS) &&
                (characteristic.getUuid().equals(CHARACTERISTIC_UUID))) {
                notifyCharacteristicRead(characteristic.getStringValue(0));
                sleep(1000);
                writeDescriptor(WRITE_VALUE);
            } else {
                notifyError("Failed to read characteristic");
            }
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor,
                                      int status) {
            if (DEBUG) Log.d(TAG, "onDescriptorWrite");
            if ((status == BluetoothGatt.GATT_SUCCESS) &&
                (descriptor.getUuid().equals(DESCRIPTOR_UUID))) {
                notifyDescriptorWrite(new String(descriptor.getValue()));
                sleep(1000);
                readDescriptor();
            } else {
                notifyError("Failed to write descriptor");
            }
        }

        @Override
        public void onDescriptorRead(BluetoothGatt gatt, BluetoothGattDescriptor descriptor,
                                     int status) {
            if (DEBUG) Log.d(TAG, "onDescriptorRead");
            if ((status == BluetoothGatt.GATT_SUCCESS) &&
                (descriptor.getUuid() != null) &&
                (descriptor.getUuid().equals(DESCRIPTOR_UUID))) {
                notifyDescriptorRead(new String(descriptor.getValue()));
                sleep(1000);
                setNotification(true);
            } else {
                notifyError("Failed to read descriptor");
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            if (DEBUG) Log.d(TAG, "onCharacteristicChanged");
            if ((characteristic.getUuid() != null) &&
                (characteristic.getUuid().equals(UPDATE_CHARACTERISTIC_UUID))) {
                notifyCharacteristicChanged(characteristic.getStringValue(0));
                setNotification(false);
                sleep(1000);
                mBluetoothGatt.readRemoteRssi();
            }
        }

        @Override
        public void onReliableWriteCompleted(BluetoothGatt gatt, int status) {
            if (DEBUG) Log.d(TAG, "onReliableWriteComplete: " + status);
            if (status == BluetoothGatt.GATT_SUCCESS) {
                notifyReliableWriteCompleted();
            } else {
                notifyError("Failed to complete reliable write: " + status);
            }
            sleep(1000);
            mBluetoothGatt.disconnect();
        }

        @Override
        public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
            if (DEBUG) Log.d(TAG, "onReadRemoteRssi");
            if (status == BluetoothGatt.GATT_SUCCESS) {
                notifyReadRemoteRssi(rssi);
            } else {
                notifyError("Failed to read remote rssi");
            }
            sleep(1000);
            reliableWrite();
        }
    };

    private final ScanCallback mScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            if (mBluetoothGatt == null) {
                mBluetoothGatt = result.getDevice().connectGatt(mContext, false, mGattCallbacks);
            }
        }
    };

    private void startScan() {
        if (DEBUG) Log.d(TAG, "startScan");
        List<ScanFilter> filter = Arrays.asList(new ScanFilter.Builder().setServiceUuid(
                new ParcelUuid(BleServerService.ADV_SERVICE_UUID)).build());
        ScanSettings setting = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build();
        mScanner.startScan(filter, setting, mScanCallback);
    }

    private void stopScan() {
        if (DEBUG) Log.d(TAG, "stopScan");
        mScanner.stopScan(mScanCallback);
    }
}
