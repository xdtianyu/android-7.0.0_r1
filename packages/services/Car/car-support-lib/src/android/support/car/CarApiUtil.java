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

/**
 * Internal helper utilities
 * @hide
 */
public class CarApiUtil {

    /**
     * CarService throws IllegalStateException with this message is re-thrown as
     * {@link CarNotConnectedException}.
     *
     * @hide
     */
    public static final String CAR_NOT_CONNECTED_EXCEPTION_MSG = "CarNotConnected";

    /**
     * CarService throw IllegalStateException with this message is re-thrown as
     * {@link CarNotSupportedException}.
     *
     * @hide
     */
    public static final String CAR_NOT_SUPPORTED_EXCEPTION_MSG = "CarNotSupported";

    /**
     * IllegalStateException from CarService with special message is re-thrown as a different
     * exception.
     *
     * @param e exception from CarService
     * @throws CarNotConnectedException
     * @throws CarNotSupportedException
     * @hide
     */
    public static void checkAllIllegalStateExceptionsFromCarService(IllegalStateException e)
            throws CarNotConnectedException, CarNotSupportedException {
        String message = e.getMessage();
        if (message.equals(CAR_NOT_CONNECTED_EXCEPTION_MSG)) {
            throw new CarNotConnectedException();
        } else if (message.equals(CAR_NOT_SUPPORTED_EXCEPTION_MSG)) {
            throw new CarNotSupportedException();
        } else {
            throw e;
        }
    }

    /**
     * Re-throw IllegalStateException from CarService with
     * {@link #CAR_NOT_CONNECTED_EXCEPTION_MSG} message as {@link CarNotConnectedException}.
     * exception.
     *
     * @param e exception from CarService
     * @throws CarNotConnectedException
     * @hide
     */
    public static void checkCarNotConnectedExceptionFromCarService(IllegalStateException e)
            throws CarNotConnectedException {
        if (e.getMessage().equals(CAR_NOT_CONNECTED_EXCEPTION_MSG)) {
            throw new CarNotConnectedException();
        } else {
            throw e;
        }
    }

    /** do not use */
    private CarApiUtil() {};
}
