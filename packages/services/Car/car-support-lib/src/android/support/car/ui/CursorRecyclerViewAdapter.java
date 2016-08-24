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
import android.database.DataSetObserver;
import android.provider.BaseColumns;
import android.support.v7.widget.RecyclerView;

/**
 * Adapter that exposes data from a Cursor to a {@link RecyclerView} widget.
 * The Cursor must include an Id column or this class will not work.
 */
public abstract  class CursorRecyclerViewAdapter<VH extends RecyclerView.ViewHolder>
        extends RecyclerView.Adapter<VH> {

    protected Context mContext;
    protected Cursor mCursor;
    protected int mRowIdColumn;

    public CursorRecyclerViewAdapter(Context context, Cursor cursor) {
        mContext = context;
        mCursor = cursor;
        mRowIdColumn = -1;
        if (mCursor != null) {
            mRowIdColumn = getRowIdColumnIndex(mCursor);
            mCursor.registerDataSetObserver(mDataSetObserver);
        }
    }

    public Cursor getCursor() {
        return mCursor;
    }

    @Override
    public int getItemCount() {
        if (mCursor != null) {
            return mCursor.getCount();
        }
        return 0;
    }

    @Override
    public long getItemId(int position) {
        if (mCursor != null && mCursor.moveToPosition(position)) {
            return mCursor.getLong(mRowIdColumn);
        }
        return 0;
    }

    public void onBindViewHolder(VH viewHolder, Cursor cursor) {}

    @Override
    public void onBindViewHolder(VH viewHolder, int position) {
        if (!mCursor.moveToPosition(position)) {
            throw new IllegalStateException("can't move cursor to position " + position);
        }
        onBindViewHolder(viewHolder, mCursor);
    }

    /**
     * Change the underlying cursor to a new cursor. If there is an existing cursor it will
     * be closed.
     *
     * @param cursor The new cursor to be used.
     */
    public void changeCursor(Cursor cursor) {
        Cursor old = swapCursor(cursor);
        if (old != null) {
            old.close();
        }
    }

    /**
     * Swap in a new Cursor, returning the old Cursor. Unlike changeCursor(android.database.Cursor),
     * the returned old Cursor is not closed.
     *
     * @param newCursor The new cursor to be used.
     */
    public Cursor swapCursor(Cursor newCursor) {
        if (newCursor == mCursor) {
            return null;
        }

        Cursor oldCursor = mCursor;
        if (oldCursor != null) {
            if (mDataSetObserver != null) {
                oldCursor.unregisterDataSetObserver(mDataSetObserver);
            }
        }

        mCursor = newCursor;
        if (mCursor != null) {
            if (mDataSetObserver != null) {
                mCursor.registerDataSetObserver(mDataSetObserver);
            }
            mRowIdColumn = getRowIdColumnIndex(mCursor);
            notifyDataSetChanged();
        } else {
            mRowIdColumn = -1;
            notifyDataSetChanged();
        }
        return oldCursor;
    }

    protected int getRowIdColumnIndex(Cursor cursor) {
        return cursor.getColumnIndex(BaseColumns._ID);
    }

    protected DataSetObserver mDataSetObserver = new DataSetObserver() {
        @Override
        public void onChanged() {
            super.onChanged();
            notifyDataSetChanged();
        }

        @Override
        public void onInvalidated() {
            super.onInvalidated();
            mCursor = null;
            notifyDataSetChanged();
        }
    };
}
