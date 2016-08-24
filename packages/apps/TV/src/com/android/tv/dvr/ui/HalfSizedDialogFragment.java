package com.android.tv.dvr.ui;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.tv.dialog.SafeDismissDialogFragment;
import com.android.tv.R;

public class HalfSizedDialogFragment extends SafeDismissDialogFragment {
    public static final String DIALOG_TAG = HalfSizedDialogFragment.class.getSimpleName();
    public static final String TRACKER_LABEL = "Half sized dialog";

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(R.layout.halfsized_dialog, null);
    }

    @Override
    public int getTheme() {
        return R.style.Theme_TV_dialog_HalfSizedDialog;
    }

    @Override
    public String getTrackerLabel() {
        return TRACKER_LABEL;
    }
}
