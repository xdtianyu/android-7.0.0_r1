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

package android.support.car;

import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.support.car.content.pm.CarPackageManager;
import android.support.car.hardware.CarSensorManager;
import android.support.car.navigation.CarNavigationManager;
import android.util.Log;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

/**
 *   Top level car API.
 *   This API works only for device with {@link PackageManager#FEATURE_AUTOMOTIVE} feature
 *   supported or device with Google play service.
 *   Calling this API with device with no such feature will lead into an exception.
 *
 */
public class Car {

    /** Service name for {@link CarSensorManager}, to be used in {@link #getCarManager(String)}. */
    public static final String SENSOR_SERVICE = "sensor";

    /** Service name for {@link CarInfoManager}, to be used in {@link #getCarManager(String)}. */
    public static final String INFO_SERVICE = "info";

    /** Service name for {@link CarAppContextManager}. */
    public static final String APP_CONTEXT_SERVICE = "app_context";

    /** Service name for {@link CarPackageManager} */
    public static final String PACKAGE_SERVICE = "package";

    /** Service name for {@link CarAudioManager} */
    public static final String AUDIO_SERVICE = "audio";
    /**
     * Service name for {@link CarNavigationManager}
     * @hide
     */
    public static final String CAR_NAVIGATION_SERVICE = "car_navigation_service";

    /** Type of car connection: car emulator, not physical connection. */
    public static final int CONNECTION_TYPE_EMULATOR        = 0;
    /** Type of car connection: connected to a car via USB. */
    public static final int CONNECTION_TYPE_USB             = 1;
    /** Type of car connection: connected to a car via WIFI. */
    public static final int CONNECTION_TYPE_WIFI            = 2;
    /** Type of car connection: on-device car emulator, for development (e.g. Local Head Unit). */
    public static final int CONNECTION_TYPE_ON_DEVICE_EMULATOR = 3;
    /** Type of car connection: car emulator, connected over ADB (e.g. Desktop Head Unit). */
    public static final int CONNECTION_TYPE_ADB_EMULATOR = 4;
    /** Type of car connection: platform runs directly in car. */
    public static final int CONNECTION_TYPE_EMBEDDED = 5;
    /**
     * Type of car connection: platform runs directly in car but with mocked vehicle hal.
     * This will only happen in testing environment.
     * @hide
     */
    public static final int CONNECTION_TYPE_EMBEDDED_MOCKING = 6;

    /** @hide */
    @IntDef({CONNECTION_TYPE_EMULATOR, CONNECTION_TYPE_USB, CONNECTION_TYPE_WIFI,
        CONNECTION_TYPE_ON_DEVICE_EMULATOR, CONNECTION_TYPE_ADB_EMULATOR, CONNECTION_TYPE_EMBEDDED})
    @Retention(RetentionPolicy.SOURCE)
    public @interface ConnectionType {}

    /** Permission necessary to access car's mileage information. */
    public static final String PERMISSION_MILEAGE = "android.car.permission.CAR_MILEAGE";
    /** Permission necessary to access car's fuel level. */
    public static final String PERMISSION_FUEL = "android.car.permission.CAR_FUEL";
    /** Permission necessary to access car's speed. */
    public static final String PERMISSION_SPEED = "android.car.permission.CAR_SPEED";
    /** Permission necessary to access car specific communication channel. */
    public static final String PERMISSION_VENDOR_EXTENSION =
            "android.car.permission.CAR_VENDOR_EXTENSION";
    /**
     * Permission necessary to use {@link android.car.navigation.CarNavigationManager}.
     * @hide
     */
    public static final String PERMISSION_CAR_NAVIGATION_MANAGER =
            "android.car.permission.PERMISSION_CAR_NAVIGATION_MANAGER";


    /**
     * PackageManager.FEATURE_AUTOMOTIVE from M. But redefine here to support L.
     * @hide
     */
    private static final String FEATURE_AUTOMOTIVE = "android.hardware.type.automotive";

    /**
     * {@link CarServiceLoader} implementation for projected mode. Only available when projected
     * client library is linked.
     * @hide
     */
    private static final String PROJECTED_CAR_SERVICE_LOADER =
            "com.google.android.gms.car.CarServiceLoaderGms";

    /**
     * CarXyzService throws IllegalStateException with this message is re-thrown as
     * {@link CarNotConnectedException}.
     *
     * @hide
     */
    public static final String CAR_NOT_CONNECTED_EXCEPTION_MSG = "CarNotConnected";

    /** @hide */
    public static final String CAR_SERVICE_INTERFACE_NAME = "android.car.ICar";

    private final Context mContext;
    private final Looper mLooper;
    private static final int STATE_DISCONNECTED = 0;
    private static final int STATE_CONNECTING = 1;
    private static final int STATE_CONNECTED = 2;
    // @GuardedBy("this")
    private int mConnectionState;

