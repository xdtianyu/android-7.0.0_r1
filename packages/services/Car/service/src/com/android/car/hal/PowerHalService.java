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

import android.util.Log;

import com.android.car.CarLog;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerSetState;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerState;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerStateConfigFlag;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerStateIndex;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleApPowerStateShutdownParam;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.android.internal.annotations.VisibleForTesting;

import java.io.PrintWriter;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

public class PowerHalService extends HalServiceBase {

    public static final int STATE_OFF = VehicleApPowerState.VEHICLE_AP_POWER_STATE_OFF;
    public static final int STATE_DEEP_SLEEP =
            VehicleApPowerState.VEHICLE_AP_POWER_STATE_DEEP_SLEEP;
    public static final int STATE_ON_DISP_OFF =
            VehicleApPowerState.VEHICLE_AP_POWER_STATE_ON_DISP_OFF;
    public static final int STATE_ON_FULL = VehicleApPowerState.VEHICLE_AP_POWER_STATE_ON_FULL;
    public static final int STATE_SHUTDOWN_PREPARE =
            VehicleApPowerState.VEHICLE_AP_POWER_STATE_SHUTDOWN_PREPARE;

    @VisibleForTesting
    public static final int SET_BOOT_COMPLETE =
            VehicleApPowerSetState.VEHICLE_AP_POWER_SET_BOOT_COMPLETE;
    @VisibleForTesting
    public static final int SET_DEEP_SLEEP_ENTRY =
            VehicleApPowerSetState.VEHICLE_AP_POWER_SET_DEEP_SLEEP_ENTRY;
    @VisibleForTesting
    public static final int SET_DEEP_SLEEP_EXIT =
            VehicleApPowerSetState.VEHICLE_AP_POWER_SET_DEEP_SLEEP_EXIT;
    @VisibleForTesting
    public static final int SET_SHUTDOWN_POSTPONE =
            VehicleApPowerSetState.VEHICLE_AP_POWER_SET_SHUTDOWN_POSTPONE;
    @VisibleForTesting
    public static final int SET_SHUTDOWN_START =
            VehicleApPowerSetState.VEHICLE_AP_POWER_SET_SHUTDOWN_START;
    @VisibleForTesting
    public static final int SET_DISPLAY_ON = VehicleApPowerSetState.VEHICLE_AP_POWER_SET_DISPLAY_ON;
    @VisibleForTesting
    public static final int SET_DISPLAY_OFF =
            VehicleApPowerSetState.VEHICLE_AP_POWER_SET_DISPLAY_OFF;

    @VisibleForTesting
    public static final int FLAG_SHUTDOWN_PARAM_CAN_SLEEP =
            VehicleApPowerStateShutdownParam.VEHICLE_AP_POWER_SHUTDOWN_PARAM_CAN_SLEEP;
    @VisibleForTesting
    public static final int FLAG_SHUTDOWN_IMMEDIATELY =
            VehicleApPowerStateShutdownParam.VEHICLE_AP_POWER_SHUTDOWN_PARAM_SHUTDOWN_IMMEDIATELY;

    public interface PowerEventListener {
        /**
         * Received power state change event.
         * @param state One of STATE_*
         * @param param
         */
        void onApPowerStateChange(PowerState state);
        /**
         * Received display brightness change event.
         * @param brightness in percentile. 100% full.
         */
        void onDisplayBrightnessChange(int brightness);
    }

    public static final class PowerState {
        /**
         * One of STATE_*
         */
        public final int state;
        public final int param;

        public PowerState(int state, int param) {
            this.state = state;
            this.param = param;
        }

        /**
         * Whether the current PowerState allows deep sleep or not. Calling this for
         * power state other than STATE_SHUTDOWN_PREPARE will trigger exception.
         * @return
         * @throws IllegalStateException
         */
        public boolean canEnterDeepSleep() {
            if (state != STATE_SHUTDOWN_PREPARE) {
                throw new IllegalStateException("wrong state");
            }
            return (param &
                    VehicleApPowerStateShutdownParam.VEHICLE_AP_POWER_SHUTDOWN_PARAM_CAN_SLEEP) !=
                    0;
        }

