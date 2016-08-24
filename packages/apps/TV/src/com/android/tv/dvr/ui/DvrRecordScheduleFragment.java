package com.android.tv.dvr.ui;

import android.app.Activity;
import android.app.FragmentManager;
import android.os.Bundle;

import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;

import com.android.tv.data.Program;
import com.android.tv.dialog.SafeDismissDialogFragment;
import com.android.tv.dvr.ScheduledRecording;
import com.android.tv.guide.ProgramManager.TableEntry;
import com.android.tv.MainActivity;
import com.android.tv.R;

import java.util.List;

public class DvrRecordScheduleFragment extends DvrGuidedStepFragment {
    private static final int ACTION_RECORD_YES = 1;
    private static final int ACTION_RECORD_NO = 2;

    public DvrRecordScheduleFragment(TableEntry entry) {
        super(entry);
    }

    @Override
    public Guidance onCreateGuidance(Bundle savedInstanceState) {
        String title = getResources().getString(R.string.epg_dvr_dialog_message_schedule_recording);
        return new Guidance(title, null, null, null);
    }

    @Override
    public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
        Activity activity = getActivity();
        actions.add(new GuidedAction.Builder(activity)
                .id(ACTION_RECORD_YES)
                .title(getResources().getString(android.R.string.yes))
                .build());
        actions.add(new GuidedAction.Builder(activity)
                .id(ACTION_RECORD_NO)
                .title(getResources().getString(android.R.string.no))
                .build());
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        TableEntry entry = getEntry();
        Program program = entry.program;
        final List<ScheduledRecording> conflicts =
                getDvrManager().getScheduledRecordingsThatConflict(program);
        if (action.getId() == ACTION_RECORD_YES) {
            if (conflicts.isEmpty()) {
                getDvrManager().addSchedule(program, conflicts);
                dismissDialog();
            } else {
                DvrRecordConflictFragment dvrConflict = new DvrRecordConflictFragment(entry);
                SafeDismissDialogFragment currentDialog =
                ((MainActivity) getActivity()).getOverlayManager().getCurrentDialog();
                if (currentDialog instanceof DvrDialogFragment) {
                    FragmentManager fm = currentDialog.getChildFragmentManager();
                    GuidedStepFragment.add(fm, dvrConflict, R.id.halfsized_dialog_host);
                }
            }
        } else if (action.getId() == ACTION_RECORD_NO) {
            dismissDialog();
        }
    }
}
