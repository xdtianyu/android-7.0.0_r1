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
package android.support.car.app.menu.compat;

import android.car.app.menu.RootMenu;
import android.car.app.menu.SubscriptionCallbacks;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.car.app.menu.CarDrawerActivity;
import android.support.car.app.menu.CarMenu;
import android.support.car.app.menu.CarMenuCallbacks;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class EmbeddedCarMenuCallbacksCompat extends android.car.app.menu.CarMenuCallbacks {

    private final CarMenuCallbacks mCallbacks;
    private final CarDrawerActivity mActivity;

    // Map of subscribed ids to their respective callbacks.
    // @GuardedBy("this")
    private final Map<String, List<SubscriptionCallbacks>> mSubscriptionMap =
            new HashMap<String, List<SubscriptionCallbacks>>();

    private final Handler mHandler = new Handler();

    public EmbeddedCarMenuCallbacksCompat(CarDrawerActivity activity,
                                          CarMenuCallbacks callbacks) {
        mActivity = activity;
        mCallbacks = callbacks;
    }

    @Override
    public RootMenu getRootMenu(Bundle hint) {
        android.support.car.app.menu.RootMenu rootMenu = mCallbacks.onGetRoot(hint);
        return new RootMenu(rootMenu.getId(), rootMenu.getBundle());
    }

    @Override
    public void subscribe(String parentId, SubscriptionCallbacks callbacks) {
        synchronized (this) {
            if (!mSubscriptionMap.containsKey(parentId)) {
                mSubscriptionMap.put(parentId, new ArrayList<SubscriptionCallbacks>());
            }
            mSubscriptionMap.get(parentId).add(callbacks);
            loadResultsForClient(parentId, callbacks);
        }
    }

    @Override
    public void unsubscribe(String parentId, SubscriptionCallbacks callbacks) {
        synchronized (this) {
            mSubscriptionMap.get(parentId).remove(callbacks);
        }
    }

    @Override
    public void onCarMenuOpened() {
        mActivity.setDrawerShowing(true);
        mCallbacks.onCarMenuOpened();
    }

    @Override
    public void onCarMenuClosed() {
        mActivity.setDrawerShowing(false);
        mActivity.restoreSearchBox();
    }

    @Override
    public void onItemClicked(String id) {
        mCallbacks.onItemClicked(id);
        mActivity.stopInput();
    }

    @Override
    public boolean onItemLongClicked(String id) {
        return mCallbacks.onItemLongClicked(id);
    }

    @Override
    public boolean onMenuClicked() {

        return mActivity.onMenuClicked();
    }

    @Override
    public void onCarMenuOpening() {
        mActivity.stopInput();
    }

    @Override
    public void onCarMenuClosing() {
        mActivity.restoreSearchBox();
    }

    public void onChildrenChanged(final String parentId) {
        synchronized (this) {
            if (mSubscriptionMap.containsKey(parentId)) {
                final List<SubscriptionCallbacks> callbacks = new ArrayList<>();
                callbacks.addAll(mSubscriptionMap.get(parentId));
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        loadResultsForAllClients(parentId, callbacks);
                    }
                });
            }
        }
    }

    public void onChildChanged(final String parentId, final Bundle item) {
        synchronized (this) {
            if (mSubscriptionMap.containsKey(parentId)) {
                final List<SubscriptionCallbacks> callbacks = new ArrayList<>();
                callbacks.addAll(mSubscriptionMap.get(parentId));
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        for (SubscriptionCallbacks callback : callbacks) {
                            callback.onChildChanged(parentId, item);
                        }
                    }
                });

            }
        }
    }

    private void loadResultsForAllClients(final String parentId,
            @NonNull final List<SubscriptionCallbacks> callbacks) {
        final CarMenu result = new CarMenu(mActivity.getResources().getDisplayMetrics()) {
            @Override
            protected void onResultReady(List<Bundle> list) {
                for (SubscriptionCallbacks callback : callbacks) {
                    callback.onChildrenLoaded(parentId, list);
                }
            }
        };

        mCallbacks.onLoadChildren(parentId, result);
        if (!result.isDone()) {
            throw new IllegalStateException("You must either call sendResult() or detach() " +
                    "before returning!");
        }
    }

    private void loadResultsForClient(final String parentId,
            final SubscriptionCallbacks callbacks) {
        final CarMenu result = new CarMenu(mActivity.getResources().getDisplayMetrics()) {
            @Override
            protected void onResultReady(List<Bundle> list) {
                callbacks.onChildrenLoaded(parentId, list);
            }
        };

        mCallbacks.onLoadChildren(parentId, result);
        if (!result.isDone()) {
            throw new IllegalStateException("You must either call sendResult() or detach() " +
                    "before returning!");
        }
    }
}
