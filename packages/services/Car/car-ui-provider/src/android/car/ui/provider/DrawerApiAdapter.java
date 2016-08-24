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
package android.car.ui.provider;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.support.car.ui.CarListItemViewHolder;
import android.support.car.ui.PagedListView;
import android.support.car.ui.R;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.RemoteViews;
import android.widget.TextView;

import android.support.car.app.menu.CarMenu;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static android.car.app.menu.CarMenuConstants.MenuItemConstants.FLAG_BROWSABLE;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.FLAG_FIRSTITEM;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_EMPTY_PLACEHOLDER;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_FLAGS;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_ID;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_LEFTICON;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_REMOTEVIEWS;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_RIGHTICON;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_RIGHTTEXT;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_TEXT;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_TITLE;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_WIDGET;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_WIDGET_STATE;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.WIDGET_CHECKBOX;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.WIDGET_TEXT_VIEW;

public class DrawerApiAdapter extends RecyclerView.Adapter<CarListItemViewHolder>
        implements PagedListView.ItemCap {
    private static final String TAG = "CAR.UI.ADAPTER";
    private static final String INDEX_OUT_OF_BOUNDS_MESSAGE = "invalid item position";
    private static final String KEY_ID_UNAVAILABLE_CATEGORY = "UNAVAILABLE_CATEGORY";

    public interface OnItemSelectedListener {
        void onItemClicked(Bundle item, int position);
        boolean onItemLongClicked(Bundle item);
    }

    private final Map<String, Integer> mIdToPosMap = new HashMap<>();

    private final Object mItemsLock = new Object();
    private List<Bundle> mItems;
    private boolean mIsCapped;
    private OnItemSelectedListener mListener;
    private int mMaxItems;
    private boolean mUseSmallHolder;
    private boolean mNoLeftIcon;
    private boolean mIsEmptyPlaceholder;
    private int mFirstItemIndex = 0;

    private final Handler mHandler = new Handler();

    public DrawerApiAdapter() {
        setHasStableIds(true);
    }

    @Override
    public int getItemViewType(int position) {
        Bundle item;
        try {
            item = mItems.get(position);
        } catch (IndexOutOfBoundsException e) {
            Log.w(TAG, INDEX_OUT_OF_BOUNDS_MESSAGE, e);
            return 0;
        }

        if (KEY_ID_UNAVAILABLE_CATEGORY.equals(item.getString(KEY_ID))) {
            return R.layout.car_unavailable_category;
        }

        if (item.containsKey(KEY_EMPTY_PLACEHOLDER) && item.getBoolean(KEY_EMPTY_PLACEHOLDER)) {
            return R.layout.car_list_item_empty;
        }

        int flags = item.getInt(KEY_FLAGS);
        if ((flags & FLAG_BROWSABLE) != 0 || item.containsKey(KEY_RIGHTICON)) {
            return R.layout.car_imageview;
        }

        if (!item.containsKey(KEY_WIDGET)) {
            return 0;
        }

        switch (item.getInt(KEY_WIDGET)) {
            case WIDGET_CHECKBOX:
                return R.layout.car_menu_checkbox;
            case WIDGET_TEXT_VIEW:
                return R.layout.car_textview;
            default:
                return 0;
        }
    }

    @Override
    public CarListItemViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        View view;
        if (viewType == R.layout.car_unavailable_category ||
                viewType == R.layout.car_list_item_empty) {
            view = inflater.inflate(viewType, parent, false);
        } else {
            view = inflater.inflate(R.layout.car_menu_list_item, parent, false);
        }
        return new CarListItemViewHolder(view, viewType);
    }

    @Override
    public void setMaxItems(int maxItems) {
        mMaxItems = maxItems;
    }

    @Override
    public void onBindViewHolder(final CarListItemViewHolder holder, final int position) {
        if (holder.getItemViewType() == R.layout.car_list_item_empty) {
            onBindEmptyPlaceHolder(holder, position);
        } else if (holder.getItemViewType() == R.layout.car_unavailable_category) {
            onBindUnavailableCategoryView(holder);
        } else {
            onBindNormalView(holder, position);
            if (mIsCapped) {
                // Disable all menu items if it is under unavailable category case.
                // TODO(b/24163545): holder.itemView.setAlpha() doesn't work all the time,
                // which makes some items are gray out, the others are not.
                setHolderStatus(holder, false, 0.3f);
            } else {
                setHolderStatus(holder, true, 1.0f);
            }
        }

        holder.itemView.setTag(position);
        holder.itemView.setOnClickListener(mOnClickListener);
        holder.itemView.setOnLongClickListener(mOnLongClickListener);

        // Ensure correct day/night mode colors are set and not out of sync.
        setDayNightModeColors(holder);
    }

    @Override
    public int getItemCount() {
        synchronized (mItemsLock) {
            if (mItems != null) {
                return mMaxItems >= 0 ? Math.min(mItems.size(), mMaxItems) : mItems.size();
            }
        }
        return 0;
    }

    @Override
    public long getItemId(int position) {
        synchronized (mItemsLock) {
            if (mItems != null) {
                try {
                    return mItems.get(position).getString(KEY_ID).hashCode();
                } catch (IndexOutOfBoundsException e) {
                    Log.w(TAG, "invalid item index", e);
                    return RecyclerView.NO_ID;
                }
            }
        }
        return super.getItemId(position);
    }

    public synchronized void setItems(List<Bundle> items, boolean isCapped) {
        synchronized (mItemsLock) {
            mItems = items;
        }
        mIsCapped = isCapped;
        mFirstItemIndex = 0;
        if (mItems != null) {
            mIdToPosMap.clear();
            mUseSmallHolder = true;
            mNoLeftIcon = true;
            mIsEmptyPlaceholder = false;
            int index = 0;
            for (Bundle bundle : items) {
                if (bundle.containsKey(KEY_EMPTY_PLACEHOLDER)
                        && bundle.getBoolean(KEY_EMPTY_PLACEHOLDER)) {
                    mIsEmptyPlaceholder = true;
                    if (items.size() != 1) {
                        throw new IllegalStateException("Empty placeholder should be the only"
                                + "item showing in the menu list!");
                    }
                }

                if (bundle.containsKey(KEY_TEXT) || bundle.containsKey(KEY_REMOTEVIEWS)) {
                    mUseSmallHolder = false;
                }
                if (bundle.containsKey(KEY_LEFTICON)) {
                    mNoLeftIcon = false;
                }
                if (bundle.containsKey(KEY_FLAGS) &&
                        (bundle.getInt(KEY_FLAGS) & FLAG_FIRSTITEM) != 0) {
                    mFirstItemIndex = index;
                }
                mIdToPosMap.put(bundle.getString(KEY_ID), index);
                index++;
            }
        }
        notifyDataSetChanged();
    }

    public int getMaxItemsNumber() {
        return mMaxItems;
    }

    public void setItemSelectedListener(OnItemSelectedListener listener) {
        mListener = listener;
    }

    public int getFirstItemIndex() {
        return mFirstItemIndex;
    }

    public boolean isEmptyPlaceholder() {
        return mIsEmptyPlaceholder;
    }

    public void onChildChanged(RecyclerView.ViewHolder holder, Bundle bundle) {
        synchronized (mItemsLock) {
            // The holder will be null if the view has not been bound yet
            if (holder != null) {
                int position = holder.getAdapterPosition();
                if (position >= 0 && mItems != null && position < mItems.size()) {
                    final Bundle oldBundle;
                    try {
                        oldBundle = mItems.get(position);
                    } catch (IndexOutOfBoundsException e) {
                        Log.w(TAG, INDEX_OUT_OF_BOUNDS_MESSAGE, e);
                        return;
                    }
                    oldBundle.putAll(bundle);
                    notifyItemChanged(position);
                }
            } else {
                String id = bundle.getString(KEY_ID);
                int position = mIdToPosMap.get(id);
                if (position >= 0 && mItems != null && position < mItems.size()) {
                    final Bundle item;
                    try {
                        item = mItems.get(position);
                    } catch (IndexOutOfBoundsException e) {
                        Log.w(TAG, INDEX_OUT_OF_BOUNDS_MESSAGE, e);
                        return;
                    }
                    if (id.equals(item.getString(KEY_ID))) {
                        item.putAll(bundle);
                        notifyItemChanged(position);
                    }
                }
            }
        }
    }

    public void setDayNightModeColors(RecyclerView.ViewHolder viewHolder) {
        CarListItemViewHolder holder = (CarListItemViewHolder) viewHolder;
        Context context = holder.itemView.getContext();
        holder.itemView.setBackgroundResource(R.drawable.car_list_item_background);
        if (holder.getItemViewType() == R.layout.car_unavailable_category) {
            holder.title.setTextAppearance(context, R.style.CarUnavailableCategory);
            if (holder.text != null) {
                holder.text.setTextAppearance(context, R.style.CarUnavailableCategory);
            }
            holder.icon.setImageTintList(ColorStateList
                    .valueOf(context.getResources().getColor(R.color.car_unavailable_category)));
        } else {
            holder.title.setTextAppearance(context, R.style.CarBody1);
            if (holder.text != null) {
                holder.text.setTextAppearance(context, R.style.CarBody2);
            }
            if (holder.rightCheckbox != null) {
                holder.rightCheckbox.setButtonTintList(
                        ColorStateList.valueOf(context.getResources().getColor(R.color.car_tint)));
            } else if (holder.rightImage != null) {
                Object tag = holder.rightImage.getTag();
                if (tag != null && (int) tag != -1) {
                    holder.rightImage.setImageResource((int) tag);
                }
            }
        }
    }

    private void onBindEmptyPlaceHolder(final CarListItemViewHolder holder, final int position) {
        maybeSetText(position, KEY_TITLE, holder.title);
        if (!mNoLeftIcon) {
            maybeSetBitmap(position, KEY_LEFTICON, holder.icon);
            holder.iconContainer.setVisibility(View.VISIBLE);
        } else {
            holder.iconContainer.setVisibility(View.GONE);
        }
    }

    private void onBindUnavailableCategoryView(final CarListItemViewHolder holder) {
        mNoLeftIcon = false;
        holder.itemView.setEnabled(false);
    }

    private void onBindNormalView(final CarListItemViewHolder holder, final int position) {
        maybeSetText(position, KEY_TITLE, holder.title);
        maybeSetText(position, KEY_TEXT, holder.text);
        final Bundle item;
        try {
            item = new Bundle(mItems.get(position));
        } catch (IndexOutOfBoundsException e) {
            Log.w(TAG, INDEX_OUT_OF_BOUNDS_MESSAGE, e);
            return;
        }
        final int flags = item.getInt(KEY_FLAGS);
        if ((flags & FLAG_BROWSABLE) != 0) {
            // Set the resource id as the tag so we can reload it on day/night mode change.
            // If the tag is -1 or not set, then assume the app will send an updated bitmap
            holder.rightImage.setTag(R.drawable.ic_chevron_right);
            holder.rightImage.setImageResource(R.drawable.ic_chevron_right);
        } else if (holder.rightImage  != null) {
            maybeSetBitmap(position, KEY_RIGHTICON, holder.rightImage);
        }

        if (holder.rightCheckbox != null) {
            holder.rightCheckbox.setChecked(item.getBoolean(
                    KEY_WIDGET_STATE, false));
            holder.rightCheckbox.setOnClickListener(mOnClickListener);
            holder.rightCheckbox.setTag(position);
        }
        if (holder.rightText != null) {
            maybeSetText(position, KEY_RIGHTTEXT, holder.rightText);
        }
        if (!mNoLeftIcon) {
            maybeSetBitmap(position, KEY_LEFTICON, holder.icon);
            holder.iconContainer.setVisibility(View.VISIBLE);
        } else {
            holder.iconContainer.setVisibility(View.GONE);
        }
        if (item.containsKey(KEY_REMOTEVIEWS)) {
            holder.remoteViewsContainer.setVisibility(View.VISIBLE);
            RemoteViews views = item.getParcelable(KEY_REMOTEVIEWS);
            View view = views.apply(holder.remoteViewsContainer.getContext(),
                    holder.remoteViewsContainer);
            holder.remoteViewsContainer.removeAllViews();
            holder.remoteViewsContainer.addView(view);
        } else {
            holder.remoteViewsContainer.removeAllViews();
            holder.remoteViewsContainer.setVisibility(View.GONE);
        }

        // Set the view holder size
        Resources r = holder.itemView.getResources();
        ViewGroup.LayoutParams params = holder.itemView.getLayoutParams();
        params.height = mUseSmallHolder ?
                r.getDimensionPixelSize(R.dimen.car_list_item_height_small) :
                r.getDimensionPixelSize(R.dimen.car_list_item_height);
        holder.itemView.setLayoutParams(params);

        // Set Icon size
        params = holder.iconContainer.getLayoutParams();
        params.height = params.width = mUseSmallHolder ?
                r.getDimensionPixelSize(R.dimen.car_list_item_small_icon_size) :
                r.getDimensionPixelSize(R.dimen.car_list_item_icon_size);

    }

    private void maybeSetText(int position, String key, TextView view) {
        Bundle item;
        try {
            item = mItems.get(position);
        } catch (IndexOutOfBoundsException e) {
            Log.w(TAG, INDEX_OUT_OF_BOUNDS_MESSAGE, e);
            return;
        }
        if (item.containsKey(key)) {
            view.setText(item.getString(key));
            view.setVisibility(View.VISIBLE);
        } else {
            view.setVisibility(View.GONE);
        }
    }

    private void maybeSetBitmap(int position, String key, ImageView view) {
        Bundle item;
        try {
            item = mItems.get(position);
        } catch (IndexOutOfBoundsException e) {
            Log.w(TAG, INDEX_OUT_OF_BOUNDS_MESSAGE, e);
            return;
        }
        if (item.containsKey(key)) {
            view.setImageBitmap((Bitmap) item.getParcelable(key));
            view.setVisibility(View.VISIBLE);
            view.setTag(-1);
        } else {
            view.setVisibility(View.GONE);
        }
    }

    private void setHolderStatus(final CarListItemViewHolder holder,
            boolean isEnabled, float alpha) {
        holder.itemView.setEnabled(isEnabled);
        if (holder.icon != null) {
            holder.icon.setAlpha(alpha);
        }
        if (holder.title != null) {
            holder.title.setAlpha(alpha);
        }
        if (holder.text != null) {
            holder.text.setAlpha(alpha);
        }
        if (holder.rightCheckbox != null) {
            holder.rightCheckbox.setAlpha(alpha);
        }
        if (holder.rightImage != null) {
            holder.rightImage.setAlpha(alpha);
        }
        if (holder.rightText != null) {
            holder.rightText.setAlpha(alpha);
        }
    }

    private final View.OnClickListener mOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            final Bundle item;
            int position = (int) view.getTag();
            try {
                item = mItems.get(position);
            } catch (IndexOutOfBoundsException e) {
                Log.w(TAG, INDEX_OUT_OF_BOUNDS_MESSAGE, e);
                return;
            }
            View right = view.findViewById(R.id.right_item);
            if (right != null && view != right && right instanceof CompoundButton) {
                ((CompoundButton) right).toggle();
            }
            if (mListener != null) {
                mListener.onItemClicked(item, position);
            }
        }
    };

    private final View.OnLongClickListener mOnLongClickListener = new View.OnLongClickListener() {
        @Override
        public boolean onLongClick(View view) {
            final Bundle item;
            try {
                item = mItems.get((int) view.getTag());
            } catch (IndexOutOfBoundsException e) {
                Log.w(TAG, INDEX_OUT_OF_BOUNDS_MESSAGE, e);
                return true;
            }
            final String id = item.getString(KEY_ID);
            if (mListener != null) {
                return mListener.onItemLongClicked(item);
            }
            return false;
        }
    };
}
