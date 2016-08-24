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

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;

import java.util.ArrayList;
import java.util.List;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class BleServerStartActivity extends PassFailButtons.Activity {

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
        startService(new Intent(this, BleServerService.class));
    }

    @Override
    public void onResume() {
        super.onResume();

        IntentFilter filter = new IntentFilter();
        filter.addAction(BleServerService.BLE_SERVICE_ADDED);
        filter.addAction(BleServerService.BLE_SERVER_CONNECTED);
        filter.addAction(BleServerService.BLE_CHARACTERISTIC_READ_REQUEST);
        filter.addAction(BleServerService.BLE_CHARACTERISTIC_WRITE_REQUEST);
        filter.addAction(BleServerService.BLE_DESCRIPTOR_READ_REQUEST);
        filter.addAction(BleServerService.BLE_DESCRIPTOR_WRITE_REQUEST);
        filter.addAction(BleServerService.BLE_EXECUTE_WRITE);
        filter.addAction(BleServerService.BLE_SERVER_DISCONNECTED);
        filter.addAction(BleServerService.BLE_OPEN_FAIL);
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
        stopService(new Intent(this, BleServerService.class));
    }

    private List<Integer> setupTestList() {
        ArrayList<Integer> testList = new ArrayList<Integer>();
        testList.add(R.string.ble_server_add_service);
        testList.add(R.string.ble_server_receiving_connect);
        testList.add(R.string.ble_server_read_characteristic);
        testList.add(R.string.ble_server_write_characteristic);
        testList.add(R.string.ble_server_read_descriptor);
        testList.add(R.string.ble_server_write_descriptor);
        testList.add(R.string.ble_server_reliable_write);
        testList.add(R.string.ble_server_receiving_disconnect);
        return testList;
    }

    private BroadcastReceiver onBroadcast = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action == BleServerService.BLE_SERVICE_ADDED) {
                mTestAdapter.setTestPass(0);
                mAllPassed |= 0x01;
            } else if (action == BleServerService.BLE_SERVER_CONNECTED) {
                mTestAdapter.setTestPass(1);
                mAllPassed |= 0x02;
            } else if (action == BleServerService.BLE_CHARACTERISTIC_READ_REQUEST) {
                mTestAdapter.setTestPass(2);
                mAllPassed |= 0x04;
            } else if (action == BleServerService.BLE_CHARACTERISTIC_WRITE_REQUEST) {
                mTestAdapter.setTestPass(3);
                mAllPassed |= 0x08;
            } else if (action == BleServerService.BLE_DESCRIPTOR_READ_REQUEST) {
                mTestAdapter.setTestPass(4);
                mAllPassed |= 0x10;
            } else if (action == BleServerService.BLE_DESCRIPTOR_WRITE_REQUEST) {
                mTestAdapter.setTestPass(5);
                mAllPassed |= 0x20;
            } else if (action == BleServerService.BLE_EXECUTE_WRITE) {
                mTestAdapter.setTestPass(6);
                mAllPassed |= 0x40;
            } else if (action == BleServerService.BLE_SERVER_DISCONNECTED) {
                mTestAdapter.setTestPass(7);
                mAllPassed |= 0x80;
            } else if (action == BleServerService.BLE_OPEN_FAIL) {
                setTestResultAndFinish(false);
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(BleServerStartActivity.this, "Cannot open GattService",
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
            mTestAdapter.notifyDataSetChanged();
            if (mAllPassed == 0xFF) getPassButton().setEnabled(true);
        }
    };
}
