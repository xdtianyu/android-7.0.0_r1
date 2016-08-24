/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.calculator2;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.widget.TextView;

/**
 * Display a message with a dismiss putton, and optionally a second button.
 */
public class AlertDialogFragment extends DialogFragment implements DialogInterface.OnClickListener {

    public interface OnClickListener {
        /**
         * This method will be invoked when a button in the dialog is clicked.
         *
         * @param fragment the AlertDialogFragment that received the click
         * @param which the button that was clicked (e.g.
         *            {@link DialogInterface#BUTTON_POSITIVE}) or the position
         *            of the item clicked
         */
        public void onClick(AlertDialogFragment fragment, int which);
    }

    private static final String NAME = AlertDialogFragment.class.getName();
    private static final String KEY_MESSAGE = NAME + "_message";
    private static final String KEY_BUTTON_NEGATIVE = NAME + "_button_negative";
    private static final String KEY_BUTTON_POSITIVE = NAME + "_button_positive";

    /**
     * Create and show a DialogFragment with the given message.
     * @param activity originating Activity
     * @param message displayed message
     * @param positiveButtonLabel label for second button, if any.  If non-null, activity must
     * implement AlertDialogFragment.OnClickListener to respond.
     */
    public static void showMessageDialog(Activity activity, CharSequence message,
            @Nullable CharSequence positiveButtonLabel) {
        final AlertDialogFragment dialogFragment = new AlertDialogFragment();
        final Bundle args = new Bundle();
        args.putCharSequence(KEY_MESSAGE, message);
        args.putCharSequence(KEY_BUTTON_NEGATIVE, activity.getString(R.string.dismiss));
        if (positiveButtonLabel != null) {
            args.putCharSequence(KEY_BUTTON_POSITIVE, positiveButtonLabel);
        }
        dialogFragment.setArguments(args);
        dialogFragment.show(activity.getFragmentManager(), null /* tag */);
    }

    public AlertDialogFragment() {
        setStyle(STYLE_NO_TITLE, android.R.attr.alertDialogTheme);
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final Bundle args = getArguments() == null ? Bundle.EMPTY : getArguments();
        final Context context = getContext();
        final LayoutInflater inflater = LayoutInflater.from(context);
        final TextView textView = (TextView) inflater.inflate(R.layout.dialog_message,
                null /* root */);
        textView.setText(args.getCharSequence(KEY_MESSAGE));
        final AlertDialog.Builder builder = new AlertDialog.Builder(context)
                .setView(textView)
                .setNegativeButton(args.getCharSequence(KEY_BUTTON_NEGATIVE), null /* listener */);
        final CharSequence positiveButtonLabel = args.getCharSequence(KEY_BUTTON_POSITIVE);
        if (positiveButtonLabel != null) {
            builder.setPositiveButton(positiveButtonLabel, this);
        }
        return builder.create();
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        final Activity activity = getActivity();
        if (activity instanceof AlertDialogFragment.OnClickListener /* always true */) {
            ((AlertDialogFragment.OnClickListener) activity).onClick(this, which);
        }
    }
}
