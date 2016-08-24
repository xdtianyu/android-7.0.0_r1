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

#ifndef CAR_VEHICLE_NETWORK_SERVICE_H_
#define CAR_VEHICLE_NETWORK_SERVICE_H_

#include <stdint.h>
#include <sys/types.h>

#include <memory>

#include <hardware/hardware.h>
#include <hardware/vehicle.h>

#include <binder/BinderService.h>
#include <binder/IBinder.h>
#include <binder/IPCThreadState.h>
#include <cutils/compiler.h>
#include <utils/threads.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/SortedVector.h>
#include <utils/StrongPointer.h>
#include <utils/TypeHelpers.h>

#include <IVehicleNetwork.h>
#include <IVehicleNetworkListener.h>
#include <HandlerThread.h>

#include "VehiclePropertyAccessControl.h"

namespace android {

// ----------------------------------------------------------------------------

class VehicleNetworkService;

/**
 * MessageHandler to dispatch HAL callbacks to pre-defined handler thread context.
 * Init / release is handled in the handler thread to allow upper layer to allocate resource
 * for the thread.
 */
class VehicleHalMessageHandler : public MessageHandler {
    enum {
        HAL_EVENT = 0,
        HAL_ERROR = 1,
    };

    /**
     * For dispatching HAL event in batch. Hal events coming in this time frame will be batched
     * together.
     */
    static const int DISPATCH_INTERVAL_MS = 16;
    static const int NUM_PROPERTY_EVENT_LISTS = 2;
public:
    // not passing VNS as sp as this is held by VNS always.
    VehicleHalMessageHandler(const sp<Looper>& mLooper, VehicleNetworkService& service);
    virtual ~VehicleHalMessageHandler();

    void handleHalEvent(vehicle_prop_value_t *eventData);
    void handleHalError(VehicleHalError* error);
    void handleMockStart();

private:
    void handleMessage(const Message& message);
    void doHandleHalEvent();
    void doHandleHalError();

private:
    mutable Mutex mLock;
    const sp<Looper> mLooper;
    VehicleNetworkService& mService;
    int mFreeListIndex;
    List<vehicle_prop_value_t*> mHalPropertyList[NUM_PROPERTY_EVENT_LISTS];
    int64_t mLastDispatchTime;
    List<VehicleHalError*> mHalErrors;
};
// ----------------------------------------------------------------------------
class SubscriptionInfo {
public:
    float sampleRate;
    int32_t zones;
    SubscriptionInfo()
        : sampleRate(0),
          zones(0) {};
    SubscriptionInfo(float aSampleRate, int32_t aZones)
        : sampleRate(aSampleRate),
          zones(aZones) {};
    SubscriptionInfo(const SubscriptionInfo& info)
        : sampleRate(info.sampleRate),
          zones(info.zones) {};
};

// ----------------------------------------------------------------------------

class HalClient : public virtual RefBase {
public:
    HalClient(const sp<IVehicleNetworkListener> &listener, pid_t pid, uid_t uid) :
        mListener(listener),
        mPid(pid),
        mUid(uid),
        mMonitoringHalRestart(false),
        mMonitoringHalError(false) {
    }

    ~HalClient() {
        mSubscriptionInfos.clear();
    }

    pid_t getPid() {
        return mPid;
    }

    uid_t getUid() {
        return mUid;
    }

    SubscriptionInfo* getSubscriptionInfo(int32_t property) {
        Mutex::Autolock autoLock(mLock);
        ssize_t index = mSubscriptionInfos.indexOfKey(property);
        if (index < 0) {
            return NULL;
        }
        return &mSubscriptionInfos.editValueAt(index);
    }

    void setSubscriptionInfo(int32_t property, float sampleRate, int32_t zones) {
        Mutex::Autolock autoLock(mLock);
        SubscriptionInfo info(sampleRate, zones);
        mSubscriptionInfos.add(property, info);
    }

    bool removePropertyAndCheckIfActive(int32_t property) {
        Mutex::Autolock autoLock(mLock);
        mSubscriptionInfos.removeItem(property);
        return mSubscriptionInfos.size() > 0 || mMonitoringHalRestart || mMonitoringHalError;
    }

    void removeAllProperties() {
        Mutex::Autolock autoLock(mLock);
        mSubscriptionInfos.clear();
    }

