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

package com.android.cts.verifier.bluetooth;

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;

import java.util.ArrayList;
import java.util.List;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class BleClientStartActivity extends PassFailButtons.Activity {

    private TestAdapter mTestAdapter;
    private int mAllPassed;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.ble_server_start);
        setPassFailButtonClickListeners();
        setInfoResources(R.string.ble_server_start_name,
                         R.string.ble_server_start_info, -1);
        getPassButton().setEnabled(false);

        mTestAdapter = new TestAdapter(this, setupTestList());
        ListView listView = (ListView) findViewById(R.id.ble_server_tests);
        listView.setAdapter(mTestAdapter);

        mAllPassed = 0;
        startService(new Intent(this, BleClientService.class));
    }

    @Override
    public void onResume() {
        super.onResume();

        IntentFilter filter = new IntentFilter();
        filter.addAction(BleClientService.BLE_BLUETOOTH_CONNECTED);
        filter.addAction(BleClientService.BLE_BLUETOOTH_DISCONNECTED);
        filter.addAction(BleClientService.BLE_SERVICES_DISCOVERED);
        filter.addAction(BleClientService.BLE_CHARACTERISTIC_READ);
        filter.addAction(BleClientService.BLE_CHARACTERISTIC_WRITE);
        filter.addAction(BleClientService.BLE_CHARACTERISTIC_CHANGED);
        filter.addAction(BleClientService.BLE_DESCRIPTOR_READ);
        filter.addAction(BleClientService.BLE_DESCRIPTOR_WRITE);
        filter.addAction(BleClientService.BLE_RELIABLE_WRITE_COMPLETED);
        filter.addAction(BleClientService.BLE_READ_REMOTE_RSSI);
        registerReceiver(onBroadcast, filter);
    }

    @Override
    public void onPause() {
        super.onPause();
        unregisterReceiver(onBroadcast);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        stopService(new Intent(this, BleClientService.class));
    }

    private List<Integer> setupTestList() {
        ArrayList<Integer> testList = new ArrayList<Integer>();
        testList.add(R.string.ble_client_connect_name);
        testList.add(R.string.ble_discover_service_name);
        testList.add(R.string.ble_read_characteristic_name);
        testList.add(R.string.ble_write_characteristic_name);
        testList.add(R.string.ble_reliable_write_name);
        testList.add(R.string.ble_notify_characteristic_name);
        testList.add(R.string.ble_read_descriptor_name);
        testList.add(R.string.ble_write_descriptor_name);
        testList.add(R.string.ble_read_rssi_name);
        testList.add(R.string.ble_client_disconnect_name);
        return testList;
    }

    private BroadcastReceiver onBroadcast = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action == BleClientService.BLE_BLUETOOTH_CONNECTED) {
                mTestAdapter.setTestPass(0);
                mAllPassed |= 0x01;
            } else if (action == BleClientService.BLE_SERVICES_DISCOVERED) {
                mTestAdapter.setTestPass(1);
                mAllPassed |= 0x02;
            } else if (action == BleClientService.BLE_CHARACTERISTIC_READ) {
                mTestAdapter.setTestPass(2);
                mAllPassed |= 0x04;
            } else if (action == BleClientService.BLE_CHARACTERISTIC_WRITE) {
                mTestAdapter.setTestPass(3);
                mAllPassed |= 0x08;
            } else if (action == BleClientService.BLE_RELIABLE_WRITE_COMPLETED) {
                mTestAdapter.setTestPass(4);
                mAllPassed |= 0x10;
            } else if (action == BleClientService.BLE_CHARACTERISTIC_CHANGED) {
                mTestAdapter.setTestPass(5);
                mAllPassed |= 0x20;
            } else if (action == BleClientService.BLE_DESCRIPTOR_READ) {
                mTestAdapter.setTestPass(6);
                mAllPassed |= 0x40;
            } else if (action == BleClientService.BLE_DESCRIPTOR_WRITE) {
                mTestAdapter.setTestPass(7);
                mAllPassed |= 0x80;
            } else if (action == BleClientService.BLE_READ_REMOTE_RSSI) {
                mTestAdapter.setTestPass(8);
                mAllPassed |= 0x100;
            } else if (action == BleClientService.BLE_BLUETOOTH_DISCONNECTED) {
                mTestAdapter.setTestPass(9);
                mAllPassed |= 0x200;
            }
            mTestAdapter.notifyDataSetChanged();
            if (mAllPassed == 0x3FF) getPassButton().setEnabled(true);
        }
    };

}
