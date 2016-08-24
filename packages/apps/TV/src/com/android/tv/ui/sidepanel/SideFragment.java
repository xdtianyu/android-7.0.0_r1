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

package com.android.tv.ui.sidepanel;

import android.app.Activity;
import android.app.Fragment;
import android.content.Context;
import android.graphics.drawable.RippleDrawable;
import android.os.Bundle;
import android.support.v17.leanback.widget.VerticalGridView;
import android.support.v7.widget.RecyclerView;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.analytics.DurationTimer;
import com.android.tv.analytics.HasTrackerLabel;
import com.android.tv.analytics.Tracker;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.ProgramDataManager;
import com.android.tv.util.SystemProperties;

import java.util.List;

public abstract class SideFragment extends Fragment implements HasTrackerLabel {
    public static final int INVALID_POSITION = -1;

    private static final int RECYCLED_VIEW_POOL_SIZE = 7;
    private static final int[] PRELOADED_VIEW_IDS = {
        R.layout.option_item_radio_button,
        R.layout.option_item_channel_lock,
        R.layout.option_item_check_box,
        R.layout.option_item_channel_check
    };

    private static RecyclerView.RecycledViewPool sRecycledViewPool;

    private VerticalGridView mListView;
    private ItemAdapter mAdapter;
    private SideFragmentListener mListener;
    private ChannelDataManager mChannelDataManager;
    private ProgramDataManager mProgramDataManager;
    private Tracker mTracker;
    private final DurationTimer mSidePanelDurationTimer = new DurationTimer();

    private final int mHideKey;
    private final int mDebugHideKey;

    public SideFragment() {
        this(KeyEvent.KEYCODE_UNKNOWN, KeyEvent.KEYCODE_UNKNOWN);
    }

    /**
     * @param hideKey the KeyCode used to hide the fragment
     * @param debugHideKey the KeyCode used to hide the fragment if
     *            {@link SystemProperties#USE_DEBUG_KEYS}.
     */
    public SideFragment(int hideKey, int debugHideKey) {
        mHideKey = hideKey;
        mDebugHideKey = debugHideKey;
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        mChannelDataManager = getMainActivity().getChannelDataManager();
        mProgramDataManager = getMainActivity().getProgramDataManager();
        mTracker = TvApplication.getSingletons(activity).getTracker();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        if (sRecycledViewPool == null) {
            // sRecycledViewPool should be initialized by calling preloadRecycledViews()
            // before the entering animation of this fragment starts,
            // because it takes long time and if it is called after the animation starts (e.g. here)
            // it can affect the animation.
            throw new IllegalStateException("The RecyclerView pool has not been initialized.");
        }
        View view = inflater.inflate(getFragmentLayoutResourceId(), container, false);

        TextView textView = (TextView) view.findViewById(R.id.side_panel_title);
        textView.setText(getTitle());

        mListView = (VerticalGridView) view.findViewById(R.id.side_panel_list);
        mListView.setRecycledViewPool(sRecycledViewPool);

        mAdapter = new ItemAdapter(inflater, getItemList());
        mListView.setAdapter(mAdapter);
        mListView.requestFocus();

        return view;
    }

    @Override
    public void onResume() {
        super.onResume();
        mTracker.sendShowSidePanel(this);
        mTracker.sendScreenView(this.getTrackerLabel());
        mSidePanelDurationTimer.start();
    }

    @Override
    public void onPause() {
        mTracker.sendHideSidePanel(this, mSidePanelDurationTimer.reset());
        super.onPause();
    }

    @Override
    public void onDetach() {
        mTracker = null;
        super.onDetach();
    }

    public final boolean isHideKeyForThisPanel(int keyCode) {
        boolean debugKeysEnabled = SystemProperties.USE_DEBUG_KEYS.getValue();
        return mHideKey != KeyEvent.KEYCODE_UNKNOWN &&
                (mHideKey == keyCode || (debugKeysEnabled && mDebugHideKey == keyCode));
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mListView.swapAdapter(null, true);
        if (mListener != null) {
            mListener.onSideFragmentViewDestroyed();
        }
    }

    public final void setListener(SideFragmentListener listener) {
        mListener = listener;
    }

    protected void setSelectedPosition(int position) {
        mListView.setSelectedPosition(position);
    }

    protected int getSelectedPosition() {
        return mListView.getSelectedPosition();
    }

    public void setItems(List<Item> items) {
        mAdapter.reset(items);
    }

    protected void closeFragment() {
        getMainActivity().getOverlayManager().getSideFragmentManager().popSideFragment();
    }

    protected MainActivity getMainActivity() {
        return (MainActivity) getActivity();
    }

    protected ChannelDataManager getChannelDataManager() {
        return mChannelDataManager;
    }

    protected ProgramDataManager getProgramDataManager() {
        return mProgramDataManager;
    }

    protected void notifyDataSetChanged() {
        mAdapter.notifyDataSetChanged();
    }