    bool isActive() {
        Mutex::Autolock autoLock(mLock);
        return mSubscriptionInfos.size() > 0 || mMonitoringHalRestart || mMonitoringHalError;
    }

    void setHalRestartMonitoringState(bool state) {
        Mutex::Autolock autoLock(mLock);
        mMonitoringHalRestart = state;
    }

    bool isMonitoringHalRestart() {
        Mutex::Autolock autoLock(mLock);
        return mMonitoringHalRestart;
    }

    void setHalErrorMonitoringState(bool state) {
        Mutex::Autolock autoLock(mLock);
        mMonitoringHalError = state;
    }

    bool isMonitoringHalError() {
        Mutex::Autolock autoLock(mLock);
        return mMonitoringHalError;
    }

    const sp<IVehicleNetworkListener>& getListener() {
        return mListener;
    }

    const sp<IBinder> getListenerAsBinder() {
        return IInterface::asBinder(mListener);
    }

    // no lock here as this should be called only from single event looper thread
    void addEvent(vehicle_prop_value_t* event) {
        mEvents.push_back(event);
    }

    // no lock here as this should be called only from single event looper thread
    void clearEvents() {
        mEvents.clear();
    }

    // no lock here as this should be called only from single event looper thread
    List<vehicle_prop_value_t*>& getEventList() {
        return mEvents;
    }

    // no lock here as this should be called only from single event looper thread
    status_t dispatchEvents(){
        ALOGV("dispatchEvents, num Events:%d", mEvents.size());
        sp<VehiclePropValueListHolder> events(new VehiclePropValueListHolder(&mEvents,
                false /*deleteInDestructor */));
        ASSERT_OR_HANDLE_NO_MEMORY(events.get(), return NO_MEMORY);
        mListener->onEvents(events);
        mEvents.clear();
        return NO_ERROR;
    }

    void dispatchHalError(int32_t errorCode, int32_t property, int32_t operation) {
        mListener->onHalError(errorCode, property, operation);
    }

    void dispatchHalRestart(bool inMocking) {
        mListener->onHalRestart(inMocking);
    }

private:
    mutable Mutex mLock;
    const sp<IVehicleNetworkListener> mListener;
    const pid_t mPid;
    const uid_t mUid;
    KeyedVector<int32_t, SubscriptionInfo> mSubscriptionInfos;
    List<vehicle_prop_value_t*> mEvents;
    bool mMonitoringHalRestart;
    bool mMonitoringHalError;
};

class HalClientSpVector : public SortedVector<sp<HalClient> >, public RefBase {
protected:
    virtual int do_compare(const void* lhs, const void* rhs) const {
        sp<HalClient>& lh = * (sp<HalClient> * )(lhs);
        sp<HalClient>& rh = * (sp<HalClient> * )(rhs);
        return compare_type(lh.get(), rh.get());
    }
};

// ----------------------------------------------------------------------------

/**
 * Keeps cached value of property values.
 * For internal property, static property, and on_change property, caching makes sense.
 */
class PropertyValueCache {
public:
    PropertyValueCache();
    virtual ~PropertyValueCache();
    void writeToCache(const vehicle_prop_value_t& value);
    bool readFromCache(vehicle_prop_value_t* value);

private:
    KeyedVector<int32_t, vehicle_prop_value_t*> mCache;
};

// ----------------------------------------------------------------------------

class MockDeathHandler: public IBinder::DeathRecipient {
public:
    MockDeathHandler(VehicleNetworkService& vns) :
        mService(vns) {};

