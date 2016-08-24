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
#define LOG_TAG "VehicleNetwork"

#include <binder/PermissionCache.h>
#include <utils/Errors.h>
#include <utils/SystemClock.h>

#include <private/android_filesystem_config.h>

#include <vehicle-internal.h>

#include "VehicleHalPropertyUtil.h"
#include "VehicleNetworkService.h"

//#define DBG_EVENT
//#define DBG_VERBOSE
#ifdef DBG_EVENT
#define EVENT_LOG(x...) ALOGD(x)
#else
#define EVENT_LOG(x...)
#endif
#ifdef DBG_VERBOSE
#define LOG_VERBOSE(x...) ALOGD(x)
#else
#define LOG_VERBOSE(x...)
#endif

namespace android {

VehicleHalMessageHandler::VehicleHalMessageHandler(const sp<Looper>& looper,
        VehicleNetworkService& service)
    : mLooper(looper),
      mService(service),
      mFreeListIndex(0),
      mLastDispatchTime(0) {
}

VehicleHalMessageHandler::~VehicleHalMessageHandler() {
    Mutex::Autolock autoLock(mLock);
    for (int i = 0; i < NUM_PROPERTY_EVENT_LISTS; i++) {
        for (auto& e : mHalPropertyList[i]) {
            VehiclePropValueUtil::deleteMembers(e);
            delete e;
        }
    }
    for (VehicleHalError* e : mHalErrors) {
        delete e;
    }
}

static const int MS_TO_NS = 1000000;

void VehicleHalMessageHandler::handleHalEvent(vehicle_prop_value_t *eventData) {
    EVENT_LOG("handleHalEvent 0x%x", eventData->prop);
    Mutex::Autolock autoLock(mLock);
    List<vehicle_prop_value_t*>& propList = mHalPropertyList[mFreeListIndex];
    propList.push_back(eventData);
    int64_t deltaFromLast = elapsedRealtime() - mLastDispatchTime;
    if (deltaFromLast > DISPATCH_INTERVAL_MS) {
        mLooper->sendMessage(this, Message(HAL_EVENT));
    } else {
        mLooper->sendMessageDelayed((DISPATCH_INTERVAL_MS - deltaFromLast) * MS_TO_NS,
                this, Message(HAL_EVENT));
    }
}

void VehicleHalMessageHandler::handleHalError(VehicleHalError* error) {
    Mutex::Autolock autoLock(mLock);
    mHalErrors.push_back(error);
    mLooper->sendMessage(this, Message(HAL_ERROR));
}

void VehicleHalMessageHandler::handleMockStart() {
    Mutex::Autolock autoLock(mLock);
    mHalPropertyList[0].clear();
    mHalPropertyList[1].clear();
    sp<MessageHandler> self(this);
    mLooper->removeMessages(self);
}

void VehicleHalMessageHandler::doHandleHalEvent() {
    // event dispatching can take time, so do it outside lock and that requires double buffering.
    // inside lock, free buffer is swapped with non-free buffer.
    List<vehicle_prop_value_t*>* events = NULL;
    do {
        Mutex::Autolock autoLock(mLock);
        mLastDispatchTime = elapsedRealtime();
        int nonFreeListIndex = mFreeListIndex ^ 0x1;
        List<vehicle_prop_value_t*>* nonFreeList = &(mHalPropertyList[nonFreeListIndex]);
        List<vehicle_prop_value_t*>* freeList = &(mHalPropertyList[mFreeListIndex]);
        if (nonFreeList->size() > 0) {
            for (auto& e : *freeList) {
                nonFreeList->push_back(e);
            }
            freeList->clear();
            events = nonFreeList;
        } else if (freeList->size() > 0) {
            events = freeList;
            mFreeListIndex = nonFreeListIndex;
        }
    } while (false);
    if (events != NULL) {
        EVENT_LOG("doHandleHalEvent, num events:%d", events->size());
        mService.dispatchHalEvents(*events);
        //TODO implement return to memory pool
        for (auto& e : *events) {
            VehiclePropValueUtil::deleteMembers(e);
            delete e;
        }
        events->clear();
    }
}

void VehicleHalMessageHandler::doHandleHalError() {
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
        mService.dispatchHalError(error);
    }
}

void VehicleHalMessageHandler::handleMessage(const Message& message) {
    switch (message.what) {
    case HAL_EVENT:
        doHandleHalEvent();
        break;
    case HAL_ERROR:
        doHandleHalError();
        break;
    }
}

// ----------------------------------------------------------------------------

void MockDeathHandler::binderDied(const wp<IBinder>& who) {
    mService.handleHalMockDeath(who);
}

// ----------------------------------------------------------------------------

PropertyValueCache::PropertyValueCache() {

}

PropertyValueCache::~PropertyValueCache() {
    for (size_t i = 0; i < mCache.size(); i++) {
        vehicle_prop_value_t* v = mCache.editValueAt(i);
        VehiclePropValueUtil::deleteMembers(v);
        delete v;
    }
    mCache.clear();
}

