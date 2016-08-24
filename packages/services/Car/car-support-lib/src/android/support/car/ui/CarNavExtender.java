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
package android.support.car.ui;

import android.app.Notification;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.NotificationCompat;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Helper class to add navigation extensions to notifications for use in Android Auto.
 * <p>
 * To create a notification with navigation extensions:
 * <ol>
 *   <li>Create a {@link android.app.Notification.Builder}, setting any desired
 *   properties.
 *   <li>Create a {@link CarNavExtender}.
 *   <li>Set car-specific properties using the
 *   {@code add} and {@code set} methods of {@link CarNavExtender}.
 *   <li>Call {@link android.app.Notification.Builder#extend} to apply the extensions to a
 *   notification.
 *   <li>Post the notification to the notification system with the
 *   {@code NotificationManager.notify(...)} methods.
 * </ol>
 *
 * <pre class="prettyprint">
 * Notification notif = new Notification.Builder(mContext)
 *         .setContentTitle("Turn right in 2.0 miles on to US 101-N")
 *         .setContentText("43 mins (32 mi) to Home")
 *         .setSmallIcon(R.drawable.ic_nav)
 *         .extend(new CarNavExtender()
 *                 .setContentTitle("US 101-N")
 *                 .setContentText("400 ft")
 *                 .setSubText("43 mins to Home")
 *         .build();
 * NotificationManager notificationManger =
 *         (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
 * notificationManger.notify(0, notif);</pre>
 *
 * <p>CarNavExtender fields can be accessed on an existing notification by using the
 * {@code CarNavExtender(Notification)} constructor,
 * and then using the {@code get} methods to access values.
 */
public class CarNavExtender implements NotificationCompat.Extender {
    /** This value must remain unchanged for compatibility. **/
    private static final String EXTRA_CAR_EXTENDER = "android.car.EXTENSIONS";
    private static final String EXTRA_IS_EXTENDED =
            "com.google.android.gms.car.support.CarNavExtender.EXTENDED";
    private static final String EXTRA_CONTENT_ID = "content_id";
    private static final String EXTRA_TYPE = "type";
    private static final String EXTRA_SUB_TEXT = "sub_text";
    private static final String EXTRA_ACTION_ICON = "action_icon";
    /** This value must remain unchanged for compatibility. **/
    private static final String EXTRA_CONTENT_INTENT = "content_intent";
    /** This value must remain unchanged for compatibility. **/
    private static final String EXTRA_COLOR = "app_color";
    private static final String EXTRA_NIGHT_COLOR = "app_night_color";
    /** This value must remain unchanged for compatibility. **/
    private static final String EXTRA_STREAM_VISIBILITY = "stream_visibility";
    /** This value must remain unchanged for compatibility. **/
    private static final String EXTRA_HEADS_UP_VISIBILITY = "heads_up_visibility";
    private static final String EXTRA_IGNORE_IN_STREAM = "ignore_in_stream";

    @IntDef({TYPE_HERO, TYPE_NORMAL})
    @Retention(RetentionPolicy.SOURCE)
    private @interface Type {}
    public static final int TYPE_HERO = 0;
    public static final int TYPE_NORMAL = 1;

    private boolean mIsExtended;
    /** <code>null</code> if not explicitly set. **/
    private Long mContentId;
    private int mType = TYPE_NORMAL;
    private CharSequence mContentTitle;
    private CharSequence mContentText;
    private CharSequence mSubText;
    private Bitmap mLargeIcon;
    private @DrawableRes int mActionIcon;
    private Intent mContentIntent;
    private int mColor = Notification.COLOR_DEFAULT;
    private int mNightColor = Notification.COLOR_DEFAULT;
    private boolean mShowInStream = true;
    private boolean mShowAsHeadsUp;
    private boolean mIgnoreInStream;

    /**
     * Create a new CarNavExtender to extend a new notification.
     */
    public CarNavExtender() {
    }

    /**
     * Reconstruct a CarNavExtender from an existing notification. Can be used to retrieve values.
     *
     * @param notification The notification to retrieve the values from.
     */
    public CarNavExtender(@NonNull Notification notification) {
        Bundle extras = NotificationCompat.getExtras(notification);
        if (extras == null) {
            return;
        }
        Bundle b = extras.getBundle(EXTRA_CAR_EXTENDER);
        if (b == null) {
            return;
        }

        mIsExtended = b.getBoolean(EXTRA_IS_EXTENDED);
        mContentId = (Long) b.getSerializable(EXTRA_CONTENT_ID);
        // The ternary guarantees that we return either TYPE_HERO or TYPE_NORMAL.
        mType = (b.getInt(EXTRA_TYPE, TYPE_NORMAL) == TYPE_HERO) ? TYPE_HERO : TYPE_NORMAL;
        mContentTitle = b.getCharSequence(Notification.EXTRA_TITLE);
        mContentText = b.getCharSequence(Notification.EXTRA_TEXT);
        mSubText = b.getCharSequence(EXTRA_SUB_TEXT);
        mLargeIcon = b.getParcelable(Notification.EXTRA_LARGE_ICON);
        mActionIcon = b.getInt(EXTRA_ACTION_ICON);
        mContentIntent = b.getParcelable(EXTRA_CONTENT_INTENT);
        mColor = b.getInt(EXTRA_COLOR, Notification.COLOR_DEFAULT);
        mNightColor = b.getInt(EXTRA_NIGHT_COLOR, Notification.COLOR_DEFAULT);
        mShowInStream = b.getBoolean(EXTRA_STREAM_VISIBILITY, true);
        mShowAsHeadsUp = b.getBoolean(EXTRA_HEADS_UP_VISIBILITY);
        mIgnoreInStream = b.getBoolean(EXTRA_IGNORE_IN_STREAM);
    }

    @Override
    public NotificationCompat.Builder extend(NotificationCompat.Builder builder) {
        Bundle b = new Bundle();
        b.putBoolean(EXTRA_IS_EXTENDED, true);
        b.putSerializable(EXTRA_CONTENT_ID, mContentId);
        b.putInt(EXTRA_TYPE, mType);
        b.putCharSequence(Notification.EXTRA_TITLE, mContentTitle);
        b.putCharSequence(Notification.EXTRA_TEXT, mContentText);
        b.putCharSequence(EXTRA_SUB_TEXT, mSubText);
        b.putParcelable(Notification.EXTRA_LARGE_ICON, mLargeIcon);
        b.putInt(EXTRA_ACTION_ICON, mActionIcon);
        b.putParcelable(EXTRA_CONTENT_INTENT, mContentIntent);
        b.putInt(EXTRA_COLOR, mColor);
        b.putInt(EXTRA_NIGHT_COLOR, mNightColor);
        b.putBoolean(EXTRA_STREAM_VISIBILITY, mShowInStream);
        b.putBoolean(EXTRA_HEADS_UP_VISIBILITY, mShowAsHeadsUp);
        b.putBoolean(EXTRA_IGNORE_IN_STREAM, mIgnoreInStream);
        builder.getExtras().putBundle(EXTRA_CAR_EXTENDER, b);
        return builder;
    }

    /**
     * @return <code>true</code> if the notification was extended with {@link CarNavExtender}.
     */
    public boolean isExtended() {
        return mIsExtended;
    }

    /**
     * Static version of {@link #isExtended()}.
     */
    public static boolean isExtended(Notification notification) {
        Bundle extras = NotificationCompat.getExtras(notification);
        if (extras == null) {
            return false;
        }

        extras = extras.getBundle(EXTRA_CAR_EXTENDER);
        return extras != null && extras.getBoolean(EXTRA_IS_EXTENDED);
    }

    /**
     * Sets an id for the content of this notification. If the content id matches an existing
     * notification, any timers that control ranking and heads up notification will remain
     * unchanged. However, if it differs from the previous notification with the same id then
     * this notification will be treated as a new notification with respect to heads up
     * notifications and ranking.
     *
     * If no content id is specified, it will be treated like a new content id.
     *
     * A content id will only be compared to the existing notification, not the entire history of
     * content ids.
     *
     * @param contentId The content id that represents this notification.
     * @return This object for method chaining.
     */
    public CarNavExtender setContentId(long contentId) {
        mContentId = contentId;
        return this;
    }

    /**
     * @return The content id for this notification or <code>null</code> if it was not specified.
     */
    @Nullable
    public Long getContentId() {
        return mContentId;
    }

    /**
     * @param type The type of notification that this will be displayed as in the Android Auto.
     * @return This object for method chaining.
     *
     * @see #TYPE_NORMAL
     * @see #TYPE_HERO
     */
    public CarNavExtender setType(@Type int type) {
        mType = type;
        return this;
    }

    /**
     * @return The type of notification
     *
     * @see #TYPE_NORMAL
     * @see #TYPE_HERO
     */
    @Type
    public int getType() {
        return mType;
    }

    /**
     * @return The type without having to construct an entire {@link CarNavExtender} object.
     */
    @Type
    public static int getType(Notification notification) {
        Bundle extras = NotificationCompat.getExtras(notification);
        if (extras == null) {
            return TYPE_NORMAL;
        }
        Bundle b = extras.getBundle(EXTRA_CAR_EXTENDER);
        if (b == null) {
            return TYPE_NORMAL;
        }

        // The ternary guarantees that we return either TYPE_HERO or TYPE_NORMAL.
        return (b.getInt(EXTRA_TYPE, TYPE_NORMAL) == TYPE_HERO) ? TYPE_HERO : TYPE_NORMAL;
    }

    /**
     * @param contentTitle Override for the notification's content title.
     * @return This object for method chaining.
     */
    public CarNavExtender setContentTitle(CharSequence contentTitle) {
        mContentTitle = contentTitle;
        return this;
    }

    /**
     * @return The content title for the notification if one was explicitly set with
     *         {@link #setContentTitle(CharSequence)}.
     */
    public CharSequence getContentTitle() {
        return mContentTitle;
    }

    /**
     * @param contentText Override for the notification's content text. If set to an empty string,
     *                    it will be treated as if there is no context text by the UI.
     * @return This object for method chaining.
     */
    public CarNavExtender setContentText(CharSequence contentText) {
        mContentText = contentText;
        return this;
    }

    /**
     * @return The content text for the notification if one was explicitly set with
     *         {@link #setContentText(CharSequence)}.
     */
    @Nullable
    public CharSequence getContentText() {
        return mContentText;
    }

    /**
     * @param subText A third text field that will be displayed on hero cards.
     * @return This object for method chaining.
     */
    public CarNavExtender setSubText(CharSequence subText) {
        mSubText = subText;
        return this;
    }

    /**
     * @return The secondary content text for the notification or null if it wasn't set.
     */
    @Nullable
    public CharSequence getSubText() {
        return mSubText;
    }

    /**
     * @param largeIcon Override for the notification's large icon.
     * @return This object for method chaining.
     */
    public CarNavExtender setLargeIcon(Bitmap largeIcon) {
        mLargeIcon = largeIcon;
        return this;
    }

    /**
     * @return The large icon for the notification if one was explicitly set with
     *         {@link #setLargeIcon(android.graphics.Bitmap)}.
     */
    public Bitmap getLargeIcon() {
        return mLargeIcon;
    }

    /**
     * By default, Android Auto will show a navigation chevron on cards. However, a separate icon
     * can be set here to override it.
     *
     * @param actionIcon The action icon resource id from your package that you would like to
     *                   use instead of the navigation chevron.
     * @return This object for method chaining.
     */
    public CarNavExtender setActionIcon(@DrawableRes int actionIcon) {
        mActionIcon = actionIcon;
        return this;
    }

    /**
     * @return The overridden action icon or 0 if one wasn't set.
     */
    @DrawableRes
    public int getActionIcon() {
        return mActionIcon;
    }

    /**
     * @param contentIntent The content intent that will be sent using
     *                      {@link com.google.android.gms.car.CarActivity#startCarProjectionActivity(android.content.Intent)}
     *                      It is STRONGLY suggested that you set a content intent or else the
     *                      notification will have no action when tapped.
     * @return This object for method chaining.
     */
    public CarNavExtender setContentIntent(Intent contentIntent) {
        mContentIntent = contentIntent;
        return this;
    }

    /**
     * @return The content intent that will be sent using
     *         {@link com.google.android.gms.car.CarActivity#startCarProjectionActivity(android.content.Intent)}
     */
    public Intent getContentIntent() {
        return mContentIntent;
    }

    /**
     * @param color Override for the notification color.
     * @return This object for method chaining.
     *
     * @see android.app.Notification.Builder#setColor(int)
     */
    public CarNavExtender setColor(int color) {
        mColor = color;
        return this;
    }

    /**
     * @return The color specified by the notification or {@link android.app.Notification#COLOR_DEFAULT} if
     *         one wasn't explicitly set with {@link #setColor(int)}.
     */
    public int getColor() {
        return mColor;
    }

    /**
     * @param nightColor Override for the notification color at night.
     * @return This object for method chaining.
     *
     * @see android.app.Notification.Builder#setColor(int)
     */
    public CarNavExtender setNightColor(int nightColor) {
        mNightColor = nightColor;
        return this;
    }

    /**
     * @return The night color specified by the notification or {@link android.app.Notification#COLOR_DEFAULT}
     *         if one wasn't explicitly set with {@link #setNightColor(int)}.
     */
    public int getNightColor() {
        return mNightColor;
    }

    /**
     * @param show Whether or not to show the notification in the stream.
     * @return This object for method chaining.
     */
    public CarNavExtender setShowInStream(boolean show) {
        mShowInStream = show;
        return this;
    }

    /**
     * @return Whether or not to show the notification in the stream.
     */
    public boolean getShowInStream() {
        return mShowInStream;
    }

    /**
     * @param show Whether or not to show the notification as a heads up notification.
     * @return This object for method chaining.
     */
    public CarNavExtender setShowAsHeadsUp(boolean show) {
        mShowAsHeadsUp = show;
        return this;
    }

    /**
     * @return Whether or not to show the notification as a heads up notification.
     */
    public boolean getShowAsHeadsUp() {
        return mShowAsHeadsUp;
    }

    /**
     * @param ignore Whether or not this notification can be shown as a heads-up notification if
     *               the user is already on the stream.
     * @return This object for method chaining.
     */
    public CarNavExtender setIgnoreInStream(boolean ignore) {
        mIgnoreInStream = ignore;
        return this;
    }

    /**
     * @return Whether or not the stream item can be shown as a heads-up notification if ther user
     *         already is on the stream.
     */
    public boolean getIgnoreInStream() {
        return mIgnoreInStream;
    }
}