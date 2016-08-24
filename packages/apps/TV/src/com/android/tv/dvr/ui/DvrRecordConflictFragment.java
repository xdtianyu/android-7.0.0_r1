package com.android.tv.dvr.ui;

import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.android.tv.MainActivity;
import com.android.tv.R;

import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;

import com.android.tv.data.Channel;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.Program;
import com.android.tv.dvr.ScheduledRecording;
import com.android.tv.guide.ProgramManager.TableEntry;

import java.text.DateFormat;
import java.util.Date;
import java.util.List;

public class DvrRecordConflictFragment extends DvrGuidedStepFragment {
    private static final int DVR_EPG_RECORD = 1;
    private static final int DVR_EPG_NOT_RECORD = 2;

    private List<ScheduledRecording> mConflicts;

    public DvrRecordConflictFragment(TableEntry entry) {
        super(entry);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        mConflicts = getDvrManager().getScheduledRecordingsThatConflict(getEntry().program);
        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public Guidance onCreateGuidance(Bundle savedInstanceState) {
        final MainActivity tvActivity = (MainActivity) getActivity();
        final ChannelDataManager channelDataManager = tvActivity.getChannelDataManager();
        StringBuilder sb = new StringBuilder();
        for (ScheduledRecording r : mConflicts) {
            Channel channel = channelDataManager.getChannel(r.getChannelId());
            if (channel == null) {
                continue;
            }
            sb.append(channel.getDisplayName())
                    .append(" : ")
                    .append(DateFormat.getDateTimeInstance().format(new Date(r.getStartTimeMs())))
                    .append("\n");
        }
        String title = getResources().getString(R.string.dvr_epg_conflict_dialog_title);
        String description = sb.toString();
        return new Guidance(title, description, null, null);
    }

    @Override
    public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
        Activity activity = getActivity();
        actions.add(new GuidedAction.Builder(activity)
                .id(DVR_EPG_RECORD)
                .title(getResources().getString(R.string.dvr_epg_record))
                .build());
        actions.add(new GuidedAction.Builder(activity)
                .id(DVR_EPG_NOT_RECORD)
                .title(getResources().getString(R.string.dvr_epg_do_not_record))
                .build());
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        Program program = getEntry().program;
        if (action.getId() == DVR_EPG_RECORD) {
            getDvrManager().addSchedule(program, mConflicts);
        }
        dismissDialog();
    }
}
