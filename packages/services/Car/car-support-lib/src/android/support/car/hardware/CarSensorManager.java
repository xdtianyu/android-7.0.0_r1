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

package android.support.car.hardware;

import android.Manifest;
import android.os.Looper;
import android.support.annotation.RequiresPermission;
import android.support.car.Car;
import android.support.car.CarManagerBase;
import android.support.car.CarNotConnectedException;

/**
 *  API for monitoring car sensor data.
 */
public abstract class CarSensorManager implements CarManagerBase {
    /**
     * SENSOR_TYPE_* represents type of sensor supported from the connected car. This sensor
     * represents the direction of the car as an angle in degree measured clockwise with 0 degree
     * pointing to north. Sensor data in {@link CarSensorEvent} is a float (floatValues[0]).
     */
    public static final int SENSOR_TYPE_COMPASS = 1;
    /**
     * This sensor represents vehicle speed in m/s. Sensor data in {@link CarSensorEvent} is a float
     * which will be >= 0. This requires {@link Car#PERMISSION_SPEED} permission.
     */
    public static final int SENSOR_TYPE_CAR_SPEED = 2;
    /**
     * Represents engine RPM of the car. Sensor data in {@link CarSensorEvent} is a float.
     */
    public static final int SENSOR_TYPE_RPM = 3;
    /**
     * Total travel distance of the car in Kilometer. Sensor data is a float. This requires {@link
     * Car#PERMISSION_MILEAGE} permission.
     */
    public static final int SENSOR_TYPE_ODOMETER = 4;
    /**
     * Indicates fuel level of the car. In {@link CarSensorEvent}, floatValues[{@link
     * CarSensorEvent#INDEX_FUEL_LEVEL_IN_PERCENTILE}] represents fuel level in percentile (0 to
     * 100) while floatValues[{@link CarSensorEvent#INDEX_FUEL_LEVEL_IN_DISTANCE}] represents
     * estimated range in Kilometer with the remaining fuel. Note that the gas mileage used for the
     * estimation may not represent the current driving condition. This requires {@link
     * Car#PERMISSION_FUEL} permission.
     */
    public static final int SENSOR_TYPE_FUEL_LEVEL = 5;
    /**
     * Represents the current status of parking brake. Sensor data in {@link CarSensorEvent} is an
     * intValues[0]. Value of 1 represents parking brake applied while 0 means the other way around.
     * For this sensor, rate in {@link #registerListener(CarSensorEventListener, int, int)} will be
     * ignored and all changes will be notified.
     */
    public static final int SENSOR_TYPE_PARKING_BRAKE = 6;
    /**
     * This represents the current position of transmission gear. Sensor data in {@link
     * CarSensorEvent} is an intValues[0]. For the meaning of the value, check {@link
     * CarSensorEvent#GEAR_NEUTRAL} and other GEAR_*.
     */
    public static final int SENSOR_TYPE_GEAR = 7;

    /**@hide*/
    public static final int SENSOR_TYPE_RESERVED8 = 8;

    /**
     * Day/night sensor. Sensor data is intValues[0].
     */
    public static final int SENSOR_TYPE_NIGHT = 9;
    /**
     * Sensor type for location. Sensor data passed in floatValues.
     */
    public static final int SENSOR_TYPE_LOCATION = 10;
    /**
     * Represents the current driving status of car. Different user interaction should be used
     * depending on the current driving status. Driving status is intValues[0].
     */
    public static final int SENSOR_TYPE_DRIVING_STATUS = 11;
    /**
     * Environment like temperature and pressure.
     */
    public static final int SENSOR_TYPE_ENVIRONMENT = 12;
    /** @hide */
    public static final int SENSOR_TYPE_RESERVED13 = 13;
    /** @hide */
    public static final int SENSOR_TYPE_ACCELEROMETER = 14;
    /** @hide */
    public static final int SENSOR_TYPE_RESERVED15 = 15;
    /** @hide */
    public static final int SENSOR_TYPE_RESERVED16 = 16;
    /** @hide */
    public static final int SENSOR_TYPE_GPS_SATELLITE = 17;
    /** @hide */
    public static final int SENSOR_TYPE_GYROSCOPE = 18;
    /** @hide */
    public static final int SENSOR_TYPE_RESERVED19 = 19;
    /** @hide */
    public static final int SENSOR_TYPE_RESERVED20 = 20;
    /** @hide */
    public static final int SENSOR_TYPE_RESERVED21 = 21;

    /**
     * Sensor type bigger than this is invalid. Always update this after adding a new sensor.
     */
    private static final int SENSOR_TYPE_MAX = SENSOR_TYPE_RESERVED21;

