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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.media.tv.TvContract;
import android.support.annotation.Nullable;
import android.support.v17.leanback.widget.Presenter;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import com.android.tv.ApplicationSingletons;
import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.data.Channel;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.Program;
import com.android.tv.data.ProgramDataManager;
import com.android.tv.dvr.DvrManager;
import com.android.tv.dvr.ScheduledRecording;
import com.android.tv.util.Utils;

/**
 * Presents a {@link ScheduledRecording} in the {@link DvrBrowseFragment}.
 */
public class ScheduledRecordingPresenter extends Presenter {
    private final ChannelDataManager mChannelDataManager;

    private static final class ScheduledRecordingViewHolder extends ViewHolder {
        private ProgramDataManager.QueryProgramTask mQueryProgramTask;

        ScheduledRecordingViewHolder(RecordingCardView view) {
            super(view);
        }
    }

    public ScheduledRecordingPresenter(Context context) {
        ApplicationSingletons singletons = TvApplication.getSingletons(context);
        mChannelDataManager = singletons.getChannelDataManager();
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent) {
        Context context = parent.getContext();
        RecordingCardView view = new RecordingCardView(context);
        return new ScheduledRecordingViewHolder(view);
    }

    @Override
    public void onBindViewHolder(ViewHolder baseHolder, Object o) {
        ScheduledRecordingViewHolder viewHolder = (ScheduledRecordingViewHolder) baseHolder;
        final ScheduledRecording recording = (ScheduledRecording) o;
        final RecordingCardView cardView = (RecordingCardView) viewHolder.view;
        final Context context = viewHolder.view.getContext();

        long programId = recording.getProgramId();
        if (programId == ScheduledRecording.ID_NOT_SET) {
            setTitleAndImage(cardView, recording, null);
        } else {
            viewHolder.mQueryProgramTask = new ProgramDataManager.QueryProgramTask(
                    context.getContentResolver(), programId) {
                @Override
                protected void onPostExecute(Program program) {
                    super.onPostExecute(program);
                    setTitleAndImage(cardView, recording, program);
                }
            };
            viewHolder.mQueryProgramTask.executeOnDbThread();

        }
        cardView.setContent(Utils.getDurationString(context, recording.getStartTimeMs(),
                recording.getEndTimeMs(), true));
        //TODO: replace with a detail card
        View.OnClickListener clickListener = new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                switch (recording.getState()) {
                    case ScheduledRecording.STATE_RECORDING_NOT_STARTED: {
                        showScheduledRecordingDialog(v.getContext(), recording);
                        break;
                    }
                    case ScheduledRecording.STATE_RECORDING_IN_PROGRESS: {
                        showCurrentlyRecordingDialog(v.getContext(), recording);
                        break;
                    }
                }
            }
        };
        baseHolder.view.setOnClickListener(clickListener);
    }

    private void setTitleAndImage(RecordingCardView cardView, ScheduledRecording recording,
            @Nullable Program program) {
        if (program != null) {
            cardView.setTitle(program.getTitle());
            cardView.setImageUri(program.getPosterArtUri());
        } else {
            cardView.setTitle(
                    cardView.getResources().getString(R.string.dvr_msg_program_title_unknown));
            Channel channel = mChannelDataManager.getChannel(recording.getChannelId());
            if (channel != null) {
                cardView.setImageUri(TvContract.buildChannelLogoUri(channel.getId()).toString());
            }
        }
    }

    @Override
    public void onUnbindViewHolder(ViewHolder baseHolder) {
        ScheduledRecordingViewHolder viewHolder = (ScheduledRecordingViewHolder) baseHolder;
        final RecordingCardView cardView = (RecordingCardView) viewHolder.view;
        if (viewHolder.mQueryProgramTask != null) {
            viewHolder.mQueryProgramTask.cancel(true);
            viewHolder.mQueryProgramTask = null;
        }
        cardView.reset();
    }

    private void showScheduledRecordingDialog(final Context context,
            final ScheduledRecording recording) {
        DialogInterface.OnClickListener removeScheduleListener
                = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                // TODO(DVR) handle success/failure.
                DvrManager dvrManager = TvApplication.getSingletons(context)
                        .getDvrManager();
                dvrManager.removeScheduledRecording((ScheduledRecording) recording);
            }
        };
        new AlertDialog.Builder(context)
                .setMessage(R.string.epg_dvr_dialog_message_remove_recording_schedule)
                .setNegativeButton(android.R.string.no, null)
                .setPositiveButton(android.R.string.yes, removeScheduleListener)
                .show();
    }

    private void showCurrentlyRecordingDialog(final Context context,
            final ScheduledRecording recording) {
        DialogInterface.OnClickListener stopRecordingListener
                = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                DvrManager dvrManager = TvApplication.getSingletons(context)
                        .getDvrManager();
                dvrManager.stopRecording((ScheduledRecording) recording);
            }
        };
        new AlertDialog.Builder(context)
                .setMessage(R.string.epg_dvr_dialog_message_stop_recording)
                .setNegativeButton(android.R.string.no, null)
                .setPositiveButton(android.R.string.yes, stopRecordingListener)
                .show();
    }
}
