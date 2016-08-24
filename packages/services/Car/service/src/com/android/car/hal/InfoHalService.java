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

import android.car.CarInfoManager;
import android.util.Log;

import com.android.car.CarLog;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;

import java.io.PrintWriter;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

public class InfoHalService extends HalServiceBase {

    private final VehicleHal mHal;
    private final HashMap<String, VehiclePropConfig> mInfoNameToHalPropertyMap =
            new HashMap<String, VehiclePropConfig>();

    public InfoHalService(VehicleHal hal) {
        mHal = hal;
    }

    @Override
    public void init() {
        //nothing to do
    }

    @Override
    public synchronized void release() {
        mInfoNameToHalPropertyMap.clear();
    }

    @Override
    public synchronized List<VehiclePropConfig> takeSupportedProperties(
            List<VehiclePropConfig> allProperties) {
        List<VehiclePropConfig> supported = new LinkedList<VehiclePropConfig>();
        for (VehiclePropConfig p: allProperties) {
            String infoName = getInfoStringFromProperty(p.getProp());
            if (infoName != null) {
                supported.add(p);
                mInfoNameToHalPropertyMap.put(infoName, p);
            }
        }
        return supported;
    }

    @Override
    public void handleHalEvents(List<VehiclePropValue> values) {
        for (VehiclePropValue v : values) {
            logUnexpectedEvent(v.getProp());
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*InfoHal*");
        writer.println("**Supported properties**");
        for (VehiclePropConfig p : mInfoNameToHalPropertyMap.values()) {
            //TODO fix toString
            writer.println(p.toString());
        }
    }

    public int[] getInt(String key) {
        VehiclePropConfig prop = getHalPropertyFromInfoString(key);
        if (prop == null) {
            return null;
        }
        // no lock here as get can take time and multiple get should be possible.
        int v = mHal.getVehicleNetwork().getIntProperty(prop.getProp());
        return new int[] { v };
    }

    public long[] getLong(String key) {
        VehiclePropConfig prop = getHalPropertyFromInfoString(key);
        if (prop == null) {
            return null;
        }
        // no lock here as get can take time and multiple get should be possible.
        long v = mHal.getVehicleNetwork().getLongProperty(prop.getProp());
        return new long[] { v };
    }

    public float[] getFloat(String key) {
        VehiclePropConfig prop = getHalPropertyFromInfoString(key);
        if (prop == null) {
            return null;
        }
        // no lock here as get can take time and multiple get should be possible.
        float v = mHal.getVehicleNetwork().getFloatProperty(prop.getProp());
        return new float[] { v };
    }

    public String getString(String key) {
        VehiclePropConfig prop = getHalPropertyFromInfoString(key);
        if (prop == null) {
            return null;
        }
        // no lock here as get can take time and multiple get should be possible.
        return mHal.getVehicleNetwork().getStringProperty(prop.getProp());
    }

    private synchronized VehiclePropConfig getHalPropertyFromInfoString(String key) {
        return mInfoNameToHalPropertyMap.get(key);
    }

    private void logUnexpectedEvent(int property) {
       Log.w(CarLog.TAG_INFO, "unexpected HAL event for property 0x" +
               Integer.toHexString(property));
    }

    private static String getInfoStringFromProperty(int property) {
        switch (property) {
            case VehicleNetworkConsts.VEHICLE_PROPERTY_INFO_MAKE:
                return CarInfoManager.KEY_MANUFACTURER;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_INFO_MODEL:
                return CarInfoManager.KEY_MODEL;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_INFO_MODEL_YEAR:
                return CarInfoManager.KEY_MODEL_YEAR;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_INFO_VIN:
                return CarInfoManager.KEY_VEHICLE_ID;
            //TODO add more properties
            default:
                return null;
        }
    }
}
