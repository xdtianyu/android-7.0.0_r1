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
#include <string.h>

#include <binder/IPCThreadState.h>
#include <binder/Status.h>

#include <utils/Log.h>

#include <IVehicleNetwork.h>
#include <VehicleNetworkProto.pb.h>

#include "BinderUtil.h"
#include "VehicleNetworkProtoUtil.h"

namespace android {

enum {
    LIST_PROPERTIES = IBinder::FIRST_CALL_TRANSACTION,
    SET_PROPERTY,
    GET_PROPERTY,
    SUBSCRIBE,
    UNSUBSCRIBE,
    INJECT_EVENT,
    START_MOCKING,
    STOP_MOCKING,
    INJECT_HAL_ERROR,
    START_ERROR_LISTENING,
    STOP_ERROR_LISTENING,
    START_HAL_RESTART_MONITORING,
    STOP_HAL_RESTART_MONITORING
};

// ----------------------------------------------------------------------------

const char IVehicleNetwork::SERVICE_NAME[] = "com.android.car.vehiclenetwork.IVehicleNetwork";

// ----------------------------------------------------------------------------

class BpVehicleNetwork : public BpInterface<IVehicleNetwork> {
public:
    BpVehicleNetwork(const sp<IBinder> & impl)
        : BpInterface<IVehicleNetwork>(impl) {
    }

    virtual sp<VehiclePropertiesHolder> listProperties(int32_t property) {
        sp<VehiclePropertiesHolder> holder;
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeInt32(property);
        status_t status = remote()->transact(LIST_PROPERTIES, data, &reply);
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

    virtual status_t setProperty(const vehicle_prop_value_t& value) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        status_t status = VehiclePropValueBinderUtil::writeToParcel(data, value);
        if (status != NO_ERROR) {
            return status;
        }
        status = remote()->transact(SET_PROPERTY, data, &reply);
        return status;
    }

    virtual status_t getProperty(vehicle_prop_value_t* value) {
        Parcel data, reply;
        if (value == NULL) {
            return BAD_VALUE;
        }
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        status_t status = VehiclePropValueBinderUtil::writeToParcel(data, *value);
        if (status != NO_ERROR) {
            ALOGE("getProperty, cannot write");
            return status;
        }
        status = remote()->transact(GET_PROPERTY, data, &reply);
        if (status == NO_ERROR) {
            int32_t exceptionCode = reply.readExceptionCode();
            if (exceptionCode != NO_ERROR) {
                if (exceptionCode == binder::Status::EX_SERVICE_SPECIFIC) {
                    return -EAGAIN;
                }
                return exceptionCode;
            }
            status = VehiclePropValueBinderUtil::readFromParcel(reply, value);
        }
        return status;
    }

    virtual status_t subscribe(const sp<IVehicleNetworkListener> &listener, int32_t property,
                float sampleRate, int32_t zones) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(listener));
        data.writeInt32(property);
        data.writeFloat(sampleRate);
        data.writeInt32(zones);
        status_t status = remote()->transact(SUBSCRIBE, data, &reply);
        return status;
    }

    virtual void unsubscribe(const sp<IVehicleNetworkListener> &listener, int32_t property) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(listener));
        data.writeInt32(property);
        status_t status = remote()->transact(UNSUBSCRIBE, data, &reply);
        if (status != NO_ERROR) {
            ALOGI("unsubscribing property %d failed %d", property, status);
        }
    }

    virtual status_t injectEvent(const vehicle_prop_value_t& value) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeInt32(1); // 0 means no value. For compatibility with aidl based code.
        std::unique_ptr<VehiclePropValue> v(new VehiclePropValue());
        ASSERT_OR_HANDLE_NO_MEMORY(v.get(), return NO_MEMORY);
        VehicleNetworkProtoUtil::toVehiclePropValue(value, *v.get());
        int size = v->ByteSize();
        WritableBlobHolder blob(new Parcel::WritableBlob());
        ASSERT_OR_HANDLE_NO_MEMORY(blob.blob, return NO_MEMORY);
        data.writeInt32(size);
        data.writeBlob(size, false, blob.blob);
        v->SerializeToArray(blob.blob->data(), size);
        status_t status = remote()->transact(INJECT_EVENT, data, &reply);
        return status;
    }

    virtual status_t startMocking(const sp<IVehicleNetworkHalMock>& mock) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(mock));
        status_t status = remote()->transact(START_MOCKING, data, &reply);
        return status;
    }

    virtual void stopMocking(const sp<IVehicleNetworkHalMock>& mock) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(mock));
        status_t status = remote()->transact(STOP_MOCKING, data, &reply);
        if (status != NO_ERROR) {
            ALOGI("stop mocking failed %d", status);
        }
    }

    status_t injectHalError(int32_t errorCode, int32_t property, int32_t operation) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeInt32(errorCode);
        data.writeInt32(property);
        data.writeInt32(operation);
        status_t status = remote()->transact(INJECT_HAL_ERROR, data, &reply);
        return status;
    }

    virtual status_t startErrorListening(const sp<IVehicleNetworkListener> &listener) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(listener));
        status_t status = remote()->transact(START_ERROR_LISTENING, data, &reply);
        return status;
    }

    virtual void stopErrorListening(const sp<IVehicleNetworkListener> &listener) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(listener));
        status_t status = remote()->transact(STOP_ERROR_LISTENING, data, &reply);
        if (status != NO_ERROR) {
            ALOGI("stopErrorListening %d", status);
        }
    }

    virtual status_t startHalRestartMonitoring(const sp<IVehicleNetworkListener> &listener) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(listener));
        status_t status = remote()->transact(START_HAL_RESTART_MONITORING, data, &reply);
        return status;
    }

    virtual void stopHalRestartMonitoring(const sp<IVehicleNetworkListener> &listener) {
        Parcel data, reply;
        data.writeInterfaceToken(IVehicleNetwork::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(listener));
        status_t status = remote()->transact(STOP_HAL_RESTART_MONITORING, data, &reply);
        if (status != NO_ERROR) {
            ALOGI("stopHalRestartMonitoring %d", status);
        }
    }
};

