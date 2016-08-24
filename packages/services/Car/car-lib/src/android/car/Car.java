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

package android.car;

import android.annotation.IntDef;
import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.car.content.pm.CarPackageManager;
import android.car.hardware.CarSensorManager;
import android.car.hardware.camera.CarCameraManager;
import android.car.hardware.hvac.CarHvacManager;
import android.car.hardware.radio.CarRadioManager;
import android.car.media.CarAudioManager;
import android.car.navigation.CarNavigationManager;
import android.car.test.CarTestManagerBinderWrapper;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.os.UserHandle;
import android.util.Log;

import com.android.internal.annotations.GuardedBy;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.HashMap;

/**
 *   Top level car API for embedded Android Auto deployments.
 *   This API works only for devices with {@link PackageManager#FEATURE_AUTOMOTIVE}
 *   Calling this API on a device with no such feature will lead to an exception.
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

    /**
     * @hide
     */
    @SystemApi
    public static final String CAMERA_SERVICE = "camera";

    /**
     * @hide
     */
    @SystemApi
    public static final String RADIO_SERVICE = "radio";

    /**
     * @hide
     */
    @SystemApi
    public static final String HVAC_SERVICE = "hvac";

    /**
     * @hide
     */
    @SystemApi
    public static final String PROJECTION_SERVICE = "projection";

    /**
     * Service for testing. This is system app only feature.
     * Service name for {@link CarTestManager}, to be used in {@link #getCarManager(String)}.
     * @hide
     */
    @SystemApi
    public static final String TEST_SERVICE = "car-service-test";

    /** Permission necessary to access car's mileage information. */
    public static final String PERMISSION_MILEAGE = "android.car.permission.CAR_MILEAGE";

    /** Permission necessary to access car's fuel level. */
    public static final String PERMISSION_FUEL = "android.car.permission.CAR_FUEL";

    /** Permission necessary to access car's speed. */
    public static final String PERMISSION_SPEED = "android.car.permission.CAR_SPEED";

    /**
     * Permission necessary to use {@link CarNavigationManager}.
     * @hide
     */
    public static final String PERMISSION_CAR_NAVIGATION_MANAGER =
            "android.car.permission.CAR_NAVIGATION_MANAGER";

    /**
     * Permission necessary to access car specific communication channel.
     * @hide
     */
    @SystemApi
    public static final String PERMISSION_VENDOR_EXTENSION =
            "android.car.permission.CAR_VENDOR_EXTENSION";

    /**
     * @hide
     */
    @SystemApi
    public static final String PERMISSION_CONTROL_APP_BLOCKING =
            "android.car.permission.CONTROL_APP_BLOCKING";

    /**
     * Permission necessary to access Car Camera APIs.
     * @hide
     */
    @SystemApi
    public static final String PERMISSION_CAR_CAMERA = "android.car.permission.CAR_CAMERA";

    /**
     * Permission necessary to access Car HVAC APIs.
     * @hide
     */
    @SystemApi
    public static final String PERMISSION_CAR_HVAC = "android.car.permission.CAR_HVAC";

    /**
     * Permission necessary to access Car RADIO system APIs.
     * @hide
     */
    @SystemApi
    public static final String PERMISSION_CAR_RADIO = "android.car.permission.CAR_RADIO";

    /**
     * Permission necesary to access Car PRJECTION system APIs.
     * @hide
     */
    @SystemApi
    public static final String PERMISSION_CAR_PROJECTION = "android.car.permission.CAR_PROJECTION";

    /**
     * Permission necessary to mock vehicle hal for testing.
     * @hide
     */
    @SystemApi
    public static final String PERMISSION_MOCK_VEHICLE_HAL =
            "android.car.permission.CAR_MOCK_VEHICLE_HAL";

    /** Type of car connection: platform runs directly in car. */
    public static final int CONNECTION_TYPE_EMBEDDED = 5;
    /**
     * Type of car connection: platform runs directly in car but with mocked vehicle hal.
     * This will only happen in testing environment.
     * @hide
     */
    public static final int CONNECTION_TYPE_EMBEDDED_MOCKING = 6;


    /** @hide */
    @IntDef({CONNECTION_TYPE_EMBEDDED, CONNECTION_TYPE_EMBEDDED_MOCKING})
    @Retention(RetentionPolicy.SOURCE)
    public @interface ConnectionType {}

    /**
     * CarXyzService throws IllegalStateException with this message is re-thrown as
     * {@link CarNotConnectedException}.
     *
     * @hide
     */
    public static final String CAR_NOT_CONNECTED_EXCEPTION_MSG = "CarNotConnected";

    /** @hide */
    public static final String CAR_SERVICE_INTERFACE_NAME = "android.car.ICar";

    private static final String CAR_SERVICE_PACKAGE = "com.android.car";

    private static final String CAR_SERVICE_CLASS = "com.android.car.CarService";

    private static final String CAR_TEST_MANAGER_CLASS = "android.car.CarTestManager";

    private static final long CAR_SERVICE_BIND_RETRY_INTERVAL_MS = 500;
    private static final long CAR_SERVICE_BIND_MAX_RETRY = 20;

    private final Context mContext;
    private final Looper mLooper;
    @GuardedBy("this")
    private ICar mService;
    private static final int STATE_DISCONNECTED = 0;
    private static final int STATE_CONNECTING = 1;
    private static final int STATE_CONNECTED = 2;
    @GuardedBy("this")
    private int mConnectionState;
    @GuardedBy("this")
    private int mConnectionRetryCount;

    private final Runnable mConnectionRetryRunnable = new Runnable() {
        @Override
        public void run() {
            startCarService();
        }
    };

    private final Runnable mConnectionRetryFailedRunnable = new Runnable() {
        @Override
        public void run() {
            mServiceConnectionListener.onServiceDisconnected(new ComponentName(CAR_SERVICE_PACKAGE,
                    CAR_SERVICE_CLASS));
        }
    };

    private final ServiceConnection mServiceConnectionListener =
            new ServiceConnection () {
        public void onServiceConnected(ComponentName name, IBinder service) {
            synchronized (Car.this) {
                mService = ICar.Stub.asInterface(service);
                mConnectionState = STATE_CONNECTED;
            }
            mServiceConnectionListenerClient.onServiceConnected(name, service);
        }

        public void onServiceDisconnected(ComponentName name) {
            synchronized (Car.this) {
                mService = null;
                if (mConnectionState  == STATE_DISCONNECTED) {
                    return;
                }
                mConnectionState = STATE_DISCONNECTED;
            }
            // unbind explicitly here.
            disconnect();
            mServiceConnectionListenerClient.onServiceDisconnected(name);
        }
    };

    private final ServiceConnection mServiceConnectionListenerClient;
    private final Object mCarManagerLock = new Object();
    @GuardedBy("mCarManagerLock")
    private final HashMap<String, CarManagerBase> mServiceMap = new HashMap<>();

    /** Handler for generic event dispatching. */
    private final Handler mEventHandler;

    private final Handler mMainThreadEvetHandler;

    /**
     * A factory method that creates Car instance for all Car API access.
     * @param context
     * @param serviceConnectionListener listener for monitoring service connection.
     * @param looper Looper to dispatch all listeners. If null, it will use main thread. Note that
     *        service connection listener will be always in main thread regardless of this Looper.
     * @return Car instance if system is in car environment and returns {@code null} otherwise.
     */
    public static Car createCar(Context context, ServiceConnection serviceConnectionListener,
            @Nullable Looper looper) {
        if (!context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE)) {
            Log.e(CarLibLog.TAG_CAR, "FEATURE_AUTOMOTIVE not declared while android.car is used");
            return null;
        }
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
     * @see #createCar(Context, ServiceConnection, Looper)
     */
    public static Car createCar(Context context, ServiceConnection serviceConnectionListener) {
      return createCar(context, serviceConnectionListener, null);
    }

    private Car(Context context, ServiceConnection serviceConnectionListener,
            @Nullable Looper looper) {
        mContext = context;
        mServiceConnectionListenerClient = serviceConnectionListener;
        if (looper == null) {
            mLooper = Looper.getMainLooper();
        } else {
            mLooper = looper;
        }
        mEventHandler = new Handler(mLooper);
        if (mLooper == Looper.getMainLooper()) {
            mMainThreadEvetHandler = mEventHandler;
        } else {
            mMainThreadEvetHandler = new Handler(Looper.getMainLooper());
        }
    }

    /**
     * Car constructor when ICar binder is already available.
     * @param context
     * @param service
     * @param looper
     *
     * @hide
     */
    public Car(Context context, ICar service, @Nullable Looper looper) {
        mContext = context;
        if (looper == null) {
            mLooper = Looper.getMainLooper();
        } else {
            mLooper = looper;
        }
        mEventHandler = new Handler(mLooper);
        if (mLooper == Looper.getMainLooper()) {
            mMainThreadEvetHandler = mEventHandler;
        } else {
            mMainThreadEvetHandler = new Handler(Looper.getMainLooper());
        }
        mService = service;
        mConnectionState = STATE_CONNECTED;
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
            startCarService();
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
            mEventHandler.removeCallbacks(mConnectionRetryRunnable);
            mMainThreadEvetHandler.removeCallbacks(mConnectionRetryFailedRunnable);
            mConnectionRetryCount = 0;
            tearDownCarManagers();
            mService = null;
            mConnectionState = STATE_DISCONNECTED;
            mContext.unbindService(mServiceConnectionListener);
        }
    }

    /**
     * Tells if it is connected to the service or not. This will return false if it is still
     * connecting.
     * @return
     */
    public boolean isConnected() {
        synchronized (this) {
            return mService != null;
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
     * Get car specific service as in {@link Context#getSystemService(String)}. Returned
     * {@link Object} should be type-casted to the desired service.
     * For example, to get sensor service,
     * SensorManagerService sensorManagerService = car.getCarManager(Car.SENSOR_SERVICE);
     * @param serviceName Name of service that should be created like {@link #SENSOR_SERVICE}.
     * @return Matching service manager or null if there is no such service.
     * @throws CarNotConnectedException
     */
    public Object getCarManager(String serviceName) throws CarNotConnectedException {
        CarManagerBase manager = null;
        ICar service = getICarOrThrow();
        synchronized (mCarManagerLock) {
            manager = mServiceMap.get(serviceName);
            if (manager == null) {
                try {
                    IBinder binder = service.getCarService(serviceName);
                    if (binder == null) {
                        Log.w(CarLibLog.TAG_CAR, "getCarManager could not get binder for service:" +
                                serviceName);
                        return null;
                    }
                    manager = createCarManager(serviceName, binder);
                    if (manager == null) {
                        Log.w(CarLibLog.TAG_CAR,
                                "getCarManager could not create manager for service:" +
                                serviceName);
                        return null;
                    }
                    mServiceMap.put(serviceName, manager);
                } catch (RemoteException e) {
                    handleRemoteException(e);
                }
            }
        }
        return manager;
    }

    /**
     * Return the type of currently connected car.
     * @return
     */
    @ConnectionType
    public int getCarConnectionType() {
        return CONNECTION_TYPE_EMBEDDED;
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

    private CarManagerBase createCarManager(String serviceName, IBinder binder)
            throws CarNotConnectedException {
        CarManagerBase manager = null;
        switch (serviceName) {
            case AUDIO_SERVICE:
                manager = new CarAudioManager(binder, mContext);
                break;
            case SENSOR_SERVICE:
                manager = new CarSensorManager(binder, mContext, mLooper);
                break;
            case INFO_SERVICE:
                manager = new CarInfoManager(binder);
                break;
            case APP_CONTEXT_SERVICE:
                manager = new CarAppContextManager(binder, mLooper);
                break;
            case PACKAGE_SERVICE:
                manager = new CarPackageManager(binder, mContext);
                break;
            case CAR_NAVIGATION_SERVICE:
                manager = new CarNavigationManager(binder, mLooper);
                break;
            case CAMERA_SERVICE:
                manager = new CarCameraManager(binder, mContext);
                break;
            case HVAC_SERVICE:
                manager = new CarHvacManager(binder, mContext, mLooper);
                break;
            case PROJECTION_SERVICE:
                manager = new CarProjectionManager(binder, mLooper);
                break;
            case RADIO_SERVICE:
                manager = new CarRadioManager(binder, mLooper);
                break;
            case TEST_SERVICE:
                /* CarTestManager exist in static library. So instead of constructing it here,
                 * only pass binder wrapper so that CarTestManager can be constructed outside. */
                manager = new CarTestManagerBinderWrapper(binder);
                break;
        }
        return manager;
    }

    private void startCarService() {
        Intent intent = new Intent();
        intent.setPackage(CAR_SERVICE_PACKAGE);
        intent.setAction(Car.CAR_SERVICE_INTERFACE_NAME);
        boolean bound = mContext.bindServiceAsUser(intent, mServiceConnectionListener,
                Context.BIND_AUTO_CREATE, UserHandle.CURRENT_OR_SELF);
        if (!bound) {
            mConnectionRetryCount++;
            if (mConnectionRetryCount > CAR_SERVICE_BIND_MAX_RETRY) {
                Log.w(CarLibLog.TAG_CAR, "cannot bind to car service after max retry");
                mMainThreadEvetHandler.post(mConnectionRetryFailedRunnable);
            } else {
                mEventHandler.postDelayed(mConnectionRetryRunnable,
                        CAR_SERVICE_BIND_RETRY_INTERVAL_MS);
            }
        } else {
            mConnectionRetryCount = 0;
        }
    }

    private synchronized ICar getICarOrThrow() throws IllegalStateException {
        if (mService == null) {
            throw new IllegalStateException("not connected");
        }
        return mService;
    }

    private void handleRemoteException(RemoteException e) {
        Log.w(CarLibLog.TAG_CAR, "RemoteException", e);
        disconnect();
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