    private final ServiceConnectionListener mServiceConnectionListener =
            new ServiceConnectionListener () {
        public void onServiceConnected(ComponentName name) {
            synchronized (Car.this) {
                mConnectionState = STATE_CONNECTED;
            }
            mServiceConnectionListenerClient.onServiceConnected(name);
        }

        public void onServiceDisconnected(ComponentName name) {
            synchronized (Car.this) {
                if (mConnectionState  == STATE_DISCONNECTED) {
                    return;
                }
                mConnectionState = STATE_DISCONNECTED;
            }
            mServiceConnectionListenerClient.onServiceDisconnected(name);
            connect();
        }

        public void onServiceSuspended(int cause) {
            mServiceConnectionListenerClient.onServiceSuspended(cause);
        }

        public void onServiceConnectionFailed(int cause) {
            mServiceConnectionListenerClient.onServiceConnectionFailed(cause);
        }
    };

    private final ServiceConnectionListener mServiceConnectionListenerClient;
    private final Object mCarManagerLock = new Object();
    //@GuardedBy("mCarManagerLock")
    private final HashMap<String, CarManagerBase> mServiceMap = new HashMap<>();
    private final CarServiceLoader mCarServiceLoader;

    /** Handler for generic event dispatching. */
    private final Handler mEventHandler;

    /**
     * A factory method that creates Car instance for all Car API access.
     * @param context
     * @param serviceConnectionListener listener for monitoring service connection.
     * @param looper Looper to dispatch all listeners. If null, it will use main thread. Note that
     *        service connection listener will be always in main thread regardless of this Looper.
     * @return Car instance if system is in car environment and returns {@code null} otherwise.
     */
    public static Car createCar(Context context,
            ServiceConnectionListener serviceConnectionListener, @Nullable Looper looper) {
        try {
          return new Car(context, serviceConnectionListener, looper);
        } catch (IllegalArgumentException e) {
          // Expected when car service loader is not available.
        }
        return null;
    }

    /**
     * A factory method that creates Car instance for all Car API access using main thread {@code
     * Looper}.
     *
     * @see #createCar(Context, ServiceConnectionListener, Looper)
     */
    public static Car createCar(Context context,
            ServiceConnectionListener serviceConnectionListener) {
      return createCar(context, serviceConnectionListener, null);
    }

    private Car(Context context, ServiceConnectionListener serviceConnectionListener,
            @Nullable Looper looper) {
        mContext = context;
        mServiceConnectionListenerClient = serviceConnectionListener;
        if (looper == null) {
            mLooper = Looper.getMainLooper();
        } else {
            mLooper = looper;
        }
        mEventHandler = new Handler(mLooper);
        if (mContext.getPackageManager().hasSystemFeature(FEATURE_AUTOMOTIVE)) {
            mCarServiceLoader = new CarServiceLoaderEmbedded(context, mServiceConnectionListener,
                    mLooper);
        } else {
            mCarServiceLoader = loadCarServiceLoader(PROJECTED_CAR_SERVICE_LOADER, context,
                    mServiceConnectionListener, mLooper);
        }
    }

    private CarServiceLoader loadCarServiceLoader(String carServiceLoaderClassName,
            Context context, ServiceConnectionListener serviceConnectionListener, Looper looper)
                    throws IllegalArgumentException {
        Class carServiceLoaderClass = null;
        try {
            carServiceLoaderClass = Class.forName(carServiceLoaderClassName);
        } catch (ClassNotFoundException e) {
            throw new IllegalArgumentException("Cannot find CarServiceLoader implementation:" +
                    carServiceLoaderClassName, e);
        }
        Constructor<?> ctor;
        try {
            ctor = carServiceLoaderClass.getDeclaredConstructor(Context.class,
                    ServiceConnectionListener.class, Looper.class);
        } catch (NoSuchMethodException e) {
            throw new IllegalArgumentException("Cannot construct CarServiceLoader, no constructor: "
                    + carServiceLoaderClassName, e);
        }
        try {
            return (CarServiceLoader) ctor.newInstance(context,
                    serviceConnectionListener, looper);
        } catch (InstantiationException | IllegalAccessException | IllegalArgumentException
                | InvocationTargetException e) {
            throw new IllegalArgumentException(
                    "Cannot construct CarServiceLoader, constructor failed for "
                    + carServiceLoaderClass.getName(), e);
        }
    }

    /**
     * Car constructor when CarServiceLoader is already available.
     * @param context
     * @param serviceLoader
     * @param looper
     *
     * @hide
     */
    public Car(Context context, CarServiceLoader serviceLoader, @Nullable Looper looper) {
        mContext = context;
        if (looper == null) {
            mLooper = Looper.getMainLooper();
        } else {
            mLooper = looper;
        }
        mEventHandler = new Handler(mLooper);
        mConnectionState = STATE_CONNECTED;
        mCarServiceLoader = serviceLoader;
        mServiceConnectionListenerClient = null;
    }

