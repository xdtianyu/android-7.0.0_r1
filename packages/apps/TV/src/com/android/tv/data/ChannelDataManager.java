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

package com.android.tv.data;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.database.ContentObserver;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import android.media.tv.TvInputManager.TvInputCallback;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.VisibleForTesting;
import android.util.ArraySet;
import android.util.Log;
import android.util.MutableInt;

import com.android.tv.common.SharedPreferencesUtils;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.WeakHandler;
import com.android.tv.util.AsyncDbTask;
import com.android.tv.util.PermissionUtils;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.Utils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The class to manage channel data.
 * Basic features: reading channel list and each channel's current program, and updating
 * the values of {@link Channels#COLUMN_BROWSABLE}, {@link Channels#COLUMN_LOCKED}.
 * This class is not thread-safe and under an assumption that its public methods are called in
 * only the main thread.
 */
@MainThread
public class ChannelDataManager {
    private static final String TAG = "ChannelDataManager";
    private static final boolean DEBUG = false;

    private static final int MSG_UPDATE_CHANNELS = 1000;

    private final Context mContext;
    private final TvInputManagerHelper mInputManager;
    private boolean mStarted;
    private boolean mDbLoadFinished;
    private QueryAllChannelsTask mChannelsUpdateTask;
    private final List<Runnable> mPostRunnablesAfterChannelUpdate = new ArrayList<>();

    private final Set<Listener> mListeners = new ArraySet<>();
    private final Map<Long, ChannelWrapper> mChannelWrapperMap = new HashMap<>();
    private final Map<String, MutableInt> mChannelCountMap = new HashMap<>();
    private final Channel.DefaultComparator mChannelComparator;
    private final List<Channel> mChannels = new ArrayList<>();

    private final Handler mHandler;
    private final Set<Long> mBrowsableUpdateChannelIds = new HashSet<>();
    private final Set<Long> mLockedUpdateChannelIds = new HashSet<>();

    private final ContentResolver mContentResolver;
    private final ContentObserver mChannelObserver;
    private final boolean mStoreBrowsableInSharedPreferences;
    private final SharedPreferences mBrowsableSharedPreferences;

    private final TvInputCallback mTvInputCallback = new TvInputCallback() {
        @Override
        public void onInputAdded(String inputId) {
            boolean channelAdded = false;
            for (ChannelWrapper channel : mChannelWrapperMap.values()) {
                if (channel.mChannel.getInputId().equals(inputId)) {
                    channel.mInputRemoved = false;
                    addChannel(channel.mChannel);
                    channelAdded = true;
                }
            }
            if (channelAdded) {
                Collections.sort(mChannels, mChannelComparator);
                notifyChannelListUpdated();
            }
        }

        @Override
        public void onInputRemoved(String inputId) {
            boolean channelRemoved = false;
            ArrayList<ChannelWrapper> removedChannels = new ArrayList<>();
            for (ChannelWrapper channel : mChannelWrapperMap.values()) {
                if (channel.mChannel.getInputId().equals(inputId)) {
                    channel.mInputRemoved = true;
                    channelRemoved = true;
                    removedChannels.add(channel);
                }
            }
            if (channelRemoved) {
                clearChannels();
                for (ChannelWrapper channelWrapper : mChannelWrapperMap.values()) {
                    if (!channelWrapper.mInputRemoved) {
                        addChannel(channelWrapper.mChannel);
                    }
                }
                Collections.sort(mChannels, mChannelComparator);
                notifyChannelListUpdated();
                for (ChannelWrapper channel : removedChannels) {
                    channel.notifyChannelRemoved();
                }
            }
        }
    };

    public ChannelDataManager(Context context, TvInputManagerHelper inputManager) {
        this(context, inputManager, context.getContentResolver());
    }

    @VisibleForTesting
    ChannelDataManager(Context context, TvInputManagerHelper inputManager,
            ContentResolver contentResolver) {
        mContext = context;
        mInputManager = inputManager;
        mContentResolver = contentResolver;
        mChannelComparator = new Channel.DefaultComparator(context, inputManager);
        // Detect duplicate channels while sorting.
        mChannelComparator.setDetectDuplicatesEnabled(true);
        mHandler = new ChannelDataManagerHandler(this);
        mChannelObserver = new ContentObserver(mHandler) {
            @Override
            public void onChange(boolean selfChange) {
                if (!mHandler.hasMessages(MSG_UPDATE_CHANNELS)) {
                    mHandler.sendEmptyMessage(MSG_UPDATE_CHANNELS);
                }
            }
        };
        mStoreBrowsableInSharedPreferences = !PermissionUtils.hasAccessAllEpg(mContext);
        mBrowsableSharedPreferences = context.getSharedPreferences(
                SharedPreferencesUtils.SHARED_PREF_BROWSABLE, Context.MODE_PRIVATE);
    }

    @VisibleForTesting
    ContentObserver getContentObserver() {
        return mChannelObserver;
    }

    /**
     * Starts the manager. If data is ready, {@link Listener#onLoadFinished()} will be called.
     */
    public void start() {
        if (mStarted) {
            return;
        }
        mStarted = true;
        // Should be called directly instead of posting MSG_UPDATE_CHANNELS message to the handler.
        // If not, other DB tasks can be executed before channel loading.
        handleUpdateChannels();
        mContentResolver.registerContentObserver(TvContract.Channels.CONTENT_URI, true,
                mChannelObserver);
        mInputManager.addCallback(mTvInputCallback);
    }

    /**
     * Stops the manager. It clears manager states and runs pending DB operations. Added listeners
     * aren't automatically removed by this method.
     */
    @VisibleForTesting
    public void stop() {
        if (!mStarted) {
            return;
        }
        mStarted = false;
        mDbLoadFinished = false;

        ChannelLogoFetcher.stopFetchingChannelLogos();
        mInputManager.removeCallback(mTvInputCallback);
        mContentResolver.unregisterContentObserver(mChannelObserver);
        mHandler.removeCallbacksAndMessages(null);

        mChannelWrapperMap.clear();
        clearChannels();
        mPostRunnablesAfterChannelUpdate.clear();
        if (mChannelsUpdateTask != null) {
            mChannelsUpdateTask.cancel(true);
            mChannelsUpdateTask = null;
        }
        applyUpdatedValuesToDb();
    }

    /**
     * Adds a {@link Listener}.
     */
    public void addListener(Listener listener) {
        if (DEBUG) Log.d(TAG, "addListener " + listener);
        SoftPreconditions.checkNotNull(listener);
        if (listener != null) {
            mListeners.add(listener);
        }
    }

    /**
     * Removes a {@link Listener}.
     */
    public void removeListener(Listener listener) {
        if (DEBUG) Log.d(TAG, "removeListener " + listener);
        SoftPreconditions.checkNotNull(listener);
        if (listener != null) {
            mListeners.remove(listener);
        }
    }

    /**
     * Adds a {@link ChannelListener} for a specific channel with the channel ID {@code channelId}.
     */
    public void addChannelListener(Long channelId, ChannelListener listener) {
        ChannelWrapper channelWrapper = mChannelWrapperMap.get(channelId);
        if (channelWrapper == null) {
            return;
        }
        channelWrapper.addListener(listener);
    }

    /**
     * Removes a {@link ChannelListener} for a specific channel with the channel ID
     * {@code channelId}.
     */
    public void removeChannelListener(Long channelId, ChannelListener listener) {
        ChannelWrapper channelWrapper = mChannelWrapperMap.get(channelId);
        if (channelWrapper == null) {
            return;
        }
        channelWrapper.removeListener(listener);
    }

    /**
     * Checks whether data is ready.
     */
    public boolean isDbLoadFinished() {
        return mDbLoadFinished;
    }

    /**
     * Returns the number of channels.
     */
    public int getChannelCount() {
        return mChannels.size();
    }

    /**
     * Returns a list of channels.
     */
    public List<Channel> getChannelList() {
        return Collections.unmodifiableList(mChannels);
    }

    /**
     * Returns a list of browsable channels.
     */
    public List<Channel> getBrowsableChannelList() {
        List<Channel> channels = new ArrayList<>();
        for (Channel channel : mChannels) {
            if (channel.isBrowsable()) {
                channels.add(channel);
            }
        }
        return channels;
    }

    /**
     * Returns the total channel count for a given input.
     *
     * @param inputId The ID of the input.
     */
    public int getChannelCountForInput(String inputId) {
        MutableInt count = mChannelCountMap.get(inputId);
        return count == null ? 0 : count.value;
    }

    /**
     * Returns true if and only if there exists at least one channel and all channels are hidden.
     */
    public boolean areAllChannelsHidden() {
        if (mChannels.isEmpty()) {
            return false;
        }
        for (Channel channel : mChannels) {
            if (channel.isBrowsable()) {
                return false;
            }
        }
        return true;
    }

    /**
     * Gets the channel with the channel ID {@code channelId}.
     */
    public Channel getChannel(Long channelId) {
        ChannelWrapper channelWrapper = mChannelWrapperMap.get(channelId);
        if (channelWrapper == null || channelWrapper.mInputRemoved) {
            return null;
        }
        return channelWrapper.mChannel;
    }

    /**
     * The value change will be applied to DB when applyPendingDbOperation is called.
     */
    public void updateBrowsable(Long channelId, boolean browsable) {
        updateBrowsable(channelId, browsable, false);
    }

    /**
     * The value change will be applied to DB when applyPendingDbOperation is called.
     *
     * @param skipNotifyChannelBrowsableChanged If it's true, {@link Listener
     *        #onChannelBrowsableChanged()} is not called, when this method is called.
     *        {@link #notifyChannelBrowsableChanged} should be directly called, once browsable
     *        update is completed.
     */
    public void updateBrowsable(Long channelId, boolean browsable,
            boolean skipNotifyChannelBrowsableChanged) {
        ChannelWrapper channelWrapper = mChannelWrapperMap.get(channelId);
        if (channelWrapper == null) {
            return;
        }
        if (channelWrapper.mChannel.isBrowsable() != browsable) {
            channelWrapper.mChannel.setBrowsable(browsable);
            if (browsable == channelWrapper.mBrowsableInDb) {
                mBrowsableUpdateChannelIds.remove(channelWrapper.mChannel.getId());
            } else {
                mBrowsableUpdateChannelIds.add(channelWrapper.mChannel.getId());
            }
            channelWrapper.notifyChannelUpdated();
            // When updateBrowsable is called multiple times in a method, we don't need to
            // notify Listener.onChannelBrowsableChanged multiple times but only once. So
            // we send a message instead of directly calling onChannelBrowsableChanged.
            if (!skipNotifyChannelBrowsableChanged) {
                notifyChannelBrowsableChanged();
            }
        }
    }

    public void notifyChannelBrowsableChanged() {
        // Copy the original collection to allow the callee to modify the listeners.
        for (Listener l : mListeners.toArray(new Listener[mListeners.size()])) {
            l.onChannelBrowsableChanged();
        }
    }

    private void notifyChannelListUpdated() {
        // Copy the original collection to allow the callee to modify the listeners.
        for (Listener l : mListeners.toArray(new Listener[mListeners.size()])) {
            l.onChannelListUpdated();
        }
    }

    private void notifyLoadFinished() {
        // Copy the original collection to allow the callee to modify the listeners.
        for (Listener l : mListeners.toArray(new Listener[mListeners.size()])) {
            l.onLoadFinished();
        }
    }

    /**
     * Updates channels from DB. Once the update is done, {@code postRunnable} will
     * be called.
     */
    public void updateChannels(Runnable postRunnable) {
        if (mChannelsUpdateTask != null) {
            mChannelsUpdateTask.cancel(true);
            mChannelsUpdateTask = null;
        }
        mPostRunnablesAfterChannelUpdate.add(postRunnable);
        if (!mHandler.hasMessages(MSG_UPDATE_CHANNELS)) {
            mHandler.sendEmptyMessage(MSG_UPDATE_CHANNELS);
        }
    }

    /**
     * The value change will be applied to DB when applyPendingDbOperation is called.
     */
    public void updateLocked(Long channelId, boolean locked) {
        ChannelWrapper channelWrapper = mChannelWrapperMap.get(channelId);
        if (channelWrapper == null) {
            return;
        }
        if (channelWrapper.mChannel.isLocked() != locked) {
            channelWrapper.mChannel.setLocked(locked);
            if (locked == channelWrapper.mLockedInDb) {
                mLockedUpdateChannelIds.remove(channelWrapper.mChannel.getId());
            } else {
                mLockedUpdateChannelIds.add(channelWrapper.mChannel.getId());
            }
            channelWrapper.notifyChannelUpdated();
        }
    }

    /**
     * Applies the changed values by {@link #updateBrowsable} and {@link #updateLocked}
     * to DB.
     */
    public void applyUpdatedValuesToDb() {
        ArrayList<Long> browsableIds = new ArrayList<>();
        ArrayList<Long> unbrowsableIds = new ArrayList<>();
        for (Long id : mBrowsableUpdateChannelIds) {
            ChannelWrapper channelWrapper = mChannelWrapperMap.get(id);
            if (channelWrapper == null) {
                continue;
            }
            if (channelWrapper.mChannel.isBrowsable()) {
                browsableIds.add(id);
            } else {
                unbrowsableIds.add(id);
            }
            channelWrapper.mBrowsableInDb = channelWrapper.mChannel.isBrowsable();
        }
        String column = TvContract.Channels.COLUMN_BROWSABLE;
        if (mStoreBrowsableInSharedPreferences) {
            Editor editor = mBrowsableSharedPreferences.edit();
            for (Long id : browsableIds) {
                editor.putBoolean(getBrowsableKey(getChannel(id)), true);
            }
            for (Long id : unbrowsableIds) {
                editor.putBoolean(getBrowsableKey(getChannel(id)), false);
            }
            editor.apply();
        } else {
            if (browsableIds.size() != 0) {
                updateOneColumnValue(column, 1, browsableIds);
            }
            if (unbrowsableIds.size() != 0) {
                updateOneColumnValue(column, 0, unbrowsableIds);
            }
        }
        mBrowsableUpdateChannelIds.clear();

        ArrayList<Long> lockedIds = new ArrayList<>();
        ArrayList<Long> unlockedIds = new ArrayList<>();
        for (Long id : mLockedUpdateChannelIds) {
            ChannelWrapper channelWrapper = mChannelWrapperMap.get(id);
            if (channelWrapper == null) {
                continue;
            }
            if (channelWrapper.mChannel.isLocked()) {
                lockedIds.add(id);
            } else {
                unlockedIds.add(id);
            }
            channelWrapper.mLockedInDb = channelWrapper.mChannel.isLocked();
        }
        column = TvContract.Channels.COLUMN_LOCKED;
        if (lockedIds.size() != 0) {
            updateOneColumnValue(column, 1, lockedIds);
        }
        if (unlockedIds.size() != 0) {
            updateOneColumnValue(column, 0, unlockedIds);
        }
        mLockedUpdateChannelIds.clear();
        if (DEBUG) {
            Log.d(TAG, "applyUpdatedValuesToDb"
                    + "\n browsableIds size:" + browsableIds.size()
                    + "\n unbrowsableIds size:" + unbrowsableIds.size()
                    + "\n lockedIds size:" + lockedIds.size()
                    + "\n unlockedIds size:" + unlockedIds.size());
        }
    }

    private void addChannel(Channel channel) {
        mChannels.add(channel);
        String inputId = channel.getInputId();
        MutableInt count = mChannelCountMap.get(inputId);
        if (count == null) {
            mChannelCountMap.put(inputId, new MutableInt(1));
        } else {
            count.value++;
        }
    }

    private void clearChannels() {
        mChannels.clear();
        mChannelCountMap.clear();
    }

    private void handleUpdateChannels() {
        if (mChannelsUpdateTask != null) {
            mChannelsUpdateTask.cancel(true);
        }
        mChannelsUpdateTask = new QueryAllChannelsTask(mContentResolver);
        mChannelsUpdateTask.executeOnDbThread();
    }

    /**
     * Reloads channel data.
     */
    public void reload() {
        if (mDbLoadFinished && !mHandler.hasMessages(MSG_UPDATE_CHANNELS)) {
            mHandler.sendEmptyMessage(MSG_UPDATE_CHANNELS);
        }
    }

    public interface Listener {
        /**
         * Called when data load is finished.
         */
        void onLoadFinished();

        /**
         * Called when channels are added, deleted, or updated. But, when browsable is changed,
         * it won't be called. Instead, {@link #onChannelBrowsableChanged} will be called.
         */
        void onChannelListUpdated();

        /**
         * Called when browsable of channels are changed.
         */
        void onChannelBrowsableChanged();
    }

    public interface ChannelListener {
        /**
         * Called when the channel has been removed in DB.
         */
        void onChannelRemoved(Channel channel);

        /**
         * Called when values of the channel has been changed.
         */
        void onChannelUpdated(Channel channel);
    }

    private class ChannelWrapper {
        final Set<ChannelListener> mChannelListeners = new ArraySet<>();
        final Channel mChannel;
        boolean mBrowsableInDb;
        boolean mLockedInDb;
        boolean mInputRemoved;

        ChannelWrapper(Channel channel) {
            mChannel = channel;
            mBrowsableInDb = channel.isBrowsable();
            mLockedInDb = channel.isLocked();
            mInputRemoved = !mInputManager.hasTvInputInfo(channel.getInputId());
        }

        void addListener(ChannelListener listener) {
            mChannelListeners.add(listener);
        }

        void removeListener(ChannelListener listener) {
            mChannelListeners.remove(listener);
        }

        void notifyChannelUpdated() {
            for (ChannelListener l : mChannelListeners) {
                l.onChannelUpdated(mChannel);
            }
        }

        void notifyChannelRemoved() {
            for (ChannelListener l : mChannelListeners) {
                l.onChannelRemoved(mChannel);
            }
        }
    }

    private final class QueryAllChannelsTask extends AsyncDbTask.AsyncChannelQueryTask {

        public QueryAllChannelsTask(ContentResolver contentResolver) {
            super(contentResolver);
        }

        @Override
        protected void onPostExecute(List<Channel> channels) {
            mChannelsUpdateTask = null;
            if (channels == null) {
                if (DEBUG) Log.e(TAG, "onPostExecute with null channels");
                return;
            }
            Set<Long> removedChannelIds = new HashSet<>(mChannelWrapperMap.keySet());
            List<ChannelWrapper> removedChannelWrappers = new ArrayList<>();
            List<ChannelWrapper> updatedChannelWrappers = new ArrayList<>();

            boolean channelAdded = false;
            boolean channelUpdated = false;
            boolean channelRemoved = false;
            Map<String, ?> deletedBrowsableMap = null;
            if (mStoreBrowsableInSharedPreferences) {
                deletedBrowsableMap = new HashMap<>(mBrowsableSharedPreferences.getAll());
            }
            for (Channel channel : channels) {
                if (mStoreBrowsableInSharedPreferences) {
                    String browsableKey = getBrowsableKey(channel);
                    channel.setBrowsable(mBrowsableSharedPreferences.getBoolean(browsableKey,
                            false));
                    deletedBrowsableMap.remove(browsableKey);
                }
                long channelId = channel.getId();
                boolean newlyAdded = !removedChannelIds.remove(channelId);
                ChannelWrapper channelWrapper;
                if (newlyAdded) {
                    channelWrapper = new ChannelWrapper(channel);
                    mChannelWrapperMap.put(channel.getId(), channelWrapper);
                    if (!channelWrapper.mInputRemoved) {
                        channelAdded = true;
                    }
                } else {
                    channelWrapper = mChannelWrapperMap.get(channelId);
                    if (!channelWrapper.mChannel.hasSameReadOnlyInfo(channel)) {
                        // Channel data updated
                        Channel oldChannel = channelWrapper.mChannel;
                        // We assume that mBrowsable and mLocked are controlled by only TV app.
                        // The values for mBrowsable and mLocked are updated when
                        // {@link #applyUpdatedValuesToDb} is called. Therefore, the value
                        // between DB and ChannelDataManager could be different for a while.
                        // Therefore, we'll keep the values in ChannelDataManager.
                        channelWrapper.mChannel.copyFrom(channel);
                        channel.setBrowsable(oldChannel.isBrowsable());
                        channel.setLocked(oldChannel.isLocked());
                        if (!channelWrapper.mInputRemoved) {
                            channelUpdated = true;
                            updatedChannelWrappers.add(channelWrapper);
                        }
                    }
                }
            }
            if (mStoreBrowsableInSharedPreferences && !deletedBrowsableMap.isEmpty()
                    && PermissionUtils.hasReadTvListings(mContext)) {
                // If hasReadTvListings(mContext) is false, the given channel list would
                // empty. In this case, we skip the browsable data clean up process.
                Editor editor = mBrowsableSharedPreferences.edit();
                for (String key : deletedBrowsableMap.keySet()) {
                    if (DEBUG) Log.d(TAG, "remove key: " + key);
                    editor.remove(key);
                }
                editor.apply();
            }

            for (long id : removedChannelIds) {
                ChannelWrapper channelWrapper = mChannelWrapperMap.remove(id);
                if (!channelWrapper.mInputRemoved) {
                    channelRemoved = true;
                    removedChannelWrappers.add(channelWrapper);
                }
            }
            clearChannels();
            for (ChannelWrapper channelWrapper : mChannelWrapperMap.values()) {
                if (!channelWrapper.mInputRemoved) {
                    addChannel(channelWrapper.mChannel);
                }
            }
            Collections.sort(mChannels, mChannelComparator);

            if (!mDbLoadFinished) {
                mDbLoadFinished = true;
                notifyLoadFinished();
            } else if (channelAdded || channelUpdated || channelRemoved) {
                notifyChannelListUpdated();
            }
            for (ChannelWrapper channelWrapper : removedChannelWrappers) {
                channelWrapper.notifyChannelRemoved();
            }
            for (ChannelWrapper channelWrapper : updatedChannelWrappers) {
                channelWrapper.notifyChannelUpdated();
            }
            for (Runnable r : mPostRunnablesAfterChannelUpdate) {
                r.run();
            }
            mPostRunnablesAfterChannelUpdate.clear();
            ChannelLogoFetcher.startFetchingChannelLogos(mContext);
        }
    }

    /**
     * Updates a column {@code columnName} of DB table {@code uri} with the value
     * {@code columnValue}. The selective rows in the ID list {@code ids} will be updated.
     * The DB operations will run on {@link AsyncDbTask#getExecutor()}.
     */
    private void updateOneColumnValue(
            final String columnName, final int columnValue, final List<Long> ids) {
        if (!PermissionUtils.hasAccessAllEpg(mContext)) {
            // TODO: support this feature for non-system LC app. b/23939816
            return;
        }
        AsyncDbTask.execute(new Runnable() {
            @Override
            public void run() {
                String selection = Utils.buildSelectionForIds(Channels._ID, ids);
                ContentValues values = new ContentValues();
                values.put(columnName, columnValue);
                mContentResolver.update(TvContract.Channels.CONTENT_URI, values, selection, null);
            }
        });
    }

    private String getBrowsableKey(Channel channel) {
        return channel.getInputId() + "|" + channel.getId();
    }

    private static class ChannelDataManagerHandler extends WeakHandler<ChannelDataManager> {
        public ChannelDataManagerHandler(ChannelDataManager channelDataManager) {
            super(Looper.getMainLooper(), channelDataManager);
        }

        @Override
        public void handleMessage(Message msg, @NonNull ChannelDataManager channelDataManager) {
            if (msg.what == MSG_UPDATE_CHANNELS) {
                channelDataManager.handleUpdateChannels();
            }
        }
    }
}
