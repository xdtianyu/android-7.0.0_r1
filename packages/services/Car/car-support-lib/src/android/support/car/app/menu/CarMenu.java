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

import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.car.app.menu.compat.CarMenuConstantsComapt.MenuItemConstants;
import android.util.DisplayMetrics;
import android.widget.RemoteViews;

import java.util.ArrayList;
import java.util.List;

/**
 * CarMenu is used to pass back the menu items of a sublevel to the CarMenu subscriber.
 * Use the {@link Builder} to populate the contents of the sublevel.
 */
public class CarMenu {
    private boolean mDetachCalled;
    private boolean mSendResultCalled;
    private final DisplayMetrics mMetrics;

    public CarMenu(DisplayMetrics metrics) {
        mMetrics = metrics;
    }

    /**
     * Send the result back to the caller.
     */
    public void sendResult(List<Item> results) {
        if (mSendResultCalled) {
            throw new IllegalStateException("sendResult() called twice.");
        }
        mSendResultCalled = true;

        List<Bundle> resultBundle = new ArrayList<>();
        for (Item item : results) {
            ItemImpl impl = (ItemImpl) item;
            if (impl.mIcon != null) {
                impl.mBundle.putParcelable(MenuItemConstants.KEY_LEFTICON, snapshot(impl.mIcon));
            }
            if (impl.mRightIcon != null) {
                impl.mBundle.putParcelable(
                        MenuItemConstants.KEY_RIGHTICON, snapshot(impl.mRightIcon));
            }
            resultBundle.add(impl.mBundle);
        }
        onResultReady(resultBundle);
    }

    private Bitmap snapshot(Drawable drawable) {
        return Utils.snapshot(mMetrics, drawable);
    }

    /**
     * Detach this message from the current thread and allow the {@link #sendResult}
     * call to happen later. This stops blocking the current thread.
     */
    public void detach() {
        if (mDetachCalled) {
            throw new IllegalStateException("detach() called when detach() had already"
                    + " been called.");
        }
        if (mSendResultCalled) {
            throw new IllegalStateException("detach() called when sendResult() had already"
                    + " been called");
        }
        mDetachCalled = true;
    }

    /**
     * Returns whether results were actually sent.
     *
     * @return {@code true} if {@link #sendResult(java.util.List)} or {@link #detach()} has been called.
     * @hide
     */
    public boolean isDone() {
        return mDetachCalled || mSendResultCalled;
    }

    /**
     * Called when the result is sent, after assertions about not being called twice
     * have happened.
     * @hide
     */
    protected void onResultReady(List<Bundle> result) {
    }

    /**
     * An individual item in a menu.
     */
    public interface Item {
        /**
         * Gets the id of the menu item.
         *
         * @return The id of the menu item.
         */
        String getId();

        /**
         * Gets the title of the menu item.
         *
         * @return The title of the menu item. {@code null} if there is no title.
         */
        String getTitle();

        /**
         * Gets the text of the menu item.
         *
         * @return The text of the menu item. {@code null} if there is no text.
         */
        String getText();

        /**
         * Gets the integer constant for the widget.
         *
         * @return Either {@link MenuItemConstants.WidgetTypes#WIDGET_CHECKBOX} or -1,
         *         if no widget was set.
         */
        int getWidget();

        /**
         * Gets the widget state. The return value is only valid if a widget was set.
         *
         * @return {@code true} if the widget is enabled, {@code false} if the widget is disabled.
         */
        boolean getWidgetState();

        /**
         * Gets the flags set for this menu item.
         *
         * @return The flags set.
         */
        int getFlags();
    }

    /** @hide */
    static class ItemImpl implements Item {
        final Bundle mBundle;
        final Drawable mIcon;
        final Drawable mRightIcon;

        ItemImpl(Bundle bundle, Drawable icon, Drawable rightIcon) {
            mBundle = bundle;
            mIcon = icon;
            mRightIcon = rightIcon;
        }

        @Override
        public String getId() {
            return mBundle.getString(MenuItemConstants.KEY_ID);
        }

        @Override
        public String getTitle() {
            return mBundle.getString(MenuItemConstants.KEY_TITLE);
        }

        @Override
        public String getText() {
            return mBundle.getString(MenuItemConstants.KEY_TEXT);
        }

        @Override
        public int getWidget() {
            return mBundle.getInt(MenuItemConstants.KEY_WIDGET, -1);
        }

        @Override
        public boolean getWidgetState() {
            return mBundle.getBoolean(MenuItemConstants.KEY_WIDGET_STATE);
        }

        @Override
        public int getFlags() {
            return mBundle.getInt(MenuItemConstants.KEY_FLAGS);
        }
    }

    /**
     * Builder to build an {@link Item}. Calls to the builder can be chained.
     */
    public static class Builder {
        private final Bundle mBundle = new Bundle();
        // Drawable icons that will later be turned into Bitmaps and inserted into the Bundle
        private Drawable mIcon;
        private Drawable mRightIcon;

