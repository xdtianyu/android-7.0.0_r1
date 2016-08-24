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

package com.android.tv.dialog;

import android.app.Activity;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.os.Bundle;
import android.view.KeyEvent;

import com.android.tv.MainActivity;
import com.android.tv.TvApplication;
import com.android.tv.analytics.HasTrackerLabel;
import com.android.tv.analytics.Tracker;

/**
 * Provides the safe dismiss feature regardless of the DialogFragment's life cycle.
 */
public abstract class SafeDismissDialogFragment extends DialogFragment
        implements HasTrackerLabel {
    private MainActivity mActivity;
    private boolean mAttached = false;
    private boolean mDismissPending = false;
    private Tracker mTracker;

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        return new TvDialog(getActivity(), getTheme());
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        mAttached = true;
        mActivity = (MainActivity) activity;
        mTracker = TvApplication.getSingletons(activity).getTracker();
        if (mDismissPending) {
            mDismissPending = false;
            dismiss();
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        mTracker.sendScreenView(getTrackerLabel());
    }

    @Override
    public void onDestroy() {
        if (mActivity != null) {
            mActivity.getOverlayManager().onDialogDestroyed();
        }
        super.onDestroy();
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mAttached = false;
        mTracker = null;
    }

    /**
     * Dismiss safely regardless of the DialogFragment's life cycle.
     */
    @Override
    public void dismiss() {
        if (!mAttached) {
            // dismiss() before getFragmentManager() is set cause NPE in dismissInternal().
            // FragmentManager is set when a fragment is used in a transaction,
            // so wait here until we can dismiss safely.
            mDismissPending = true;
        } else {
            super.dismiss();
        }
    }

    protected class TvDialog extends Dialog {
        public TvDialog(Context context, int theme) {
            super(context, theme);
        }

        @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            // When a dialog is showing, key events are handled by the dialog instead of
            // MainActivity. Therefore, unless a key is a global key, it should be handled here.
            if (mAttached && keyCode == KeyEvent.KEYCODE_SEARCH) {
                mActivity.showSearchActivity();
                return true;
            }
            return super.onKeyUp(keyCode, event);
        }
    }
}
