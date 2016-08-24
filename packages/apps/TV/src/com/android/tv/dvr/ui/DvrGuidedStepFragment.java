package com.android.tv.dvr.ui;

import android.content.Context;
import android.os.Bundle;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.VerticalGridView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.tv.MainActivity;
import com.android.tv.TvApplication;
import com.android.tv.dialog.SafeDismissDialogFragment;
import com.android.tv.dvr.DvrManager;
import com.android.tv.guide.ProgramManager.TableEntry;
import com.android.tv.R;

public class DvrGuidedStepFragment extends GuidedStepFragment {
    private final TableEntry mEntry;
    private DvrManager mDvrManager;

    public DvrGuidedStepFragment(TableEntry entry) {
        mEntry = entry;
    }

    protected TableEntry getEntry() {
        return mEntry;
    }

    protected DvrManager getDvrManager() {
        return mDvrManager;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mDvrManager = TvApplication.getSingletons(context).getDvrManager();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, container, savedInstanceState);
        VerticalGridView gridView = getGuidedActionsStylist().getActionsGridView();
        gridView.setWindowAlignment(VerticalGridView.WINDOW_ALIGN_BOTH_EDGE);
        return view;
    }

    @Override
    public GuidanceStylist onCreateGuidanceStylist() {
        // Workaround: b/28448653
        return new GuidanceStylist() {
            @Override
            public int onProvideLayoutId() {
                return R.layout.halfsized_guidance;
            }
        };
    }

    @Override
    public int onProvideTheme() {
        return R.style.Theme_TV_Dvr_GuidedStep;
    }

    protected void dismissDialog() {
        SafeDismissDialogFragment currentDialog =
                ((MainActivity) getActivity()).getOverlayManager().getCurrentDialog();
        if (currentDialog instanceof DvrDialogFragment) {
            currentDialog.dismiss();
        }
    }
}
