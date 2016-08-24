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

#include <memory>

#include <binder/IPCThreadState.h>
#include <private/android_filesystem_config.h>

#include <utils/Log.h>

#include <IVehicleNetwork.h>
#include <IVehicleNetworkHalMock.h>
#include <VehicleNetworkProto.pb.h>

#include "BinderUtil.h"
#include "VehicleNetworkProtoUtil.h"

namespace android {

enum {
    ON_LIST_PROPERTIES = IBinder::FIRST_CALL_TRANSACTION,
    ON_PROPERTY_SET,
    ON_PROPERTY_GET,
    ON_SUBSCRIBE,
    ON_UNSUBSCRIBE,
};

// ----------------------------------------------------------------------------

const char IVehicleNetworkHalMock::SERVICE_NAME[] =
        "com.android.car.vehiclenetwork.IVehicleNetworkHalMock";

// ----------------------------------------------------------------------------

class BpVehicleNetworkHalMock : public BpInterface<IVehicleNetworkHalMock> {
public:
    BpVehicleNetworkHalMock(const sp<IBinder> & impl)
        : BpInterface<IVehicleNetworkHalMock>(impl) {
    }

    virtual sp<VehiclePropertiesHolder> onListProperties() {
        sp<VehiclePropertiesHolder> holder;
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetworkHalMock::getInterfaceDescriptor());
        status_t status = remote()->transact(ON_LIST_PROPERTIES, data, &reply);
        if (status == NO_ERROR) {
            reply.readExceptionCode(); // for compatibility with java
            if (reply.readInt32() == 0) { // no result
                return holder;
            }
            ReadableBlobHolder blob(new Parcel::ReadableBlob());
            if (blob.blob == NULL) {
                ALOGE("listProperties, no memory");
                return holder;
            }
            int32_t size = reply.readInt32();
            status = reply.readBlob(size, blob.blob);
            if (status != NO_ERROR) {
                ALOGE("listProperties, cannot read blob %d", status);
                return holder;
            }
            //TODO make this more memory efficient
            std::unique_ptr<VehiclePropConfigs> configs(new VehiclePropConfigs());
            if (configs.get() == NULL) {
                return holder;
            }
            if(!configs->ParseFromArray(blob.blob->data(), size)) {
                ALOGE("listProperties, cannot parse reply");
                return holder;
            }
            holder = new VehiclePropertiesHolder();
            ASSERT_OR_HANDLE_NO_MEMORY(holder.get(), return);
            status = VehicleNetworkProtoUtil::fromVehiclePropConfigs(*configs.get(),
                    holder->getList());
            if (status != NO_ERROR) {
                ALOGE("listProperties, cannot convert VehiclePropConfigs %d", status);
                return holder;
            }

        }
        return holder;
    }

    virtual status_t onPropertySet(const vehicle_prop_value_t& value) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetworkHalMock::getInterfaceDescriptor());
        status_t status = VehiclePropValueBinderUtil::writeToParcel(data, value);
        if (status != NO_ERROR) {
            return status;
        }
        status = remote()->transact(ON_PROPERTY_SET, data, &reply);
        return status;
    }

    virtual status_t onPropertyGet(vehicle_prop_value_t* value) {
        if (value == NULL) {
            return BAD_VALUE;
        }
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetworkHalMock::getInterfaceDescriptor());
        status_t status = VehiclePropValueBinderUtil::writeToParcel(data, *value);
        if (status != NO_ERROR) {
            return status;
        }
        status = remote()->transact(ON_PROPERTY_GET, data, &reply);
        if (status == NO_ERROR) {
            reply.readExceptionCode(); // for compatibility with java
            status = VehiclePropValueBinderUtil::readFromParcel(reply, value);
        }
        return status;
    }

    virtual status_t onPropertySubscribe(int32_t property, float sampleRate, int32_t zones) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetworkHalMock::getInterfaceDescriptor());
        data.writeInt32(property);
        data.writeFloat(sampleRate);
        data.writeInt32(zones);
        status_t status = remote()->transact(ON_SUBSCRIBE, data, &reply);
        return status;
    }

    virtual void onPropertyUnsubscribe(int32_t property) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetworkHalMock::getInterfaceDescriptor());
        data.writeInt32(property);
        status_t status = remote()->transact(ON_UNSUBSCRIBE, data, &reply);
        if (status != NO_ERROR) {
            ALOGI("onPropertyUnsubscribe property %d failed %d", property, status);
        }
    }
};

IMPLEMENT_META_INTERFACE(VehicleNetworkHalMock, IVehicleNetworkHalMock::SERVICE_NAME);

// ----------------------------------------------------------------------

