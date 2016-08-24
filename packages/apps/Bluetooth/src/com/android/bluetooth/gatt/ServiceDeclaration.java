/*
 * Copyright (C) 2013 The Android Open Source Project
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

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

class ServiceDeclaration {
    private static final boolean DBG = GattServiceConfig.DBG;
    private static final String TAG = GattServiceConfig.TAG_PREFIX + "ServiceDeclaration";

    public static final byte TYPE_UNDEFINED = 0;
    public static final byte TYPE_SERVICE = 1;
    public static final byte TYPE_CHARACTERISTIC = 2;
    public static final byte TYPE_DESCRIPTOR = 3;
    public static final byte TYPE_INCLUDED_SERVICE = 4;

    class Entry {
        byte type = TYPE_UNDEFINED;
        UUID uuid = null;
        int instance = 0;
        int permissions = 0;
        int properties = 0;
        int serviceType = 0;
        int serviceHandle = 0;
        boolean advertisePreferred = false;

        Entry(UUID uuid, int serviceType, int instance) {
            this.type = TYPE_SERVICE;
            this.uuid = uuid;
            this.instance = instance;
            this.serviceType = serviceType;
        }

        Entry(UUID uuid, int serviceType, int instance, boolean advertisePreferred) {
            this.type = TYPE_SERVICE;
            this.uuid = uuid;
            this.instance = instance;
            this.serviceType = serviceType;
            this.advertisePreferred = advertisePreferred;
        }

        Entry(UUID uuid, int properties, int permissions, int instance) {
            this.type = TYPE_CHARACTERISTIC;
            this.uuid = uuid;
            this.instance = instance;
            this.permissions = permissions;
            this.properties = properties;
        }

        Entry(UUID uuid, int permissions) {
            this.type = TYPE_DESCRIPTOR;
            this.uuid = uuid;
            this.permissions = permissions;
        }
    }

    // guards access to mEntries and mNumHandles in order to make this class thread-safe
    private final Object mLock = new Object();
    private final List<Entry> mEntries = new ArrayList<>();
    private int mNumHandles = 0;

    void addService(UUID uuid, int serviceType, int instance, int minHandles,
            boolean advertisePreferred) {
        synchronized (mLock) {
            mEntries.add(new Entry(uuid, serviceType, instance, advertisePreferred));
            if (minHandles == 0) {
                ++mNumHandles;
            } else {
                mNumHandles = minHandles;
            }
        }
    }

    void addIncludedService(UUID uuid, int serviceType, int instance) {
        Entry entry = new Entry(uuid, serviceType, instance);
        entry.type = TYPE_INCLUDED_SERVICE;
        synchronized (mLock) {
            mEntries.add(entry);
            ++mNumHandles;
        }
    }

    void addCharacteristic(UUID uuid, int properties, int permissions) {
        synchronized (mLock) {
            mEntries.add(new Entry(uuid, properties, permissions, 0 /*instance*/));
            mNumHandles += 2;
        }
    }

    void addDescriptor(UUID uuid, int permissions) {
        synchronized (mLock) {
            mEntries.add(new Entry(uuid, permissions));
            ++mNumHandles;
        }
    }

    Entry getNext() {
        synchronized (mLock) {
            if (mEntries.isEmpty()) return null;
            Entry entry = mEntries.get(0);
            mEntries.remove(0);
            return entry;
        }
    }

    boolean isServiceAdvertisePreferred(UUID uuid) {
        synchronized (mLock) {
            for (Entry entry : mEntries) {
                if (entry.uuid.equals(uuid)) {
                    return entry.advertisePreferred;
                }
            }
            return false;
        }
    }

    int getNumHandles() {
        synchronized (mLock) {
            return mNumHandles;
        }
    }
}
