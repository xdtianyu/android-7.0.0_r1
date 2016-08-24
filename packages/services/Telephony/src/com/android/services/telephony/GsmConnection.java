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

package com.android.services.telephony;

import com.android.internal.telephony.Connection;

/**
 * Manages a single phone call handled by GSM.
 */
final class GsmConnection extends TelephonyConnection {
    GsmConnection(Connection connection, String telecomCallId) {
        super(connection, telecomCallId);
    }

    /**
     * Clones the current {@link GsmConnection}.
     * <p>
     * Listeners are not copied to the new instance.
     *
     * @return The cloned connection.
     */
    @Override
    public TelephonyConnection cloneConnection() {
        GsmConnection gsmConnection = new GsmConnection(getOriginalConnection(),
                getTelecomCallId());
        return gsmConnection;
    }

    /** {@inheritDoc} */
    @Override
    public void onPlayDtmfTone(char digit) {
        if (getPhone() != null) {
            getPhone().startDtmf(digit);
        }
    }

    /** {@inheritDoc} */
    @Override
    public void onStopDtmfTone() {
        if (getPhone() != null) {
            getPhone().stopDtmf();
        }
    }

    @Override
    protected int buildConnectionCapabilities() {
        int capabilities = super.buildConnectionCapabilities();
        capabilities |= CAPABILITY_MUTE;
        if (!shouldTreatAsEmergencyCall()) {
            capabilities |= CAPABILITY_SUPPORT_HOLD;
            if (getState() == STATE_ACTIVE || getState() == STATE_HOLDING) {
                capabilities |= CAPABILITY_HOLD;
            }
        }
        return capabilities;
    }

    @Override
    void onRemovedFromCallService() {
        super.onRemovedFromCallService();
    }
}
