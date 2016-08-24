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

import android.location.GpsSatellite;
import android.location.Location;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.SystemClock;
import android.support.car.annotation.VersionDef;
import android.support.car.os.ExtendableParcelable;


/**
 * A CarSensorEvent object corresponds to a single sensor event coming from the car. The sensor
 * data is stored in a sensor-type specific format in the object's float and byte arrays.
 *
 * To aid unmarshalling the object's data arrays, this class provides static nested classes and
 * conversion methods, for example {@link EnvironmentData} and {@link #getEnvironmentData}. The
 * conversion methods each have an optional data parameter which, if not null, will be used and
 * returned. This parameter should be used to avoid unnecessary object churn whenever possible.
 * Additionally, calling a conversion method on a CarSensorEvent object with an inappropriate type
 * will result in an {@code UnsupportedOperationException} being thrown.
 */
public class CarSensorEvent extends ExtendableParcelable {

    private static final int VERSION = 1;

    /**
     * Index in {@link #floatValues} for {@link CarSensorManager#SENSOR_TYPE_FUEL_LEVEL} type of
     * sensor. This value is fuel level in percentile.
     */
    public static final int INDEX_FUEL_LEVEL_IN_PERCENTILE = 0;
    /**
     * Index in {@link #floatValues} for {@link CarSensorManager#SENSOR_TYPE_FUEL_LEVEL} type of
     * sensor. This value is fuel level in coverable distance. The unit is Km.
     */
    public static final int INDEX_FUEL_LEVEL_IN_DISTANCE = 1;
    /**
     * Index in {@link #intValues} for {@link CarSensorManager#SENSOR_TYPE_FUEL_LEVEL} type of
     * sensor. This value is set to 1 if fuel low level warning is on.
     */
    public static final int INDEX_FUEL_LOW_WARNING = 0;

    /**
     *  GEAR_* represents meaning of intValues[0] for {@link CarSensorManager#SENSOR_TYPE_GEAR}
     *  sensor type.
     *  GEAR_NEUTRAL means transmission gear is in neutral state, and the car may be moving.
     */
    public static final int GEAR_NEUTRAL    = 0;
    /**
     * intValues[0] from 1 to 99 represents transmission gear number for moving forward.
     * GEAR_FIRST is for gear number 1.
     */
    public static final int GEAR_FIRST      = 1;
    /** Gear number 2. */
    public static final int GEAR_SECOND     = 2;
    /** Gear number 3. */
    public static final int GEAR_THIRD      = 3;
    /** Gear number 4. */
    public static final int GEAR_FOURTH     = 4;
    /** Gear number 5. */
    public static final int GEAR_FIFTH      = 5;
    /** Gear number 6. */
    public static final int GEAR_SIXTH      = 6;
    /** Gear number 7. */
    public static final int GEAR_SEVENTH    = 7;
    /** Gear number 8. */
    public static final int GEAR_EIGHTH     = 8;
    /** Gear number 9. */
    public static final int GEAR_NINTH      = 9;
    /** Gear number 10. */
    public static final int GEAR_TENTH      = 10;
    /**
     * This is for transmission without specific gear number for moving forward like CVT. It tells
     * that car is in a transmission state to move it forward.
     */
    public static final int GEAR_DRIVE      = 100;
    /** Gear in parking state */
    public static final int GEAR_PARK       = 101;
    /** Gear in reverse */
    public static final int GEAR_REVERSE    = 102;

