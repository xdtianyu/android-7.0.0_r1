/*
 * Copyright (C) 2015 The Android Open Source Project
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

import android.car.hardware.radio.CarRadioEvent;
import android.car.hardware.radio.CarRadioPreset;
import android.os.ServiceSpecificException;
import android.util.Log;
import android.hardware.radio.RadioManager;

import com.android.car.CarLog;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.android.car.vehiclenetwork.VehiclePropValueUtil;

import java.util.LinkedList;
import java.util.List;
import java.io.PrintWriter;

/**
 * This class exposes the Radio related features in the HAL layer.
 *
 * The current set of features support radio presets. The rest of the radio functionality is already
 * covered under RadioManager API.
 */
public class RadioHalService extends HalServiceBase {
    public static boolean DBG = true;
    public static String TAG = CarLog.TAG_HAL + ".RadioHalService";

    private int mPresetCount = 0;
    private VehicleHal mHal;
    private RadioListener mListener;

    public interface RadioListener {
        public void onEvent(CarRadioEvent event);
    }

    public RadioHalService(VehicleHal hal) {
        mHal = hal;
    }

    @Override
    public synchronized void init() {
    }

    @Override
    public synchronized void release() {
        mListener = null;
    }

    @Override
    public synchronized List<VehiclePropConfig> takeSupportedProperties(
            List<VehiclePropConfig> allProperties) {
        List<VehiclePropConfig> supported = new LinkedList<VehiclePropConfig>();
        for (VehiclePropConfig p : allProperties) {
            if (handleRadioProperty(p)) {
                supported.add(p);
            }
        }
        return supported;
    }

