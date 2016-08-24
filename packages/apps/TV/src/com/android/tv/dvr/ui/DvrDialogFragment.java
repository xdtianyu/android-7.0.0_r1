package com.android.tv.dvr.ui;

import android.app.FragmentManager;
import android.content.Context;
import android.os.Bundle;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.guide.ProgramGuide;

public class DvrDialogFragment extends HalfSizedDialogFragment {
    private final DvrGuidedStepFragment mDvrGuidedStepFragment;

    public DvrDialogFragment(DvrGuidedStepFragment dvrGuidedStepFragment) {
        mDvrGuidedStepFragment = dvrGuidedStepFragment;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        ProgramGuide programGuide =
                ((MainActivity) getActivity()).getOverlayManager().getProgramGuide();
        if (programGuide != null && programGuide.isActive()) {
            programGuide.cancelHide();
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, container, savedInstanceState);
        FragmentManager fm = getChildFragmentManager();
        GuidedStepFragment.add(fm, mDvrGuidedStepFragment, R.id.halfsized_dialog_host);
        return view;
    }

    @Override
    public void onDetach() {
        super.onDetach();
        ProgramGuide programGuide =
                ((MainActivity) getActivity()).getOverlayManager().getProgramGuide();
        if (programGuide != null && programGuide.isActive()) {
            programGuide.scheduleHide();
        }
    }
}
