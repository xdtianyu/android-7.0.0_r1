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
#define LOG_TAG "VehicleNetwork.Lib"

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/threads.h>

#include <hardware/vehicle.h>

#include <VehicleNetwork.h>

namespace android {

VehicleNetworkEventMessageHandler::VehicleNetworkEventMessageHandler(const sp<Looper>& looper,
            sp<VehicleNetworkListener>& listener) :
            mLooper(looper),
            mListener(listener) {

}

VehicleNetworkEventMessageHandler::~VehicleNetworkEventMessageHandler() {
    Mutex::Autolock autoLock(mLock);
    mEvents.clear();
    for (VehicleHalError* e : mHalErrors) {
        delete e;
    }
    mHalErrors.clear();
    mHalRestartEvents.clear();
}

void VehicleNetworkEventMessageHandler::handleHalEvents(sp<VehiclePropValueListHolder>& events) {
    Mutex::Autolock autoLock(mLock);
    mEvents.push_back(events);
    mLooper->sendMessage(this, Message(EVENT_EVENTS));
}

void VehicleNetworkEventMessageHandler::handleHalError(int32_t errorCode, int32_t property,
        int32_t operation) {
    Mutex::Autolock autoLock(mLock);
    VehicleHalError* error = new VehicleHalError(errorCode, property, operation);
    if (error == NULL) {
        ALOGE("VehicleNetworkEventMessageHandler::handleHalError, new failed");
        return;
    }
    mHalErrors.push_back(error);
    mLooper->sendMessage(this, Message(EVENT_HAL_ERROR));
}

void VehicleNetworkEventMessageHandler::handleHalRestart(bool inMocking) {
    Mutex::Autolock autoLock(mLock);
    mHalRestartEvents.push_back(inMocking);
    mLooper->sendMessage(this, Message(EVENT_HAL_RESTART));
}

void VehicleNetworkEventMessageHandler::doHandleHalEvents() {
    sp<VehiclePropValueListHolder> values;
    do {
        Mutex::Autolock autoLock(mLock);
        if (mEvents.size() > 0) {
            auto itr = mEvents.begin();
            values = *itr;
            mEvents.erase(itr);
        }
    } while (false);
    if (values.get() != NULL) {
        mListener->onEvents(values);
    }
}

void VehicleNetworkEventMessageHandler::doHandleHalError() {
    VehicleHalError* error = NULL;
    do {
        Mutex::Autolock autoLock(mLock);
        if (mHalErrors.size() > 0) {
            auto itr = mHalErrors.begin();
            error = *itr;
            mHalErrors.erase(itr);
        }
    } while (false);
    if (error != NULL) {
        mListener->onHalError(error->errorCode, error->property, error->operation);
        delete error;
    }
}

void VehicleNetworkEventMessageHandler::doHandleHalRestart() {
    bool shouldDispatch = false;
    bool inMocking = false;
    do {
        Mutex::Autolock autoLock(mLock);
        if (mHalRestartEvents.size() > 0) {
            auto itr = mHalRestartEvents.begin();
            inMocking = *itr;
            mHalRestartEvents.erase(itr);
            shouldDispatch = true;
        }
    } while (false);
    if (shouldDispatch) {
        mListener->onHalRestart(inMocking);
    }
}

void VehicleNetworkEventMessageHandler::handleMessage(const Message& message) {
    switch (message.what) {
    case EVENT_EVENTS:
        doHandleHalEvents();
        break;
    case EVENT_HAL_ERROR:
        doHandleHalError();
        break;
    case EVENT_HAL_RESTART:
        doHandleHalRestart();
        break;
    }
}

// ----------------------------------------------------------------------------

sp<VehicleNetwork> VehicleNetwork::createVehicleNetwork(sp<VehicleNetworkListener>& listener) {
    sp<IBinder> binder = defaultServiceManager()->getService(
            String16(IVehicleNetwork::SERVICE_NAME));
    sp<VehicleNetwork> vn;
    if (binder != NULL) {
        sp<IVehicleNetwork> ivn(interface_cast<IVehicleNetwork>(binder));
        vn = new VehicleNetwork(ivn, listener);
        if (vn != NULL) {
            // in case thread pool is not started, start it.
            ProcessState::self()->startThreadPool();
        }
    }
    return vn;
}

VehicleNetwork::VehicleNetwork(sp<IVehicleNetwork>& vehicleNetwork,
        sp<VehicleNetworkListener> &listener) :
        mService(vehicleNetwork),
        mClientListener(listener) {
}

VehicleNetwork::~VehicleNetwork() {
    sp<IVehicleNetwork> service = getService();
    IInterface::asBinder(service)->unlinkToDeath(this);
    service->stopHalRestartMonitoring(this);
    mHandlerThread->quit();
}

void VehicleNetwork::onFirstRef() {
    Mutex::Autolock autoLock(mLock);
    mHandlerThread = new HandlerThread();
    status_t r = mHandlerThread->start("VNS.NATIVE_LOOP");
    if (r != NO_ERROR) {
        ALOGE("cannot start handler thread, error:%d", r);
        return;
    }
    sp<VehicleNetworkEventMessageHandler> handler(
            new VehicleNetworkEventMessageHandler(mHandlerThread->getLooper(), mClientListener));
    ASSERT_ALWAYS_ON_NO_MEMORY(handler.get());
    mEventHandler = handler;
    IInterface::asBinder(mService)->linkToDeath(this);
    mService->startHalRestartMonitoring(this);
}

status_t VehicleNetwork::setInt32Property(int32_t property, int32_t value) {
    vehicle_prop_value_t v;
    v.prop = property;
    v.value_type = VEHICLE_VALUE_TYPE_INT32;
    v.value.int32_value = value;
    return setProperty(v);
}

status_t VehicleNetwork::getInt32Property(int32_t property, int32_t* value, int64_t* timestamp) {
    vehicle_prop_value_t v;
    v.prop = property;
    // do not check error as it is always safe to access members for this data type.
    // saves one if for normal flow.
    status_t r = getProperty(&v);
    *value = v.value.int32_value;
    *timestamp = v.timestamp;
    return r;
}

status_t VehicleNetwork::setInt64Property(int32_t property, int64_t value) {
    vehicle_prop_value_t v;
    v.prop = property;
    v.value_type = VEHICLE_VALUE_TYPE_INT64;
    v.value.int64_value = value;
    return setProperty(v);
}

status_t VehicleNetwork::getInt64Property(int32_t property, int64_t* value, int64_t* timestamp) {
    vehicle_prop_value_t v;
    v.prop = property;
    status_t r = getProperty(&v);
    *value = v.value.int64_value;
    *timestamp = v.timestamp;
    return r;
}

status_t VehicleNetwork::setFloatProperty(int32_t property, float value) {
    vehicle_prop_value_t v;
    v.prop = property;
    v.value_type = VEHICLE_VALUE_TYPE_FLOAT;
    v.value.float_value = value;
    return setProperty(v);
}

status_t VehicleNetwork::getFloatProperty(int32_t property, float* value, int64_t* timestamp) {
    vehicle_prop_value_t v;
    v.prop = property;
    status_t r = getProperty(&v);
    *value = v.value.float_value;
    *timestamp = v.timestamp;
    return r;
}

status_t VehicleNetwork::setStringProperty(int32_t property, const String8& value) {
    vehicle_prop_value_t v;
    v.prop = property;
    v.value_type = VEHICLE_VALUE_TYPE_STRING;
    v.value.str_value.data = (uint8_t*)value.string();
    v.value.str_value.len = value.length();
    return setProperty(v);
}

status_t VehicleNetwork::getStringProperty(int32_t property, String8& value, int64_t* timestamp) {
    vehicle_prop_value_t v;
    v.prop = property;
    v.value.str_value.len = 0;
    status_t r = getProperty(&v);
    if (r == NO_ERROR) {
        value.setTo((char*)v.value.str_value.data, v.value.str_value.len);
    }
    *timestamp = v.timestamp;
    return r;
}

sp<VehiclePropertiesHolder> VehicleNetwork::listProperties(int32_t property) {
    return getService()->listProperties(property);
}

status_t VehicleNetwork::setProperty(const vehicle_prop_value_t& value) {
    return getService()->setProperty(value);
}

status_t VehicleNetwork::getProperty(vehicle_prop_value_t* value) {
    return getService()->getProperty(value);
}

status_t VehicleNetwork::subscribe(int32_t property, float sampleRate, int32_t zones) {
    return getService()->subscribe(this, property, sampleRate, zones);
}

void VehicleNetwork::unsubscribe(int32_t property) {
    getService()->unsubscribe(this, property);
}

status_t VehicleNetwork::injectEvent(const vehicle_prop_value_t& value) {
    return getService()->injectEvent(value);
}

status_t VehicleNetwork::startMocking(const sp<IVehicleNetworkHalMock>& mock) {
    return getService()->startMocking(mock);
}

void VehicleNetwork::stopMocking(const sp<IVehicleNetworkHalMock>& mock) {
    getService()->stopMocking(mock);
}

status_t VehicleNetwork::startErrorListening() {
    return getService()->startErrorListening(this);
}

void VehicleNetwork::stopErrorListening() {
    getService()->stopErrorListening(this);
}

void VehicleNetwork::binderDied(const wp<IBinder>& who) {
    ALOGE("service died");
    do {
        Mutex::Autolock autoLock(mLock);
        sp<IBinder> ibinder = who.promote();
        ibinder->unlinkToDeath(this);
        sp<IBinder> binder = defaultServiceManager()->getService(
                String16(IVehicleNetwork::SERVICE_NAME));
        mService = interface_cast<IVehicleNetwork>(binder);
        IInterface::asBinder(mService)->linkToDeath(this);
        mService->startHalRestartMonitoring(this);
    } while (false);
    onHalRestart(false);
}

sp<IVehicleNetwork> VehicleNetwork::getService() {
    Mutex::Autolock autoLock(mLock);
    return mService;
}

sp<VehicleNetworkEventMessageHandler> VehicleNetwork::getEventHandler() {
    Mutex::Autolock autoLock(mLock);
    return mEventHandler;
}

void VehicleNetwork::onEvents(sp<VehiclePropValueListHolder>& events) {
    getEventHandler()->handleHalEvents(events);
}

void VehicleNetwork::onHalError(int32_t errorCode, int32_t property, int32_t operation) {
    getEventHandler()->handleHalError(errorCode, property, operation);
}

void VehicleNetwork::onHalRestart(bool inMocking) {
    getEventHandler()->handleHalRestart(inMocking);
}
}; // namespace android
