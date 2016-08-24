/*
 * Copyright (C) 2016 The Android Open Source Project
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
package android.support.car.navigation;

import android.graphics.Bitmap;
import android.support.car.CarManagerBase;
import android.support.car.CarNotConnectedException;

/**
 * API for providing navigation status for instrument cluster.
 * @hide
 */
public abstract class CarNavigationManager implements CarManagerBase {

    /**
     * Listener navigation related events.
     * Callbacks are called in the Looper context.
     */
    public interface CarNavigationListener {
        /** Instrument Cluster started in navigation mode */
        void onInstrumentClusterStart(CarNavigationInstrumentCluster instrumentCluster);
        /** Instrument cluster ended */
        void onInstrumentClusterStop();
    }

    /** Navigation status */
    public static final int STATUS_UNAVAILABLE = 0;
    public static final int STATUS_ACTIVE = 1;
    public static final int STATUS_INACTIVE = 2;
    /** Turn Types */
    public static final int TURN_UNKNOWN = 0;
    public static final int TURN_DEPART = 1;
    public static final int TURN_NAME_CHANGE = 2;
    public static final int TURN_SLIGHT_TURN = 3;
    public static final int TURN_TURN = 4;
    public static final int TURN_SHARP_TURN = 5;
    public static final int TURN_U_TURN = 6;
    public static final int TURN_ON_RAMP = 7;
    public static final int TURN_OFF_RAMP = 8;
    public static final int TURN_FORK = 9;
    public static final int TURN_MERGE = 10;
    public static final int TURN_ROUNDABOUT_ENTER = 11;
    public static final int TURN_ROUNDABOUT_EXIT = 12;
    public static final int TURN_ROUNDABOUT_ENTER_AND_EXIT = 13;
    public static final int TURN_STRAIGHT = 14;
    public static final int TURN_FERRY_BOAT = 16;
    public static final int TURN_FERRY_TRAIN = 17;
    public static final int TURN_DESTINATION = 19;
    /** Turn Side */
    public static final int TURN_SIDE_LEFT = 1;
    public static final int TURN_SIDE_RIGHT = 2;
    public static final int TURN_SIDE_UNSPECIFIED = 3;

    /**
     * @param status new instrument cluster navigation status.
     * @return true if successful.
     * @throws CarNotConnectedException
     */
    public abstract boolean sendNavigationStatus(int status) throws CarNotConnectedException;

    /**
     * Sends a Navigation Next Step event to the car.
     * <p>
     * Note: For an example of a roundabout: if a roundabout has 4 exits, spaced evenly, then the
     * first exit will have turnNumber=1, turnAngle=90; the second will have turnNumber=2,
     * turnAngle=180; the third will have turnNumber=3, turnAngle=270.  turnNumber and turnAngle are
     * counted in the direction of travel around the roundabout (clockwise for roads where the car
     * drives on the left-hand side of the road, such as Australia; anti-clockwise for roads where
     * the car drives on the right, such as the USA).
     *
     * @param event event type ({@link #TURN_TURN}, {@link #TURN_U_TURN},
     *        {@link #TURN_ROUNDABOUT_ENTER_AND_EXIT}, etc).
     * @param road Name of the road
     * @param turnAngle turn angle in degrees between the roundabout entry and exit (0..359).  Only
     *        used for event type {@link #TURN_ROUNDABOUT_ENTER_AND_EXIT}.  -1 if unused.
     * @param turnNumber turn number, counting around from the roundabout entry to the exit.  Only
     *        used for event type {@link #TURN_ROUNDABOUT_ENTER_AND_EXIT}.  -1 if unused.
     * @param image image to be shown in the instrument cluster (PNG format).  Null if instrument
     *        cluster type is {@link #INSTRUMENT_CLUSTER_TYPE_ENUM}, or if
     *        the image parameters are malformed (length or width non-positive, or illegal
     *        imageColorDepthBits) in the initial NavigationStatusService call.
     * @param turnSide turn side ({@link #TURN_SIDE_LEFT}, {@link #TURN_SIDE_RIGHT} or
     *        {@link #TURN_SIDE_UNSPECIFIED}).
     * @return true if successful.
     * @throws CarNotConnectedException
     *
     */
    public abstract boolean sendNavigationTurnEvent(int event, String road, int turnAngle,
            int turnNumber, Bitmap image, int turnSide) throws CarNotConnectedException;

    /**
     * Sends a Navigation Next Step Distance event to the car.
     *
     * @param distanceMeters Distance to next event in meters.
     * @param timeSeconds Time to next event in seconds.
     * @return true if successful.
     * @throws CarNotConnectedException
     */
    public abstract boolean sendNavigationTurnDistanceEvent(int distanceMeters, int timeSeconds)
            throws CarNotConnectedException;

    public abstract boolean isInstrumentClusterSupported() throws CarNotConnectedException;

    /**
     * @param listener {@link CarNavigationListener} to be registered, replacing any existing
     *        listeners.
     * @throws CarNotConnectedException
     */
    public abstract void registerListener(CarNavigationListener listener)
            throws CarNotConnectedException;

    /**
     * Unregisters {@link CarNavigationListener}.
     */
    public abstract void unregisterListener();
}
