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
package android.support.car.ui;

import android.content.Context;
import android.database.Cursor;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.List;

/**
 * Maintains a list that groups adjacent items sharing the same value of
 * a "group-by" field.  The list has three types of elements: stand-alone, group header and group
 * child. Groups are collapsible and collapsed by default.
 */
public abstract class GroupingCursorRecyclerViewAdapter<VH extends RecyclerView.ViewHolder>
        extends CursorRecyclerViewAdapter<VH> {
    private static final String TAG = "CAR.UI.GroupingCursorRecyclerViewAdapter";

    public static final int VIEW_TYPE_STANDALONE = 0;
    public static final int VIEW_TYPE_GROUP_HEADER = 1;
    public static final int VIEW_TYPE_IN_GROUP = 2;

    /**
     * Build all groups based on grouping rules given cursor and calls {@link #addGroup} for
     * each of them.
     */
    protected abstract void buildGroups(Cursor cursor);

    protected abstract VH onCreateStandAloneViewHolder(Context context, ViewGroup parent);
    protected abstract void onBindStandAloneViewHolder(VH holder, Context context, Cursor cursor);

    protected abstract VH onCreateGroupViewHolder(Context context, ViewGroup parent);
    protected abstract void onBindGroupViewHolder(VH holder, Context context, Cursor cursor,
                                                  int groupSize, boolean expanded);

    protected abstract VH onCreateChildViewHolder(Context context, ViewGroup parent);
    protected abstract void onBindChildViewHolder(VH holder, Context context, Cursor cursor);

    private int mCount;
    private List<GroupMetadata> mGroupMetadata;

    public GroupingCursorRecyclerViewAdapter(Context context, Cursor cursor) {
        super(context, cursor);
        mGroupMetadata = new ArrayList<>();
        resetGroup();
    }

    @Override
    public Cursor swapCursor(Cursor newCursor) {
        if (newCursor != mCursor) {
            resetGroup();
            if (newCursor != null) {
                buildGroups(newCursor);
                rebuildGroupMetadata();
            }
        }
        return super.swapCursor(newCursor);
    }

    @Override
    public int getItemCount() {
        if (mCursor != null && mCount != -1) {
            return mCount;
        }
        return 0;
    }

    @Override
    public long getItemId(int position) {
        Cursor cursor = getItem(position);
        if (cursor != null) {
            return cursor.getLong(mRowIdColumn);
        }
        return 0;
    }

    @Override
    public int getItemViewType(int position) {
        return getPositionMetadata(position).itemType;
    }

    public Cursor getItem(int position) {
        if (mCursor == null) {
            return null;
        }

        PositionMetadata pMetadata = getPositionMetadata(position);
        if (!mCursor.moveToPosition(pMetadata.cursorPosition)) {
            throw new IllegalStateException(
                    "can't move cursor to position " + pMetadata.cursorPosition);
        }
        return mCursor;
    }

    @Override
    public VH onCreateViewHolder(ViewGroup parent, int viewType) {
        switch (viewType) {
            case VIEW_TYPE_STANDALONE:
                return onCreateStandAloneViewHolder(mContext, parent);
            case VIEW_TYPE_GROUP_HEADER:
                return onCreateGroupViewHolder(mContext, parent);
            case VIEW_TYPE_IN_GROUP:
                return onCreateChildViewHolder(mContext, parent);
        }
        Log.e(TAG, "Unknown viewType. Returning null ViewHolder");
        return null;
    }

    @Override
    public void onBindViewHolder(VH holder, int position) {
        PositionMetadata pMetadata = getPositionMetadata(position);
        mCursor.moveToPosition(pMetadata.cursorPosition);
        switch (pMetadata.itemType) {
            case VIEW_TYPE_STANDALONE:
                onBindStandAloneViewHolder(holder, mContext, mCursor);
                break;
            case VIEW_TYPE_GROUP_HEADER:
                onBindGroupViewHolder(holder, mContext, mCursor, pMetadata.gMetadata.itemNumber,
                        pMetadata.gMetadata.isExpanded());
                break;
            case VIEW_TYPE_IN_GROUP:
                onBindChildViewHolder(holder, mContext, mCursor);
                break;
        }
    }

    public boolean toggleGroup(int position) {
        PositionMetadata pMetadata = getPositionMetadata(position);
        if (pMetadata.itemType != VIEW_TYPE_GROUP_HEADER) {
            return false;
        }

        pMetadata.gMetadata.isExpanded = !pMetadata.gMetadata.isExpanded;
        rebuildGroupMetadata();
        notifyDataSetChanged();
        return true;
    }

    /**
     * Records information about grouping in the list. Should only be called by the overridden
     * {@link #buildGroups} method.
     */
    protected void addGroup(int cursorPosition, int size, boolean expanded) {
        mGroupMetadata.add(GroupMetadata.obtain(cursorPosition, size, expanded));
    }

    private void resetGroup() {
        mCount = -1;
        mGroupMetadata.clear();
    }

    private void rebuildGroupMetadata() {
        int currentPos = 0;
        for (int groupIndex = 0; groupIndex < mGroupMetadata.size(); groupIndex++) {
            GroupMetadata gMetadata = mGroupMetadata.get(groupIndex);
            gMetadata.offsetInList = currentPos;
            currentPos += gMetadata.getActualSize();
        }
        mCount = currentPos;
    }

    private PositionMetadata getPositionMetadata(int position) {
        int left = 0;
        int right = mGroupMetadata.size() - 1;
        int mid;
        GroupMetadata midItem;

        while (left <= right) {
            mid = (right - left) / 2 + left;
            midItem = mGroupMetadata.get(mid);

            if (position > midItem.offsetInList + midItem.getActualSize() - 1) {
                left = mid + 1;
                continue;
            }

            if (position < midItem.offsetInList) {
                right = mid - 1;
                continue;
            }

            int cursorOffset = midItem.offsetInCursor + (position - midItem.offsetInList);
            int viewType;
            if (midItem.offsetInList == position) {
                if (midItem.isStandAlone()) {
                    viewType = VIEW_TYPE_STANDALONE;
                } else {
                    viewType = VIEW_TYPE_GROUP_HEADER;
                }
            } else {
                viewType = VIEW_TYPE_IN_GROUP;
                // Offset cursorOffset by 1, because the group_header and the first child
                // will share the same cursor.
                cursorOffset--;
            }
            return new PositionMetadata(viewType, cursorOffset, midItem);
        }

        throw new IllegalStateException(
                "illegal position " + position + ", total size is " + mCount);
    }

    /**
     * Information about where groups are located in the list, how large they are
     * and whether they are expanded.
     */
    protected static class GroupMetadata {
        private int offsetInList;
        private int offsetInCursor;
        private int itemNumber;
        private boolean isExpanded;

        static GroupMetadata obtain(int offset, int itemNumber, boolean isExpanded) {
            GroupMetadata gm = new GroupMetadata();
            gm.offsetInCursor = offset;
            gm.itemNumber = itemNumber;
            gm.isExpanded = isExpanded;
            return gm;
        }

        public boolean isExpanded() {
            return !isStandAlone() && isExpanded;
        }

        public boolean isStandAlone() {
            return itemNumber == 1;
        }

        public int getActualSize() {
            if (!isExpanded()) {
                return 1;
            } else {
                return itemNumber + 1;
            }
        }
    }

    protected static class PositionMetadata {
        int itemType;
        int cursorPosition;
        GroupMetadata gMetadata;

        public PositionMetadata(int itemType, int cursorPosition, GroupMetadata gMetadata) {
            this.itemType = itemType;
            this.cursorPosition = cursorPosition;
            this.gMetadata = gMetadata;
        }
    }
}
