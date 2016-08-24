/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.settings.widget.picker;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.DimenRes;
import android.support.v17.leanback.widget.OnChildSelectedListener;
import android.support.v17.leanback.widget.VerticalGridView;
import android.support.v7.widget.RecyclerView;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Interpolator;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.android.tv.settings.R;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Picker class
 */
public abstract class Picker extends Fragment {

    /**
     * Object listening for adapter events.
     */
    public interface ResultListener {
        void onCommitResult(List<String> result);
    }

    private Context mContext;
    private List<VerticalGridView> mColumnViews;
    private ResultListener mResultListener;
    private ArrayList<PickerColumn> mColumns = new ArrayList<>();

    private float mUnfocusedAlpha;
    private float mFocusedAlpha;
    private float mVisibleColumnAlpha;
    private float mInvisibleColumnAlpha;
    private int mAlphaAnimDuration;
    private Interpolator mDecelerateInterpolator;
    private Interpolator mAccelerateInterpolator;
    private boolean mKeyDown = false;
    private boolean mClicked = false;

    /**
     * selection result
     */
    private List<String> mResult;

    /**
     * Classes extending {@link Picker} should override this method to supply
     * the columns
     */
    protected abstract ArrayList<PickerColumn> getColumns();

    /**
     * Classes extending {@link Picker} can choose to override this method to
     * supply the separator string
     */
    protected abstract String getSeparator();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = getActivity();

        mFocusedAlpha = getFloat(R.dimen.list_item_selected_title_text_alpha);
        mUnfocusedAlpha = getFloat(R.dimen.list_item_unselected_text_alpha);
        mVisibleColumnAlpha = getFloat(R.dimen.picker_item_visible_column_item_alpha);
        mInvisibleColumnAlpha = getFloat(R.dimen.picker_item_invisible_column_item_alpha);

        mAlphaAnimDuration = mContext.getResources().getInteger(
                R.integer.dialog_animation_duration);

        mDecelerateInterpolator = new DecelerateInterpolator(2.5F);
        mAccelerateInterpolator = new AccelerateInterpolator(2.5F);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {

        mColumns = getColumns();
        if (mColumns == null || mColumns.size() == 0) {
            return null;
        }

        final ViewGroup rootView = (ViewGroup) inflater.inflate(R.layout.picker, container, false);
        final PickerLayout pickerView = (PickerLayout) rootView.findViewById(R.id.picker);
        pickerView.setChildFocusListener(this);
        mColumnViews = new ArrayList<>();
        mResult = new ArrayList<>();

        int totalCol = mColumns.size();
        for (int i = 0; i < totalCol; i++) {
            final String[] col = mColumns.get(i).getItems();
            mResult.add(col[0]);
            final VerticalGridView columnView = (VerticalGridView) inflater.inflate(
                    R.layout.picker_column, pickerView, false);
            columnView.setWindowAlignment(VerticalGridView.WINDOW_ALIGN_NO_EDGE);
            mColumnViews.add(columnView);
            columnView.setTag(i);

            // add view to root
            pickerView.addView(columnView);

            // add a separator if not the last element
            if (i != totalCol - 1 && getSeparator() != null) {
                final TextView separator =
                        (TextView) inflater.inflate(R.layout.picker_separator, pickerView, false);
                separator.setText(getSeparator());
                pickerView.addView(separator);
            }
        }
        initAdapters();
        mColumnViews.get(0).requestFocus();

        mClicked = false;
        mKeyDown = false;

        return rootView;
    }