    /**
     * Bitmask of driving restrictions.
     */
    /** No restrictions. */
    public static final int DRIVE_STATUS_UNRESTRICTED = 0;
    /** No video playback allowed. */
    public static final int DRIVE_STATUS_NO_VIDEO = 0x1;
    /** No keyboard or rotary controller input allowed. */
    public static final int DRIVE_STATUS_NO_KEYBOARD_INPUT = 0x2;
    /** No voice input allowed. */
    public static final int DRIVE_STATUS_NO_VOICE_INPUT = 0x4;
    /** No setup / configuration allowed. */
    public static final int DRIVE_STATUS_NO_CONFIG = 0x8;
    /** Limit displayed message length. */
    public static final int DRIVE_STATUS_LIMIT_MESSAGE_LEN = 0x10;
    /** represents case where all of the above items are restricted */
    public static final int DRIVE_STATUS_FULLY_RESTRICTED = DRIVE_STATUS_NO_VIDEO |
            DRIVE_STATUS_NO_KEYBOARD_INPUT | DRIVE_STATUS_NO_VOICE_INPUT | DRIVE_STATUS_NO_CONFIG |
            DRIVE_STATUS_LIMIT_MESSAGE_LEN;
    /**
     * Index for {@link CarSensorManager#SENSOR_TYPE_LOCATION} in floatValues.
     * Each bit intValues[0] represents whether the corresponding data is present.
     */
    public static final int INDEX_LOCATION_LATITUDE  = 0;
    public static final int INDEX_LOCATION_LONGITUDE = 1;
    public static final int INDEX_LOCATION_ACCURACY  = 2;
    public static final int INDEX_LOCATION_ALTITUDE  = 3;
    public static final int INDEX_LOCATION_SPEED     = 4;
    public static final int INDEX_LOCATION_BEARING   = 5;
    public static final int INDEX_LOCATION_MAX = INDEX_LOCATION_BEARING;
    public static final int INDEX_LOCATION_LATITUDE_INTS = 1;
    public static final int INDEX_LOCATION_LONGITUDE_INTS = 2;

    /**
     * Index for {@link CarSensorManager#SENSOR_TYPE_ENVIRONMENT} in floatValues.
     * Temperature in Celsius degrees.
     */
    public static final int INDEX_ENVIRONMENT_TEMPERATURE = 0;
    /**
     * Index for {@link CarSensorManager#SENSOR_TYPE_ENVIRONMENT} in floatValues.
     * Pressure in kPa.
     */
    public static final int INDEX_ENVIRONMENT_PRESSURE = 1;

    /**
     * Indices for {@link CarSensorManager#SENSOR_TYPE_COMPASS} in floatValues.
     * Angles are in degrees. Pitch or/and roll can be NaN if it is not available.
     */
    public static final int INDEX_COMPASS_BEARING = 0;
    public static final int INDEX_COMPASS_PITCH   = 1;
    public static final int INDEX_COMPASS_ROLL    = 2;

    /**
     * Indices for {@link CarSensorManager#SENSOR_TYPE_ACCELEROMETER} in floatValues.
     * Acceleration (gravity) is in m/s^2. Any component can be NaN if it is not available.
     */
    public static final int INDEX_ACCELEROMETER_X = 0;
    public static final int INDEX_ACCELEROMETER_Y = 1;
    public static final int INDEX_ACCELEROMETER_Z = 2;

    /**
     * Indices for {@link CarSensorManager#SENSOR_TYPE_GYROSCOPE} in floatValues.
     * Rotation speed is in rad/s. Any component can be NaN if it is not available.
     */
    public static final int INDEX_GYROSCOPE_X = 0;
    public static final int INDEX_GYROSCOPE_Y = 1;
    public static final int INDEX_GYROSCOPE_Z = 2;

