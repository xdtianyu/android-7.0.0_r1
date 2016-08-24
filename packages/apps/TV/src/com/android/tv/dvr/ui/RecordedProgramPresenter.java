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

package com.android.tv.dvr.ui;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Resources;
import android.media.tv.TvContract;
import android.support.v17.leanback.widget.Presenter;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;

import java.util.List;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.common.recording.RecordedProgram;
import com.android.tv.data.Channel;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.dvr.DvrManager;
import com.android.tv.ui.DialogUtils;
import com.android.tv.util.Utils;

/**
 * Presents a {@link RecordedProgram} in the {@link DvrBrowseFragment}.
 */
public class RecordedProgramPresenter extends Presenter {
    private final ChannelDataManager mChannelDataManager;

    public RecordedProgramPresenter(Context context) {
        mChannelDataManager = TvApplication.getSingletons(context).getChannelDataManager();
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent) {
        Context context = parent.getContext();
        RecordingCardView view = new RecordingCardView(context);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(ViewHolder viewHolder, Object o) {
        final RecordedProgram recording = (RecordedProgram) o;
        final RecordingCardView cardView = (RecordingCardView) viewHolder.view;
        final Context context = viewHolder.view.getContext();
        final Resources resources = context.getResources();

        Channel channel = mChannelDataManager.getChannel(recording.getChannelId());

        if (!TextUtils.isEmpty(recording.getTitle())) {
            cardView.setTitle(recording.getTitle());
        } else {
            cardView.setTitle(resources.getString(R.string.dvr_msg_program_title_unknown));
        }
        if (recording.getPosterArt() != null) {
            cardView.setImageUri(recording.getPosterArt());
        } else if (recording.getThumbnail() != null) {
            cardView.setImageUri(recording.getThumbnail());
        } else {
            if (channel != null) {
                cardView.setImageUri(TvContract.buildChannelLogoUri(channel.getId()).toString());
            }
        }
        cardView.setContent(Utils.getDurationString(context, recording.getStartTimeUtcMillis(),
                recording.getEndTimeUtcMillis(), true));
        //TODO: replace with a detail card
        viewHolder.view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                DialogUtils.showListDialog(v.getContext(),
                        new int[] { R.string.dvr_detail_play, R.string.dvr_detail_delete },
                        new Runnable[] {
                                new Runnable() {
                                    @Override
                                    public void run() {
                                        Intent intent = new Intent(context, MainActivity.class);
                                        intent.putExtra(Utils.EXTRA_KEY_RECORDING_URI,
                                                recording.getUri());
                                        context.startActivity(intent);
                                        ((Activity) context).finish();
                                    }
                                },
                                new Runnable() {
                                    @Override
                                    public void run() {
                                        DvrManager dvrManager = TvApplication
                                                .getSingletons(context).getDvrManager();
                                        dvrManager.removeRecordedProgram(recording);
                                    }
                                },
                        });
            }
        });

    }

    @Override
    public void onUnbindViewHolder(ViewHolder viewHolder) {
        final RecordingCardView cardView = (RecordingCardView) viewHolder.view;
        cardView.reset();
    }
}
