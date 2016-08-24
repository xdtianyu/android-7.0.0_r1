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

package com.android.tv.common.ui.setup;

import android.app.Fragment;
import android.view.View;
import android.view.View.OnClickListener;

/**
 * Helper class for the execution in the fragment.
 */
public class SetupActionHelper {
    /**
     * Executes the action of the given {@code actionId}.
     */
    public static void onActionClick(Fragment fragment, String category, int actionId) {
        OnActionClickListener listener = null;
        if (fragment instanceof SetupFragment) {
            listener = ((SetupFragment) fragment).getOnActionClickListener();
        }
        if (listener == null && fragment.getActivity() instanceof OnActionClickListener) {
            listener = (OnActionClickListener) fragment.getActivity();
        }
        if (listener != null) {
            listener.onActionClick(category, actionId);
        }
    }

    /**
     * Creates an {@link OnClickListener} to handle the action.
     */
    public static OnClickListener createOnClickListenerForAction(OnActionClickListener listener,
            String category, int actionId) {
        return new OnActionClickListenerForAction(listener, category, actionId);
    }

    private static class OnActionClickListenerForAction implements OnClickListener {
        private final OnActionClickListener mListener;
        private final String mCategory;
        private final int mActionId;

        OnActionClickListenerForAction(OnActionClickListener listener, String category,
                int actionId) {
            mListener = listener;
            mCategory = category;
            mActionId = actionId;
        }

        @Override
        public void onClick(View v) {
            if (mListener != null) {
                mListener.onActionClick(mCategory, mActionId);
            }
        }
    }

    private SetupActionHelper() { }
}