        /**
         * Whether the current PowerState allows postponing or not. Calling this for
         * power state other than STATE_SHUTDOWN_PREPARE will trigger exception.
         * @return
         * @throws IllegalStateException
         */
        public boolean canPostponeShutdown() {
            if (state != STATE_SHUTDOWN_PREPARE) {
                throw new IllegalStateException("wrong state");
            }
            return (param &
                    VehicleApPowerStateShutdownParam.VEHICLE_AP_POWER_SHUTDOWN_PARAM_SHUTDOWN_IMMEDIATELY)
                    == 0;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (!(o instanceof PowerState)) {
                return false;
            }
            PowerState that = (PowerState) o;
            return this.state == that.state && this.param == that.param;
        }

        @Override
        public String toString() {
            return "PowerState state:" + state + ",param:" + param;
        }
    }

    private final HashMap<Integer, VehiclePropConfig> mProperties = new HashMap<>();
    private final VehicleHal mHal;
    private LinkedList<VehiclePropValue> mQueuedEvents;
    private PowerEventListener mListener;
    private int mMaxDisplayBrightness;

    public PowerHalService(VehicleHal hal) {
        mHal = hal;
    }

    public void setListener(PowerEventListener listener) {
        LinkedList<VehiclePropValue> eventsToDispatch = null;
        synchronized (this) {
            mListener = listener;
            if (mQueuedEvents != null && mQueuedEvents.size() > 0) {
                eventsToDispatch = mQueuedEvents;
            }
            mQueuedEvents = null;
        }
        // do this outside lock
        if (eventsToDispatch != null) {
            dispatchEvents(eventsToDispatch, listener);
        }
    }

    public void sendBootComplete() {
        Log.i(CarLog.TAG_POWER, "send boot complete");
        int[] values = { VehicleApPowerSetState.VEHICLE_AP_POWER_SET_BOOT_COMPLETE, 0 };
        mHal.getVehicleNetwork().setIntVectorProperty(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE, values);
    }

    public void sendSleepEntry() {
        Log.i(CarLog.TAG_POWER, "send sleep entry");
        int[] values = { VehicleApPowerSetState.VEHICLE_AP_POWER_SET_DEEP_SLEEP_ENTRY, 0 };
        mHal.getVehicleNetwork().setIntVectorProperty(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE, values);
    }

    public void sendSleepExit() {
        Log.i(CarLog.TAG_POWER, "send sleep exit");
        int[] values = { VehicleApPowerSetState.VEHICLE_AP_POWER_SET_DEEP_SLEEP_EXIT, 0 };
        mHal.getVehicleNetwork().setIntVectorProperty(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE, values);
    }

    public void sendShutdownPostpone(int postponeTimeMs) {
        Log.i(CarLog.TAG_POWER, "send shutdown postpone, time:" + postponeTimeMs);
        int[] values = { VehicleApPowerSetState.VEHICLE_AP_POWER_SET_SHUTDOWN_POSTPONE,
                postponeTimeMs };
        mHal.getVehicleNetwork().setIntVectorProperty(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE, values);
    }

    public void sendShutdownStart(int wakeupTimeSec) {
        Log.i(CarLog.TAG_POWER, "send shutdown start");
        int[] values = { VehicleApPowerSetState.VEHICLE_AP_POWER_SET_SHUTDOWN_START, 0 };
        mHal.getVehicleNetwork().setIntVectorProperty(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE, values);
    }

    public void sendDisplayOn() {
        Log.i(CarLog.TAG_POWER, "send display on");
        int[] values = { VehicleApPowerSetState.VEHICLE_AP_POWER_SET_DISPLAY_ON, 0 };
        mHal.getVehicleNetwork().setIntVectorProperty(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE, values);
    }

    public void sendDisplayOff() {
        Log.i(CarLog.TAG_POWER, "send display off");
        int[] values = { VehicleApPowerSetState.VEHICLE_AP_POWER_SET_DISPLAY_OFF, 0 };
        mHal.getVehicleNetwork().setIntVectorProperty(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE, values);
    }