    @Override
    public void handleHalEvents(List<VehiclePropValue> values) {
        if (DBG) {
            Log.d(TAG, "handleHalEvents");
        }
        RadioHalService.RadioListener radioListener = null;
        synchronized (this) {
            radioListener = mListener;
        }

        if (radioListener == null) {
            Log.e(TAG, "radio listener is null, ignoring event: " + values);
            return;
        }

        for (VehiclePropValue v : values) {
            CarRadioEvent radioEvent = createCarRadioEvent(v);
            if (radioEvent != null) {
                if (DBG) {
                    Log.d(TAG, "Sending event to listener: " + radioEvent);
                }
                radioListener.onEvent(radioEvent);
            } else {
                Log.w(TAG, "Value conversion failed: " + v);
            }
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*RadioHal*");
        writer.println("**Supported properties**");
        writer.println(VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET);
        if (mListener != null) {
            writer.println("Hal service registered.");
        }
    }

    public synchronized void registerListener(RadioListener listener) {
        if (DBG) {
            Log.d(TAG, "registerListener");
        }
        mListener = listener;

        // Subscribe to all radio properties.
        mHal.subscribeProperty(this, VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET, 0);
    }

    public synchronized void unregisterListener() {
        if (DBG) {
            Log.d(TAG, "unregisterListener");
        }
        mListener = null;

        // Unsubscribe from all propreties.
        mHal.unsubscribeProperty(this, VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET);
    }

    public synchronized int getPresetCount() {
        Log.d(TAG, "get preset count: " + mPresetCount);
        return mPresetCount;
    }

    public CarRadioPreset getRadioPreset(int presetNumber) {
        // Check if the preset number is out of range. We should return NULL if that is the case.
        if (DBG) {
            Log.d(TAG, "getRadioPreset called with preset number " + presetNumber);
        }
        if (!isValidPresetNumber(presetNumber)) {
            throw new IllegalArgumentException("Preset number not valid: " + presetNumber);
        }

        int[] presetArray = {presetNumber, 0, 0, 0};
        VehiclePropValue presetNumberValue =
            VehiclePropValueUtil.createIntVectorValue(
                VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET, presetArray, 0);

        VehiclePropValue presetConfig;
        try {
            presetConfig = mHal.getVehicleNetwork().getProperty(presetNumberValue);
        } catch (ServiceSpecificException e) {
            Log.e(TAG, "property VEHICLE_PROPERTY_RADIO_PRESET not ready");
            return null;
        }
        // Sanity check the output from HAL.
        if (presetConfig.getInt32ValuesCount() != 4) {
            Log.e(TAG, "Return value does not have 4 elements: " +
                presetConfig.getInt32ValuesList());
            throw new IllegalStateException(
                "Invalid preset returned from service: " + presetConfig.getInt32ValuesList());
        }

        int retPresetNumber = presetConfig.getInt32Values(0);
        int retBand = presetConfig.getInt32Values(1);
        int retChannel = presetConfig.getInt32Values(2);
        int retSubChannel = presetConfig.getInt32Values(3);
        if (retPresetNumber != presetNumber) {
            Log.e(TAG, "Preset number is not the same: " + presetNumber + " vs " + retPresetNumber);
            return null;
        }
        if (!isValidBand(retBand)) return null;

        // Return the actual config.
        CarRadioPreset retConfig =
            new CarRadioPreset(retPresetNumber, retBand, retChannel, retSubChannel);
        if (DBG) {
            Log.d(TAG, "Preset obtained: " + retConfig);
        }
        return retConfig;
    }

    public boolean setRadioPreset(CarRadioPreset preset) {
        if (DBG) {
            Log.d(TAG, "setRadioPreset with config " + preset);
        }

        if (!isValidPresetNumber(preset.getPresetNumber()) ||
            !isValidBand(preset.getBand())) {
            return false;
        }

        VehiclePropValue setPresetValue =
            VehiclePropValueUtil.createBuilder(
                VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET,
                VehicleValueType.VEHICLE_VALUE_TYPE_INT32_VEC4, 0)
            .addInt32Values(preset.getPresetNumber())
            .addInt32Values(preset.getBand())
            .addInt32Values(preset.getChannel())
            .addInt32Values(preset.getSubChannel())
            .build();
        mHal.getVehicleNetwork().setProperty(setPresetValue);
        return true;
    }

    private boolean isValidPresetNumber(int presetNumber) {
        // Check for preset number.
        if (presetNumber < VehicleNetworkConsts.VehicleRadioConsts.VEHICLE_RADIO_PRESET_MIN_VALUE
            || presetNumber > mPresetCount) {
            Log.e(TAG, "Preset number not in range (1, " + mPresetCount + ") - " + presetNumber);
            return false;
        }
        return true;
    }

    private boolean isValidBand(int band) {
        // Check for band info.
        if (band != RadioManager.BAND_AM &&
            band != RadioManager.BAND_FM &&
            band != RadioManager.BAND_FM_HD &&
            band != RadioManager.BAND_AM_HD) {
            Log.e(TAG, "Preset band is not valid: " + band);
            return false;
        }
        return true;
    }

    private boolean handleRadioProperty(VehiclePropConfig property) {
        switch (property.getProp()) {
            case VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET:
                // Extract the count of presets.
                mPresetCount = property.getConfigArray(0);
                Log.d(TAG, "Read presets count: " + mPresetCount);
                return true;
            default:
                return false;
        }
        // Should never come here.
    }

    private CarRadioEvent createCarRadioEvent(VehiclePropValue v) {
        switch (v.getProp()) {
            case VehicleNetworkConsts.VEHICLE_PROPERTY_RADIO_PRESET:
                if (v.getInt32ValuesCount() != 4) {
                    Log.e(TAG, "Returned a wrong array size: " + v.getInt32ValuesCount());
                    return null;
                }

                Integer intValues[] = new Integer[4];
                v.getInt32ValuesList().toArray(intValues);

                // Verify the correctness of the values.
                if (!isValidPresetNumber(intValues[0]) && !isValidBand(intValues[1])) {
                    return null;
                }

                CarRadioPreset preset =
                    new CarRadioPreset(intValues[0], intValues[1], intValues[2], intValues[3]);
                CarRadioEvent event = new CarRadioEvent(CarRadioEvent.RADIO_PRESET, preset);
                return event;
            default:
                Log.e(TAG, "createCarRadioEvent: Value not supported as event: " + v);
                return null;
        }
    }
}
