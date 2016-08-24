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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.util.AttributeSet;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.data.Channel;
import com.android.tv.data.Program;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.DvrManager;
import com.android.tv.dvr.ScheduledRecording;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * A view to render an item of TV options.
 */
public class RecordCardView extends SimpleCardView implements
        DvrDataManager.ScheduledRecordingListener {
    private static final String TAG = MenuView.TAG;
    private static final boolean DEBUG = MenuView.DEBUG;
    private static final long MIN_PROGRAM_RECORD_DURATION = TimeUnit.MINUTES.toMillis(5);

    private ImageView mIconView;
    private TextView mLabelView;
    private Channel mCurrentChannel;
    private final DvrManager mDvrManager;
    private final DvrDataManager mDvrDataManager;
    private boolean mIsRecording;
    private ScheduledRecording mCurrentRecording;

    public RecordCardView(Context context) {
        this(context, null);
    }

    public RecordCardView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public RecordCardView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mDvrManager = TvApplication.getSingletons(context).getDvrManager();
        mDvrDataManager = TvApplication.getSingletons(context).getDvrDataManager();
    }

    @Override
    public void onBind(Channel channel, boolean selected) {
        super.onBind(channel, selected);
        mIconView = (ImageView) findViewById(R.id.record_icon);
        mLabelView = (TextView) findViewById(R.id.record_label);
        mCurrentChannel = channel;
        mCurrentRecording = null;
        for (ScheduledRecording recording : mDvrDataManager.getStartedRecordings()) {
            if (recording.getChannelId() == channel.getId()) {
                mIsRecording = true;
                mCurrentRecording = recording;
            }
        }
        mDvrDataManager.addScheduledRecordingListener(this);
        updateCardView();
    }

    @Override
    public void onRecycled() {
        super.onRecycled();
        mDvrDataManager.removeScheduledRecordingListener(this);
    }

    public boolean isRecording() {
        return mIsRecording;
    }

    public void startRecording() {
        showStartRecordingDialog();
    }

    public void stopRecording() {
        mDvrManager.stopRecording(mCurrentRecording);
    }

    private void updateCardView() {
        if (mIsRecording) {
            mIconView.setImageResource(R.drawable.ic_record_stop);
            mLabelView.setText(R.string.channels_item_record_stop);
        } else {
            mIconView.setImageResource(R.drawable.ic_record_start);
            mLabelView.setText(R.string.channels_item_record_start);
        }
    }

    private void showStartRecordingDialog() {
        final long endOfProgram = -1;

        final List<CharSequence> items = new ArrayList<>();
        final List<Long> durations = new ArrayList<>();
        Resources res = getResources();
        items.add(res.getString(R.string.recording_start_dialog_10_min_duration));
        durations.add(TimeUnit.MINUTES.toMillis(10));
        items.add(res.getString(R.string.recording_start_dialog_30_min_duration));
        durations.add(TimeUnit.MINUTES.toMillis(30));
        items.add(res.getString(R.string.recording_start_dialog_1_hour_duration));
        durations.add(TimeUnit.HOURS.toMillis(1));
        items.add(res.getString(R.string.recording_start_dialog_3_hours_duration));
        durations.add(TimeUnit.HOURS.toMillis(3));

        Program currenProgram = ((MainActivity) getContext()).getCurrentProgram(false);
        if (currenProgram != null) {
            long duration = currenProgram.getEndTimeUtcMillis() - System.currentTimeMillis();
            if (duration > MIN_PROGRAM_RECORD_DURATION) {
                items.add(res.getString(R.string.recording_start_dialog_till_end_of_program));
                durations.add(duration);
            }
        }

        DialogInterface.OnClickListener onClickListener
                = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(final DialogInterface dialog, int which) {
                long startTime = System.currentTimeMillis();
                long endTime = System.currentTimeMillis() + durations.get(which);
                mDvrManager.addSchedule(mCurrentChannel, startTime, endTime);
                dialog.dismiss();
            }
        };
        new AlertDialog.Builder(getContext())
                .setItems(items.toArray(new CharSequence[items.size()]), onClickListener)
                .create()
                .show();
    }

    @Override
    public void onScheduledRecordingAdded(ScheduledRecording recording) {
    }

    @Override
    public void onScheduledRecordingRemoved(ScheduledRecording recording) {
        if (recording.getChannelId() != mCurrentChannel.getId()) {
            return;
        }
        if (mIsRecording) {
            mIsRecording = false;
            mCurrentRecording = null;
            updateCardView();
        }
    }

    @Override
    public void onScheduledRecordingStatusChanged(ScheduledRecording recording) {
        if (recording.getChannelId() != mCurrentChannel.getId()) {
            return;
        }
        int state = recording.getState();
        if (state == ScheduledRecording.STATE_RECORDING_FAILED
                || state == ScheduledRecording.STATE_RECORDING_FINISHED) {
            mIsRecording = false;
            mCurrentRecording = null;
            updateCardView();
        } else if (state == ScheduledRecording.STATE_RECORDING_IN_PROGRESS) {
            mIsRecording = true;
            mCurrentRecording = recording;
            updateCardView();
        }
    }
}
