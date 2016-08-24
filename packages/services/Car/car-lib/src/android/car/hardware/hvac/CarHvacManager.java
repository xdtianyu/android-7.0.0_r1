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

package android.car.hardware.hvac;

import static java.lang.Integer.toHexString;

import android.annotation.IntDef;
import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.car.Car;
import android.car.CarManagerBase;
import android.car.CarNotConnectedException;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.content.Context;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.util.ArraySet;
import android.util.Log;

import java.lang.ref.WeakReference;
import java.util.Collection;
import java.util.List;

/**
 * API for controlling HVAC system in cars
 * @hide
 */
@SystemApi
public class CarHvacManager implements CarManagerBase {
    public final static boolean DBG = true;
    public final static String TAG = "CarHvacManager";

    /**
     * HVAC property IDs for get/set methods
     */
    @IntDef({
            HvacPropertyId.MIRROR_DEFROSTER_ON,
            HvacPropertyId.STEERING_WHEEL_TEMP,
            HvacPropertyId.MAX_GLOBAL_PROPERTY_ID,
            HvacPropertyId.ZONED_TEMP_SETPOINT,
            HvacPropertyId.ZONED_TEMP_ACTUAL,
            HvacPropertyId.ZONED_TEMP_IS_FAHRENHEIT,
            HvacPropertyId.ZONED_FAN_SPEED_SETPOINT,
            HvacPropertyId.ZONED_FAN_SPEED_RPM,
            HvacPropertyId.ZONED_FAN_POSITION_AVAILABLE,
            HvacPropertyId.ZONED_FAN_POSITION,
            HvacPropertyId.ZONED_SEAT_TEMP,
            HvacPropertyId.ZONED_AC_ON,
            HvacPropertyId.ZONED_AUTOMATIC_MODE_ON,
            HvacPropertyId.ZONED_AIR_RECIRCULATION_ON,
            HvacPropertyId.WINDOW_DEFROSTER_ON,
    })
    public @interface HvacPropertyId {
        /**
         * Global HVAC properties.  There is only a single instance in a car.
         * Global properties are in the range of 0-0x3FFF.
         */
        /** Mirror defrosters state, bool. */
        int MIRROR_DEFROSTER_ON = 0x0001;
        /** Steering wheel temp:  negative values indicate cooling, positive values indicate
         * heat, int. */
        int STEERING_WHEEL_TEMP = 0x0002;

        /** The maximum id that can be assigned to global (non-zoned) property. */
        int MAX_GLOBAL_PROPERTY_ID = 0x3fff;

        /**
         * ZONED_* represents properties available on a per-zone basis.  All zones in a car are
         * not required to have the same properties.  Zone specific properties start at 0x4000.
         */
        /** Temperature setpoint desired by the user, in terms of F or C, depending on
         * TEMP_IS_FAHRENHEIT, int */
        int ZONED_TEMP_SETPOINT = 0x4001;
        /** Actual zone temperature is read only integer, in terms of F or C, int. */
        int ZONED_TEMP_ACTUAL = 0x4002;
        /** Temperature is in degrees fahrenheit if this is true, bool. */
        int ZONED_TEMP_IS_FAHRENHEIT = 0x4003;
        /** Fan speed setpoint is an integer from 0-n, depending on the number of fan speeds
         * available. Selection determines the fan position, int. */
        int ZONED_FAN_SPEED_SETPOINT = 0x4004;
        /** Actual fan speed is a read-only value, expressed in RPM, int. */
        int ZONED_FAN_SPEED_RPM = 0x4005;
        /** Fan position available is a bitmask of positions available for each zone, int. */
        int ZONED_FAN_POSITION_AVAILABLE = 0x4006;
        /** Current fan position setting, int. */
        int ZONED_FAN_POSITION = 0x4007;
        /** Seat temperature is negative for cooling, positive for heating.  Temperature is a
         * setting, i.e. -3 to 3 for 3 levels of cooling and 3 levels of heating.  int. */
        int ZONED_SEAT_TEMP = 0x4008;
        /** Air conditioner state, bool */
        int ZONED_AC_ON = 0x4009;
        /** HVAC is in automatic mode, bool. */
        int ZONED_AUTOMATIC_MODE_ON = 0x400A;
        /** Air recirculation is active, bool. */
        int ZONED_AIR_RECIRCULATION_ON = 0x400B;
        /** Defroster is based off of window position, bool */
        int WINDOW_DEFROSTER_ON = 0x5001;
    }

    // Constants handled in the handler (see mHandler below).
    private final static int MSG_HVAC_EVENT = 0;

