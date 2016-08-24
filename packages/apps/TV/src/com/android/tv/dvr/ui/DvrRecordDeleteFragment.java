package com.android.tv.dvr.ui;

import android.app.Activity;
import android.os.Bundle;

import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;

import com.android.tv.R;
import com.android.tv.guide.ProgramManager.TableEntry;

import java.util.List;

public class DvrRecordDeleteFragment extends DvrGuidedStepFragment {
    private static final int ACTION_DELETE_YES = 1;
    private static final int ACTION_DELETE_NO = 2;

    public DvrRecordDeleteFragment(TableEntry entry) {
        super(entry);
    }

    @Override
    public Guidance onCreateGuidance(Bundle savedInstanceState) {
        String title = getResources().getString(R.string.epg_dvr_dialog_message_delete_schedule);
        return new Guidance(title, null, null, null);
    }

    @Override
    public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
        Activity activity = getActivity();
        actions.add(new GuidedAction.Builder(activity)
                .id(ACTION_DELETE_YES)
                .title(getResources().getString(android.R.string.yes))
                .build());
        actions.add(new GuidedAction.Builder(activity)
                .id(ACTION_DELETE_NO)
                .title(getResources().getString(android.R.string.no))
                .build());
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        if (action.getId() == ACTION_DELETE_YES) {
            getDvrManager().removeScheduledRecording(getEntry().scheduledRecording);
        }
        dismissDialog();
    }
}
