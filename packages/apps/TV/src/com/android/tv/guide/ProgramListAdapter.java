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

package com.android.tv.guide;

import android.content.res.Resources;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.tv.R;
import com.android.tv.data.Channel;
import com.android.tv.guide.ProgramManager.TableEntriesUpdatedListener;
import com.android.tv.guide.ProgramManager.TableEntry;

/**
 * Adapts a program list for a specific channel from {@link ProgramManager} to a row of the program
 * guide table.
 */
public class ProgramListAdapter extends RecyclerView.Adapter<ProgramListAdapter.ProgramViewHolder>
        implements TableEntriesUpdatedListener {
    private static final String TAG = "ProgramListAdapter";
    private static final boolean DEBUG = false;

    private final ProgramManager mProgramManager;
    private final int mChannelIndex;
    private final String mNoInfoProgramTitle;
    private final String mBlockedProgramTitle;

    private long mChannelId;

    public ProgramListAdapter(Resources res, ProgramManager programManager, int channelIndex) {
        setHasStableIds(true);
        mProgramManager = programManager;
        mChannelIndex = channelIndex;
        mNoInfoProgramTitle = res.getString(R.string.program_title_for_no_information);
        mBlockedProgramTitle = res.getString(R.string.program_title_for_blocked_channel);
        onTableEntriesUpdated();
    }

    @Override
    public void onTableEntriesUpdated() {
        Channel channel = mProgramManager.getChannel(mChannelIndex);
        if (channel == null) {
            // The channel has just been removed. Do nothing.
        } else {
            mChannelId = channel.getId();
            if (DEBUG) Log.d(TAG, "update for channel " + mChannelId);
            notifyDataSetChanged();
        }
    }

    public ProgramManager getProgramManager() {
        return mProgramManager;
    }

    @Override
    public int getItemCount() {
        return mProgramManager.getTableEntryCount(mChannelId);
    }

    @Override
    public int getItemViewType(int position) {
        return R.layout.program_guide_table_item;
    }

    @Override
    public long getItemId(int position) {
        return mProgramManager.getTableEntry(mChannelId, position).getId();
    }

    @Override
    public void onBindViewHolder(ProgramViewHolder holder, int position) {
        TableEntry tableEntry = mProgramManager.getTableEntry(mChannelId, position);
        String gapTitle = tableEntry.isBlocked() ? mBlockedProgramTitle : mNoInfoProgramTitle;
        holder.onBind(tableEntry, this.getProgramManager(), gapTitle);
    }

    @Override
    public void onViewRecycled(ProgramViewHolder holder) {
        holder.onUnbind();
    }

    @Override
    public ProgramViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View itemView = LayoutInflater.from(parent.getContext()).inflate(viewType, parent, false);
        return new ProgramViewHolder(itemView);
    }

    public static class ProgramViewHolder extends RecyclerView.ViewHolder {
        // Should be called from main thread.
        public ProgramViewHolder(View itemView) {
            super(itemView);
        }

        public void onBind(TableEntry entry, ProgramManager programManager, String gapTitle) {
            if (DEBUG) {
                Log.d(TAG, "onBind. View = " + itemView + ", Entry = " + entry);
            }
            ((ProgramItemView) itemView).setValues(entry, programManager.getSelectedGenreId(),
                    programManager.getFromUtcMillis(), programManager.getToUtcMillis(), gapTitle);
        }

        public void onUnbind() {
            ((ProgramItemView) itemView).clearValues();
        }
    }
}
