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
package com.android.car.vehiclenetwork;

import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;

public class VehicleNetworkProtoUtil {
    public static String VehiclePropValueToString(VehiclePropValue value) {
        StringBuilder sb = new StringBuilder();
        sb.append("prop:0x");
        sb.append(Integer.toHexString(value.getProp()));
        sb.append(" type:0x");
        sb.append(Integer.toHexString(value.getValueType()));
        return sb.toString();
    }

    public static String VehiclePropConfigToString(VehiclePropConfig config) {
        StringBuilder sb = new StringBuilder();
        sb.append("prop:0x");
        sb.append(Integer.toHexString(config.getProp()));
        sb.append(" access:0x");
        sb.append(Integer.toHexString(config.getAccess()));
        sb.append(" change_mode:0x");
        sb.append(Integer.toHexString(config.getChangeMode()));
        sb.append(" value_type:0x");
        sb.append(Integer.toHexString(config.getValueType()));
        sb.append(" permission_model:0x");
        sb.append(Integer.toHexString(config.getPermissionModel()));
        return sb.toString();
    }

    public static boolean VehiclePropConfigEquals(VehiclePropConfig l, VehiclePropConfig r) {
        if (l.getProp() != r.getProp()) {
            return false;
        }
        if (l.getAccess() != r.getAccess()) {
            return false;
        }
        if (l.getChangeMode() != r.getChangeMode()) {
            return false;
        }
        if (l.getValueType() != r.getValueType()) {
            return false;
        }
        if (l.getPermissionModel() != r.getPermissionModel()) {
            return false;
        }
        //TODO add more comparison
        return true;
    }
}
