/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tv.dvr.ui;

import android.support.v17.leanback.widget.ObjectAdapter;
import android.support.v17.leanback.widget.PresenterSelector;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * Keeps a set of {@code T} items sorted, but leaving a {@link EmptyHolder}
 * if there is no items.
 *
 * <p>{@code T} must have stable IDs.
 */
abstract class SortedArrayAdapter<T> extends ObjectAdapter {
    private final List<T> mItems = new ArrayList<>();
    private final Comparator<T> mComparator;

    SortedArrayAdapter(PresenterSelector presenterSelector, Comparator<T> comparator) {
        super(presenterSelector);
        mComparator = comparator;
        setHasStableIds(true);
    }

    @Override
    public final int size() {
        return mItems.isEmpty() ? 1 : mItems.size();
    }

    @Override
    public final Object get(int position) {
        return isEmpty() ? EmptyHolder.EMPTY_HOLDER : getItem(position);
    }

    @Override
    public final long getId(int position) {
        if (isEmpty()) {
            return NO_ID;
        }
        T item = mItems.get(position);
        return item == null ? NO_ID : getId(item);
    }

    /**
     * Returns the id of the the given {@code item}.
     *
     * The id must be stable.
     */
    abstract long getId(T item);

    /**
     * Returns the item at the given {@code position}.
     *
     * @throws IndexOutOfBoundsException if the position is out of range
     *         (<tt>position &lt; 0 || position &gt;= size()</tt>)
     */
    final T getItem(int position) {
        return mItems.get(position);
    }

    /**
     * Returns {@code true} if the list of items is empty.
     *
     * <p><b>NOTE</b> when the item list is empty the adapter has a size of 1 and
     * {@link EmptyHolder#EMPTY_HOLDER} at position 0;
     */
    final boolean isEmpty() {
        return mItems.isEmpty();
    }

    /**
     * Removes all elements from the list.
     *
     * <p><b>NOTE</b> when the item list is empty the adapter has a size of 1 and
     * {@link EmptyHolder#EMPTY_HOLDER} at position 0;
     */
    final void clear() {
        mItems.clear();
        notifyChanged();
    }

    /**
     * Adds the objects in the given collection to the adapter keeping the elements sorted.
     * If the index is >= {@link #size} an exception will be thrown.
     *
     * @param items A {@link Collection} of items to insert.
     */
    final void addAll(Collection<T> items) {
        mItems.addAll(items);
        Collections.sort(mItems, mComparator);
        notifyChanged();
    }

    /**
     * Adds an item in sorted order to the adapter.
     *
     * @param item The item to add in sorted order to the adapter.
     */
    final void add(T item) {
        int i = findWhereToInsert(item);
        mItems.add(i, item);
        if (mItems.size() == 1) {
            notifyItemRangeChanged(0, 1);
        } else {
            notifyItemRangeInserted(i, 1);
        }
    }

    /**
     * Remove an item from the list
     *
     * @param item The item to remove from the adapter.
     */
    final void remove(T item) {
        int index = indexOf(item);
        if (index != -1) {
            mItems.remove(index);
            if (mItems.isEmpty()) {
                notifyItemRangeChanged(0, 1);
            } else {
                notifyItemRangeRemoved(index, 1);
            }
        }
    }

    /**
     * Change an item in the list.
     * @param item The item to change.
     */
    final void change(T item) {
        int oldIndex = indexOf(item);
        if (oldIndex != -1) {
            T old = mItems.get(oldIndex);
            if (mComparator.compare(old, item) == 0) {
                mItems.set(oldIndex, item);
                notifyItemRangeChanged(oldIndex, 1);
                return;
            }
            mItems.remove(oldIndex);
        }
        int newIndex = findWhereToInsert(item);
        mItems.add(newIndex, item);

        if (oldIndex != -1) {
            notifyItemRangeRemoved(oldIndex, 1);
        }
        if (newIndex != -1) {
            notifyItemRangeInserted(newIndex, 1);
        }
    }

    private int indexOf(T item) {
        long id = getId(item);
        for (int i = 0; i < mItems.size(); i++) {
            T r = mItems.get(i);
            if (getId(r) == id) {
                return i;
            }
        }
        return -1;
    }

    private int findWhereToInsert(T item) {
        int i;
        int size = mItems.size();
        for (i = 0; i < size; i++) {
            T r = mItems.get(i);
            if (mComparator.compare(r, item) > 0) {
                return i;
            }
        }
        return size;
    }
}