    private void initAdapters() {
        final int totalCol = mColumns.size();
        for (int i = 0; i < totalCol; i++) {
            VerticalGridView gridView = mColumnViews.get(i);
            gridView.setAdapter(new Adapter(i, Arrays.asList(mColumns.get(i).getItems())));
            gridView.setOnKeyInterceptListener(new VerticalGridView.OnKeyInterceptListener() {
                @Override
                public boolean onInterceptKeyEvent(KeyEvent event) {
                    switch (event.getKeyCode()) {
                        case KeyEvent.KEYCODE_DPAD_CENTER:
                        case KeyEvent.KEYCODE_ENTER:
                            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                                // We are only interested in the Key DOWN event here,
                                // because the Key UP event will generate a click, and
                                // will be handled by OnItemClickListener.
                                if (!mKeyDown) {
                                    mKeyDown = true;
                                    updateAllColumnsForClick(false);
                                }
                            }
                            break;
                    }
                    return false;
                }
            });
        }
    }

    protected void updateAdapter(int index, PickerColumn pickerColumn) {
        final VerticalGridView gridView = mColumnViews.get(index);
        final Adapter adapter = (Adapter) gridView.getAdapter();

        mColumns.set(index, pickerColumn);
        adapter.setItems(Arrays.asList(pickerColumn.getItems()));

        gridView.post(new Runnable() {
            @Override
            public void run() {
                updateColumn(gridView, false, null);
            }
        });
    }

    protected void updateSelection(int columnIndex, int selectedIndex) {
        VerticalGridView columnView = mColumnViews.get(columnIndex);
        if (columnView != null) {
            columnView.setSelectedPosition(selectedIndex);
            String text = mColumns.get(columnIndex).getItems()[selectedIndex];
            mResult.set(columnIndex, text);
        }
    }

    public void setResultListener(ResultListener listener) {
        mResultListener = listener;
    }

    private void updateAllColumnsForClick(boolean keyUp) {
        final ArrayList<Animator> animList = new ArrayList<>();

        for (final VerticalGridView column : mColumnViews) {
            final int selected = column.getSelectedPosition();

            final RecyclerView.LayoutManager manager = column.getLayoutManager();
            final int size = manager.getChildCount();

            for (int i = 0; i < size; i++) {
                final View item = manager.getChildAt(i);
                if (item != null) {
                    if (selected == i) {
                        // set alpha for main item (selected) in the column
                        if (keyUp) {
                            setOrAnimateAlphaInternal(item, true, mFocusedAlpha, mUnfocusedAlpha,
                                    animList, mAccelerateInterpolator);
                        } else {
                            setOrAnimateAlphaInternal(item, true, mUnfocusedAlpha, -1, animList,
                                    mDecelerateInterpolator);
                        }
                    } else if (!keyUp) {
                        // hide all non selected items on key down
                        setOrAnimateAlphaInternal(item, true, mInvisibleColumnAlpha, -1, animList,
                                mDecelerateInterpolator);
                    }
                }
            }
        }

        if (!animList.isEmpty()) {
            AnimatorSet animSet = new AnimatorSet();
            animSet.playTogether(animList);

            if (mClicked) {
                animSet.addListener(new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        if (mResultListener != null) {
                            mResultListener.onCommitResult(mResult);
                        }
                    }
                });
            }
            animSet.start();
        } else {
            if (mClicked && mResultListener != null) {
                mResultListener.onCommitResult(mResult);
            }
        }
    }

    public void childFocusChanged() {
        final ArrayList<Animator> animList = new ArrayList<>();

        for (final VerticalGridView column : mColumnViews) {
            updateColumn(column, column.hasFocus(), animList);
        }

        if (!animList.isEmpty()) {
            AnimatorSet animSet = new AnimatorSet();
            animSet.playTogether(animList);
            animSet.start();
        }
    }

    private void updateColumn(VerticalGridView column, boolean animateAlpha,
            ArrayList<Animator> animList) {
        if (column == null) {
            return;
        }

        final int selected = column.getSelectedPosition();
        final boolean focused = column.hasFocus();

        ArrayList<Animator> localAnimList = animList;
        if (animateAlpha && localAnimList == null) {
            // no global animation list, create a local one for the current set
            localAnimList = new ArrayList<>();
        }

        // Iterate through the visible views
        final RecyclerView.LayoutManager manager = column.getLayoutManager();
        final int size = manager.getChildCount();

        for (int i = 0; i < size; i++) {
            final View item = manager.getChildAt(i);
            if (item != null) {
                setOrAnimateAlpha(item, (selected == column.getChildAdapterPosition(item)), focused,
                        animateAlpha, localAnimList);
            }
        }
        if (animateAlpha && animList == null && !localAnimList.isEmpty()) {
            // No global animation list, so play these start the current set of animations now
            AnimatorSet animSet = new AnimatorSet();
            animSet.playTogether(localAnimList);
            animSet.start();
        }
    }

    private void setOrAnimateAlpha(View view, boolean selected, boolean focused, boolean animate,
            ArrayList<Animator> animList) {
        if (selected) {
            // set alpha for main item (selected) in the column
            if ((focused && !mKeyDown) || mClicked) {
                setOrAnimateAlphaInternal(view, animate, mFocusedAlpha, -1, animList,
                        mDecelerateInterpolator);
            } else {
                setOrAnimateAlphaInternal(view, animate, mUnfocusedAlpha, -1, animList,
                        mDecelerateInterpolator);
            }
        } else {
            // set alpha for remaining items in the column
            if (focused && !mClicked && !mKeyDown) {
                setOrAnimateAlphaInternal(view, animate, mVisibleColumnAlpha, -1, animList,
                        mDecelerateInterpolator);
            } else {
                setOrAnimateAlphaInternal(view, animate, mInvisibleColumnAlpha, -1, animList,
                        mDecelerateInterpolator);
            }
        }
    }

    private void setOrAnimateAlphaInternal(View view, boolean animate, float destAlpha,
            float startAlpha, ArrayList<Animator> animList, Interpolator interpolator) {
        view.clearAnimation();
        if (!animate) {
            view.setAlpha(destAlpha);
        } else {
            ObjectAnimator anim;
            if (startAlpha >= 0.0f) {
                // set a start alpha
                anim = ObjectAnimator.ofFloat(view, "alpha", startAlpha, destAlpha);
            } else {
                // no start alpha
                anim = ObjectAnimator.ofFloat(view, "alpha", destAlpha);
            }
            anim.setDuration(mAlphaAnimDuration);
            anim.setInterpolator(interpolator);
            if (animList != null) {
                animList.add(anim);
            } else {
                anim.start();
            }
        }
    }

    /**
     * Classes extending {@link Picker} can override this function to supply the
     * behavior when a list has been scrolled
     */
    protected void onScroll(int column, View v, int position) {}

    private float getFloat(@DimenRes int resourceId) {
        TypedValue buffer = new TypedValue();
        mContext.getResources().getValue(resourceId, buffer, true);
        return buffer.getFloat();
    }

    private class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
        private final TextView mTextView;

        public ViewHolder(View itemView) {
            super(itemView);
            mTextView = (TextView) itemView.findViewById(R.id.list_item);
            itemView.setOnClickListener(this);
        }

        public TextView getTextView() {
            return mTextView;
        }

        @Override
        public void onClick(View v) {
            if (mKeyDown) {
                mKeyDown = false;
                mClicked = true;
                updateAllColumnsForClick(true);
            }
        }
    }

    private class Adapter extends RecyclerView.Adapter<ViewHolder>
            implements OnChildSelectedListener {

        private final int mColumnId;

        private List<String> mItems;
        private VerticalGridView mGridView;

        public Adapter(int columnId, List<String> items) {
            mColumnId = columnId;
            mItems = items;
            setHasStableIds(true);
        }

        @Override
        public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            final View view = getLayoutInflater(null).inflate(R.layout.picker_item, parent, false);
            return new ViewHolder(view);
        }

        @Override
        public void onBindViewHolder(ViewHolder holder, int position) {
            final TextView textView = holder.getTextView();
            textView.setText(mItems.get(position));
            setOrAnimateAlpha(textView, mGridView.getSelectedPosition() == position,
                    mGridView.hasFocus(), false, null);
        }

        @Override
        public int getItemCount() {
            return mItems.size();
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public void onAttachedToRecyclerView(RecyclerView recyclerView) {
            mGridView = (VerticalGridView) recyclerView;
            mGridView.setOnChildSelectedListener(this);
        }

        @Override
        public void onDetachedFromRecyclerView(RecyclerView recyclerView) {
            mGridView = null;
        }

        @Override
        public void onChildSelected(ViewGroup parent, View view, int position, long id) {
            if (mGridView == null) {
                return;
            }
            final ViewHolder vh = (ViewHolder) mGridView.getChildViewHolder(view);
            final TextView textView = vh.getTextView();

            updateColumn(mGridView, mGridView.hasFocus(), null);
            mResult.set(mColumnId, textView.getText().toString());
            onScroll(mColumnId, textView, position);
        }

        public void setItems(List<String> items) {
            final List<String> oldItems = mItems;
            mItems = items;
            if (oldItems.size() < items.size()) {
                notifyItemRangeInserted(oldItems.size(), oldItems.size() - items.size());
            } else if (items.size() < oldItems.size()) {
                notifyItemRangeRemoved(items.size(), items.size() - oldItems.size());
            }
        }
    }

    public static class PickerLayout extends LinearLayout {

        private Picker mChildFocusListener;

        public PickerLayout(Context context) {
            super(context);
        }

        public PickerLayout(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        public PickerLayout(Context context, AttributeSet attrs, int defStyleAttr) {
            super(context, attrs, defStyleAttr);
        }

        public PickerLayout(Context context, AttributeSet attrs, int defStyleAttr,
                int defStyleRes) {
            super(context, attrs, defStyleAttr, defStyleRes);
        }

        @Override
        public void requestChildFocus(View child, View focused) {
            super.requestChildFocus(child, focused);

            mChildFocusListener.childFocusChanged();
        }

        public void setChildFocusListener(Picker childFocusListener) {
            mChildFocusListener = childFocusListener;
        }
    }
}