    /**
     * Indices for {@link CarSensorManager#SENSOR_TYPE_GPS_SATELLITE}.
     * Both byte values and float values are used.
     * Two first bytes encode number of satellites in-use/in-view (or 0xFF if unavailable).
     * Then optionally with INDEX_GPS_SATELLITE_ARRAY_BYTE_OFFSET offset and interval
     * INDEX_GPS_SATELLITE_ARRAY_BYTE_INTERVAL between elements are encoded boolean flags of whether
     * particular satellite from in-view participate in in-use subset.
     * Float values with INDEX_GPS_SATELLITE_ARRAY_FLOAT_OFFSET offset and interval
     * INDEX_GPS_SATELLITE_ARRAY_FLOAT_INTERVAL between elements can optionally contain
     * per-satellite values of signal strength and other values or NaN if unavailable.
     */
    public static final int INDEX_GPS_SATELLITE_NUMBER_IN_USE = 0;
    public static final int INDEX_GPS_SATELLITE_NUMBER_IN_VIEW = 1;
    public static final int INDEX_GPS_SATELLITE_ARRAY_INT_OFFSET = 2;
    public static final int INDEX_GPS_SATELLITE_ARRAY_INT_INTERVAL = 1;
    public static final int INDEX_GPS_SATELLITE_ARRAY_FLOAT_OFFSET = 0;
    public static final int INDEX_GPS_SATELLITE_ARRAY_FLOAT_INTERVAL = 4;
    public static final int INDEX_GPS_SATELLITE_PRN_OFFSET = 0;
    public static final int INDEX_GPS_SATELLITE_SNR_OFFSET = 1;
    public static final int INDEX_GPS_SATELLITE_AZIMUTH_OFFSET = 2;
    public static final int INDEX_GPS_SATELLITE_ELEVATION_OFFSET = 3;

    private static final long MILLI_IN_NANOS = 1000000L;

    /** Sensor type for this event like {@link CarSensorManager#SENSOR_TYPE_CAR_SPEED}. */
    @VersionDef(version = 1)
    public int sensorType;

    /**
     * When this data was acquired in car or received from car. It is elapsed real-time of data
     * reception from car in nanoseconds since system boot.
     */
    @VersionDef(version = 1)
    public long timeStampNs;
    /**
     * array holding float type of sensor data. If the sensor has single value, only floatValues[0]
     * should be used. */
    @VersionDef(version = 1)
    public final float[] floatValues;
    /** array holding int type of sensor data */
    @VersionDef(version = 1)
    public final int[] intValues;

    public CarSensorEvent(Parcel in) {
        super(in, VERSION);
        int lastPosition = readHeader(in);
        sensorType = in.readInt();
        timeStampNs = in.readLong();
        int len = in.readInt();
        floatValues = new float[len];
        in.readFloatArray(floatValues);
        len = in.readInt();
        intValues = new int[len];
        in.readIntArray(intValues);
        // version 1 up to here
        completeReading(in, lastPosition);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        int startingPosition = writeHeader(dest);
        dest.writeInt(sensorType);
        dest.writeLong(timeStampNs);
        dest.writeInt(floatValues.length);
        dest.writeFloatArray(floatValues);
        dest.writeInt(intValues.length);
        dest.writeIntArray(intValues);
        // version 1 up to here
        completeWriting(dest, startingPosition);
    }

    public static final Parcelable.Creator<CarSensorEvent> CREATOR
    = new Parcelable.Creator<CarSensorEvent>() {
        public CarSensorEvent createFromParcel(Parcel in) {
            return new CarSensorEvent(in);
        }

        public CarSensorEvent[] newArray(int size) {
            return new CarSensorEvent[size];
        }
    };

    public CarSensorEvent(int sensorType, long timeStampNs, int floatValueSize, int intValueSize) {
        super(VERSION);
        this.sensorType = sensorType;
        this.timeStampNs = timeStampNs;
        floatValues = new float[floatValueSize];
        intValues = new int[intValueSize];
    }

    /** @hide */
    CarSensorEvent(int sensorType, long timeStampNs, float[] floatValues, int[] intValues) {
        super(VERSION);
        this.sensorType = sensorType;
        this.timeStampNs = timeStampNs;
        this.floatValues = floatValues;
        this.intValues = intValues;
    }

    private void checkType(int type) {
        if (sensorType == type) {
            return;
        }
        throw new UnsupportedOperationException(String.format(
                "Invalid sensor type: expected %d, got %d", type, sensorType));
    }