static bool isSystemUser() {
    uid_t uid =  IPCThreadState::self()->getCallingUid();
    switch (uid) {
        // This list will be expanded. Only those UIDs are allowed to access vehicle network
        // for now. There can be per property based UID check built-in as well.
        case AID_ROOT:
        case AID_SYSTEM:
        case AID_AUDIO: {
            return true;
        } break;
        default: {
            ALOGE("non-system user tried access, uid %d", uid);
        } break;
    }
    return false;
}

status_t BnVehicleNetworkHalMock::onTransact(uint32_t code, const Parcel& data, Parcel* reply,
        uint32_t flags) {
    if (!isSystemUser()) {
        return PERMISSION_DENIED;
    }
    status_t r;
    switch (code) {
        case ON_LIST_PROPERTIES: {
            CHECK_INTERFACE(IVehicleNetworkHalMock, data, reply);
            sp<VehiclePropertiesHolder> holder = onListProperties();
            if (holder.get() == NULL) { // given property not found
                BinderUtil::fillObjectResultReply(reply, false /* isValid */);
                return NO_ERROR;
            }
            std::unique_ptr<VehiclePropConfigs> configs(new VehiclePropConfigs());
            ASSERT_OR_HANDLE_NO_MEMORY(configs.get(), return NO_MEMORY);
            VehicleNetworkProtoUtil::toVehiclePropConfigs(holder->getList(), *configs.get());
            int size = configs->ByteSize();
            WritableBlobHolder blob(new Parcel::WritableBlob());
            ASSERT_OR_HANDLE_NO_MEMORY(blob.blob, return NO_MEMORY);
            BinderUtil::fillObjectResultReply(reply, true);
            reply->writeInt32(size);
            reply->writeBlob(size, false, blob.blob);
            configs->SerializeToArray(blob.blob->data(), size);
            return NO_ERROR;
        } break;
        case ON_PROPERTY_SET: {
            CHECK_INTERFACE(IVehicleNetworkHalMock, data, reply);
            if (data.readInt32() == 0) { // java side allows passing null with this.
                return BAD_VALUE;
            }
            ScopedVehiclePropValue value;
            ReadableBlobHolder blob(new Parcel::ReadableBlob());
            ASSERT_OR_HANDLE_NO_MEMORY(blob.blob, return NO_MEMORY);
            int32_t size = data.readInt32();
            r = data.readBlob(size, blob.blob);
            if (r != NO_ERROR) {
                ALOGE("setProperty:service, cannot read blob");
                return r;
            }
            std::unique_ptr<VehiclePropValue> v(new VehiclePropValue());
            ASSERT_OR_HANDLE_NO_MEMORY(v.get(), return NO_MEMORY);
            if (!v->ParseFromArray(blob.blob->data(), size)) {
                ALOGE("setProperty:service, cannot parse data");
                return BAD_VALUE;
            }
            r = VehicleNetworkProtoUtil::fromVehiclePropValue(*v.get(), value.value);
            if (r != NO_ERROR) {
                ALOGE("setProperty:service, cannot convert data");
                return BAD_VALUE;
            }
            r = onPropertySet(value.value);
            BinderUtil::fillNoResultReply(reply);
            return r;
        } break;
        case ON_PROPERTY_GET: {
            CHECK_INTERFACE(IVehicleNetworkHalMock, data, reply);
            ScopedVehiclePropValue value;
            r = VehiclePropValueBinderUtil::readFromParcel(data, &value.value,
                                false /* deleteMembers */, true /*canIgnoreNoData*/);
            if (r != NO_ERROR) {
                ALOGE("onPropertyGet cannot read %d", r);
                return r;
            }
            r = onPropertyGet(&(value.value));
            if (r == NO_ERROR) {
                BinderUtil::fillObjectResultReply(reply, true);
                std::unique_ptr<VehiclePropValue> v(new VehiclePropValue());
                ASSERT_OR_HANDLE_NO_MEMORY(v.get(), return NO_MEMORY);
                VehicleNetworkProtoUtil::toVehiclePropValue(value.value, *v.get());
                int size = v->ByteSize();
                WritableBlobHolder blob(new Parcel::WritableBlob());
                ASSERT_OR_HANDLE_NO_MEMORY(blob.blob, return NO_MEMORY);
                reply->writeInt32(size);
                reply->writeBlob(size, false, blob.blob);
                v->SerializeToArray(blob.blob->data(), size);
            }
            return r;
        } break;
        case ON_SUBSCRIBE: {
            CHECK_INTERFACE(IVehicleNetworkHalMock, data, reply);
            int32_t property = data.readInt32();
            float sampleRate = data.readFloat();
            int32_t zones = data.readInt32();
            r = onPropertySubscribe(property, sampleRate, zones);
            BinderUtil::fillNoResultReply(reply);
            return r;
        } break;
        case ON_UNSUBSCRIBE: {
            CHECK_INTERFACE(IVehicleNetworkHalMock, data, reply);
            int32_t property = data.readInt32();
            onPropertyUnsubscribe(property);
            BinderUtil::fillNoResultReply(reply);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}; // namespace android