IMPLEMENT_META_INTERFACE(VehicleNetwork, IVehicleNetwork::SERVICE_NAME);

// ----------------------------------------------------------------------

status_t BnVehicleNetwork::onTransact(uint32_t code, const Parcel& data, Parcel* reply,
        uint32_t flags) {
    status_t r;
    switch (code) {
        case LIST_PROPERTIES: {
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            if (!isOperationAllowed(0, false)) {
                return PERMISSION_DENIED;
            }
            int32_t property = data.readInt32();
            sp<VehiclePropertiesHolder> holder = listProperties(property);
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
        case SET_PROPERTY: {
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            ScopedVehiclePropValue value;
            r = VehiclePropValueBinderUtil::readFromParcel(data, &value.value,
                    false /* deleteMembers */);
            if (r != NO_ERROR) {
                return r;
            }
            if (!isOperationAllowed(value.value.prop, true)) {
                return PERMISSION_DENIED;
            }
            r = setProperty(value.value);
            BinderUtil::fillNoResultReply(reply);
            return r;
        } break;
        case GET_PROPERTY: {
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            vehicle_prop_value_t value;
            memset(&value, 0, sizeof(value));
            r = VehiclePropValueBinderUtil::readFromParcel(data, &value,
                    false /* deleteMembers */, true /*canIgnoreNoData*/);
            if (r != NO_ERROR) {
                ALOGE("getProperty cannot read %d", r);
                return r;
            }
            if (!isOperationAllowed(value.prop, false)) {
                return PERMISSION_DENIED;
            }
            r = getProperty(&value);
            if (r == NO_ERROR) {
                reply->writeNoException();
                r = VehiclePropValueBinderUtil::writeToParcel(*reply, value);
                releaseMemoryFromGet(&value);
            } else if (r == -EAGAIN) {
                // this should be handled specially to throw ServiceSpecificException in java.
                reply->writeInt32(binder::Status::EX_SERVICE_SPECIFIC);
                return NO_ERROR;
            }
            return r;
        } break;
        case SUBSCRIBE: {
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            sp<IVehicleNetworkListener> listener =
                    interface_cast<IVehicleNetworkListener>(data.readStrongBinder());
            int32_t property = data.readInt32();
            if (!isOperationAllowed(property, false)) {
                return PERMISSION_DENIED;
            }
            float sampleRate = data.readFloat();
            int32_t zones = data.readInt32();
            r = subscribe(listener, property, sampleRate, zones);
            BinderUtil::fillNoResultReply(reply);
            return r;
        } break;
        case UNSUBSCRIBE: {
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            sp<IVehicleNetworkListener> listener =
                    interface_cast<IVehicleNetworkListener>(data.readStrongBinder());
            int32_t property = data.readInt32();
            if (!isOperationAllowed(property, false)) {
                return PERMISSION_DENIED;
            }
            unsubscribe(listener, property);
            BinderUtil::fillNoResultReply(reply);
            return NO_ERROR;
        } break;
        case INJECT_EVENT: {
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            if (data.readInt32() == 0) { // java side allows passing null with this.
                return BAD_VALUE;
            }
            if (!isOperationAllowed(0, true)) {
                return PERMISSION_DENIED;
            }
            ScopedVehiclePropValue value;
            ReadableBlobHolder blob(new Parcel::ReadableBlob());
            ASSERT_OR_HANDLE_NO_MEMORY(blob.blob, return NO_MEMORY);
            int32_t size = data.readInt32();
            r = data.readBlob(size, blob.blob);
            if (r != NO_ERROR) {
                ALOGE("injectEvent:service, cannot read blob");
                return r;
            }
            std::unique_ptr<VehiclePropValue> v(new VehiclePropValue());
            ASSERT_OR_HANDLE_NO_MEMORY(v.get(), return NO_MEMORY);
            if (!v->ParseFromArray(blob.blob->data(), size)) {
                ALOGE("injectEvent:service, cannot parse data");
                return BAD_VALUE;
            }
            r = VehicleNetworkProtoUtil::fromVehiclePropValue(*v.get(), value.value);
            if (r != NO_ERROR) {
                ALOGE("injectEvent:service, cannot convert data");
                return BAD_VALUE;
            }
            r = injectEvent(value.value);
            BinderUtil::fillNoResultReply(reply);
            return r;
        } break;
        case START_MOCKING: {
            if (!isOperationAllowed(0, true)) {
                return PERMISSION_DENIED;
            }
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            sp<IVehicleNetworkHalMock> mock =
                    interface_cast<IVehicleNetworkHalMock>(data.readStrongBinder());
            r = startMocking(mock);
            BinderUtil::fillNoResultReply(reply);
            return r;
        } break;
        case STOP_MOCKING: {
            if (!isOperationAllowed(0, true)) {
                return PERMISSION_DENIED;
            }
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            sp<IVehicleNetworkHalMock> mock =
                    interface_cast<IVehicleNetworkHalMock>(data.readStrongBinder());
            stopMocking(mock);
            BinderUtil::fillNoResultReply(reply);
            return NO_ERROR;
        } break;
        case INJECT_HAL_ERROR: {
            if (!isOperationAllowed(0, true)) {
                return PERMISSION_DENIED;
            }
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            int32_t errorCode = data.readInt32();
            int32_t property = data.readInt32();
            int32_t operation = data.readInt32();
            r = injectHalError(errorCode, property, operation);
            BinderUtil::fillNoResultReply(reply);
            return r;
        } break;
        case START_ERROR_LISTENING: {
            if (!isOperationAllowed(0, false)) {
                return PERMISSION_DENIED;
            }
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            sp<IVehicleNetworkListener> listener =
                    interface_cast<IVehicleNetworkListener>(data.readStrongBinder());
            r = startErrorListening(listener);
            BinderUtil::fillNoResultReply(reply);
            return r;
        } break;
        case STOP_ERROR_LISTENING: {
            if (!isOperationAllowed(0, false)) {
                return PERMISSION_DENIED;
            }
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            sp<IVehicleNetworkListener> listener =
                    interface_cast<IVehicleNetworkListener>(data.readStrongBinder());
            stopErrorListening(listener);
            BinderUtil::fillNoResultReply(reply);
            return NO_ERROR;
        } break;
        case START_HAL_RESTART_MONITORING: {
            if (!isOperationAllowed(0, false)) {
                return PERMISSION_DENIED;
            }
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            sp<IVehicleNetworkListener> listener =
                    interface_cast<IVehicleNetworkListener>(data.readStrongBinder());
            r = startHalRestartMonitoring(listener);
            BinderUtil::fillNoResultReply(reply);
            return r;
        } break;
        case STOP_HAL_RESTART_MONITORING: {
            if (!isOperationAllowed(0, false)) {
                return PERMISSION_DENIED;
            }
            CHECK_INTERFACE(IVehicleNetwork, data, reply);
            sp<IVehicleNetworkListener> listener =
                    interface_cast<IVehicleNetworkListener>(data.readStrongBinder());
            stopHalRestartMonitoring(listener);
            BinderUtil::fillNoResultReply(reply);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}; // namespace android
