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

package com.android.tv.menu;

import android.content.Context;
import android.content.Intent;
import android.media.tv.TvInputInfo;
import android.os.Build;
import android.support.v4.os.BuildCompat;
import android.view.View;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.analytics.Tracker;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.data.Channel;
import com.android.tv.recommendation.Recommender;
import com.android.tv.util.SetupUtils;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.Utils;

import java.util.ArrayList;
import java.util.List;

/**
 * An adapter of the Channels row.
 */
public class ChannelsRowAdapter extends ItemListRowView.ItemListAdapter<Channel> {
    // There are four special cards: guide, setup, dvr, record, applink.
    private static final int SIZE_OF_VIEW_TYPE = 5;

    private final Context mContext;
    private final Tracker mTracker;
    private final Recommender mRecommender;
    private final int mMaxCount;
    private final int mMinCount;
    private final boolean mDvrFeatureEnabled;
    private final int[] mViewType = new int[SIZE_OF_VIEW_TYPE];

    private final View.OnClickListener mGuideOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            mTracker.sendMenuClicked(R.string.channels_item_program_guide);
            getMainActivity().getOverlayManager().showProgramGuide();
        }
    };

    private final View.OnClickListener mSetupOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            mTracker.sendMenuClicked(R.string.channels_item_setup);
            getMainActivity().getOverlayManager().showSetupFragment();
        }
    };

    private final View.OnClickListener mDvrOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            Utils.showToastMessageForDeveloperFeature(view.getContext());
            mTracker.sendMenuClicked(R.string.channels_item_dvr);
            getMainActivity().getOverlayManager().showDvrManager();
        }
    };

    private final View.OnClickListener mRecordOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            Utils.showToastMessageForDeveloperFeature(view.getContext());
            RecordCardView v = ((RecordCardView) view);
            boolean isRecording = v.isRecording();
            mTracker.sendMenuClicked(isRecording ? R.string.channels_item_record_start
                    : R.string.channels_item_record_stop);
            if (!isRecording) {
                v.startRecording();
            } else {
                v.stopRecording();
            }
        }
    };

    private final View.OnClickListener mAppLinkOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            mTracker.sendMenuClicked(R.string.channels_item_app_link);
            Intent intent = ((AppLinkCardView) view).getIntent();
            if (intent != null) {
                getMainActivity().startActivitySafe(intent);
            }
        }
    };

    private final View.OnClickListener mChannelOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            // Always send the label "Channels" because the channel ID or name or number might be
            // sensitive.
            mTracker.sendMenuClicked(R.string.menu_title_channels);
            getMainActivity().tuneToChannel((Channel) view.getTag());
            getMainActivity().hideOverlaysForTune();
        }
    };

    public ChannelsRowAdapter(Context context, Recommender recommender,
            int minCount, int maxCount) {
        super(context);
        mTracker = TvApplication.getSingletons(context).getTracker();
        mContext = context;
        mRecommender = recommender;
        mMinCount = minCount;
        mMaxCount = maxCount;
        mDvrFeatureEnabled = CommonFeatures.DVR.isEnabled(mContext) && BuildCompat.isAtLeastN();
    }

    @Override
    public int getItemViewType(int position) {
        if (position >= SIZE_OF_VIEW_TYPE) {
            return R.layout.menu_card_channel;
        }
        return mViewType[position];
    }

    @Override
    protected int getLayoutResId(int viewType) {
        return viewType;
    }

    @Override
    public void onBindViewHolder(MyViewHolder viewHolder, int position) {
        super.onBindViewHolder(viewHolder, position);

        int viewType = getItemViewType(position);
        if (viewType == R.layout.menu_card_guide) {
            viewHolder.itemView.setOnClickListener(mGuideOnClickListener);
        } else if (viewType == R.layout.menu_card_setup) {
            viewHolder.itemView.setOnClickListener(mSetupOnClickListener);
        } else if (viewType == R.layout.menu_card_app_link) {
            viewHolder.itemView.setOnClickListener(mAppLinkOnClickListener);
        } else if (viewType == R.layout.menu_card_dvr) {
            viewHolder.itemView.setOnClickListener(mDvrOnClickListener);
        } else if (viewType == R.layout.menu_card_record) {
            viewHolder.itemView.setOnClickListener(mRecordOnClickListener);
        } else {
            viewHolder.itemView.setTag(getItemList().get(position));
            viewHolder.itemView.setOnClickListener(mChannelOnClickListener);
        }
    }

    @Override
    public void update() {
        List<Channel> channelList = new ArrayList<>();
        Channel dummyChannel = new Channel.Builder().build();
        // For guide item
        channelList.add(dummyChannel);
        // For setup item
        TvInputManagerHelper inputManager = TvApplication.getSingletons(mContext)
                .getTvInputManagerHelper();
        boolean showSetupCard = SetupUtils.getInstance(mContext).hasNewInput(inputManager);
        Channel currentChannel = ((MainActivity) mContext).getCurrentChannel();
        boolean showAppLinkCard = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                && currentChannel != null
                && currentChannel.getAppLinkType(mContext) != Channel.APP_LINK_TYPE_NONE;
        boolean showDvrCard = false;
        boolean showRecordCard = false;
        if (mDvrFeatureEnabled) {
            for (TvInputInfo info : inputManager.getTvInputInfos(true, true)) {
                if (info.canRecord()) {
                    showDvrCard = true;
                    break;
                }
            }
            if (currentChannel != null && currentChannel.getInputId() != null) {
                TvInputInfo inputInfo = inputManager.getTvInputInfo(currentChannel.getInputId());
                if ((inputInfo.canRecord() && inputInfo.getTunerCount() > 1)) {
                    showRecordCard = true;
                }
            }
        }

        mViewType[0] = R.layout.menu_card_guide;
        int index = 1;
        if (showSetupCard) {
            channelList.add(dummyChannel);
            mViewType[index++] = R.layout.menu_card_setup;
        }
        if (showDvrCard) {
            channelList.add(dummyChannel);
            mViewType[index++] = R.layout.menu_card_dvr;
        }
        if (showRecordCard) {
            channelList.add(currentChannel);
            mViewType[index++] = R.layout.menu_card_record;
        }
        if (showAppLinkCard) {
            channelList.add(currentChannel);
            mViewType[index++] = R.layout.menu_card_app_link;
        }
        for ( ; index < mViewType.length; ++index) {
            mViewType[index] = R.layout.menu_card_channel;
        }
        channelList.addAll(getRecentChannels());
        setItemList(channelList);
    }

    private List<Channel> getRecentChannels() {
        List<Channel> channelList = new ArrayList<>();
        for (Channel channel : mRecommender.recommendChannels(mMaxCount)) {
            if (channel.isBrowsable()) {
                channelList.add(channel);
            }
        }
        int count = channelList.size();
        // If the number of recommended channels is not enough, add more from the recent channel
        // list.
        if (count < mMinCount && mContext instanceof MainActivity) {
            for (long channelId : ((MainActivity) mContext).getRecentChannels()) {
                Channel channel = mRecommender.getChannel(channelId);
                if (channel == null || channelList.contains(channel)
                        || !channel.isBrowsable()) {
                   continue;
                }
                channelList.add(channel);
                if (++count >= mMinCount) {
                    break;
                }
            }
        }
        return channelList;
    }
}