    virtual void binderDied(const wp<IBinder>& who);

private:
    VehicleNetworkService& mService;
};

// ----------------------------------------------------------------------------
class VehicleNetworkService :
    public BinderService<VehicleNetworkService>,
    public BnVehicleNetwork,
    public IBinder::DeathRecipient {
public:
    static const char* getServiceName() ANDROID_API { return IVehicleNetwork::SERVICE_NAME; };

    VehicleNetworkService();
    ~VehicleNetworkService();
    virtual status_t dump(int fd, const Vector<String16>& args);
    void release();
    status_t onHalEvent(const vehicle_prop_value_t *eventData, bool isInjection = false);
    status_t onHalError(int32_t errorCode, int32_t property, int32_t operation,
            bool isInjection = false);
    /**
     * Called by VehicleHalMessageHandler for batching events
     */
    void dispatchHalEvents(List<vehicle_prop_value_t*>& events);
    void dispatchHalError(VehicleHalError* error);
    virtual sp<VehiclePropertiesHolder> listProperties(int32_t property = 0);
    virtual status_t setProperty(const vehicle_prop_value_t& value);
    virtual status_t getProperty(vehicle_prop_value_t* value);
    virtual void releaseMemoryFromGet(vehicle_prop_value_t* value);
    virtual status_t subscribe(const sp<IVehicleNetworkListener> &listener, int32_t property,
            float sampleRate, int32_t zones);
    virtual void unsubscribe(const sp<IVehicleNetworkListener> &listener, int32_t property);
    virtual status_t injectEvent(const vehicle_prop_value_t& value);
    virtual status_t startMocking(const sp<IVehicleNetworkHalMock>& mock);
    virtual void stopMocking(const sp<IVehicleNetworkHalMock>& mock);
    virtual status_t injectHalError(int32_t errorCode, int32_t property, int32_t operation);
    virtual status_t startErrorListening(const sp<IVehicleNetworkListener> &listener);
    virtual void stopErrorListening(const sp<IVehicleNetworkListener> &listener);
    virtual status_t startHalRestartMonitoring(const sp<IVehicleNetworkListener> &listener);
    virtual void stopHalRestartMonitoring(const sp<IVehicleNetworkListener> &listener);
    virtual void binderDied(const wp<IBinder>& who);
    bool isPropertySubsribed(int32_t property);

    void handleHalMockDeath(const wp<IBinder>& who);
protected:
    virtual bool isOperationAllowed(int32_t property, bool isWrite);
private:
    // RefBase
    virtual void onFirstRef();
    status_t loadHal();
    void closeHal();
    vehicle_prop_config_t const * findConfigLocked(int32_t property);
    bool isGettableLocked(int32_t property);
    bool isSettableLocked(int32_t property, int32_t valueType);
    bool isSubscribableLocked(int32_t property);
    static bool isZonedProperty(vehicle_prop_config_t const * config);
    sp<HalClient> findClientLocked(sp<IBinder>& ibinder);
    sp<HalClient> findOrCreateClientLocked(sp<IBinder>& ibinder,
            const sp<IVehicleNetworkListener> &listener);
    sp<HalClientSpVector> findClientsVectorForPropertyLocked(int32_t property);
    sp<HalClientSpVector> findOrCreateClientsVectorForPropertyLocked(int32_t property);
    bool removePropertyFromClientLocked(sp<IBinder>& ibinder, sp<HalClient>& client,
            int32_t property);
    void handleHalRestartAndGetClientsToDispatchLocked(List<sp<HalClient> >& clientsToDispatch);

    static int eventCallback(const vehicle_prop_value_t *eventData);
    static int errorCallback(int32_t errorCode, int32_t property, int32_t operation);
private:
    static const int GET_WAIT_US = 100000;
    static const int MAX_GET_RETRY_FOR_NOT_READY = 50;

    VehiclePropertyAccessControl mVehiclePropertyAccessControl;
    static VehicleNetworkService* sInstance;
    sp<HandlerThread> mHandlerThread;
    sp<VehicleHalMessageHandler> mHandler;
    mutable Mutex mLock;
    vehicle_module_t* mModule;
    vehicle_hw_device_t* mDevice;
    sp<VehiclePropertiesHolder> mProperties;
    KeyedVector<sp<IBinder>, sp<HalClient> > mBinderToClientMap;
    // client subscribing properties
    KeyedVector<int32_t, sp<HalClientSpVector> > mPropertyToClientsMap;
    KeyedVector<int32_t, SubscriptionInfo> mSubscriptionInfos;
    KeyedVector<int32_t, int> mEventsCount;
    PropertyValueCache mCache;
    bool mMockingEnabled;
    sp<IVehicleNetworkHalMock> mHalMock;
    sp<VehiclePropertiesHolder> mPropertiesForMocking;
    sp<MockDeathHandler> mHalMockDeathHandler;
};

};

#endif /* CAR_VEHICLE_NETWORK_SERVICE_H_ */
