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

#define LOG_TAG "VehicleNetwork"

#include <utils/Log.h>

#include <IVehicleNetwork.h>
#include "VehicleNetworkProtoUtil.h"

namespace android {

static status_t copyString(const std::string& in, uint8_t** out, int32_t* len) {
    *len = in.length();
    if (*len == 0) {
        *out = NULL;
        return NO_ERROR;
    }
    *out = new uint8_t[*len];
    ASSERT_OR_HANDLE_NO_MEMORY(*out, return NO_MEMORY);
    memcpy(*out, in.data(), *len);
    return NO_ERROR;
}

status_t VehicleNetworkProtoUtil::toVehiclePropValue(const vehicle_prop_value_t& in,
        VehiclePropValue& out, bool /*inPlace*/) {
    out.set_prop(in.prop);
    out.set_value_type(in.value_type);
    out.set_timestamp(in.timestamp);
    out.set_zone(in.zone);
    switch (in.value_type) {
        case VEHICLE_VALUE_TYPE_STRING: {
            //TODO fix ugly copy here for inplace mode
            if (in.value.str_value.len > 0) {
                out.set_string_value((char*)in.value.str_value.data, in.value.str_value.len);
            }
        } break;
        case VEHICLE_VALUE_TYPE_BYTES: {
            if (in.value.bytes_value.len > 0) {
                out.set_bytes_value(in.value.bytes_value.data, in.value.bytes_value.len);
            }
        } break;
        case VEHICLE_VALUE_TYPE_FLOAT:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC2:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC3:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC4: {
            int expectedSize = in.value_type - VEHICLE_VALUE_TYPE_FLOAT + 1;
            for (int i = 0; i < expectedSize; i++) {
                out.add_float_values(in.value.float_array[i]);
            }
        } break;
        case VEHICLE_VALUE_TYPE_INT64: {
            out.set_int64_value(in.value.int64_value);
        } break;
        case VEHICLE_VALUE_TYPE_BOOLEAN:
        case VEHICLE_VALUE_TYPE_ZONED_BOOLEAN: {
            out.add_int32_values(in.value.int32_value);
        } break;
        case VEHICLE_VALUE_TYPE_INT32:
        case VEHICLE_VALUE_TYPE_INT32_VEC2:
        case VEHICLE_VALUE_TYPE_INT32_VEC3:
        case VEHICLE_VALUE_TYPE_INT32_VEC4: {
            int expectedSize = in.value_type - VEHICLE_VALUE_TYPE_INT32 + 1;
            for (int i = 0; i < expectedSize; i++) {
                out.add_int32_values(in.value.int32_array[i]);
            }
        } break;
        case VEHICLE_VALUE_TYPE_ZONED_INT32:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC4: {
            int expectedSize = in.value_type - VEHICLE_VALUE_TYPE_ZONED_INT32 + 1;
            for (int i = 0; i < expectedSize; i++) {
                out.add_int32_values(in.value.int32_array[i]);
            }
        } break;
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC4: {
            int expectedSize = in.value_type - VEHICLE_VALUE_TYPE_ZONED_FLOAT + 1;
            for (int i = 0; i < expectedSize; i++) {
                out.add_float_values(in.value.float_array[i]);
            }
        } break;
    }
    return NO_ERROR;
}

status_t VehicleNetworkProtoUtil::fromVehiclePropValue(const VehiclePropValue& in,
        vehicle_prop_value_t& out, bool /*inPlace*/, bool canIgnoreNoData) {
    out.prop = in.prop();
    out.value_type = in.value_type();
    out.timestamp = in.timestamp();
    out.zone = in.zone();
    switch (out.value_type) {
        case VEHICLE_VALUE_TYPE_STRING: {
            if (!in.has_string_value()) {
                // set to NULL so that client can just delete this safely.
                out.value.str_value.data = NULL;
                out.value.str_value.len = 0;
                if (canIgnoreNoData) {
                    return NO_ERROR;
                } else {
                    ALOGE("fromVehiclePropValue, no string data");
                    return BAD_VALUE;
                }
            }
            //TODO fix copy...
            status_t r = copyString(in.string_value(), &(out.value.str_value.data),
                    &(out.value.str_value.len));
            if (r != NO_ERROR) {
                out.value.str_value.data = NULL;
                out.value.str_value.len = 0;
                ALOGE("copyString for string failed %d", r);
                return r;
            }
        } break;
        case VEHICLE_VALUE_TYPE_BYTES: {
            if (!in.has_bytes_value()) {
                out.value.bytes_value.data = NULL;
                out.value.bytes_value.len = 0;
                if (canIgnoreNoData) {
                    return NO_ERROR;
                } else {
                    ALOGE("fromVehiclePropValue, no byte data");
                    return BAD_VALUE;
                }
            }
            status_t r = copyString(in.bytes_value(), &(out.value.bytes_value.data),
                    &(out.value.bytes_value.len));
            if (r != NO_ERROR) {
                out.value.bytes_value.data = NULL;
                out.value.bytes_value.len = 0;
                ALOGE("copyString for bytes failed %d", r);
                return r;
            }
        } break;
        case VEHICLE_VALUE_TYPE_FLOAT:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC2:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC3:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC4: {
            int expectedSize = out.value_type - VEHICLE_VALUE_TYPE_FLOAT + 1;
            if (in.float_values_size() != expectedSize) {
                if (canIgnoreNoData) {
                    return NO_ERROR;
                }
                ALOGE("float value, wrong size %d, expecting %d", in.float_values_size(),
                        expectedSize);
                return BAD_VALUE;
            }
            for (int i = 0; i < expectedSize; i++) {
                out.value.float_array[i] = in.float_values(i);
            }
        } break;
        case VEHICLE_VALUE_TYPE_INT64: {
            if (!in.has_int64_value()) {
                if (canIgnoreNoData) {
                    return NO_ERROR;
                }
                ALOGE("no int64 value");
                return BAD_VALUE;
            }
            out.value.int64_value = in.int64_value();
        } break;
        case VEHICLE_VALUE_TYPE_BOOLEAN:
        case VEHICLE_VALUE_TYPE_ZONED_BOOLEAN: {
            if (in.int32_values_size() != 1) {
                if (canIgnoreNoData) {
                    return NO_ERROR;
                }
                ALOGE("no int32 value");
                return BAD_VALUE;
            }
            out.value.int32_value = in.int32_values(0);
        } break;
        case VEHICLE_VALUE_TYPE_INT32:
        case VEHICLE_VALUE_TYPE_INT32_VEC2:
        case VEHICLE_VALUE_TYPE_INT32_VEC3:
        case VEHICLE_VALUE_TYPE_INT32_VEC4: {
            int expectedSize = out.value_type - VEHICLE_VALUE_TYPE_INT32 + 1;
            if (in.int32_values_size() != expectedSize) {
                if (canIgnoreNoData) {
                    return NO_ERROR;
                }
                ALOGE("int32 value, wrong size %d, expecting %d", in.int32_values_size(),
                        expectedSize);
                return BAD_VALUE;
            }
            for (int i = 0; i < expectedSize; i++) {
                out.value.int32_array[i] = in.int32_values(i);
            }
        } break;

        case VEHICLE_VALUE_TYPE_ZONED_INT32:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC4: {
            int expectedSize = out.value_type - VEHICLE_VALUE_TYPE_ZONED_INT32 + 1;
            if (in.int32_values_size() != expectedSize) {
                if (canIgnoreNoData) {
                    return NO_ERROR;
                }
                ALOGE("int32 value, wrong size %d, expecting %d", in.int32_values_size(),
                        expectedSize);
                return BAD_VALUE;
            }
            for (int i = 0; i < expectedSize; i++) {
                out.value.int32_array[i] = in.int32_values(i);
            }
        } break;
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC4:{
            int expectedSize = out.value_type - VEHICLE_VALUE_TYPE_ZONED_FLOAT + 1;
            if (in.float_values_size() != expectedSize) {
                if (canIgnoreNoData) {
                    return NO_ERROR;
                }
                ALOGE("float value, wrong size %d, expecting %d", in.float_values_size(),
                        expectedSize);
                return BAD_VALUE;
            }
            for (int i = 0; i < expectedSize; i++) {
                out.value.float_array[i] = in.float_values(i);
            }
        } break;
        default: {
            if (canIgnoreNoData) {
                return NO_ERROR;
            }
            ALOGE("fromVehiclePropValue unknown type 0x%x", out.value_type);
            return BAD_VALUE;
        }
    }
    return NO_ERROR;
}

status_t VehicleNetworkProtoUtil::toVehiclePropValues(const List<vehicle_prop_value_t*>& in,
            VehiclePropValues& out) {
    status_t r;
    for (auto& v : in) {
        VehiclePropValue* value = out.add_values();
        r = toVehiclePropValue(*v, *value);
        if (r != NO_ERROR) {
            out.clear_values();
            return r;
        }
    }
    return NO_ERROR;
}

status_t VehicleNetworkProtoUtil::fromVehiclePropValues(const VehiclePropValues& in,
            List<vehicle_prop_value_t*>& out) {
    status_t r;
    for (int i = 0; i < in.values_size(); i++) {
        vehicle_prop_value_t* v =  new vehicle_prop_value_t();
        memset(v, 0, sizeof(vehicle_prop_value_t));
        ASSERT_OR_HANDLE_NO_MEMORY(v, r = NO_MEMORY;goto error);
        r = fromVehiclePropValue(in.values(i), *v);
        if (r != NO_ERROR) {
            delete v;
            goto error;
        }
        out.push_back(v);
    }
    return NO_ERROR;
error:
    // clean up everything in List
    for (auto pv : out) {
        VehiclePropValueUtil::deleteMembers(pv);
    }
    return r;
}

status_t VehicleNetworkProtoUtil::toVehiclePropConfig(const vehicle_prop_config_t& in,
        VehiclePropConfig& out) {
    out.set_prop(in.prop);
    out.set_access(in.access);
    out.set_change_mode(in.change_mode);
    out.set_value_type(in.value_type);
    out.set_permission_model(in.permission_model);
    out.set_zones(in.vehicle_zone_flags);
    for (unsigned int i = 0; i < sizeof(in.config_array) / sizeof(int32_t); i++) {
        out.add_config_array(in.config_array[i]);
    }
    if (in.config_string.data != NULL && in.config_string.len != 0) {
        out.set_config_string((char*)in.config_string.data, in.config_string.len);
    } else {
        out.clear_config_string();
    }
    switch (in.value_type) {
        case VEHICLE_VALUE_TYPE_FLOAT:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC2:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC3:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC4: {
            out.add_float_maxs(in.float_max_value);
            out.add_float_mins(in.float_min_value);
        } break;
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC4: {
            int numZones = VehicleNetworkUtil::countNumberOfZones(in.vehicle_zone_flags);
            if (in.float_min_values == NULL) {
                if (in.float_max_values == NULL) {
                    // all the same min/max
                    for (int i = 0; i < numZones; i++) {
                        out.add_float_maxs(in.float_max_value);
                        out.add_float_mins(in.float_min_value);
                    }
                } else { // invalid combination
                    ALOGW("Zoned property 0x%x, min_values NULL while max_values not NULL",
                            in.prop);
                    return BAD_VALUE;
                }
            } else {
                if (in.float_max_values != NULL) {
                    for (int i = 0; i < numZones; i++) {
                        out.add_float_maxs(in.float_max_values[i]);
                        out.add_float_mins(in.float_min_values[i]);
                    }
                } else { // invalid combination
                    ALOGW("Zoned property 0x%x, max_values NULL while min_values not NULL",
                            in.prop);
                    return BAD_VALUE;
                }
            }
        } break;
        case VEHICLE_VALUE_TYPE_INT64: {
            out.add_int64_maxs(in.int64_max_value);
            out.add_int64_mins(in.int64_min_value);
        } break;
        case VEHICLE_VALUE_TYPE_INT32:
        case VEHICLE_VALUE_TYPE_INT32_VEC2:
        case VEHICLE_VALUE_TYPE_INT32_VEC3:
        case VEHICLE_VALUE_TYPE_INT32_VEC4:  {
            out.add_int32_maxs(in.int32_max_value);
            out.add_int32_mins(in.int32_min_value);
        } break;
        case VEHICLE_VALUE_TYPE_ZONED_INT32:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC4: {
            int numZones = VehicleNetworkUtil::countNumberOfZones(in.vehicle_zone_flags);
            if (in.int32_min_values == NULL) {
                if (in.int32_max_values == NULL) {
                    // all the same min/max
                    for (int i = 0; i < numZones; i++) {
                        out.add_int32_maxs(in.int32_max_value);
                        out.add_int32_mins(in.int32_min_value);
                    }
                } else { // invalid combination
                    ALOGW("Zoned property 0x%x, min_values NULL while max_values not NULL",
                            in.prop);
                    return BAD_VALUE;
                }
            } else {
                if (in.int32_max_values != NULL) {
                    for (int i = 0; i < numZones; i++) {
                        out.add_int32_maxs(in.int32_max_values[i]);
                        out.add_int32_mins(in.int32_min_values[i]);
                    }
                } else { // invalid combination
                    ALOGW("Zoned property 0x%x, max_values NULL while min_values not NULL",
                            in.prop);
                    return BAD_VALUE;
                }
            }
        } break;
    }
    out.set_sample_rate_max(in.max_sample_rate);
    out.set_sample_rate_min(in.min_sample_rate);
    return NO_ERROR;
}

status_t VehicleNetworkProtoUtil::fromVehiclePropConfig(const VehiclePropConfig& in,
        vehicle_prop_config_t& out) {
    out.prop = in.prop();
    out.access = in.access();
    out.change_mode = in.change_mode();
    out.value_type = in.value_type();
    out.permission_model = in.permission_model();
    out.vehicle_zone_flags = in.zones();
    int maxConfigSize = sizeof(out.config_array) / sizeof(int32_t);
    int configSize = in.config_array_size();
    if (configSize > maxConfigSize) {
        return BAD_VALUE;
    }
    int i = 0;
    for (; i < configSize; i++) {
        out.config_array[i] = in.config_array(i);
    }
    for (; i < maxConfigSize; i++) {
        out.config_array[i] = 0;
    }
    if (in.has_config_string()) {
        status_t r = copyString(in.config_string(), &(out.config_string.data),
                &(out.config_string.len));
        if (r != NO_ERROR) {
            return r;
        }
    } else {
        out.config_string.data = NULL;
        out.config_string.len = 0;
    }
    switch (out.value_type) {
        case VEHICLE_VALUE_TYPE_FLOAT:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC2:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC3:
        case VEHICLE_VALUE_TYPE_FLOAT_VEC4: {
            if ((in.float_maxs_size() == 1) && (in.float_mins_size() == 1)) {
                out.float_max_value = in.float_maxs(0);
                out.float_min_value = in.float_mins(0);
            } else {
                ALOGW("no float max/min for property 0x%x", out.prop);
                out.float_max_value = 0;
                out.float_min_value = 0;
            }
        } break;
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3:
        case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC4: {
            int numZones = VehicleNetworkUtil::countNumberOfZones(out.vehicle_zone_flags);
            int maxSize = in.float_maxs_size();
            int minSize = in.float_mins_size();
            if (maxSize != minSize) {
                ALOGW("Zoned property 0x%x, config maxSize %d minSize %d", out.prop, maxSize,
                        minSize);
                return BAD_VALUE;
            }
            if (maxSize == 0) {
                out.float_max_value = 0;
                out.float_min_value = 0;
                out.float_max_values = NULL;
                out.float_min_values = NULL;
            } else if (maxSize == 1) { // one for all
                out.float_max_value = in.float_maxs(0);
                out.float_min_value = in.float_mins(0);
                out.float_max_values = NULL;
                out.float_min_values = NULL;
            } else if (numZones > 1){
                if (numZones != maxSize) {
                    ALOGW("Zoned property 0x%x, config maxSize %d num Zones %d", out.prop, maxSize,
                                            numZones);
                    return BAD_VALUE;
                }
                out.float_max_values = new float[numZones];
                ASSERT_OR_HANDLE_NO_MEMORY(out.float_max_values, return NO_MEMORY);
                out.float_min_values = new float[numZones];
                ASSERT_OR_HANDLE_NO_MEMORY(out.float_min_values, return NO_MEMORY);
                for (int i = 0; i < numZones; i++) {
                    out.float_max_values[i] = in.float_maxs(i);
                    out.float_min_values[i] = in.float_mins(i);
                }
            }
        } break;
        case VEHICLE_VALUE_TYPE_INT64: {
            if ((in.int64_maxs_size() == 1) && (in.int64_mins_size() == 1)) {
                out.int64_max_value = in.int64_maxs(0);
                out.int64_min_value = in.int64_mins(0);
            } else {
                ALOGW("no int64 max/min for property 0x%x", out.prop);
                out.int64_max_value = 0;
                out.int64_min_value = 0;
            }
        } break;
        case VEHICLE_VALUE_TYPE_INT32:
        case VEHICLE_VALUE_TYPE_INT32_VEC2:
        case VEHICLE_VALUE_TYPE_INT32_VEC3:
        case VEHICLE_VALUE_TYPE_INT32_VEC4: {
            if ((in.int32_maxs_size() == 1) && (in.int32_mins_size() == 1)) {
                out.int32_max_value = in.int32_maxs(0);
                out.int32_min_value = in.int32_mins(0);
            } else {
                ALOGW("no int32 max/min for property 0x%x", out.prop);
                out.int32_max_value = 0;
                out.int32_min_value = 0;
            }
        } break;
        case VEHICLE_VALUE_TYPE_ZONED_INT32:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3:
        case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC4: {
            int numZones = VehicleNetworkUtil::countNumberOfZones(out.vehicle_zone_flags);
            int maxSize = in.int32_maxs_size();
            int minSize = in.int32_mins_size();
            if (maxSize != minSize) {
                ALOGW("Zoned property 0x%x, config maxSize %d minSize %d", out.prop, maxSize,
                        minSize);
                return BAD_VALUE;
            }
            if (maxSize == 0) {
                out.int32_max_value = 0;
                out.int32_min_value = 0;
                out.int32_max_values = NULL;
                out.int32_min_values = NULL;
            } else if (maxSize == 1) { // one for all
                out.int32_max_value = in.int32_maxs(0);
                out.int32_min_value = in.int32_mins(0);
                out.int32_max_values = NULL;
                out.int32_min_values = NULL;
            } else if (numZones > 1){
                if (numZones != maxSize) {
                    ALOGW("Zoned property 0x%x, config maxSize %d num Zones %d", out.prop, maxSize,
                                            numZones);
                    return BAD_VALUE;
                }
                out.int32_max_values = new int32_t[numZones];
                ASSERT_OR_HANDLE_NO_MEMORY(out.int32_max_values, return NO_MEMORY);
                out.int32_min_values = new int32_t[numZones];
                ASSERT_OR_HANDLE_NO_MEMORY(out.int32_min_values, return NO_MEMORY);
                for (int i = 0; i < numZones; i++) {
                    out.int32_max_values[i] = in.int32_maxs(i);
                    out.int32_min_values[i] = in.int32_mins(i);
                }
            }
        } break;
    }
    out.max_sample_rate = in.sample_rate_max();
    out.min_sample_rate = in.sample_rate_min();
    return NO_ERROR;
}

status_t VehicleNetworkProtoUtil::toVehiclePropConfigs(List<vehicle_prop_config_t const*> &in,
        VehiclePropConfigs& out) {
    status_t r;
    for (auto& inEntry : in) {
        VehiclePropConfig* config = out.add_configs();
        r = toVehiclePropConfig(*inEntry, *config);
        if (r != NO_ERROR) {
            out.clear_configs();
            return r;
        }
    }
    return NO_ERROR;
}

status_t VehicleNetworkProtoUtil::fromVehiclePropConfigs(const VehiclePropConfigs& in,
        List<vehicle_prop_config_t const*>& out) {
    int32_t n = in.configs_size();
    status_t r;
    for (int32_t i = 0; i < n; i++) {
        vehicle_prop_config_t* entry = new vehicle_prop_config_t();
        ASSERT_OR_HANDLE_NO_MEMORY(entry, r = NO_MEMORY; goto error);
        memset(entry, 0, sizeof(vehicle_prop_config_t));
        r = fromVehiclePropConfig(in.configs(i), *entry);
        if (r != NO_ERROR) {
            goto error;
        }
        out.push_back(entry);
    }
    return NO_ERROR;
error:
    for (auto& e : out) {
        vehicle_prop_config_t* eDelete = const_cast<vehicle_prop_config_t*>(e);
        VehiclePropertiesUtil::deleteMembers(eDelete);
        delete eDelete;
    }
    out.clear();
    return r;
}

status_t VehiclePropValueBinderUtil::writeToParcel(Parcel& parcel,
        const vehicle_prop_value_t& value) {
    parcel.writeInt32(1); // 0 means no value. For compatibility with aidl based code.
    std::unique_ptr<VehiclePropValue> v(new VehiclePropValue());
    ASSERT_OR_HANDLE_NO_MEMORY(v.get(), return NO_MEMORY);
    VehicleNetworkProtoUtil::toVehiclePropValue(value, *v.get());
    int size = v->ByteSize();
    WritableBlobHolder blob(new Parcel::WritableBlob());
    ASSERT_OR_HANDLE_NO_MEMORY(blob.blob, return NO_MEMORY);
    parcel.writeInt32(size);
    parcel.writeBlob(size, false, blob.blob);
    v->SerializeToArray(blob.blob->data(), size);
    return NO_ERROR;
}

status_t VehiclePropValueBinderUtil::readFromParcel(const Parcel& parcel,
        vehicle_prop_value_t* value, bool deleteMembers, bool canIgnoreNoData) {
    if (parcel.readInt32() == 0) { // no result
        ALOGE("readFromParcel, null data");
        return BAD_VALUE;
    }
    ReadableBlobHolder blob(new Parcel::ReadableBlob());
    ASSERT_OR_HANDLE_NO_MEMORY(blob.blob, return NO_MEMORY);
    int32_t size = parcel.readInt32();
    status_t status = parcel.readBlob(size, blob.blob);
    if (status != NO_ERROR) {
        ALOGE("readFromParcel, cannot read blob");
        return status;
    }
    std::unique_ptr<VehiclePropValue> v(new VehiclePropValue());
    ASSERT_OR_HANDLE_NO_MEMORY(v.get(), return NO_MEMORY);
    if (!v->ParseFromArray(blob.blob->data(), size)) {
        ALOGE("readFromParcel, cannot parse");
        return BAD_VALUE;
    }
    if (deleteMembers) {
        VehiclePropValueUtil::deleteMembers(value);
    }
    return VehicleNetworkProtoUtil::fromVehiclePropValue(*v.get(), *value, false /*inPlace*/,
            canIgnoreNoData);
}

}; //namespace android