void PropertyValueCache::writeToCache(const vehicle_prop_value_t& value) {
    vehicle_prop_value_t* v;
    ssize_t index = mCache.indexOfKey(value.prop);
    if (index < 0) {
        v = VehiclePropValueUtil::allocVehicleProp(value);
        ASSERT_OR_HANDLE_NO_MEMORY(v, return);
        mCache.add(value.prop, v);
    } else {
        v = mCache.editValueAt(index);
        VehiclePropValueUtil::copyVehicleProp(v, value, true /* deleteOldData */);
    }
}

bool PropertyValueCache::readFromCache(vehicle_prop_value_t* value) {
    ssize_t index = mCache.indexOfKey(value->prop);
    if (index < 0) {
        ALOGE("readFromCache 0x%x, not found", value->prop);
        return false;
    }
    const vehicle_prop_value_t* cached = mCache.valueAt(index);
    //TODO this can be improved by just passing pointer and not deleting members.
    status_t r = VehiclePropValueUtil::copyVehicleProp(value, *cached);
    if (r != NO_ERROR) {
        ALOGD("readFromCache 0x%x, copy failed %d", value->prop, r);
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------


VehicleNetworkService* VehicleNetworkService::sInstance = NULL;

status_t VehicleNetworkService::dump(int fd, const Vector<String16>& /*args*/) {
    static const String16 sDump("android.permission.DUMP");
    String8 msg;
    if (!PermissionCache::checkCallingPermission(sDump)) {
        msg.appendFormat("Permission Denial: "
                    "can't dump VNS from pid=%d, uid=%d\n",
                    IPCThreadState::self()->getCallingPid(),
                    IPCThreadState::self()->getCallingUid());
        write(fd, msg.string(), msg.size());
        return NO_ERROR;
    }
    msg.append("MockingEnabled=%d\n", mMockingEnabled ? 1 : 0);
    msg.append("*Properties\n");
    for (auto& prop : mProperties->getList()) {
        VechilePropertyUtil::dumpProperty(msg, *prop);
    }
    if (mMockingEnabled) {
        msg.append("*Mocked Properties\n");
        for (auto& prop : mPropertiesForMocking->getList()) {
            //TODO dump more info
            msg.appendFormat("property 0x%x\n", prop->prop);
        }
    }
    msg.append("*Active clients*\n");
    for (size_t i = 0; i < mBinderToClientMap.size(); i++) {
        msg.appendFormat("pid %d uid %d\n", mBinderToClientMap.valueAt(i)->getPid(),
                mBinderToClientMap.valueAt(i)->getUid());
    }
    msg.append("*Active clients per property*\n");
    for (size_t i = 0; i < mPropertyToClientsMap.size(); i++) {
        msg.appendFormat("prop 0x%x, pids:", mPropertyToClientsMap.keyAt(i));
        sp<HalClientSpVector> clients = mPropertyToClientsMap.valueAt(i);
        for (size_t j = 0; j < clients->size(); j++) {
            msg.appendFormat("%d,", clients->itemAt(j)->getPid());
        }
        msg.append("\n");
    }
    msg.append("*Subscription info per property*\n");
    for (size_t i = 0; i < mSubscriptionInfos.size(); i++) {
        const SubscriptionInfo& info = mSubscriptionInfos.valueAt(i);
        msg.appendFormat("prop 0x%x, sample rate %f Hz, zones 0x%x\n", mSubscriptionInfos.keyAt(i),
                info.sampleRate, info.zones);
    }
    msg.append("*Event counts per property*\n");
    for (size_t i = 0; i < mEventsCount.size(); i++) {
        msg.appendFormat("prop 0x%x: %d\n", mEventsCount.keyAt(i),
                mEventsCount.valueAt(i));
    }
    msg.append("*Vehicle Network Service Permissions*\n");
    mVehiclePropertyAccessControl.dump(msg);

    write(fd, msg.string(), msg.size());
    return NO_ERROR;
}

bool VehicleNetworkService::isOperationAllowed(int32_t property, bool isWrite) {
    const uid_t uid = IPCThreadState::self()->getCallingUid();

    bool allowed = mVehiclePropertyAccessControl.testAccess(property, uid, isWrite);
    if (!allowed) {
        ALOGW("Property 0x%x: access not allowed for uid %d, isWrite %d", property, uid, isWrite);
    }
    return allowed;
}

VehicleNetworkService::VehicleNetworkService()
    : mModule(NULL),
      mMockingEnabled(false) {
    sInstance = this;

   // Load vehicle network services policy file
   if(!mVehiclePropertyAccessControl.init()) {
     LOG_ALWAYS_FATAL("Vehicle property access policy could not be initialized.");
   }
}

VehicleNetworkService::~VehicleNetworkService() {
    sInstance = NULL;
    for (size_t i = 0; i < mPropertyToClientsMap.size(); i++) {
        sp<HalClientSpVector> clients = mPropertyToClientsMap.editValueAt(i);
        clients->clear();
    }
    mBinderToClientMap.clear();
    mPropertyToClientsMap.clear();
    mSubscriptionInfos.clear();
}

void VehicleNetworkService::binderDied(const wp<IBinder>& who) {
    List<int32_t> propertiesToUnsubscribe;
    do {
        Mutex::Autolock autoLock(mLock);
        sp<IBinder> ibinder = who.promote();
        ibinder->unlinkToDeath(this);
        ssize_t index = mBinderToClientMap.indexOfKey(ibinder);
        if (index < 0) {
            // already removed. ignore
            return;
        }
        sp<HalClient> currentClient = mBinderToClientMap.editValueAt(index);
        ALOGW("client binder death, pid: %d, uid:%d", currentClient->getPid(),
                currentClient->getUid());
        mBinderToClientMap.removeItemsAt(index);

        for (size_t i = 0; i < mPropertyToClientsMap.size(); i++) {
            sp<HalClientSpVector>& clients = mPropertyToClientsMap.editValueAt(i);
            clients->remove(currentClient);
            // TODO update frame rate
            if (clients->size() == 0) {
                int32_t property = mPropertyToClientsMap.keyAt(i);
                propertiesToUnsubscribe.push_back(property);
                mSubscriptionInfos.removeItem(property);
            }
        }
        for (int32_t property : propertiesToUnsubscribe) {
            mPropertyToClientsMap.removeItem(property);
        }
    } while (false);
    for (int32_t property : propertiesToUnsubscribe) {
        mDevice->unsubscribe(mDevice, property);
    }
}

void VehicleNetworkService::handleHalMockDeath(const wp<IBinder>& who) {
    ALOGE("Hal mock binder died");
    sp<IBinder> ibinder = who.promote();
    stopMocking(IVehicleNetworkHalMock::asInterface(ibinder));
}

int VehicleNetworkService::eventCallback(const vehicle_prop_value_t *eventData) {
    EVENT_LOG("eventCallback 0x%x");
    sInstance->onHalEvent(eventData);
    return NO_ERROR;
}

int VehicleNetworkService::errorCallback(int32_t errorCode, int32_t property, int32_t operation) {
    status_t r = sInstance->onHalError(errorCode, property, operation);
    if (r != NO_ERROR) {
        ALOGE("VehicleNetworkService::errorCallback onHalError failed with %d", r);
    }
    return NO_ERROR;
}

extern "C" {
vehicle_prop_config_t const * getInternalProperties();
int getNumInternalProperties();
};

void VehicleNetworkService::onFirstRef() {
    Mutex::Autolock autoLock(mLock);
    status_t r = loadHal();
    if (r!= NO_ERROR) {
        ALOGE("cannot load HAL, error:%d", r);
        return;
    }
    mHandlerThread = new HandlerThread();
    r = mHandlerThread->start("HAL.NATIVE_LOOP");
    if (r != NO_ERROR) {
        ALOGE("cannot start handler thread, error:%d", r);
        return;
    }
    sp<VehicleHalMessageHandler> handler(new VehicleHalMessageHandler(mHandlerThread->getLooper(),
            *this));
    ASSERT_ALWAYS_ON_NO_MEMORY(handler.get());
    mHandler = handler;
    r = mDevice->init(mDevice, eventCallback, errorCallback);
    if (r != NO_ERROR) {
        ALOGE("HAL init failed:%d", r);
        return;
    }
    int numConfigs = 0;
    vehicle_prop_config_t const* configs = mDevice->list_properties(mDevice, &numConfigs);
    mProperties = new VehiclePropertiesHolder(false /* deleteConfigsInDestructor */);
    ASSERT_ALWAYS_ON_NO_MEMORY(mProperties);
    for (int i = 0; i < numConfigs; i++) {
        mProperties->getList().push_back(&configs[i]);
    }
    configs = getInternalProperties();
    for (int i = 0; i < getNumInternalProperties(); i++) {
        mProperties->getList().push_back(&configs[i]);
    }
}

void VehicleNetworkService::release() {
    do {
        Mutex::Autolock autoLock(mLock);
        mHandlerThread->quit();
    } while (false);
    mDevice->release(mDevice);
}

vehicle_prop_config_t const * VehicleNetworkService::findConfigLocked(int32_t property) {
    for (auto& config : (mMockingEnabled ?
            mPropertiesForMocking->getList() : mProperties->getList())) {
        if (config->prop == property) {
            return config;
        }
    }
    ALOGW("property not found 0x%x", property);
    return NULL;
}

bool VehicleNetworkService::isGettableLocked(int32_t property) {
    vehicle_prop_config_t const * config = findConfigLocked(property);
    if (config == NULL) {
        return false;
    }
    if ((config->access & VEHICLE_PROP_ACCESS_READ) == 0) {
        ALOGI("cannot get, property 0x%x is write only", property);
        return false;
    }
    return true;
}

bool VehicleNetworkService::isSettableLocked(int32_t property, int32_t valueType) {
    vehicle_prop_config_t const * config = findConfigLocked(property);
    if (config == NULL) {
        return false;
    }
    if ((config->access & VEHICLE_PROP_ACCESS_WRITE) == 0) {
        ALOGI("cannot set, property 0x%x is read only", property);
        return false;
    }
    if (config->value_type != valueType) {
        ALOGW("cannot set, property 0x%x expects type 0x%x while got 0x%x", property,
                config->value_type, valueType);
        return false;
    }
    return true;
}

bool VehicleNetworkService::isSubscribableLocked(int32_t property) {
    vehicle_prop_config_t const * config = findConfigLocked(property);
    if (config == NULL) {
        return false;
    }
    if ((config->access & VEHICLE_PROP_ACCESS_READ) == 0) {
        ALOGI("cannot subscribe, property 0x%x is write only", property);
        return false;
    }
    if (config->change_mode == VEHICLE_PROP_CHANGE_MODE_STATIC) {
        ALOGI("cannot subscribe, property 0x%x is static", property);
        return false;
    }
    return true;
}

bool VehicleNetworkService::isZonedProperty(vehicle_prop_config_t const * config) {
    if (config == NULL) {
        return false;
    }
    switch (config->value_type) {
    case VEHICLE_VALUE_TYPE_ZONED_INT32:
    case VEHICLE_VALUE_TYPE_ZONED_FLOAT:
    case VEHICLE_VALUE_TYPE_ZONED_BOOLEAN:
    case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC2:
    case VEHICLE_VALUE_TYPE_ZONED_INT32_VEC3:
    case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC2:
    case VEHICLE_VALUE_TYPE_ZONED_FLOAT_VEC3:
        return true;
    }
    return false;
}

sp<VehiclePropertiesHolder> VehicleNetworkService::listProperties(int32_t property) {
    Mutex::Autolock autoLock(mLock);
    if (property == 0) {
        if (!mMockingEnabled) {
            return mProperties;
        } else {
            return mPropertiesForMocking;
        }
    } else {
        sp<VehiclePropertiesHolder> p;
        const vehicle_prop_config_t* config = findConfigLocked(property);
        if (config != NULL) {
            p = new VehiclePropertiesHolder(false /* deleteConfigsInDestructor */);
            p->getList().push_back(config);
            ASSERT_OR_HANDLE_NO_MEMORY(p.get(), return p);
        }
        return p;
    }
}

status_t VehicleNetworkService::getProperty(vehicle_prop_value_t *data) {
    bool inMocking = false;
    do { // for lock scoping
        Mutex::Autolock autoLock(mLock);
        if (!isGettableLocked(data->prop)) {
            ALOGW("getProperty, cannot get 0x%x", data->prop);
            return BAD_VALUE;
        }
        if ((data->prop >= (int32_t)VEHICLE_PROPERTY_INTERNAL_START) &&
                (data->prop <= (int32_t)VEHICLE_PROPERTY_INTERNAL_END)) {
            if (!mCache.readFromCache(data)) {
                return BAD_VALUE;
            }
            return NO_ERROR;
        }
        //TODO caching for static, on-change type?
        if (mMockingEnabled) {
            inMocking = true;
        }
    } while (false);
    // set done outside lock to allow concurrent access
    if (inMocking) {
        status_t r = mHalMock->onPropertyGet(data);
        if (r != NO_ERROR) {
            ALOGW("getProperty 0x%x failed, mock returned %d", data->prop, r);
        }
        return r;
    }
    /*
     * get call can return -EAGAIN error when hal has not fetched all data. In that case,
     * keep retrying for certain time with some sleep. This will happen only at initial stage.
     */
    status_t r = -EAGAIN;
    int retryCount = 0;
    while (true) {
        r = mDevice->get(mDevice, data);
        if (r == NO_ERROR) {
            break;
        }
        if (r != -EAGAIN) {
            break;
        }
        retryCount++;
        if (retryCount > MAX_GET_RETRY_FOR_NOT_READY) {
            ALOGE("Vehicle hal keep retrying not ready after multiple retries");
            break;
        }
        usleep(GET_WAIT_US);
    }
    if (r != NO_ERROR) {
        ALOGW("getProperty 0x%x failed, HAL returned %d", data->prop, r);
    }
    return r;
}

void VehicleNetworkService::releaseMemoryFromGet(vehicle_prop_value_t* value) {
    switch (value->prop) {
    case VEHICLE_VALUE_TYPE_STRING:
    case VEHICLE_VALUE_TYPE_BYTES: {
        Mutex::Autolock autoLock(mLock);
        mDevice->release_memory_from_get(mDevice, value);
    } break;
    }
}

status_t VehicleNetworkService::setProperty(const vehicle_prop_value_t& data) {
    bool isInternalProperty = false;
    bool inMocking = false;
    do { // for lock scoping
        Mutex::Autolock autoLock(mLock);
        if (!isSettableLocked(data.prop, data.value_type)) {
            ALOGW("setProperty, cannot set 0x%x", data.prop);
            return BAD_VALUE;
        }
        if ((data.prop >= (int32_t)VEHICLE_PROPERTY_INTERNAL_START) &&
                            (data.prop <= (int32_t)VEHICLE_PROPERTY_INTERNAL_END)) {
            isInternalProperty = true;
            mCache.writeToCache(data);
        }
        if (mMockingEnabled) {
            inMocking = true;
        }
    } while (false);
    if (inMocking) {
        status_t r = mHalMock->onPropertySet(data);
        if (r != NO_ERROR) {
            ALOGW("setProperty 0x%x failed, mock returned %d", data.prop, r);
            return r;
        }
    }
    if (isInternalProperty) {
        // for internal property, just publish it.
        onHalEvent(&data, inMocking);
        return NO_ERROR;
    }
    if (inMocking) {
        return NO_ERROR;
    }
    //TODO add value check requires auto generated code to return value range for enum types
    // set done outside lock to allow concurrent access
    status_t r = mDevice->set(mDevice, &data);
    if (r != NO_ERROR) {
        ALOGW("setProperty 0x%x failed, HAL returned %d", data.prop, r);
    }
    return r;
}

status_t VehicleNetworkService::subscribe(const sp<IVehicleNetworkListener> &listener, int32_t prop,
        float sampleRate, int32_t zones) {
    bool shouldSubscribe = false;
    bool inMock = false;
    int32_t newZones = zones;
    do {
        Mutex::Autolock autoLock(mLock);
        if (!isSubscribableLocked(prop)) {
            return BAD_VALUE;
        }
        vehicle_prop_config_t const * config = findConfigLocked(prop);
        if (config->change_mode == VEHICLE_PROP_CHANGE_MODE_ON_CHANGE) {
            if (sampleRate != 0) {
                ALOGW("Sample rate set to non-zeo for on change type. Ignore it");
                sampleRate = 0;
            }
        } else {
            if (sampleRate > config->max_sample_rate) {
                ALOGW("sample rate %f higher than max %f. limit to max", sampleRate,
                        config->max_sample_rate);
                sampleRate = config->max_sample_rate;
            }
            if (sampleRate < config->min_sample_rate) {
                ALOGW("sample rate %f lower than min %f. limit to min", sampleRate,
                                    config->min_sample_rate);
                sampleRate = config->min_sample_rate;
            }
        }
        if (isZonedProperty(config)) {
            if ((zones != 0) && ((zones & config->vehicle_zone_flags) != zones)) {
                ALOGE("subscribe requested zones 0x%x out of range, supported:0x%x", zones,
                        config->vehicle_zone_flags);
                return BAD_VALUE;
            }
        } else { // ignore zone
            zones = 0;
        }
        sp<IBinder> ibinder = IInterface::asBinder(listener);
        LOG_VERBOSE("subscribe, binder 0x%x prop 0x%x", ibinder.get(), prop);
        sp<HalClient> client = findOrCreateClientLocked(ibinder, listener);
        if (client.get() == NULL) {
            ALOGE("subscribe, no memory, cannot create HalClient");
            return NO_MEMORY;
        }
        sp<HalClientSpVector> clientsForProperty = findOrCreateClientsVectorForPropertyLocked(prop);
        if (clientsForProperty.get() == NULL) {
            ALOGE("subscribe, no memory, cannot create HalClientSpVector");
            return NO_MEMORY;
        }
        clientsForProperty->add(client);
        ssize_t index = mSubscriptionInfos.indexOfKey(prop);
        if (index < 0) {
            // first time subscription for this property
            shouldSubscribe = true;
        } else {
            const SubscriptionInfo& info = mSubscriptionInfos.valueAt(index);
            if (info.sampleRate < sampleRate) {
                shouldSubscribe = true;
            }
            newZones = (info.zones == 0) ? 0 : ((zones == 0) ? 0 : (info.zones | zones));
            if (info.zones != newZones) {
                shouldSubscribe = true;
            }
        }
        client->setSubscriptionInfo(prop, sampleRate, zones);
        if (shouldSubscribe) {
            inMock = mMockingEnabled;
            SubscriptionInfo info(sampleRate, newZones);
            mSubscriptionInfos.add(prop, info);
            if ((prop >= (int32_t)VEHICLE_PROPERTY_INTERNAL_START) &&
                                (prop <= (int32_t)VEHICLE_PROPERTY_INTERNAL_END)) {
                LOG_VERBOSE("subscribe to internal property, prop 0x%x", prop);
                return NO_ERROR;
            }
        }
    } while (false);
    if (shouldSubscribe) {
        status_t r;
        if (inMock) {
            r = mHalMock->onPropertySubscribe(prop, sampleRate, newZones);
            if (r != NO_ERROR) {
                ALOGW("subscribe 0x%x failed, mock returned %d", prop, r);
            }
        } else {
            LOG_VERBOSE("subscribe to HAL, prop 0x%x sample rate:%f zones:0x%x", prop, sampleRate,
                    newZones);
            r = mDevice->subscribe(mDevice, prop, sampleRate, newZones);
            if (r != NO_ERROR) {
                ALOGW("subscribe 0x%x failed, HAL returned %d", prop, r);
            }
        }
        return r;
    }
    return NO_ERROR;
}

void VehicleNetworkService::unsubscribe(const sp<IVehicleNetworkListener> &listener, int32_t prop) {
    bool shouldUnsubscribe = false;
    bool inMocking = false;
    do {
        Mutex::Autolock autoLock(mLock);
        if (!isSubscribableLocked(prop)) {
            return;
        }
        sp<IBinder> ibinder = IInterface::asBinder(listener);
        LOG_VERBOSE("unsubscribe, binder 0x%x, prop 0x%x", ibinder.get(), prop);
        sp<HalClient> client = findClientLocked(ibinder);
        if (client.get() == NULL) {
            ALOGD("unsubscribe client not found in binder map");
            return;
        }
        shouldUnsubscribe = removePropertyFromClientLocked(ibinder, client, prop);
        if ((prop >= (int32_t)VEHICLE_PROPERTY_INTERNAL_START) &&
                (prop <= (int32_t)VEHICLE_PROPERTY_INTERNAL_END)) {
            LOG_VERBOSE("unsubscribe to internal property, prop 0x%x", prop);
            return;
        }
        if (mMockingEnabled) {
            inMocking = true;
        }
    } while (false);
    if (shouldUnsubscribe) {
        if (inMocking) {
            mHalMock->onPropertyUnsubscribe(prop);
        } else {
            mDevice->unsubscribe(mDevice, prop);
        }
    }
}

sp<HalClient> VehicleNetworkService::findClientLocked(sp<IBinder>& ibinder) {
    sp<HalClient> client;
    ssize_t index = mBinderToClientMap.indexOfKey(ibinder);
    if (index < 0) {
        return client;
    }
    return mBinderToClientMap.editValueAt(index);
}

sp<HalClient> VehicleNetworkService::findOrCreateClientLocked(sp<IBinder>& ibinder,
        const sp<IVehicleNetworkListener> &listener) {
    sp<HalClient> client;
    ssize_t index = mBinderToClientMap.indexOfKey(ibinder);
    if (index < 0) {
        IPCThreadState* self = IPCThreadState::self();
        pid_t pid = self->getCallingPid();
        uid_t uid = self->getCallingUid();
        client = new HalClient(listener, pid, uid);
        ASSERT_OR_HANDLE_NO_MEMORY(client.get(), return client);
        ibinder->linkToDeath(this);
        LOG_VERBOSE("add binder 0x%x to map", ibinder.get());
        mBinderToClientMap.add(ibinder, client);
    } else {
        client = mBinderToClientMap.editValueAt(index);
    }
    return client;
}

sp<HalClientSpVector> VehicleNetworkService::findClientsVectorForPropertyLocked(int32_t property) {
    sp<HalClientSpVector> clientsForProperty;
    ssize_t index = mPropertyToClientsMap.indexOfKey(property);
    if (index >= 0) {
        clientsForProperty = mPropertyToClientsMap.editValueAt(index);
    }
    return clientsForProperty;
}

sp<HalClientSpVector> VehicleNetworkService::findOrCreateClientsVectorForPropertyLocked(
        int32_t property) {
    sp<HalClientSpVector> clientsForProperty;
    ssize_t index = mPropertyToClientsMap.indexOfKey(property);
    if (index >= 0) {
        clientsForProperty = mPropertyToClientsMap.editValueAt(index);
    } else {
        clientsForProperty = new HalClientSpVector();
        ASSERT_OR_HANDLE_NO_MEMORY(clientsForProperty.get(), return clientsForProperty);
        mPropertyToClientsMap.add(property, clientsForProperty);
    }
    return clientsForProperty;
}

/**
 * remove given property from client and remove HalCLient if necessary.
 * @return true if the property should be unsubscribed from HAL (=no more clients).
 */
bool VehicleNetworkService::removePropertyFromClientLocked(sp<IBinder>& ibinder,
        sp<HalClient>& client, int32_t property) {
    if(!client->removePropertyAndCheckIfActive(property)) {
        // client is no longer necessary
        mBinderToClientMap.removeItem(ibinder);
        ibinder->unlinkToDeath(this);
    }
    sp<HalClientSpVector> clientsForProperty = findClientsVectorForPropertyLocked(property);
    if (clientsForProperty.get() == NULL) {
        // no subscription
        return false;
    }
    clientsForProperty->remove(client);
    //TODO reset sample rate. do not care for now.
    if (clientsForProperty->size() == 0) {
        mPropertyToClientsMap.removeItem(property);
        mSubscriptionInfos.removeItem(property);
        return true;
    }
    return false;
}

status_t VehicleNetworkService::injectEvent(const vehicle_prop_value_t& value) {
    ALOGI("injectEvent property:0x%x", value.prop);
    return onHalEvent(&value, true);
}

status_t VehicleNetworkService::startMocking(const sp<IVehicleNetworkHalMock>& mock) {
    sp<VehicleHalMessageHandler> handler;
    List<sp<HalClient> > clientsToDispatch;
    do {
        Mutex::Autolock autoLock(mLock);
        if (mMockingEnabled) {
            ALOGW("startMocking while already enabled");
            // allow it as test can fail without clearing
            if (mHalMock != NULL) {
                IInterface::asBinder(mHalMock)->unlinkToDeath(mHalMockDeathHandler.get());
            }
        }
        ALOGW("starting vehicle HAL mocking");
        sp<IBinder> ibinder = IInterface::asBinder(mock);
        if (mHalMockDeathHandler.get() == NULL) {
            mHalMockDeathHandler = new MockDeathHandler(*this);
        }
        ibinder->linkToDeath(mHalMockDeathHandler);
        mHalMock = mock;
        mMockingEnabled = true;
        // Mock implementation should make sure that its startMocking call is not blocking its
        // onlistProperties call. Otherwise, this will lead into dead-lock.
        mPropertiesForMocking = mock->onListProperties();
        handleHalRestartAndGetClientsToDispatchLocked(clientsToDispatch);
        //TODO handle binder death
        handler = mHandler;
    } while (false);
    handler->handleMockStart();
    for (auto& client : clientsToDispatch) {
        client->dispatchHalRestart(true);
    }
    clientsToDispatch.clear();
    return NO_ERROR;
}

void VehicleNetworkService::stopMocking(const sp<IVehicleNetworkHalMock>& mock) {
    List<sp<HalClient> > clientsToDispatch;
    do {
        Mutex::Autolock autoLock(mLock);
        if (mHalMock.get() == NULL) {
            return;
        }
        sp<IBinder> ibinder = IInterface::asBinder(mock);
        if (ibinder != IInterface::asBinder(mHalMock)) {
            ALOGE("stopMocking, not the one started");
            return;
        }
        ALOGW("stopping vehicle HAL mocking");
        ibinder->unlinkToDeath(mHalMockDeathHandler.get());
        mHalMock = NULL;
        mMockingEnabled = false;
        handleHalRestartAndGetClientsToDispatchLocked(clientsToDispatch);
    } while (false);
    for (auto& client : clientsToDispatch) {
        client->dispatchHalRestart(false);
    }
    clientsToDispatch.clear();
}

void VehicleNetworkService::handleHalRestartAndGetClientsToDispatchLocked(
        List<sp<HalClient> >& clientsToDispatch) {
    // all subscriptions are invalid
    mPropertyToClientsMap.clear();
    mSubscriptionInfos.clear();
    mEventsCount.clear();
    List<sp<HalClient> > clientsToRemove;
    for (size_t i = 0; i < mBinderToClientMap.size(); i++) {
        sp<HalClient> client = mBinderToClientMap.valueAt(i);
        client->removeAllProperties();
        if (client->isMonitoringHalRestart()) {
            clientsToDispatch.push_back(client);
        }
        if (!client->isActive()) {
            clientsToRemove.push_back(client);
        }
    }
    for (auto& client : clientsToRemove) {
        // client is no longer necessary
        sp<IBinder> ibinder = IInterface::asBinder(client->getListener());
        mBinderToClientMap.removeItem(ibinder);
        ibinder->unlinkToDeath(this);
    }
    clientsToRemove.clear();
}

status_t VehicleNetworkService::injectHalError(int32_t errorCode, int32_t property,
        int32_t operation) {
    return onHalError(errorCode, property, operation, true /*isInjection*/);
}

status_t VehicleNetworkService::startErrorListening(const sp<IVehicleNetworkListener> &listener) {
    sp<IBinder> ibinder = IInterface::asBinder(listener);
    sp<HalClient> client;
    do {
        Mutex::Autolock autoLock(mLock);
        client = findOrCreateClientLocked(ibinder, listener);
    } while (false);
    if (client.get() == NULL) {
        ALOGW("startErrorListening failed, no memory");
        return NO_MEMORY;
    }
    client->setHalErrorMonitoringState(true);
    return NO_ERROR;
}

void VehicleNetworkService::stopErrorListening(const sp<IVehicleNetworkListener> &listener) {
    sp<IBinder> ibinder = IInterface::asBinder(listener);
    sp<HalClient> client;
    do {
        Mutex::Autolock autoLock(mLock);
        client = findClientLocked(ibinder);
    } while (false);
    if (client.get() != NULL) {
        client->setHalErrorMonitoringState(false);
    }
}

status_t VehicleNetworkService::startHalRestartMonitoring(
        const sp<IVehicleNetworkListener> &listener) {
    sp<IBinder> ibinder = IInterface::asBinder(listener);
    sp<HalClient> client;
    do {
        Mutex::Autolock autoLock(mLock);
        client = findOrCreateClientLocked(ibinder, listener);
    } while (false);
    if (client.get() == NULL) {
        ALOGW("startHalRestartMonitoring failed, no memory");
        return NO_MEMORY;
    }
    client->setHalRestartMonitoringState(true);
    return NO_ERROR;
}

void VehicleNetworkService::stopHalRestartMonitoring(const sp<IVehicleNetworkListener> &listener) {
    sp<IBinder> ibinder = IInterface::asBinder(listener);
    sp<HalClient> client;
    do {
        Mutex::Autolock autoLock(mLock);
        client = findClientLocked(ibinder);
    } while (false);
    if (client.get() != NULL) {
        client->setHalRestartMonitoringState(false);
    }
}

status_t VehicleNetworkService::onHalEvent(const vehicle_prop_value_t* eventData, bool isInjection)
{
    sp<VehicleHalMessageHandler> handler;
    do {
        Mutex::Autolock autoLock(mLock);
        if (!isInjection) {
            if (mMockingEnabled) {
                // drop real HAL event if mocking is enabled
                return NO_ERROR;
            }
        }
        ssize_t index = mEventsCount.indexOfKey(eventData->prop);
        if (index < 0) {
            mEventsCount.add(eventData->prop, 1);
        } else {
            int count = mEventsCount.valueAt(index);
            count++;
            mEventsCount.add(eventData->prop, count);
        }
        handler = mHandler;
    } while (false);
    //TODO add memory pool
    vehicle_prop_value_t* copy = VehiclePropValueUtil::allocVehicleProp(*eventData);
    ASSERT_OR_HANDLE_NO_MEMORY(copy, return NO_MEMORY);
    handler->handleHalEvent(copy);
    return NO_ERROR;
}

status_t VehicleNetworkService::onHalError(int32_t errorCode, int32_t property, int32_t operation,
        bool isInjection) {
    sp<VehicleHalMessageHandler> handler;
    VehicleHalError* error = NULL;
    do {
        Mutex::Autolock autoLock(mLock);
        if (!isInjection) {
            if (mMockingEnabled) {
                // drop real HAL error if mocking is enabled
                return NO_ERROR;
            }
        }

        error = new VehicleHalError(errorCode, property, operation);
        if (error == NULL) {
            return NO_MEMORY;
        }
        handler = mHandler;
    } while (false);
    ALOGI("HAL error, error code:%d, property:0x%x, operation:%d, isInjection:%d",
            errorCode, property, operation, isInjection? 1 : 0);
    handler->handleHalError(error);
    return NO_ERROR;
}

void VehicleNetworkService::dispatchHalEvents(List<vehicle_prop_value_t*>& events) {
    HalClientSpVector activeClients;
    do { // for lock scoping
        Mutex::Autolock autoLock(mLock);
        for (vehicle_prop_value_t* e : events) {
            ssize_t index = mPropertyToClientsMap.indexOfKey(e->prop);
            if (index < 0) {
                EVENT_LOG("HAL event for not subscribed property 0x%x", e->prop);
                continue;
            }
            sp<HalClientSpVector>& clients = mPropertyToClientsMap.editValueAt(index);
            EVENT_LOG("dispatchHalEvents, prop 0x%x, active clients %d", e->prop, clients->size());
            for (size_t i = 0; i < clients->size(); i++) {
                sp<HalClient>& client = clients->editItemAt(i);
                activeClients.add(client);
                client->addEvent(e);
            }
        }
    } while (false);
    EVENT_LOG("dispatchHalEvents num events %d, active clients:%d", events.size(),
            activeClients.size());
    for (size_t i = 0; i < activeClients.size(); i++) {
        sp<HalClient> client = activeClients.editItemAt(i);
        client->dispatchEvents();
    }
    activeClients.clear();
}

void VehicleNetworkService::dispatchHalError(VehicleHalError* error) {
    List<sp<HalClient> > clientsToDispatch;
    do {
        Mutex::Autolock autoLock(mLock);
        if (error->property != 0) {
            sp<HalClientSpVector> clientsForProperty = findClientsVectorForPropertyLocked(
                    error->property);
            if (clientsForProperty.get() != NULL) {
                for (size_t i = 0; i < clientsForProperty->size(); i++) {
                    sp<HalClient> client = clientsForProperty->itemAt(i);
                    clientsToDispatch.push_back(client);
                }
            }
        }
        // Send to global error handler if property is 0 or if no client subscribing.
        if (error->property == 0 || clientsToDispatch.size() == 0) {
            for (size_t i = 0; i < mBinderToClientMap.size(); i++) {
                sp<HalClient> client = mBinderToClientMap.valueAt(i);
                if (client->isMonitoringHalError()) {
                    clientsToDispatch.push_back(client);
                }
            }
        }
    } while (false);
    ALOGI("dispatchHalError error:%d, property:0x%x, operation:%d, num clients to dispatch:%d",
            error->errorCode, error->property, error->operation, clientsToDispatch.size());
    for (auto& client : clientsToDispatch) {
        client->dispatchHalError(error->errorCode, error->property, error->operation);
    }
    clientsToDispatch.clear();
}

status_t VehicleNetworkService::loadHal() {
    int r = hw_get_module(VEHICLE_HARDWARE_MODULE_ID, (hw_module_t const**)&mModule);
    if (r != NO_ERROR) {
        ALOGE("cannot load HAL module, error:%d", r);
        return r;
    }
    r = mModule->common.methods->open(&mModule->common, VEHICLE_HARDWARE_DEVICE,
            (hw_device_t**)&mDevice);
    return r;
}

void VehicleNetworkService::closeHal() {
    mDevice->common.close(&mDevice->common);
}
};