    /** Callback functions for HVAC events */
    public interface CarHvacEventListener {
        /** Called when an HVAC property is updated */
        void onChangeEvent(final CarPropertyValue value);

        /** Called when an error is detected with a property */
        void onErrorEvent(final int propertyId, final int zone);
    }

    private final ICarHvac mService;
    private final ArraySet<CarHvacEventListener> mListeners = new ArraySet<>();
    private CarHvacEventListenerToService mListenerToService = null;

    private static final class EventCallbackHandler extends Handler {
        WeakReference<CarHvacManager> mMgr;

        EventCallbackHandler(CarHvacManager mgr, Looper looper) {
            super(looper);
            mMgr = new WeakReference<>(mgr);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_HVAC_EVENT:
                    CarHvacManager mgr = mMgr.get();
                    if (mgr != null) {
                        mgr.dispatchEventToClient((CarHvacEvent) msg.obj);
                    }
                    break;
                default:
                    Log.e(TAG, "Event type not handled?" + msg);
                    break;
            }
        }
    }

    private final Handler mHandler;

    private static class CarHvacEventListenerToService extends ICarHvacEventListener.Stub {
        private final WeakReference<CarHvacManager> mManager;

        public CarHvacEventListenerToService(CarHvacManager manager) {
            mManager = new WeakReference<>(manager);
        }

        @Override
        public void onEvent(CarHvacEvent event) {
            CarHvacManager manager = mManager.get();
            if (manager != null) {
                manager.handleEvent(event);
            }
        }
    }

    /**
     * Get an instance of the CarHvacManager.
     *
     * Should not be obtained directly by clients, use {@link Car#getCarManager(String)} instead.
     * @hide
     */
    public CarHvacManager(IBinder service, Context context, Looper looper) {
        mService = ICarHvac.Stub.asInterface(service);
        mHandler = new EventCallbackHandler(this, looper);
    }

    /** Returns true if the property is a zoned type. */
    public static boolean isZonedProperty(int propertyId) {
        return propertyId > HvacPropertyId.MAX_GLOBAL_PROPERTY_ID;
    }

    /**
     * Register {@link CarHvacEventListener} to get HVAC property changes
     *
     * @param listener Implements onEvent() for property change updates
     */
    public synchronized void registerListener(CarHvacEventListener listener)
            throws CarNotConnectedException {
        if(mListeners.isEmpty()) {
            try {
                mListenerToService = new CarHvacEventListenerToService(this);
                mService.registerListener(mListenerToService);
            } catch (RemoteException ex) {
                Log.e(TAG, "Could not connect: " + ex.toString());
                throw new CarNotConnectedException(ex);
            } catch (IllegalStateException ex) {
                Car.checkCarNotConnectedExceptionFromCarService(ex);
            }
        }
        mListeners.add(listener);
    }

    /**
     * Unregister {@link CarHvacEventListener}.
     * @param listener CarHvacEventListener to unregister
     */
    public synchronized void unregisterListener(CarHvacEventListener listener)
            throws CarNotConnectedException {
        if (DBG) {
            Log.d(TAG, "unregisterListener");
        }
        try {
            mService.unregisterListener(mListenerToService);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not unregister: " + e.toString());
            throw new CarNotConnectedException(e);

        }
        mListeners.remove(listener);
        if(mListeners.isEmpty()) {
            mListenerToService = null;
        }
    }

    /**
     * Returns the list of HVAC properties available.
     *
     * @return Caller must check the property type and typecast to the appropriate subclass
     * (CarHvacBooleanProperty, CarHvacFloatProperty, CarrHvacIntProperty)
     */
    public List<CarPropertyConfig> getPropertyList()  throws CarNotConnectedException {
        List<CarPropertyConfig> carProps;
        try {
            carProps = mService.getHvacProperties();
        } catch (RemoteException e) {
            Log.w(TAG, "Exception in getPropertyList", e);
            throw new CarNotConnectedException(e);
        }
        return carProps;
    }

    /**
     * Returns value of a bool property
     *
     * @param prop Property ID to get
     * @param area Area of the property to get
     */
    public boolean getBooleanProperty(int prop, int area) throws CarNotConnectedException {
        CarPropertyValue<Boolean> carProp = getProperty(Boolean.class, prop, area);
        return carProp != null ? carProp.getValue() : false;
    }

    /**
     * Returns value of a float property
     *
     * @param prop Property ID to get
     * @param area Area of the property to get
     */
    public float getFloatProperty(int prop, int area) throws CarNotConnectedException {
        CarPropertyValue<Float> carProp = getProperty(Float.class, prop, area);
        return carProp != null ? carProp.getValue() : 0f;
    }

    /**
     * Returns value of a integer property
     *
     * @param prop Property ID to get
     * @param area Zone of the property to get
     */
    public int getIntProperty(int prop, int area) throws CarNotConnectedException {
        CarPropertyValue<Integer> carProp = getProperty(Integer.class, prop, area);
        return carProp != null ? carProp.getValue() : 0;
    }

    @Nullable
    @SuppressWarnings("unchecked")
    private <E> CarPropertyValue<E> getProperty(Class<E> clazz, int prop, int area)
            throws CarNotConnectedException {
        if (DBG) {
            Log.d(TAG, "getProperty, prop: 0x" + toHexString(prop)
                    + ", area: 0x" + toHexString(area) + ", clazz: " + clazz);
        }
        try {
            CarPropertyValue<E> hvacProperty = mService.getProperty(prop, area);
            if (hvacProperty != null && hvacProperty.getValue() != null) {
                Class<?> actualClass = hvacProperty.getValue().getClass();
                if (actualClass != clazz) {
                throw new IllegalArgumentException("Invalid property type. "
                        + "Expected: " + clazz + ", but was: " + actualClass);
                }
            }
            return hvacProperty;
        } catch (RemoteException e) {
            Log.e(TAG, "getProperty failed with " + e.toString()
                    + ", propId: 0x" + toHexString(prop) + ", area: 0x" + toHexString(area), e);
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * Modifies a property.  If the property modification doesn't occur, an error event shall be
     * generated and propagated back to the application.
     *
     * @param prop Property ID to modify
     * @param area Area to apply the modification.
     * @param val Value to set
     */
    public void setBooleanProperty(int prop, int area, boolean val)
            throws CarNotConnectedException {
        if (DBG) {
            Log.d(TAG, "setBooleanProperty:  prop = " + prop + " area = " + area + " val = " + val);
        }
        try {
            mService.setProperty(new CarPropertyValue<>(prop, area, val));
        } catch (RemoteException e) {
            Log.e(TAG, "setBooleanProperty failed with " + e.toString(), e);
            throw new CarNotConnectedException(e);
        }
    }

    public void setFloatProperty(int prop, int area, float val) throws CarNotConnectedException {
        if (DBG) {
            Log.d(TAG, "setFloatProperty:  prop = " + prop + " area = " + area + " val = " + val);
        }
        try {
            mService.setProperty(new CarPropertyValue<>(prop, area, val));
        } catch (RemoteException e) {
            Log.e(TAG, "setBooleanProperty failed with " + e.toString(), e);
            throw new CarNotConnectedException(e);
        }
    }

    public void setIntProperty(int prop, int area, int val) throws CarNotConnectedException {
        if (DBG) {
            Log.d(TAG, "setIntProperty:  prop = " + prop + " area = " + area + " val = " + val);
        }
        try {
            mService.setProperty(new CarPropertyValue<>(prop, area, val));
        } catch (RemoteException e) {
            Log.e(TAG, "setIntProperty failed with " + e.toString(), e);
            throw new CarNotConnectedException(e);
        }
    }

    private void dispatchEventToClient(CarHvacEvent event) {
        Collection<CarHvacEventListener> listeners;
        synchronized (this) {
            listeners = mListeners;
        }
        if (!listeners.isEmpty()) {
            CarPropertyValue hvacProperty = event.getCarPropertyValue();
            switch(event.getEventType()) {
                case CarHvacEvent.HVAC_EVENT_PROPERTY_CHANGE:
                    for (CarHvacEventListener l: listeners) {
                        l.onChangeEvent(hvacProperty);
                    }
                case CarHvacEvent.HVAC_EVENT_ERROR:
                    for (CarHvacEventListener l: listeners) {
                        l.onErrorEvent(hvacProperty.getPropertyId(), hvacProperty.getAreaId());
                    }
                    break;
                default:
                    throw new IllegalArgumentException();
            }
        } else {
            Log.e(TAG, "Listener died, not dispatching event.");
        }
    }

    private void handleEvent(CarHvacEvent event) {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_HVAC_EVENT, event));
    }

    /** @hide */
    @Override
    public void onCarDisconnected() {
        for(CarHvacEventListener l: mListeners) {
            try {
                unregisterListener(l);
            } catch (CarNotConnectedException e) {
                // Ignore, car is disconnecting.
            }
        }
    }
}
