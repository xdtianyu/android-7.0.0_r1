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

package android.car.hardware;

import android.os.Parcel;
import android.os.Parcelable;


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
public class CarSensorEvent implements Parcelable {

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
     * Index for {@link CarSensorManager#SENSOR_TYPE_ENVIRONMENT} in floatValues.
     * Temperature in Celsius degrees.
     */
    public static final int INDEX_ENVIRONMENT_TEMPERATURE = 0;
    /**
     * Index for {@link CarSensorManager#SENSOR_TYPE_ENVIRONMENT} in floatValues.
     * Pressure in kPa.
     */
    public static final int INDEX_ENVIRONMENT_PRESSURE = 1;

    private static final long MILLI_IN_NANOS = 1000000L;

    /** Sensor type for this event like {@link CarSensorManager#SENSOR_TYPE_CAR_SPEED}. */
    public int sensorType;

    /**
     * When this data was acquired in car or received from car. It is elapsed real-time of data
     * reception from car in nanoseconds since system boot.
     */
    public long timeStampNs;
    /**
     * array holding float type of sensor data. If the sensor has single value, only floatValues[0]
     * should be used. */
    public final float[] floatValues;
    /** array holding int type of sensor data */
    public final int[] intValues;

    public CarSensorEvent(Parcel in) {
        sensorType = in.readInt();
        timeStampNs = in.readLong();
        int len = in.readInt();
        floatValues = new float[len];
        in.readFloatArray(floatValues);
        len = in.readInt();
        intValues = new int[len];
        in.readIntArray(intValues);
        // version 1 up to here
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(sensorType);
        dest.writeLong(timeStampNs);
        dest.writeInt(floatValues.length);
        dest.writeFloatArray(floatValues);
        dest.writeInt(intValues.length);
        dest.writeIntArray(intValues);
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
        this.sensorType = sensorType;
        this.timeStampNs = timeStampNs;
        floatValues = new float[floatValueSize];
        intValues = new int[intValueSize];
    }

    /** @hide */
    CarSensorEvent(int sensorType, long timeStampNs, float[] floatValues, int[] intValues) {
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
