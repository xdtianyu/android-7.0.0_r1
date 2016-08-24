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

package com.android.tv.onboarding;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager.TvInputCallback;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;
import android.support.v17.leanback.widget.GuidedActionsStylist;
import android.support.v17.leanback.widget.VerticalGridView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.android.tv.ApplicationSingletons;
import com.android.tv.Features;
import com.android.tv.R;
import com.android.tv.SetupPassthroughActivity;
import com.android.tv.TvApplication;
import com.android.tv.common.TvCommonUtils;
import com.android.tv.common.ui.setup.SetupGuidedStepFragment;
import com.android.tv.common.ui.setup.SetupMultiPaneFragment;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.TvInputNewComparator;
import com.android.tv.util.SetupUtils;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.Utils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * A fragment for channel source info/setup.
 */
public class SetupSourcesFragment extends SetupMultiPaneFragment {
    private static final String TAG = "SetupSourcesFragment";

    public static final String ACTION_CATEGORY =
            "com.android.tv.onboarding.SetupSourcesFragment";
    public static final int ACTION_PLAY_STORE = 1;

    private static final String SETUP_TRACKER_LABEL = "Setup fragment";

    private InputSetupRunnable mInputSetupRunnable;

    private ContentFragment mContentFragment;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, container, savedInstanceState);
        TvApplication.getSingletons(getActivity()).getTracker().sendScreenView(SETUP_TRACKER_LABEL);
        return view;
    }

    @Override
    protected void onEnterTransitionEnd() {
        if (mContentFragment != null) {
            mContentFragment.executePendingAction();
        }
    }

    @Override
    protected SetupGuidedStepFragment onCreateContentFragment() {
        mContentFragment = new ContentFragment();
        mContentFragment.setParentFragment(this);
        Bundle arguments = new Bundle();
        arguments.putBoolean(SetupGuidedStepFragment.KEY_THREE_PANE, true);
        mContentFragment.setArguments(arguments);
        return mContentFragment;
    }

    @Override
    protected String getActionCategory() {
        return ACTION_CATEGORY;
    }

    /**
     * Call this method to run customized input setup.
     *
     * @param runnable runnable to be called when the input setup is necessary.
     */
    public void setInputSetupRunnable(InputSetupRunnable runnable) {
        mInputSetupRunnable = runnable;
    }

    /**
     * Interface for the customized input setup.
     */
    public interface InputSetupRunnable {
        /**
         * Called for the input setup.
         *
         * @param input TV input for setup.
         */
        void runInputSetup(TvInputInfo input);
    }

    public static class ContentFragment extends SetupGuidedStepFragment {
        private static final int REQUEST_CODE_START_SETUP_ACTIVITY = 1;

        // ACTION_PLAY_STORE is defined in the outer class.
        private static final int ACTION_DIVIDER = 2;
        private static final int ACTION_HEADER = 3;
        private static final int ACTION_INPUT_START = 4;

        private static final int PENDING_ACTION_NONE = 0;
        private static final int PENDING_ACTION_INPUT_CHANGED = 1;
        private static final int PENDING_ACTION_CHANNEL_CHANGED = 2;

        private TvInputManagerHelper mInputManager;
        private ChannelDataManager mChannelDataManager;
        private SetupUtils mSetupUtils;
        private List<TvInputInfo> mInputs;
        private int mKnownInputStartIndex;
        private int mDoneInputStartIndex;

        private SetupSourcesFragment mParentFragment;

        private String mNewlyAddedInputId;

        private int mPendingAction = PENDING_ACTION_NONE;

        private final TvInputCallback mInputCallback = new TvInputCallback() {
            @Override
            public void onInputAdded(String inputId) {
                handleInputChanged();
            }

            @Override
            public void onInputRemoved(String inputId) {
                handleInputChanged();
            }

            @Override
            public void onInputUpdated(String inputId) {
                handleInputChanged();
            }

            private void handleInputChanged() {
                // The actions created while enter transition is running will not be included in the
                // fragment transition.
                if (mParentFragment.isEnterTransitionRunning()) {
                    mPendingAction = PENDING_ACTION_INPUT_CHANGED;
                    return;
                }
                buildInputs();
                updateActions();
            }
        };

        void setParentFragment(SetupSourcesFragment parentFragment) {
            mParentFragment = parentFragment;
        }

        private final ChannelDataManager.Listener mChannelDataManagerListener
                = new ChannelDataManager.Listener() {
            @Override
            public void onLoadFinished() {
                handleChannelChanged();
            }

            @Override
            public void onChannelListUpdated() {
                handleChannelChanged();
            }

            @Override
            public void onChannelBrowsableChanged() {
                handleChannelChanged();
            }

            private void handleChannelChanged() {
                // The actions created while enter transition is running will not be included in the
                // fragment transition.
                if (mParentFragment.isEnterTransitionRunning()) {
                    if (mPendingAction != PENDING_ACTION_INPUT_CHANGED) {
                        mPendingAction = PENDING_ACTION_CHANNEL_CHANGED;
                    }
                    return;
                }
                updateActions();
            }
        };

        @Override
        public void onCreate(Bundle savedInstanceState) {
            // TODO: Handle USB TV tuner differently.
            Context context = getActivity();
            ApplicationSingletons app = TvApplication.getSingletons(context);
            mInputManager = app.getTvInputManagerHelper();
            mChannelDataManager = app.getChannelDataManager();
            mSetupUtils = SetupUtils.getInstance(context);
            buildInputs();
            mInputManager.addCallback(mInputCallback);
            mChannelDataManager.addListener(mChannelDataManagerListener);
            super.onCreate(savedInstanceState);
        }

        @Override
        public void onDestroy() {
            super.onDestroy();
            mChannelDataManager.removeListener(mChannelDataManagerListener);
            mInputManager.removeCallback(mInputCallback);
        }

        @NonNull
        @Override
        public Guidance onCreateGuidance(Bundle savedInstanceState) {
            String title = getString(R.string.setup_sources_text);
            String description = getString(R.string.setup_sources_description);
            return new Guidance(title, description, null, null);
        }

        @Override
        public GuidedActionsStylist onCreateActionsStylist() {
            return new SetupSourceGuidedActionsStylist();
        }

        @Override
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            createActionsInternal(actions);
        }

        private void buildInputs() {
            List<TvInputInfo> oldInputs = mInputs;
            mInputs = mInputManager.getTvInputInfos(true, true);
            // Get newly installed input ID.
            if (oldInputs != null) {
                List<TvInputInfo> newList = new ArrayList<>(mInputs);
                for (TvInputInfo input : oldInputs) {
                    newList.remove(input);
                }
                if (newList.size() > 0 && mSetupUtils.isNewInput(newList.get(0).getId())) {
                    mNewlyAddedInputId = newList.get(0).getId();
                } else {
                    mNewlyAddedInputId = null;
                }
            }
            Collections.sort(mInputs, new TvInputNewComparator(mSetupUtils, mInputManager));
            mKnownInputStartIndex = 0;
            mDoneInputStartIndex = 0;
            for (TvInputInfo input : mInputs) {
                if (mSetupUtils.isNewInput(input.getId())) {
                    mSetupUtils.markAsKnownInput(input.getId());
                    ++mKnownInputStartIndex;
                }
                if (!mSetupUtils.isSetupDone(input.getId())) {
                    ++mDoneInputStartIndex;
                }
            }
        }

        private void updateActions() {
            List<GuidedAction> actions = new ArrayList<>();
            createActionsInternal(actions);
            setActions(actions);
        }

        private void createActionsInternal(List<GuidedAction> actions) {
            int newPosition = -1;
            int position = 0;
            if (mDoneInputStartIndex > 0) {
                // Need a "New" category
                actions.add(new GuidedAction.Builder(getActivity()).id(ACTION_HEADER)
                        .title(null).description(getString(R.string.setup_category_new))
                        .focusable(false).build());
            }
            for (int i = 0; i < mInputs.size(); ++i) {
                if (i == mDoneInputStartIndex) {
                    ++position;
                    actions.add(new GuidedAction.Builder(getActivity()).id(ACTION_HEADER)
                            .title(null).description(getString(R.string.setup_category_done))
                            .focusable(false).build());
                }
                TvInputInfo input = mInputs.get(i);
                String inputId = input.getId();
                String description;
                int channelCount = mChannelDataManager.getChannelCountForInput(inputId);
                if (mSetupUtils.isSetupDone(inputId) || channelCount > 0) {
                    if (channelCount == 0) {
                        description = getString(R.string.setup_input_no_channels);
                    } else {
                        description = getResources().getQuantityString(
                                R.plurals.setup_input_channels, channelCount, channelCount);
                    }
                } else if (i >= mKnownInputStartIndex) {
                    description = getString(R.string.setup_input_setup_now);
                } else {
                    description = getString(R.string.setup_input_new);
                }
                ++position;
                if (input.getId().equals(mNewlyAddedInputId)) {
                    newPosition = position;
                }
                actions.add(new GuidedAction.Builder(getActivity()).id(ACTION_INPUT_START + i)
                        .title(input.loadLabel(getActivity()).toString()).description(description)
                        .build());
            }
            if (Features.ONBOARDING_PLAY_STORE.isEnabled(getActivity())) {
                if (mInputs.size() > 0) {
                    // Divider
                    ++position;
                    actions.add(new GuidedAction.Builder(getActivity()).id(ACTION_DIVIDER)
                            .title(null).description(null).focusable(false).build());
                }
                // Play store action
                ++position;
                actions.add(new GuidedAction.Builder(getActivity()).id(ACTION_PLAY_STORE)
                        .title(getString(R.string.setup_play_store_action_title))
                        .description(getString(R.string.setup_play_store_action_description))
                        .icon(R.drawable.ic_playstore).build());
            }
            if (newPosition != -1) {
                VerticalGridView gridView = getGuidedActionsStylist().getActionsGridView();
                gridView.setSelectedPosition(newPosition);
            }
        }

        @Override
        protected String getActionCategory() {
            return ACTION_CATEGORY;
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            if (action.getId() == ACTION_PLAY_STORE) {
                mParentFragment.onActionClick(ACTION_CATEGORY, (int) action.getId());
                return;
            }
            TvInputInfo input = mInputs.get((int) action.getId() - ACTION_INPUT_START);
            if (mParentFragment.mInputSetupRunnable != null) {
                mParentFragment.mInputSetupRunnable.runInputSetup(input);
                return;
            }
            Intent intent = TvCommonUtils.createSetupIntent(input);
            if (intent == null) {
                Toast.makeText(getActivity(), R.string.msg_no_setup_activity, Toast.LENGTH_SHORT)
                        .show();
                return;
            }
            // Even though other app can handle the intent, the setup launched by Live channels
            // should go through Live channels SetupPassthroughActivity.
            intent.setComponent(new ComponentName(getActivity(), SetupPassthroughActivity.class));
            try {
                // Now we know that the user intends to set up this input. Grant permission for
                // writing EPG data.
                SetupUtils.grantEpgPermission(getActivity(), input.getServiceInfo().packageName);
                startActivityForResult(intent, REQUEST_CODE_START_SETUP_ACTIVITY);
            } catch (ActivityNotFoundException e) {
                Toast.makeText(getActivity(), getString(R.string.msg_unable_to_start_setup_activity,
                        input.loadLabel(getActivity())), Toast.LENGTH_SHORT).show();
            }
        }

        @Override
        public void onActivityResult(int requestCode, int resultCode, Intent data) {
            updateActions();
        }

        void executePendingAction() {
            switch (mPendingAction) {
                case PENDING_ACTION_INPUT_CHANGED:
                    buildInputs();
                    // Fall through
                case PENDING_ACTION_CHANNEL_CHANGED:
                    updateActions();
                    break;
            }
            mPendingAction = PENDING_ACTION_NONE;
        }

        private class SetupSourceGuidedActionsStylist extends GuidedActionsStylist {
            private static final int VIEW_TYPE_DIVIDER = 1;

            private static final float ALPHA_CATEGORY = 1.0f;
            private static final float ALPHA_INPUT_DESCRIPTION = 0.5f;

            @Override
            public int getItemViewType(GuidedAction action) {
                if (action.getId() == ACTION_DIVIDER) {
                    return VIEW_TYPE_DIVIDER;
                }
                return super.getItemViewType(action);
            }

            @Override
            public int onProvideItemLayoutId(int viewType) {
                if (viewType == VIEW_TYPE_DIVIDER) {
                    return R.layout.onboarding_item_divider;
                }
                return super.onProvideItemLayoutId(viewType);
            }

            @Override
            public void onBindViewHolder(ViewHolder vh, GuidedAction action) {
                super.onBindViewHolder(vh, action);
                TextView descriptionView = vh.getDescriptionView();
                if (descriptionView != null) {
                    if (action.getId() == ACTION_HEADER) {
                        descriptionView.setAlpha(ALPHA_CATEGORY);
                        descriptionView.setTextColor(Utils.getColor(getResources(),
                                R.color.setup_category));
                        descriptionView.setTypeface(Typeface.create(
                                getString(R.string.condensed_font), 0));
                    } else {
                        descriptionView.setAlpha(ALPHA_INPUT_DESCRIPTION);
                        descriptionView.setTextColor(Utils.getColor(getResources(),
                                R.color.common_setup_input_description));
                        descriptionView.setTypeface(Typeface.create(getString(R.string.font), 0));
                    }
                }
                // Workaround for b/26473407.
                ImageView iconView = vh.getIconView();
                if (iconView != null) {
                    Drawable icon = action.getIcon();
                    if (icon != null) {
                        // setImageDrawable resets the drawable's level unless we set the view level
                        // first.
                        iconView.setImageLevel(icon.getLevel());
                        iconView.setImageDrawable(icon);
                        iconView.setVisibility(View.VISIBLE);
                    } else {
                        iconView.setVisibility(View.GONE);
                    }
                }
            }
        }
    }
}
