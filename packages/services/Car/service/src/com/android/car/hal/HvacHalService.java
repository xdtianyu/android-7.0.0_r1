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

import static com.android.car.hal.CarPropertyUtils.toCarPropertyValue;
import static com.android.car.hal.CarPropertyUtils.toVehiclePropValue;
import static java.lang.Integer.toHexString;

import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.hvac.CarHvacEvent;
import android.car.hardware.hvac.CarHvacManager.HvacPropertyId;
import android.os.ServiceSpecificException;
import android.util.Log;
import android.util.SparseIntArray;

import com.android.car.CarLog;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

public class HvacHalService extends HalServiceBase {
    private static final boolean   DBG = true;
    private static final String    TAG = CarLog.TAG_HVAC + ".HvacHalService";
    private HvacHalListener        mListener;
    private final VehicleHal       mVehicleHal;

    private final HashMap<Integer, CarPropertyConfig<?>> mProps = new HashMap<>();
    private final SparseIntArray mHalPropToValueType = new SparseIntArray();

    public interface HvacHalListener {
        void onPropertyChange(CarHvacEvent event);
        void onError(int zone, int property);
    }

    public HvacHalService(VehicleHal vehicleHal) {
        mVehicleHal = vehicleHal;
        if (DBG) {
            Log.d(TAG, "started HvacHalService!");
        }
    }

    public void setListener(HvacHalListener listener) {
        synchronized (this) {
            mListener = listener;
        }
    }

    public List<CarPropertyConfig> getHvacProperties() {
        List<CarPropertyConfig> propList;
        synchronized (mProps) {
            propList = new ArrayList<>(mProps.values());
        }
        return propList;
    }

    public CarPropertyValue getHvacProperty(int hvacPropertyId, int areaId) {
        int halProp = hvacToHalPropId(hvacPropertyId);

        VehiclePropValue value = null;
        try {
            VehiclePropValue valueRequest = VehiclePropValue.newBuilder()
                    .setProp(halProp)
                    .setZone(areaId)
                    .setValueType(mHalPropToValueType.get(halProp))
                    .build();

            value = mVehicleHal.getVehicleNetwork().getProperty(valueRequest);
        } catch (ServiceSpecificException e) {
            Log.e(CarLog.TAG_HVAC, "property not ready 0x" + toHexString(halProp), e);
        }

        return value == null ? null : toCarPropertyValue(value, hvacPropertyId);
    }

    public void setHvacProperty(CarPropertyValue prop) {
        VehiclePropValue halProp = toVehiclePropValue(prop, hvacToHalPropId(prop.getPropertyId()));
        mVehicleHal.getVehicleNetwork().setProperty(halProp);
    }

    @Override
    public void init() {
        if (DBG) {
            Log.d(TAG, "init()");
        }
        synchronized (mProps) {
            // Subscribe to each of the HVAC properties
            for (Integer prop : mProps.keySet()) {
                mVehicleHal.subscribeProperty(this, prop, 0);
            }
        }
    }

    @Override
    public void release() {
        if (DBG) {
            Log.d(TAG, "release()");
        }
        synchronized (mProps) {
            for (Integer prop : mProps.keySet()) {
                mVehicleHal.unsubscribeProperty(this, prop);
            }

            // Clear the property list
            mProps.clear();
        }
        mListener = null;
    }

    @Override
    public synchronized List<VehiclePropConfig> takeSupportedProperties(
            List<VehiclePropConfig> allProperties) {
        List<VehiclePropConfig> taken = new LinkedList<>();

        for (VehiclePropConfig p : allProperties) {
            int hvacPropId;
            try {
                hvacPropId = halToHvacPropId(p.getProp());
            } catch (IllegalArgumentException e) {
                Log.i(TAG, "Property not supported by HVAC: 0x" + toHexString(p.getProp()));
                continue;
            }
            CarPropertyConfig hvacConfig = CarPropertyUtils.toCarPropertyConfig(p, hvacPropId);

            taken.add(p);
            mProps.put(p.getProp(), hvacConfig);
            mHalPropToValueType.put(p.getProp(), p.getValueType());

            if (DBG) {
                Log.d(TAG, "takeSupportedProperties:  " + toHexString(p.getProp()));
            }
        }
        return taken;
    }

    @Override
    public void handleHalEvents(List<VehiclePropValue> values) {
        HvacHalListener listener;
        synchronized (this) {
            listener = mListener;
        }
        if (listener != null) {
            dispatchEventToListener(listener, values);
        }
    }

    private void dispatchEventToListener(HvacHalListener listener, List<VehiclePropValue> values) {
        for (VehiclePropValue v : values) {
            int prop = v.getProp();

            int hvacPropId;
            try {
                hvacPropId = halToHvacPropId(prop);
            } catch (IllegalArgumentException ex) {
                Log.e(TAG, "Property is not supported: 0x" + toHexString(prop), ex);
                continue;
            }

            CarHvacEvent event;
            CarPropertyValue<?> hvacProperty = toCarPropertyValue(v, hvacPropId);
            event = new CarHvacEvent(CarHvacEvent.HVAC_EVENT_PROPERTY_CHANGE, hvacProperty);

            listener.onPropertyChange(event);
            if (DBG) {
                Log.d(TAG, "handleHalEvents event: " + event);
            }
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*HVAC HAL*");
        writer.println("  Properties available:");
        for (CarPropertyConfig prop : mProps.values()) {
            writer.println("    " + prop.toString());
        }
    }

    // Convert the HVAC public API property ID to HAL property ID
    private static int hvacToHalPropId(int hvacPropId) {
        switch (hvacPropId) {
            case HvacPropertyId.ZONED_FAN_SPEED_SETPOINT:
                return VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_FAN_SPEED;
            case HvacPropertyId.ZONED_FAN_POSITION:
                return VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_FAN_DIRECTION;
            case HvacPropertyId.ZONED_TEMP_ACTUAL:
                return VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_TEMPERATURE_CURRENT;
            case HvacPropertyId.ZONED_TEMP_SETPOINT:
                return VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_TEMPERATURE_SET;
            case HvacPropertyId.WINDOW_DEFROSTER_ON:
                return VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_DEFROSTER;
            case HvacPropertyId.ZONED_AC_ON:
                return VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_AC_ON;
            case HvacPropertyId.ZONED_AIR_RECIRCULATION_ON:
                return VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_RECIRC_ON;
            default:
                throw new IllegalArgumentException("hvacPropId " + hvacPropId + " is not supported");
        }
    }

    // Convert he HAL specific property ID to HVAC public API
    private static int halToHvacPropId(int halPropId) {
        switch (halPropId) {
            case VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_FAN_SPEED:
                return HvacPropertyId.ZONED_FAN_SPEED_SETPOINT;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_FAN_DIRECTION:
                return HvacPropertyId.ZONED_FAN_POSITION;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_TEMPERATURE_CURRENT:
                return HvacPropertyId.ZONED_TEMP_ACTUAL;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_TEMPERATURE_SET:
                return HvacPropertyId.ZONED_TEMP_SETPOINT;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_DEFROSTER:
                return HvacPropertyId.WINDOW_DEFROSTER_ON;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_AC_ON:
                return HvacPropertyId.ZONED_AC_ON;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_HVAC_RECIRC_ON:
                return HvacPropertyId.ZONED_AIR_RECIRCULATION_ON;
            default:
                throw new IllegalArgumentException("halPropId " + halPropId + " is not supported");
        }
    }
}