    public static class EnvironmentData {
        public long timeStampNs;
        /** If unsupported by the car, this value is NaN. */
        public float temperature;
        /** If unsupported by the car, this value is NaN. */
        public float pressure;
    }

    /**
     * Convenience method for obtaining an {@link EnvironmentData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_ENVIRONMENT}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return an EnvironmentData object corresponding to the data contained in the CarSensorEvent.
     */
    public EnvironmentData getEnvironmentData(EnvironmentData data) {
        checkType(CarSensorManager.SENSOR_TYPE_ENVIRONMENT);
        if (data == null) {
            data = new EnvironmentData();
        }
        data.timeStampNs = timeStampNs;
        data.temperature = floatValues[INDEX_ENVIRONMENT_TEMPERATURE];
        data.pressure = floatValues[INDEX_ENVIRONMENT_PRESSURE];
        return data;
    }

    public static class NightData {
        public long timeStampNs;
        public boolean isNightMode;
    }

    /**
     * Convenience method for obtaining a {@link NightData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_NIGHT}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a NightData object corresponding to the data contained in the CarSensorEvent.
     */
    public NightData getNightData(NightData data) {
        checkType(CarSensorManager.SENSOR_TYPE_NIGHT);
        if (data == null) {
            data = new NightData();
        }
        data.timeStampNs = timeStampNs;
        data.isNightMode = intValues[0] == 1;
        return data;
    }

    public static class GearData {
        public long timeStampNs;
        public int gear;
    }

    /**
     * Convenience method for obtaining a {@link GearData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_GEAR}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a GearData object corresponding to the data contained in the CarSensorEvent.
     */
    public GearData getGearData(GearData data) {
        checkType(CarSensorManager.SENSOR_TYPE_GEAR);
        if (data == null) {
            data = new GearData();
        }
        data.timeStampNs = timeStampNs;
        data.gear = intValues[0];
        return data;
    }

    public static class ParkingBrakeData {
        public long timeStampNs;
        public boolean isEngaged;
    }

    /**
     * Convenience method for obtaining a {@link ParkingBrakeData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_PARKING_BRAKE}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a ParkingBreakData object corresponding to the data contained in the CarSensorEvent.
     */
    public ParkingBrakeData getParkingBrakeData(ParkingBrakeData data) {
        checkType(CarSensorManager.SENSOR_TYPE_PARKING_BRAKE);
        if (data == null) {
            data = new ParkingBrakeData();
        }
        data.timeStampNs = timeStampNs;
        data.isEngaged = intValues[0] == 1;
        return data;
    }

    public static class FuelLevelData {
        public long timeStampNs;
        /** Fuel level in %. If unsupported by the car, this value is -1. */
        public int level;
        /** Fuel as possible range in Km. If unsupported by the car, this value is -1. */
        public float range;
        /** If unsupported by the car, this value is false. */
        public boolean lowFuelWarning;
    }

    /**
     * Convenience method for obtaining a {@link FuelLevelData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_FUEL_LEVEL}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a FuelLevel object corresponding to the data contained in the CarSensorEvent.
     */
    public FuelLevelData getFuelLevelData(FuelLevelData data) {
        checkType(CarSensorManager.SENSOR_TYPE_FUEL_LEVEL);
        if (data == null) {
            data = new FuelLevelData();
        }
        data.timeStampNs = timeStampNs;
        if (floatValues == null) {
            data.level = -1;
            data.range = -1;
        } else {
            if (floatValues[INDEX_FUEL_LEVEL_IN_PERCENTILE] < 0) {
                data.level = -1;
            } else {
                data.level = (int) floatValues[INDEX_FUEL_LEVEL_IN_PERCENTILE];
            }
            if (floatValues[INDEX_FUEL_LEVEL_IN_DISTANCE] < 0) {
                data.range = -1;
            } else {
                data.range = floatValues[INDEX_FUEL_LEVEL_IN_DISTANCE];
            }
        }
        data.lowFuelWarning = intValues[0] == 1;
        return data;
    }

