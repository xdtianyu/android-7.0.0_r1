/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.bluetooth.gatt;

import android.bluetooth.BluetoothUuid;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.ParcelUuid;
import android.os.RemoteException;
import android.util.Log;

import com.android.bluetooth.Utils;
import com.android.bluetooth.btservice.AdapterService;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Manages Bluetooth LE advertising operations and interacts with bluedroid stack. TODO: add tests.
 *
 * @hide
 */
class AdvertiseManager {
    private static final boolean DBG = GattServiceConfig.DBG;
    private static final String TAG = GattServiceConfig.TAG_PREFIX + "AdvertiseManager";

    // Timeout for each controller operation.
    private static final int OPERATION_TIME_OUT_MILLIS = 500;

    // Message for advertising operations.
    private static final int MSG_START_ADVERTISING = 0;
    private static final int MSG_STOP_ADVERTISING = 1;

    private final GattService mService;
    private final AdapterService mAdapterService;
    private final Set<AdvertiseClient> mAdvertiseClients;
    private final AdvertiseNative mAdvertiseNative;

    // Handles advertise operations.
    private ClientHandler mHandler;

    // CountDownLatch for blocking advertise operations.
    private CountDownLatch mLatch;

    /**
     * Constructor of {@link AdvertiseManager}.
     */
    AdvertiseManager(GattService service, AdapterService adapterService) {
        logd("advertise manager created");
        mService = service;
        mAdapterService = adapterService;
        mAdvertiseClients = new HashSet<AdvertiseClient>();
        mAdvertiseNative = new AdvertiseNative();
    }

    /**
     * Start a {@link HandlerThread} that handles advertising operations.
     */
    void start() {
        HandlerThread thread = new HandlerThread("BluetoothAdvertiseManager");
        thread.start();
        mHandler = new ClientHandler(thread.getLooper());
    }

    void cleanup() {
        logd("advertise clients cleared");
        mAdvertiseClients.clear();
    }

    /**
     * Start BLE advertising.
     *
     * @param client Advertise client.
     */
    void startAdvertising(AdvertiseClient client) {
        if (client == null) {
            return;
        }
        Message message = new Message();
        message.what = MSG_START_ADVERTISING;
        message.obj = client;
        mHandler.sendMessage(message);
    }

    /**
     * Stop BLE advertising.
     */
    void stopAdvertising(AdvertiseClient client) {
        if (client == null) {
            return;
        }
        Message message = new Message();
        message.what = MSG_STOP_ADVERTISING;
        message.obj = client;
        mHandler.sendMessage(message);
    }

    /**
     * Signals the callback is received.
     *
     * @param clientIf Identifier for the client.
     * @param status Status of the callback.
     */
    void callbackDone(int clientIf, int status) {
        if (status == AdvertiseCallback.ADVERTISE_SUCCESS) {
            mLatch.countDown();
        } else {
            // Note in failure case we'll wait for the latch to timeout(which takes 100ms) and
            // the mClientHandler thread will be blocked till timeout.
            postCallback(clientIf, AdvertiseCallback.ADVERTISE_FAILED_INTERNAL_ERROR);
        }
    }

    // Post callback status to app process.
    private void postCallback(int clientIf, int status) {
        try {
            AdvertiseClient client = getAdvertiseClient(clientIf);
            AdvertiseSettings settings = (client == null) ? null : client.settings;
            boolean isStart = true;
            mService.onMultipleAdvertiseCallback(clientIf, status, isStart, settings);
        } catch (RemoteException e) {
            loge("failed onMultipleAdvertiseCallback", e);
        }
    }

    private AdvertiseClient getAdvertiseClient(int clientIf) {
        for (AdvertiseClient client : mAdvertiseClients) {
            if (client.clientIf == clientIf) {
                return client;
            }
        }
        return null;
    }

    // Handler class that handles BLE advertising operations.
    private class ClientHandler extends Handler {

        ClientHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            logd("message : " + msg.what);
            AdvertiseClient client = (AdvertiseClient) msg.obj;
            switch (msg.what) {
                case MSG_START_ADVERTISING:
                    handleStartAdvertising(client);
                    break;
                case MSG_STOP_ADVERTISING:
                    handleStopAdvertising(client);
                    break;
                default:
                    // Shouldn't happen.
                    Log.e(TAG, "recieve an unknown message : " + msg.what);
                    break;
            }
        }

