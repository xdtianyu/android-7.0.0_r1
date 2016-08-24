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
package com.google.android.car.kitchensink.keyboard;

import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.Nullable;
import android.support.car.app.menu.CarDrawerActivity;
import android.support.car.app.menu.SearchBoxEditListener;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.google.android.car.kitchensink.R;

public class KeyboardFragment extends Fragment {
    public static final int CARD = 0xfffafafa;
    public static final int TEXT_PRIMARY_DAY = 0xde000000;
    public static final int TEXT_SECONDARY_DAY = 0x8a000000;

    private Button mImeButton;
    private Button mCloseImeButton;
    private Button mShowHideInputButton;
    private CarDrawerActivity mActivity;
    private TextView mOnSearchText;
    private TextView mOnEditText;

    private final Handler mHandler = new Handler();

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.keyboard_test, container, false);
        mActivity = (CarDrawerActivity) getHost();
        mImeButton = (Button) v.findViewById(R.id.ime_button);
        mImeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mActivity.startInput("Hint");
            }
        });

        mCloseImeButton = (Button) v.findViewById(R.id.stop_ime_button);
        mCloseImeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mActivity.stopInput();
                resetInput();
            }
        });

        mShowHideInputButton = (Button) v.findViewById(R.id.ime_button2);
        mShowHideInputButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mActivity.isShowingSearchBox()) {
                    mActivity.hideSearchBox();
                } else {
                    resetInput();
                }
            }
        });

        mOnSearchText = (TextView) v.findViewById(R.id.search_text);
        mOnEditText = (TextView) v.findViewById(R.id.edit_text);
        resetInput();
        mActivity.setSearchBoxEndView(View.inflate(getContext(), R.layout.keyboard_end_view, null));

        return v;
    }

    private void resetInput() {
        mActivity.showSearchBox(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mActivity.startInput("Hint");
            }
        });
        mActivity.setSearchBoxEditListener(mEditListener);
        mActivity.setSearchBoxColors(CARD, TEXT_SECONDARY_DAY,
                TEXT_PRIMARY_DAY, TEXT_SECONDARY_DAY);
    }


    private final SearchBoxEditListener mEditListener = new SearchBoxEditListener() {
        @Override
        public void onSearch(final String text) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mOnSearchText.setText("Search: " + text);
                    resetInput();
                }
            });
        }

        @Override
        public void onEdit(final String text) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mOnEditText.setText("Edit: " + text);
                }
            });
        }
    };
}