    public static class OdometerData {
        public long timeStampNs;
        public float kms;
    }

    /**
     * Convenience method for obtaining an {@link OdometerData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_ODOMETER}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return an OdometerData object corresponding to the data contained in the CarSensorEvent.
     */
    public OdometerData getOdometerData(OdometerData data) {
        checkType(CarSensorManager.SENSOR_TYPE_ODOMETER);
        if (data == null) {
            data = new OdometerData();
        }
        data.timeStampNs = timeStampNs;
        data.kms = floatValues[0];
        return data;
    }

    public static class RpmData {
        public long timeStampNs;
        public float rpm;
    }

    /**
     * Convenience method for obtaining a {@link RpmData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_RPM}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a RpmData object corresponding to the data contained in the CarSensorEvent.
     */
    public RpmData getRpmData(RpmData data) {
        checkType(CarSensorManager.SENSOR_TYPE_RPM);
        if (data == null) {
            data = new RpmData();
        }
        data.timeStampNs = timeStampNs;
        data.rpm = floatValues[0];
        return data;
    }

    public static class CarSpeedData {
        public long timeStampNs;
        public float carSpeed;
    }

    /**
     * Convenience method for obtaining a {@link CarSpeedData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_CAR_SPEED}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a CarSpeedData object corresponding to the data contained in the CarSensorEvent.
     */
    public CarSpeedData getCarSpeedData(CarSpeedData data) {
        checkType(CarSensorManager.SENSOR_TYPE_CAR_SPEED);
        if (data == null) {
            data = new CarSpeedData();
        }
        data.timeStampNs = timeStampNs;
        data.carSpeed = floatValues[0];
        return data;
    }

    public static class CompassData {
        public long timeStampNs;
        /** If unsupported by the car, this value is NaN. */
        public float bearing;
        /** If unsupported by the car, this value is NaN. */
        public float pitch;
        /** If unsupported by the car, this value is NaN. */
        public float roll;
    }

    /**
     * Convenience method for obtaining a {@link CompassData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_COMPASS}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a CompassData object corresponding to the data contained in the CarSensorEvent.
     */
    public CompassData getCompassData(CompassData data) {
        checkType(CarSensorManager.SENSOR_TYPE_COMPASS);
        if (data == null) {
            data = new CompassData();
        }
        data.bearing = floatValues[INDEX_COMPASS_BEARING];
        data.pitch = floatValues[INDEX_COMPASS_PITCH];
        data.roll = floatValues[INDEX_COMPASS_ROLL];
        return data;
    }

    /**
     * Convenience method for obtaining a {@link Location} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_LOCATION}.
     *
     * @param location an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a Location object corresponding to the data contained in the CarSensorEvent.
     */
    public Location getLocation(Location location) {
        checkType(CarSensorManager.SENSOR_TYPE_LOCATION);
        if (location == null) {
            location = new Location("Car-GPS");
        }
        // intValues[0]: bit flags for the presence of other values following.
        int presense = intValues[0];
        if ((presense & (0x1 << INDEX_LOCATION_LATITUDE)) != 0) {
            int latE7 = intValues[INDEX_LOCATION_LATITUDE_INTS];
            location.setLatitude(latE7 * 1e-7);
        }
        if ((presense & (0x1 << INDEX_LOCATION_LONGITUDE)) != 0) {
            int longE7 = intValues[INDEX_LOCATION_LONGITUDE_INTS];
            location.setLongitude(longE7 * 1e-7);
        }
        if ((presense & (0x1 << INDEX_LOCATION_ACCURACY)) != 0) {
            location.setAccuracy(floatValues[INDEX_LOCATION_ACCURACY]);
        }
        if ((presense & (0x1 << INDEX_LOCATION_ALTITUDE)) != 0) {
            location.setAltitude(floatValues[INDEX_LOCATION_ALTITUDE]);
        }
        if ((presense & (0x1 << INDEX_LOCATION_SPEED)) != 0) {
            location.setSpeed(floatValues[INDEX_LOCATION_SPEED]);
        }
        if ((presense & (0x1 << INDEX_LOCATION_BEARING)) != 0) {
            location.setBearing(floatValues[INDEX_LOCATION_BEARING]);
        }
        location.setElapsedRealtimeNanos(timeStampNs);
        // There is a risk of scheduler delaying 2nd elapsedRealtimeNs value.
        // But will not try to fix it assuming that is acceptable as UTC time's accuracy is not
        // guaranteed in Location data.
        long currentTimeMs = System.currentTimeMillis();
        long elapsedRealtimeNs = SystemClock.elapsedRealtimeNanos();
        location.setTime(
                currentTimeMs - (elapsedRealtimeNs - timeStampNs) / MILLI_IN_NANOS);
        return location;
    }