        private void handleStartAdvertising(AdvertiseClient client) {
            Utils.enforceAdminPermission(mService);
            int clientIf = client.clientIf;
            if (mAdvertiseClients.contains(clientIf)) {
                postCallback(clientIf, AdvertiseCallback.ADVERTISE_FAILED_ALREADY_STARTED);
                return;
            }

            if (mAdvertiseClients.size() >= maxAdvertiseInstances()) {
                postCallback(clientIf,
                        AdvertiseCallback.ADVERTISE_FAILED_TOO_MANY_ADVERTISERS);
                return;
            }
            if (!mAdvertiseNative.startAdverising(client)) {
                postCallback(clientIf, AdvertiseCallback.ADVERTISE_FAILED_INTERNAL_ERROR);
                return;
            }
            mAdvertiseClients.add(client);
            postCallback(clientIf, AdvertiseCallback.ADVERTISE_SUCCESS);
        }

        // Handles stop advertising.
        private void handleStopAdvertising(AdvertiseClient client) {
            Utils.enforceAdminPermission(mService);
            if (client == null) {
                return;
            }
            logd("stop advertise for client " + client.clientIf);
            mAdvertiseNative.stopAdvertising(client);
            if (client.appDied) {
                logd("app died - unregistering client : " + client.clientIf);
                mService.unregisterClient(client.clientIf);
            }
            if (mAdvertiseClients.contains(client)) {
                mAdvertiseClients.remove(client);
            }
        }