        /**
         * Construct a Builder with a specific id.
         *
         * @param id Unique id used to identify this menu item. If it is browsable, then it
         * will also be used to fetch this item's submenu.
         */
        public Builder(String id) {
            if (id == null) {
                throw new IllegalStateException("Cannot pass a null id to the Builder.");
            }
            mBundle.putString(MenuItemConstants.KEY_ID, id);
        }

        /**
         * Sets the title.
         *
         * @param title Title to set
         * @return This to chain calls
         */
        public Builder setTitle(String title) {
            mBundle.putString(MenuItemConstants.KEY_TITLE, title);
            return this;
        }

        /**
         * Sets the body text.
         *
         * @param text Text to set
         * @return This {@link Builder} to chain calls
         */
        public Builder setText(String text) {
            mBundle.putString(MenuItemConstants.KEY_TEXT, text);
            return this;
        }

        /**
         * Sets the icon.
         *
         * @param bitmap Icon to set
         * @return This {@link Builder} to chain calls
         */
        public Builder setIcon(Bitmap bitmap) {
            mBundle.putParcelable(MenuItemConstants.KEY_LEFTICON, bitmap);
            return this;
        }

        /**
         * Sets the icon.
         *
         * A snapshot of the {@link android.graphics.drawable.Drawable} is captured at the time the {@link Item} obtained
         * from this Builder is passed to {@link #sendResult(java.util.List)}. Any changes that are
         * made to the Drawable after that point will not affect what is displayed by the menu.
         *
         * @param drawable Icon to set
         * @return This {@link Builder} to chain calls
         */
        public Builder setIconFromSnapshot(Drawable drawable) {
            mIcon = drawable;
            return this;
        }

        /**
         * Sets the right icon.
         *
         * @param bitmap Icon to set
         * @return This {@link Builder} to chain calls
         */
        public Builder setRightIcon(Bitmap bitmap) {
            mBundle.putParcelable(MenuItemConstants.KEY_RIGHTICON, bitmap);
            return this;
        }

        /**
         * Sets the right icon.
         *
         * A snapshot of the {@link android.graphics.drawable.Drawable} is captured at the time the
         * {@link Item} obtained
         * from this Builder is passed to {@link #sendResult(java.util.List)}. Any changes that are
         * made to the Drawable after that point will not affect what is displayed by the menu.
         *
         * @param drawable Icon to set
         * @return This {@link Builder} to chain calls
         */
        public Builder setRightIconFromSnapshot(Drawable drawable) {
            mRightIcon = drawable;
            return this;
        }

        /**
         * The widget to set.
         * It can be anyone of the following: button, checkbox, toggle
         *
         * @param widget
         * @return This {@link Builder} to chain calls
         */
        public Builder setWidget(int widget) {
            mBundle.putInt(MenuItemConstants.KEY_WIDGET, widget);
            return this;
        }

        /**
         * If a widget is set, the state the widget is in.
         * This is only applicable if the widget is "checkbox" or "toggle"
         *
         * @param on If true, a checkbox is checked and a toggle is set to "on".
         *           If false, a checkbox will be unchecked and a toggle will be set to "off".
         * @return This {@link Builder} to chain calls
         */
        public Builder setWidgetState(boolean on) {
            mBundle.putBoolean(MenuItemConstants.KEY_WIDGET_STATE, on);
            return this;
        }

        /**
         * Indicates that this is an empty placeholder menu item.
         * Only title and icon will be available in this situation.
         *
         * @param isEmptyPlaceHolder If true, this CarMenu will be a placeholder item for no data
         *                           in menu list.
         * @return This {@link Builder} to chain calls
         */
        public Builder setIsEmptyPlaceHolder(boolean isEmptyPlaceHolder) {
            mBundle.putBoolean(MenuItemConstants.KEY_EMPTY_PLACEHOLDER, isEmptyPlaceHolder);
            return this;
        }

        /**
         * If the widget is {@link MenuItemConstants#WIDGET_TEXT_VIEW}, then this will allow setting
         * the right text.
         *
         * @param text The text to set
         * @return this {@link Builder} to chain calls
         */
        public Builder setRightText(String text) {
            mBundle.putString(MenuItemConstants.KEY_RIGHTTEXT, text);
            return this;
        }

        public Builder setRemoteViews(RemoteViews views) {
            mBundle.putParcelable(MenuItemConstants.KEY_REMOTEVIEWS, views);
            return this;
        }

        /**
         * Sets additional flags for this item.
         * {@link MenuItemConstants#FLAG_BROWSABLE} is the only one that can be currently set
         *
         * @param flags flags to set
         * @return This {@link Builder} to chain calls
         */
        public Builder setFlags(@MenuItemConstants.MenuItemFlags int flags) {
            mBundle.putInt(MenuItemConstants.KEY_FLAGS, flags);
            return this;
        }

        /**
         * Add this item to the list of items to be sent when {@link CarMenu#sendResult(java.util.List)}
         * is called
         */
        public Item build() {
            return new ItemImpl(mBundle, mIcon, mRightIcon);
        }
    }
}