    /**
     * Sensors defined in this range [{@link #SENSOR_TYPE_VENDOR_EXTENSION_START},
     * {@link #SENSOR_TYPE_VENDOR_EXTENSION_END}] is for each car vendor's to use.
     * This should be only used for system app to access sensors not defined as standard types.
     * So the sensor supproted in this range can vary depending on car models / manufacturers.
     * 3rd party apps should not use sensors in this range as they are not compatible across
     * different cars. Additionally 3rd party apps trying to access sensor in this range will get
     * security exception as their access is restricted to system apps.
     *
     * @hide
     */
    public static final int SENSOR_TYPE_VENDOR_EXTENSION_START = 0x60000000;
    public static final int SENSOR_TYPE_VENDOR_EXTENSION_END   = 0x6fffffff;

    /** Read sensor in default normal rate set for each sensors. This is default rate. */
    public static final int SENSOR_RATE_NORMAL  = 3;
    public static final int SENSOR_RATE_UI = 2;
    public static final int SENSOR_RATE_FAST = 1;
    /** Read sensor at the maximum rate. Actual rate will be different depending on the sensor. */
    public static final int SENSOR_RATE_FASTEST = 0;

    /**
     * Listener for car sensor data change.
     * Callbacks are called in the Looper context.
     */
    public interface CarSensorEventListener {
        /**
         * Called when there is a new sensor data from car.
         * @param event Incoming sensor event for the given sensor type.
         */
        void onSensorChanged(final CarSensorEvent event);
    }

    /**
     * Give the list of CarSensors available in the connected car.
     * @return array of all sensor types supported.
     * @throws CarNotConnectedException
     */
    public abstract int[] getSupportedSensors() throws CarNotConnectedException;

    /**
     * Tells if given sensor is supported or not.
     * @param sensorType
     * @return true if the sensor is supported.
     * @throws CarNotConnectedException
     */
    public abstract boolean isSensorSupported(int sensorType) throws CarNotConnectedException;

    /**
     * Check if given sensorList is including the sensorType.
     * @param sensorList
     * @param sensorType
     * @return
     */
    public static boolean isSensorSupported(int[] sensorList, int sensorType) {
        for (int sensorSupported: sensorList) {
            if (sensorType == sensorSupported) {
                return true;
            }
        }
        return false;
    }

    /**
     * Register {@link CarSensorEventListener} to get repeated sensor updates. Multiple listeners
     * can be registered for a single sensor or the same listener can be used for different sensors.
     * If the same listener is registered again for the same sensor, it will be either ignored or
     * updated depending on the rate.
     * <p>
     * Requires {@link android.Manifest.permission#ACCESS_FINE_LOCATION} for
     * {@link #SENSOR_TYPE_LOCATION}, {@link Car#PERMISSION_SPEED} for
     * {@link #SENSOR_TYPE_CAR_SPEED}, {@link Car#PERMISSION_MILEAGE} for
     * {@link #SENSOR_TYPE_ODOMETER}, or {@link Car#PERMISSION_FUEL} for
     * {@link #SENSOR_TYPE_FUEL_LEVEL}.
     *
     * @param listener
     * @param sensorType sensor type to subscribe.
     * @param rate how fast the sensor events are delivered. It should be one of
     *        {@link #SENSOR_RATE_FASTEST} or {@link #SENSOR_RATE_NORMAL}. Rate may not be respected
     *        especially when the same sensor is registered with different listener with different
     *        rates.
     * @return if the sensor was successfully enabled.
     * @throws CarNotConnectedException
     * @throws IllegalArgumentException for wrong argument like wrong rate
     * @throws SecurityException if missing the appropriate permission
     */
    @RequiresPermission(anyOf={Manifest.permission.ACCESS_FINE_LOCATION, Car.PERMISSION_SPEED,
            Car.PERMISSION_MILEAGE, Car.PERMISSION_FUEL}, conditional=true)
    public abstract boolean registerListener(CarSensorEventListener listener, int sensorType,
            int rate) throws CarNotConnectedException, IllegalArgumentException;

    /**
     * Stop getting sensor update for the given listener. If there are multiple registrations for
     * this listener, all listening will be stopped.
     * @param listener
     */
    public abstract  void unregisterListener(CarSensorEventListener listener)
            throws CarNotConnectedException;

    /**
     * Stop getting sensor update for the given listener and sensor. If the same listener is used
     * for other sensors, those subscriptions will not be affected.
     * @param listener
     * @param sensorType
     */
    public abstract  void unregisterListener(CarSensorEventListener listener, int sensorType)
            throws CarNotConnectedException;

    /**
     * Get the most recent CarSensorEvent for the given type.
     * @param type A sensor to request
     * @return null if there was no sensor update since connected to the car.
     * @throws CarNotConnectedException
     */
    public abstract CarSensorEvent getLatestSensorEvent(int type) throws CarNotConnectedException;
}