        // Returns maximum advertise instances supported by controller.
        int maxAdvertiseInstances() {
            // Note numOfAdvtInstances includes the standard advertising instance.
            // TODO: remove - 1 once the stack is able to include standard instance for multiple
            // advertising.
            if (mAdapterService.isMultiAdvertisementSupported()) {
                return mAdapterService.getNumOfAdvertisementInstancesSupported() - 1;
            }
            if (mAdapterService.isPeripheralModeSupported()) {
                return 1;
            }
            return 0;
        }
    }

    // Class that wraps advertise native related constants, methods etc.
    private class AdvertiseNative {
        // Advertise interval for different modes.
        private static final int ADVERTISING_INTERVAL_HIGH_MILLS = 1000;
        private static final int ADVERTISING_INTERVAL_MEDIUM_MILLS = 250;
        private static final int ADVERTISING_INTERVAL_LOW_MILLS = 100;

        // Add some randomness to the advertising min/max interval so the controller can do some
        // optimization.
        private static final int ADVERTISING_INTERVAL_DELTA_UNIT = 10;

        // The following constants should be kept the same as those defined in bt stack.
        private static final int ADVERTISING_CHANNEL_37 = 1 << 0;
        private static final int ADVERTISING_CHANNEL_38 = 1 << 1;
        private static final int ADVERTISING_CHANNEL_39 = 1 << 2;
        private static final int ADVERTISING_CHANNEL_ALL =
                ADVERTISING_CHANNEL_37 | ADVERTISING_CHANNEL_38 | ADVERTISING_CHANNEL_39;

        private static final int ADVERTISING_TX_POWER_MIN = 0;
        private static final int ADVERTISING_TX_POWER_LOW = 1;
        private static final int ADVERTISING_TX_POWER_MID = 2;
        private static final int ADVERTISING_TX_POWER_UPPER = 3;
        // Note this is not exposed to the Java API.
        private static final int ADVERTISING_TX_POWER_MAX = 4;

        // Note we don't expose connectable directed advertising to API.
        private static final int ADVERTISING_EVENT_TYPE_CONNECTABLE = 0;
        private static final int ADVERTISING_EVENT_TYPE_SCANNABLE = 2;
        private static final int ADVERTISING_EVENT_TYPE_NON_CONNECTABLE = 3;

        // TODO: Extract advertising logic into interface as we have multiple implementations now.
        boolean startAdverising(AdvertiseClient client) {
            if (!mAdapterService.isMultiAdvertisementSupported() &&
                    !mAdapterService.isPeripheralModeSupported()) {
                return false;
            }
            if (mAdapterService.isMultiAdvertisementSupported()) {
                return startMultiAdvertising(client);
            }
            return startSingleAdvertising(client);
        }

        boolean startMultiAdvertising(AdvertiseClient client) {
            logd("starting multi advertising");
            resetCountDownLatch();
            enableAdvertising(client);
            if (!waitForCallback()) {
                return false;
            }
            resetCountDownLatch();
            setAdvertisingData(client, client.advertiseData, false);
            if (!waitForCallback()) {
                return false;
            }
            if (client.scanResponse != null) {
                resetCountDownLatch();
                setAdvertisingData(client, client.scanResponse, true);
                if (!waitForCallback()) {
                    return false;
                }
            }
            return true;
        }

        boolean startSingleAdvertising(AdvertiseClient client) {
            logd("starting single advertising");
            resetCountDownLatch();
            enableAdvertising(client);
            if (!waitForCallback()) {
                return false;
            }
            setAdvertisingData(client, client.advertiseData, false);
            return true;
        }

        void stopAdvertising(AdvertiseClient client) {
            if (mAdapterService.isMultiAdvertisementSupported()) {
                gattClientDisableAdvNative(client.clientIf);
            } else {
                gattAdvertiseNative(client.clientIf, false);
                try {
                    mService.onAdvertiseInstanceDisabled(
                            AdvertiseCallback.ADVERTISE_SUCCESS, client.clientIf);
                } catch (RemoteException e) {
                    Log.d(TAG, "failed onAdvertiseInstanceDisabled", e);
                }
            }
        }

        private void resetCountDownLatch() {
            mLatch = new CountDownLatch(1);
        }

        // Returns true if mLatch reaches 0, false if timeout or interrupted.
        private boolean waitForCallback() {
            try {
                return mLatch.await(OPERATION_TIME_OUT_MILLIS, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return false;
            }
        }

        private void enableAdvertising(AdvertiseClient client) {
            int clientIf = client.clientIf;
            int minAdvertiseUnit = (int) getAdvertisingIntervalUnit(client.settings);
            int maxAdvertiseUnit = minAdvertiseUnit + ADVERTISING_INTERVAL_DELTA_UNIT;
            int advertiseEventType = getAdvertisingEventType(client);
            int txPowerLevel = getTxPowerLevel(client.settings);
            int advertiseTimeoutSeconds = (int) TimeUnit.MILLISECONDS.toSeconds(
                    client.settings.getTimeout());
            if (mAdapterService.isMultiAdvertisementSupported()) {
                gattClientEnableAdvNative(
                        clientIf,
                        minAdvertiseUnit, maxAdvertiseUnit,
                        advertiseEventType,
                        ADVERTISING_CHANNEL_ALL,
                        txPowerLevel,
                        advertiseTimeoutSeconds);
            } else {
                gattAdvertiseNative(client.clientIf, true);
            }
        }

        private void setAdvertisingData(AdvertiseClient client, AdvertiseData data,
                boolean isScanResponse) {
            if (data == null) {
                return;
            }
            boolean includeName = data.getIncludeDeviceName();
            boolean includeTxPower = data.getIncludeTxPowerLevel();
            int appearance = 0;
            byte[] manufacturerData = getManufacturerData(data);

            byte[] serviceData = getServiceData(data);
            byte[] serviceUuids;
            if (data.getServiceUuids() == null) {
                serviceUuids = new byte[0];
            } else {
                ByteBuffer advertisingUuidBytes = ByteBuffer.allocate(
                        data.getServiceUuids().size() * 16)
                        .order(ByteOrder.LITTLE_ENDIAN);
                for (ParcelUuid parcelUuid : data.getServiceUuids()) {
                    UUID uuid = parcelUuid.getUuid();
                    // Least significant bits first as the advertising UUID should be in
                    // little-endian.
                    advertisingUuidBytes.putLong(uuid.getLeastSignificantBits())
                            .putLong(uuid.getMostSignificantBits());
                }
                serviceUuids = advertisingUuidBytes.array();
            }
            if (mAdapterService.isMultiAdvertisementSupported()) {
                gattClientSetAdvDataNative(client.clientIf, isScanResponse, includeName,
                        includeTxPower, appearance,
                        manufacturerData, serviceData, serviceUuids);
            } else {
                gattSetAdvDataNative(client.clientIf, isScanResponse, includeName,
                        includeTxPower, 0, 0, appearance,
                        manufacturerData, serviceData, serviceUuids);
            }
        }

        // Combine manufacturer id and manufacturer data.
        private byte[] getManufacturerData(AdvertiseData advertiseData) {
            if (advertiseData.getManufacturerSpecificData().size() == 0) {
                return new byte[0];
            }
            int manufacturerId = advertiseData.getManufacturerSpecificData().keyAt(0);
            byte[] manufacturerData = advertiseData.getManufacturerSpecificData().get(
                    manufacturerId);
            int dataLen = 2 + (manufacturerData == null ? 0 : manufacturerData.length);
            byte[] concated = new byte[dataLen];
            // / First two bytes are manufacturer id in little-endian.
            concated[0] = (byte) (manufacturerId & 0xFF);
            concated[1] = (byte) ((manufacturerId >> 8) & 0xFF);
            if (manufacturerData != null) {
                System.arraycopy(manufacturerData, 0, concated, 2, manufacturerData.length);
            }
            return concated;
        }

        // Combine service UUID and service data.
        private byte[] getServiceData(AdvertiseData advertiseData) {
            if (advertiseData.getServiceData().isEmpty()) {
                return new byte[0];
            }
            ParcelUuid uuid = advertiseData.getServiceData().keySet().iterator().next();
            byte[] serviceData = advertiseData.getServiceData().get(uuid);
            int dataLen = 2 + (serviceData == null ? 0 : serviceData.length);
            byte[] concated = new byte[dataLen];
            // Extract 16 bit UUID value.
            int uuidValue = BluetoothUuid.getServiceIdentifierFromParcelUuid(
                    uuid);
            // First two bytes are service data UUID in little-endian.
            concated[0] = (byte) (uuidValue & 0xFF);
            concated[1] = (byte) ((uuidValue >> 8) & 0xFF);
            if (serviceData != null) {
                System.arraycopy(serviceData, 0, concated, 2, serviceData.length);
            }
            return concated;
        }

        // Convert settings tx power level to stack tx power level.
        private int getTxPowerLevel(AdvertiseSettings settings) {
            switch (settings.getTxPowerLevel()) {
                case AdvertiseSettings.ADVERTISE_TX_POWER_ULTRA_LOW:
                    return ADVERTISING_TX_POWER_MIN;
                case AdvertiseSettings.ADVERTISE_TX_POWER_LOW:
                    return ADVERTISING_TX_POWER_LOW;
                case AdvertiseSettings.ADVERTISE_TX_POWER_MEDIUM:
                    return ADVERTISING_TX_POWER_MID;
                case AdvertiseSettings.ADVERTISE_TX_POWER_HIGH:
                    return ADVERTISING_TX_POWER_UPPER;
                default:
                    // Shouldn't happen, just in case.
                    return ADVERTISING_TX_POWER_MID;
            }
        }

        // Convert advertising event type to stack values.
        private int getAdvertisingEventType(AdvertiseClient client) {
            AdvertiseSettings settings = client.settings;
            if (settings.isConnectable()) {
                return ADVERTISING_EVENT_TYPE_CONNECTABLE;
            }
            return client.scanResponse == null ? ADVERTISING_EVENT_TYPE_NON_CONNECTABLE
                    : ADVERTISING_EVENT_TYPE_SCANNABLE;
        }

        // Convert advertising milliseconds to advertising units(one unit is 0.625 millisecond).
        private long getAdvertisingIntervalUnit(AdvertiseSettings settings) {
            switch (settings.getMode()) {
                case AdvertiseSettings.ADVERTISE_MODE_LOW_POWER:
                    return Utils.millsToUnit(ADVERTISING_INTERVAL_HIGH_MILLS);
                case AdvertiseSettings.ADVERTISE_MODE_BALANCED:
                    return Utils.millsToUnit(ADVERTISING_INTERVAL_MEDIUM_MILLS);
                case AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY:
                    return Utils.millsToUnit(ADVERTISING_INTERVAL_LOW_MILLS);
                default:
                    // Shouldn't happen, just in case.
                    return Utils.millsToUnit(ADVERTISING_INTERVAL_HIGH_MILLS);
            }
        }

        // Native functions
        private native void gattClientDisableAdvNative(int client_if);

        private native void gattClientEnableAdvNative(int client_if,
                int min_interval, int max_interval, int adv_type, int chnl_map,
                int tx_power, int timeout_s);

        private native void gattClientUpdateAdvNative(int client_if,
                int min_interval, int max_interval, int adv_type, int chnl_map,
                int tx_power, int timeout_s);

        private native void gattClientSetAdvDataNative(int client_if,
                boolean set_scan_rsp, boolean incl_name, boolean incl_txpower, int appearance,
                byte[] manufacturer_data, byte[] service_data, byte[] service_uuid);

        private native void gattSetAdvDataNative(int serverIf, boolean setScanRsp, boolean inclName,
                boolean inclTxPower, int minSlaveConnectionInterval, int maxSlaveConnectionInterval,
                int appearance, byte[] manufacturerData, byte[] serviceData, byte[] serviceUuid);

        private native void gattAdvertiseNative(int client_if, boolean start);
    }

    private void logd(String s) {
        if (DBG) {
            Log.d(TAG, s);
        }
    }

    private void loge(String s, Exception e) {
        Log.e(TAG, s, e);
    }

}
