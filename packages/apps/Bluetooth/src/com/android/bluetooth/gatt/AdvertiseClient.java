/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.bluetooth.gatt;

import android.annotation.Nullable;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;

import java.util.Objects;

/**
 * Helper class that represents a client for Bluetooth LE advertise operations.
 *
 * @hide
 */
class AdvertiseClient {
    int clientIf;
    // Associated application died.
    boolean appDied;
    AdvertiseSettings settings;
    AdvertiseData advertiseData;
    @Nullable
    AdvertiseData scanResponse;

    /**
     * @param clientIf - Identifier of the client.
     */
    AdvertiseClient(int clientIf) {
        this.clientIf = clientIf;
    }

    /**
     * @param clientIf - Identifier of the client.
     * @param settings - Settings for the advertising.
     * @param advertiseData - Advertise data broadcasted over the air.
     * @param scanResponse - Response of scan request, could be null.
     */
    AdvertiseClient(int clientIf, AdvertiseSettings settings, AdvertiseData advertiseData,
            AdvertiseData scanResponse) {
        this.clientIf = clientIf;
        this.settings = settings;
        this.advertiseData = advertiseData;
        this.scanResponse = scanResponse;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null || getClass() != obj.getClass()) {
            return false;
        }
        AdvertiseClient other = (AdvertiseClient) obj;
        return clientIf == other.clientIf;
    }

    @Override
    public int hashCode() {
        return Objects.hash(clientIf);
    }
}
