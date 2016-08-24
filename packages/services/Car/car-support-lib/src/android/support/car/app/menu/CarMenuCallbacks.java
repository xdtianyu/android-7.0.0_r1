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

package android.support.car.app.menu;

import android.graphics.drawable.Drawable;
import android.os.Bundle;

/**
 * Class that the CarMenu communicates with to fetch the contents of the CarMenu
 */
public abstract class CarMenuCallbacks {
    /**
     * Listens for calls to notifyChildrenChanged and onChildrenChanged
     * @hide
     */
    public interface OnChildrenChangedListener {
        /** Called when app wants to notify that contents of an entire menu have changed. */
        void onChildrenChanged(String parentId);
        /**
         * Called when app wants to notify that contents of a single item has changed.
         *
         * @param item Contents of {@link android.os.Bundle} are used to update the contents of the existing
         *             item.
         * @param leftIcon Drawable to convert to bitmap
         * @param rightIcon Drawable to convert to bitmap
         */
        void onChildChanged(String parentId, Bundle item, Drawable leftIcon, Drawable rightIcon);
    }

    private OnChildrenChangedListener mListener;

    /**
     * Called when the CarMenu wants to get the root
     *
     * @param hints Hints that the Drawer can use to modify behavior. It can be null.
     * @return The {@link RootMenu} which contains the root id and any hints
     */
    public abstract RootMenu onGetRoot(Bundle hints);

    /**
     * Called when the CarMenu subscribes to to a certain id
     *
     * @param parentId ID to subscribe to
     * @param result {@link CarMenu} that is used to communicate back the results
     */
    public abstract void onLoadChildren(String parentId,
            CarMenu result);

    /**
     * Notify the CarMenu that the menu defined by the id has changed. This will cause the CarMenu
     * to fetch the menu items.
     *
     * @param parentId The id which identifies the menu that changed
     */
    public void notifyChildrenChanged(String parentId) {
        if (mListener != null) {
            mListener.onChildrenChanged(parentId);
        }
    }

    /**
     * Register an OnChildrenChangedListener to detect when a menu has changed
     *
     * @param listener listener to register
     * @hide
     */
    public void registerOnChildrenChangedListener(OnChildrenChangedListener listener) {
        mListener = listener;
    }

    /**
     * Unregister an OnChildrenChangedListener to detect when a menu has changed.
     *
     * @param listener listener to unregister
     * @hide
     */
    public void unregisterOnChildrenChangedListener(OnChildrenChangedListener listener) {
        if (listener != mListener) {
            throw new IllegalStateException(
                    "Trying to unregister a listener that was not registered!");
        }
        mListener = null;
    }

    /**
     * Called when the CarMenu is opened
     */
    public void onCarMenuOpened() {}

    /**
     * Called when the CarMenu is closed
     */
    public void onCarMenuClosed() {}

    /**
     * Called when the CarMenu is opening
     */
    public void onCarMenuOpening() {}

    /**
     * Called when the CarMenu is closing
     */
    public void onCarMenuClosing() {}

    /**
     * Called when an item is clicked
     *
     * @param id Id of the item that is clicked
     */
    public void onItemClicked(String id) {}

    /**
     * Called when an item is long clicked
     *
     * @param id Id of the item that is long clicked
     *
     * @return Return true if handled, false if not. Returning false also means that the
     * onItemClicked handler will be called.
     */
    public boolean onItemLongClicked(String id) {
        return false;
    }

    /**
     * Called when the state of the CarMenu has changed.
     * TODO: Describe the state. This may be removed moving forward depending on if it is useful
     *
     * @param newState The new state of the CarMenu
     */
    public void onStateChanged(int newState) {}

    /**
     * Notify that an item has changed. Use a {@link CarMenu.Builder} to build the item and pu the
     * updated contents inside. Note that this cannot be used to change an item's layout, but to
     * modify existing contents.
     *
     * @param parentId parentId of the item.
     * @param item Updated contents of the item.
     */
    public void notifyChildChanged(String parentId, CarMenu.Item item) {
        if (mListener != null) {
            CarMenu.ItemImpl realItem = (CarMenu.ItemImpl) item;
            mListener.onChildChanged(parentId,
                    realItem.mBundle,
                    realItem.mIcon,
                    realItem.mRightIcon);
        }
    }
}