    public static class DrivingStatusData {
        public long timeStampNs;
        public int status;
    }

    /**
     * Convenience method for obtaining a {@link DrivingStatusData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_DRIVING_STATUS}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a DrivingStatusData object corresponding to the data contained in the CarSensorEvent.
     */
    public DrivingStatusData getDrivingStatusData(DrivingStatusData data) {
        checkType(CarSensorManager.SENSOR_TYPE_DRIVING_STATUS);
        if (data == null) {
            data = new DrivingStatusData();
        }
        data.status = intValues[0];
        return data;
    }

    public static class AccelerometerData  {
        public long timeStampNs;
        /** If unsupported by the car, this value is NaN. */
        public float x;
        /** If unsupported by the car, this value is NaN. */
        public float y;
        /** If unsupported by the car, this value is NaN. */
        public float z;
    }

    /**
     * Convenience method for obtaining an {@link AccelerometerData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_ACCELEROMETER}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a AccelerometerData object corresponding to the data contained in the CarSensorEvent.
     */
    public AccelerometerData getAccelerometerData(AccelerometerData data) {
        checkType(CarSensorManager.SENSOR_TYPE_ACCELEROMETER);
        if (data == null) {
            data = new AccelerometerData();
        }
        data.x = floatValues[INDEX_ACCELEROMETER_X];
        data.y = floatValues[INDEX_ACCELEROMETER_Y];
        data.z = floatValues[INDEX_ACCELEROMETER_Z];
        return data;
    }

    public static class GyroscopeData {
        public long timeStampNs;
        /** If unsupported by the car, this value is NaN. */
        public float x;
        /** If unsupported by the car, this value is NaN. */
        public float y;
        /** If unsupported by the car, this value is NaN. */
        public float z;
    }

    /**
     * Convenience method for obtaining a {@link GyroscopeData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_GYROSCOPE}.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @return a GyroscopeData object corresponding to the data contained in the CarSensorEvent.
     */
    public GyroscopeData getGyroscopeData(GyroscopeData data) {
        checkType(CarSensorManager.SENSOR_TYPE_GYROSCOPE);
        if (data == null) {
            data = new GyroscopeData();
        }
        data.x = floatValues[INDEX_GYROSCOPE_X];
        data.y = floatValues[INDEX_GYROSCOPE_Y];
        data.z = floatValues[INDEX_GYROSCOPE_Z];
        return data;
    }

