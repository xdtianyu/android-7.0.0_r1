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
 * limitations under the License.
 */
package com.android.car.hal;

import android.util.Log;
import android.view.KeyEvent;

import com.android.car.CarLog;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleDisplay;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleHwKeyInputAction;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;

import java.io.PrintWriter;
import java.util.LinkedList;
import java.util.List;

public class InputHalService extends HalServiceBase {

    public static final int DISPLAY_MAIN = VehicleDisplay.VEHICLE_DISPLAY_MAIN;
    public static final int DISPLAY_INSTRUMENT_CLUSTER =
            VehicleDisplay.VEHICLE_DISPLAY_INSTRUMENT_CLUSTER;

    public interface InputListener {
        void onKeyEvent(KeyEvent event, int targetDisplay);
    }

    private static final boolean DBG = false;

    private boolean mKeyInputSupported = false;
    private InputListener mListener;

    public void setInputListener(InputListener listener) {
        synchronized (this) {
            if (!mKeyInputSupported) {
                Log.w(CarLog.TAG_INPUT, "input listener set while key input not supported");
                return;
            }
            mListener = listener;
        }
        VehicleHal.getInstance().subscribeProperty(this,
                VehicleNetworkConsts.VEHICLE_PROPERTY_HW_KEY_INPUT, 0);
    }

    public synchronized boolean isKeyInputSupported() {
        return mKeyInputSupported;
    }

    @Override
    public void init() {
    }

    @Override
    public void release() {
        synchronized (this) {
            mListener = null;
            mKeyInputSupported = false;
        }
    }

    @Override
    public List<VehiclePropConfig> takeSupportedProperties(List<VehiclePropConfig> allProperties) {
        List<VehiclePropConfig> supported = new LinkedList<VehiclePropConfig>();
        for (VehiclePropConfig p: allProperties) {
            if (p.getProp() == VehicleNetworkConsts.VEHICLE_PROPERTY_HW_KEY_INPUT) {
                supported.add(p);
                synchronized (this) {
                    mKeyInputSupported = true;
                }
            }
        }
        return supported;
    }

    @Override
    public void handleHalEvents(List<VehiclePropValue> values) {
        InputListener listener = null;
        synchronized (this) {
            listener = mListener;
        }
        if (listener == null) {
            Log.w(CarLog.TAG_INPUT, "Input event while listener is null");
            return;
        }
        for (VehiclePropValue v : values) {
            if (v.getProp() != VehicleNetworkConsts.VEHICLE_PROPERTY_HW_KEY_INPUT) {
                Log.e(CarLog.TAG_INPUT, "Wrong event dispatched, prop:0x" +
                        Integer.toHexString(v.getProp()));
                continue;
            }
            int action = (v.getInt32Values(0) ==
                    VehicleHwKeyInputAction.VEHICLE_HW_KEY_INPUT_ACTION_DOWN) ?
                            KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP;
            int code = v.getInt32Values(1);
            int display = v.getInt32Values(2);
            if (DBG) {
                Log.i(CarLog.TAG_INPUT, "hal event code:" + code + ",action:" + action +
                        ",display:" + display);
            }
            KeyEvent event = new KeyEvent(action, code);
            listener.onKeyEvent(event, display);
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*Input HAL*");
        writer.println("mKeyInputSupported:" + mKeyInputSupported);
    }

}