    /*
     * HACK: The following methods bypass the updating mechanism of RecyclerView.Adapter and
     * directly updates each item. This works around a bug in the base libraries where calling
     * Adapter.notifyItemsChanged() causes the VerticalGridView to lose track of displayed item
     * position.
     */

    protected void notifyItemChanged(int position) {
        notifyItemChanged(mAdapter.getItem(position));
    }

    protected void notifyItemChanged(Item item) {
        item.notifyUpdated();
    }

    /**
     * Notifies all items of ItemAdapter has changed without structural changes.
     */
    protected void notifyItemsChanged() {
        notifyItemsChanged(0, mAdapter.getItemCount());
    }

    /**
     * Notifies some items of ItemAdapter has changed starting from position
     * <code>positionStart</code> to the end without structural changes.
     */
    protected void notifyItemsChanged(int positionStart) {
        notifyItemsChanged(positionStart, mAdapter.getItemCount() - positionStart);
    }

    protected void notifyItemsChanged(int positionStart, int itemCount) {
        while (itemCount-- != 0) {
            notifyItemChanged(positionStart++);
        }
    }

    /*
     * END HACK
     */

    protected int getFragmentLayoutResourceId() {
        return R.layout.option_fragment;
    }

    protected abstract String getTitle();
    @Override
    public abstract String getTrackerLabel();
    protected abstract List<Item> getItemList();

    public interface SideFragmentListener {
        void onSideFragmentViewDestroyed();
    }

    public static void preloadRecycledViews(Context context) {
        if (sRecycledViewPool != null) {
            return;
        }
        sRecycledViewPool = new RecyclerView.RecycledViewPool();
        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        for (int id : PRELOADED_VIEW_IDS) {
            sRecycledViewPool.setMaxRecycledViews(id, RECYCLED_VIEW_POOL_SIZE);
            for (int j = 0; j < RECYCLED_VIEW_POOL_SIZE; ++j) {
                ItemAdapter.ViewHolder viewHolder = new ItemAdapter.ViewHolder(
                        inflater.inflate(id, null, false));
                sRecycledViewPool.putRecycledView(viewHolder);
            }
        }
    }

    private static class ItemAdapter extends RecyclerView.Adapter<ItemAdapter.ViewHolder> {
        private final LayoutInflater mLayoutInflater;
        private List<Item> mItems;

        private ItemAdapter(LayoutInflater layoutInflater, List<Item> items) {
            mLayoutInflater = layoutInflater;
            mItems = items;
        }

        private void reset(List<Item> items) {
            mItems = items;
            notifyDataSetChanged();
        }

        @Override
        public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            return new ViewHolder(mLayoutInflater.inflate(viewType, parent, false));
        }

        @Override
        public void onBindViewHolder(ViewHolder holder, int position) {
            holder.onBind(this, getItem(position));
        }

        @Override
        public void onViewRecycled(ViewHolder holder) {
            holder.onUnbind();
        }

        @Override
        public int getItemViewType(int position) {
            return getItem(position).getResourceId();
        }

        @Override
        public int getItemCount() {
            return mItems == null ? 0 : mItems.size();
        }

        private Item getItem(int position) {
            return mItems.get(position);
        }

        private void clearRadioGroup(Item item) {
            int position = mItems.indexOf(item);
            for (int i = position - 1; i >= 0; --i) {
                if ((item = mItems.get(i)) instanceof RadioButtonItem) {
                    ((RadioButtonItem) item).setChecked(false);
                } else {
                    break;
                }
            }
            for (int i = position + 1; i < mItems.size(); ++i) {
                if ((item = mItems.get(i)) instanceof RadioButtonItem) {
                    ((RadioButtonItem) item).setChecked(false);
                } else {
                    break;
                }
            }
        }

        private static class ViewHolder extends RecyclerView.ViewHolder
                implements View.OnClickListener, View.OnFocusChangeListener {
            private ItemAdapter mAdapter;
            public Item mItem;

            private ViewHolder(View view) {
                super(view);
                itemView.setOnClickListener(this);
                itemView.setOnFocusChangeListener(this);
            }

            public void onBind(ItemAdapter adapter, Item item) {
                mAdapter = adapter;
                mItem = item;
                mItem.onBind(itemView);
                mItem.onUpdate();
            }

            public void onUnbind() {
                mItem.onUnbind();
                mItem = null;
                mAdapter = null;
            }

            @Override
            public void onClick(View view) {
                if (mItem instanceof RadioButtonItem) {
                    mAdapter.clearRadioGroup(mItem);
                }
                if (view.getBackground() instanceof RippleDrawable) {
                    view.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            if (mItem != null) {
                                mItem.onSelected();
                            }
                        }
                    }, view.getResources().getInteger(R.integer.side_panel_ripple_anim_duration));
                } else {
                    mItem.onSelected();
                }
            }

            @Override
            public void onFocusChange(View view, boolean focusGained) {
                if (focusGained) {
                    mItem.onFocused();
                }
            }
        }
    }
}