    // android.location.GpsSatellite doesn't have a public constructor, so that can't be used.
    /**
     * Class that contains GPS satellite status. For more info on meaning of these fields refer
     * to the documentation to the {@link GpsSatellite} class.
     */
    public static class GpsSatelliteData {
        public long timeStampNs;
        /**
         * Number of satellites used in GPS fix or -1 of unavailable.
         */
        public int numberInUse = -1;
        /**
         * Number of satellites in view or -1 of unavailable.
         */
        public int numberInView = -1;
        /**
         * Per-satellite flag if this satellite was used for GPS fix.
         * Can be null if per-satellite data is unavailable.
         */
        public boolean[] usedInFix = null;
        /**
         * Per-satellite pseudo-random id.
         * Can be null if per-satellite data is unavailable.
         */
        public int[] prn = null;
        /**
         * Per-satellite signal to noise ratio.
         * Can be null if per-satellite data is unavailable.
         */
        public float[] snr = null;
        /**
         * Per-satellite azimuth.
         * Can be null if per-satellite data is unavailable.
         */
        public float[] azimuth = null;
        /**
         * Per-satellite elevation.
         * Can be null if per-satellite data is unavailable.
         */
        public float[] elevation = null;
    }

    /**
     * Convenience method for obtaining a {@link GpsSatelliteData} object from a CarSensorEvent
     * object with type {@link CarSensorManager#SENSOR_TYPE_HVAC} with optional per-satellite info.
     *
     * @param data an optional output parameter which, if non-null, will be used by this method
     *     instead of a newly created object.
     * @param withPerSatellite whether to include per-satellite data.
     * @return a GpsSatelliteData object corresponding to the data contained in the CarSensorEvent.
     */
    public GpsSatelliteData getGpsSatelliteData(GpsSatelliteData data,
            boolean withPerSatellite) {
        checkType(CarSensorManager.SENSOR_TYPE_GPS_SATELLITE);
        if (data == null) {
            data = new GpsSatelliteData();
        }
        final int intOffset = CarSensorEvent.INDEX_GPS_SATELLITE_ARRAY_INT_OFFSET;
        final int intInterval = CarSensorEvent.INDEX_GPS_SATELLITE_ARRAY_INT_INTERVAL;
        final int floatOffset = CarSensorEvent.INDEX_GPS_SATELLITE_ARRAY_FLOAT_OFFSET;
        final int floatInterval = CarSensorEvent.INDEX_GPS_SATELLITE_ARRAY_FLOAT_INTERVAL;
        final int numberOfSats = (floatValues.length - floatOffset) / floatInterval;

        data.numberInUse = intValues[CarSensorEvent.INDEX_GPS_SATELLITE_NUMBER_IN_USE];
        data.numberInView = intValues[CarSensorEvent.INDEX_GPS_SATELLITE_NUMBER_IN_VIEW];
        if (withPerSatellite && data.numberInView >= 0) {
            data.usedInFix = new boolean[numberOfSats];
            data.prn = new int[numberOfSats];
            data.snr = new float[numberOfSats];
            data.azimuth = new float[numberOfSats];
            data.elevation = new float[numberOfSats];

            for (int i = 0; i < numberOfSats; ++i) {
                int iInt = intOffset + intInterval * i;
                int iFloat = floatOffset + floatInterval * i;
                data.usedInFix[i] = intValues[iInt] != 0;
                data.prn[i] = Math.round(
                        floatValues[iFloat + CarSensorEvent.INDEX_GPS_SATELLITE_PRN_OFFSET]);
                data.snr[i] =
                        floatValues[iFloat + CarSensorEvent.INDEX_GPS_SATELLITE_SNR_OFFSET];
                data.azimuth[i] = floatValues[iFloat
                        + CarSensorEvent.INDEX_GPS_SATELLITE_AZIMUTH_OFFSET];
                data.elevation[i] = floatValues[iFloat
                        + CarSensorEvent.INDEX_GPS_SATELLITE_ELEVATION_OFFSET];
            }
        }
        return data;
    }

    /** @hide */
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(getClass().getName() + "[");
        sb.append("type:" + Integer.toHexString(sensorType));
        if (floatValues != null && floatValues.length > 0) {
            sb.append(" float values:");
            for (float v: floatValues) {
                sb.append(" " + v);
            }
        }
        if (intValues != null && intValues.length > 0) {
            sb.append(" int values:");
            for (int v: intValues) {
                sb.append(" " + v);
            }
        }
        sb.append("]");
        return sb.toString();
    }
}