    public PowerState getCurrentPowerState() {
        int[] state = mHal.getVehicleNetwork().getIntVectorProperty(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE);
        return new PowerState(state[VehicleApPowerStateIndex.VEHICLE_AP_POWER_STATE_INDEX_STATE],
                state[VehicleApPowerStateIndex.VEHICLE_AP_POWER_STATE_INDEX_ADDITIONAL]);
    }

    public synchronized boolean isPowerStateSupported() {
        VehiclePropConfig config = mProperties.get(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE);
        return config != null;
    }

    public synchronized boolean isDeepSleepAllowed() {
        VehiclePropConfig config = mProperties.get(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE);
        if (config == null) {
            return false;
        }
        return (config.getConfigArray(0) &
                VehicleApPowerStateConfigFlag.VEHICLE_AP_POWER_STATE_CONFIG_ENABLE_DEEP_SLEEP_FLAG)
                != 0;
    }

    public synchronized boolean isTimedWakeupAllowed() {
        VehiclePropConfig config = mProperties.get(
                VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE);
        if (config == null) {
            return false;
        }
        return (config.getConfigArray(0) &
                VehicleApPowerStateConfigFlag.VEHICLE_AP_POWER_STATE_CONFIG_SUPPORT_TIMER_POWER_ON_FLAG)
                != 0;
    }

    @Override
    public synchronized void init() {
        for (VehiclePropConfig config : mProperties.values()) {
            if (VehicleHal.isPropertySubscribable(config)) {
                mHal.subscribeProperty(this, config.getProp(), 0);
            }
        }
        VehiclePropConfig brightnessProperty = mProperties.get(
                VehicleNetworkConsts.VEHICLE_PROPERTY_DISPLAY_BRIGHTNESS);
        if (brightnessProperty != null) {
            mMaxDisplayBrightness = brightnessProperty.getInt32Maxs(0);
            if (mMaxDisplayBrightness <= 0) {
                Log.w(CarLog.TAG_POWER, "Max display brightness from vehicle HAL is invald:" +
                        mMaxDisplayBrightness);
                mMaxDisplayBrightness = 1;
            }
        }
    }

    @Override
    public synchronized void release() {
        mProperties.clear();
    }

    @Override
    public List<VehiclePropConfig> takeSupportedProperties(List<VehiclePropConfig> allProperties) {
        for (VehiclePropConfig config : allProperties) {
            switch (config.getProp()) {
                case VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE:
                case VehicleNetworkConsts.VEHICLE_PROPERTY_DISPLAY_BRIGHTNESS:
                    mProperties.put(config.getProp(), config);
                    break;
            }
        }
        return new LinkedList<VehiclePropConfig>(mProperties.values());
    }

    @Override
    public void handleHalEvents(List<VehiclePropValue> values) {
        PowerEventListener listener;
        synchronized (this) {
            if (mListener == null) {
                if (mQueuedEvents == null) {
                    mQueuedEvents = new LinkedList<VehiclePropValue>();
                }
                mQueuedEvents.addAll(values);
                return;
            }
            listener = mListener;
        }
        dispatchEvents(values, listener);
    }

    private void dispatchEvents(List<VehiclePropValue> values, PowerEventListener listener) {
        for (VehiclePropValue v : values) {
            switch (v.getProp()) {
                case VehicleNetworkConsts.VEHICLE_PROPERTY_AP_POWER_STATE:
                    listener.onApPowerStateChange(new PowerState(
                            v.getInt32Values(
                                VehicleApPowerStateIndex.VEHICLE_AP_POWER_STATE_INDEX_STATE),
                            v.getInt32Values(
                                VehicleApPowerStateIndex.VEHICLE_AP_POWER_STATE_INDEX_ADDITIONAL)));
                    break;
                case VehicleNetworkConsts.VEHICLE_PROPERTY_DISPLAY_BRIGHTNESS:
                    int maxBrightness;
                    synchronized (this) {
                        maxBrightness = mMaxDisplayBrightness;
                    }
                    listener.onDisplayBrightnessChange(v.getInt32Values(0) * 100 / maxBrightness);
                    break;
            }
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*Power HAL*");
        writer.println("isPowerStateSupported:" + isPowerStateSupported() +
                ",isDeepSleepAllowed:" + isDeepSleepAllowed());
    }
}