    /**
     * Connect to car service. This can be called while it is disconnected.
     * @throws IllegalStateException If connection is still on-going from previous
     *         connect call or it is already connected
     */
    public void connect() throws IllegalStateException {
        synchronized (this) {
            if (mConnectionState != STATE_DISCONNECTED) {
                throw new IllegalStateException("already connected or connecting");
            }
            mConnectionState = STATE_CONNECTING;
            mCarServiceLoader.connect();
        }
    }

    /**
     * Disconnect from car service. This can be called while disconnected. Once disconnect is
     * called, all Car*Managers from this instance becomes invalid, and
     * {@link Car#getCarManager(String)} will return different instance if it is connected again.
     */
    public void disconnect() {
        synchronized (this) {
            if (mConnectionState == STATE_DISCONNECTED) {
                return;
            }
            tearDownCarManagers();
            mConnectionState = STATE_DISCONNECTED;
            mCarServiceLoader.disconnect();
        }
    }

    /**
     * Tells if it is connected to the service or not. This will return false if it is still
     * connecting.
     * @return
     */
    public boolean isConnected() {
        synchronized (this) {
            return mConnectionState == STATE_CONNECTED;
        }
    }

    /**
     * Tells if this instance is already connecting to car service or not.
     * @return
     */
    public boolean isConnecting() {
        synchronized (this) {
            return mConnectionState == STATE_CONNECTING;
        }
    }

    /**
     * Tells if car is connected to car or not. In some car environments, being connected to service
     * does not necessarily mean being connected to car.
     */
    public boolean isConnectedToCar() {
        return mCarServiceLoader.isConnectedToCar();
    }

    /**
     * Get car specific service as in {@link Context#getSystemService(String)}. Returned
     * {@link Object} should be type-casted to the desired service.
     * For example, to get sensor service,
     * SensorManagerService sensorManagerService = car.getCarManager(Car.SENSOR_SERVICE);
     * @param serviceName Name of service that should be created like {@link #SENSOR_SERVICE}.
     * @return Matching service manager or null if there is no such service.
     */
    public Object getCarManager(String serviceName)
            throws CarNotSupportedException, CarNotConnectedException {
        Object manager = null;
        synchronized (mCarManagerLock) {
            manager = mServiceMap.get(serviceName);
            if (manager == null) {
                manager = mCarServiceLoader.getCarManager(serviceName);
            }
            // do not store if it is not CarManagerBase. This can happen when system version
            // is retrieved from this call.
            if (manager != null && manager instanceof CarManagerBase) {
                mServiceMap.put(serviceName, (CarManagerBase) manager);
            }
        }
        return manager;
    }

    /**
     * Return the type of currently connected car.
     * @return
     * @throws CarNotConnectedException
     */
    @ConnectionType
    public int getCarConnectionType() throws CarNotConnectedException {
        return mCarServiceLoader.getCarConnectionType();
    }

    /**
     * Registers a {@link CarConnectionListener}.
     *
     * Avoid reregistering unregistered listeners. If an unregistered listener is reregistered,
     * it may receive duplicate calls to {@link CarConnectionListener#onConnected}.
     *
     * @throws IllegalStateException if service is not connected.
     */
    public void registerCarConnectionListener(CarConnectionListener listener)
            throws IllegalStateException, CarNotConnectedException {
        assertCarConnection();
        mCarServiceLoader.registerCarConnectionListener(listener);
    }

    /**
     * Unregisters a {@link CarConnectionListener}.
     *
     * <b>Note:</b> If this method is called from a thread besides the client's looper thread,
     * there is no guarantee that the unregistered listener will not receive callbacks after
     * this method returns.
     */
    public void unregisterCarConnectionListener(CarConnectionListener listener) {
        mCarServiceLoader.unregisterCarConnectionListener(listener);
    }

    /**
     * IllegalStateException from XyzCarService with special message is re-thrown as a different
     * exception. If the IllegalStateException is not understood then this message will throw the
     * original exception.
     *
     * @param e exception from XyzCarService.
     * @throws CarNotConnectedException
     * @hide
     */
    public static void checkCarNotConnectedExceptionFromCarService(
            IllegalStateException e) throws CarNotConnectedException, IllegalStateException {
        String message = e.getMessage();
        if (message.equals(CAR_NOT_CONNECTED_EXCEPTION_MSG)) {
            throw new CarNotConnectedException();
        } else {
            throw e;
        }
    }

    private synchronized void assertCarConnection() throws IllegalStateException {
        if (!mCarServiceLoader.isConnectedToCar()) {
            throw new IllegalStateException("not connected");
        }
    }

    private void tearDownCarManagers() {
        synchronized (mCarManagerLock) {
            for (CarManagerBase manager: mServiceMap.values()) {
                manager.onCarDisconnected();
            }
            mServiceMap.clear();
        }
    }
}
