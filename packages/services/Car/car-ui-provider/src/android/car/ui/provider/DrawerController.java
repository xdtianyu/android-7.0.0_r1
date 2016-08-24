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

import android.car.app.menu.CarMenuCallbacks;
import android.content.Context;
import android.graphics.Canvas;
import android.os.Bundle;
import android.support.car.ui.PagedListView;
import android.support.car.ui.R;
import android.support.v7.widget.CardView;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ProgressBar;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
import java.util.Stack;

import static android.car.app.menu.CarMenuConstants.MenuItemConstants.FLAG_BROWSABLE;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_FLAGS;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_ID;
import static android.car.app.menu.CarMenuConstants.MenuItemConstants.KEY_TITLE;

/**
 * Controls the drawer for SDK app
 */
public class DrawerController
        implements CarDrawerLayout.DrawerListener, DrawerApiAdapter.OnItemSelectedListener,
        CarDrawerLayout.DrawerControllerListener {
    private static final String TAG = "CAR.UI.DrawerController";
    // Qualify with full package name to make it less likely there will be a collision
    private static final String KEY_IDS = "android.support.car.ui.drawer.sdk.IDS";
    private static final String KEY_DRAWERSTATE =
            "android.support.car.ui.drawer.sdk.DRAWER_STATE";
    private static final String KEY_TITLES = "android.support.car.ui.drawer.sdk.TITLES";
    private static final String KEY_ROOT = "android.support.car.ui.drawer.sdk.ROOT";
    private static final String KEY_ID_UNAVAILABLE_CATEGORY = "UNAVAILABLE_CATEGORY";
    private static final String KEY_CLICK_STACK =
            "android.support.car.ui.drawer.sdk.CLICK_STACK";
    private static final String KEY_MAX_PAGES =
            "android.support.car.ui.drawer.sdk.MAX_PAGES";
    private static final String KEY_IS_CAPPED =
            "android.support.car.ui.drawer.sdk.IS_CAPPED";

    /** Drawer is in Auto dark/light mode */
    private static final int MODE_AUTO = 0;
    /** Drawer is in Light mode */
    private static final int MODE_LIGHT = 1;
    /** Drawer is in Dark mode */
    private static final int MODE_DARK = 2;

    private final Stack<String> mSubscriptionIds = new Stack<>();
    private final Stack<CharSequence> mTitles = new Stack<>();
    private final SubscriptionCallbacks mSubscriptionCallbacks = new SubscriptionCallbacks();
    // Named to be consistent with CarDrawerFragment to make copying code easier and less error
    // prone
    private final CarDrawerLayout mContainer;
    private final PagedListView mListView;
//    private final CardView mTruncatedListCardView;
    private final ProgressBar mProgressBar;
    private final Context mContext;
    private final ViewAnimationController mPlvAnimationController;
    private final CardView mTruncatedListCardView;
    private final Stack<Integer> mClickCountStack = new Stack<>();

    private CarMenuCallbacks mCarMenuCallbacks;
    private DrawerApiAdapter mAdapter;
    private int mScrimColor = CarDrawerLayout.DEFAULT_SCRIM_COLOR;
    private boolean mIsDrawerOpen;
    private boolean mIsDrawerAnimating;
    private boolean mIsCapped;
    private int mItemsNumber;
    private int mDrawerMode;
    private CharSequence mContentTitle;
    private String mRootId;
    private boolean mRestartedFromDayNightMode;
    private CarUiEntry mUiEntry;

    public DrawerController(CarUiEntry uiEntry, View menuButton, CarDrawerLayout drawerLayout,
                            PagedListView listView, CardView cardView) {
        //mCarAppLayout = appLayout;
        menuButton.setOnClickListener(mMenuClickListener);
        mContainer = drawerLayout;
        mListView = listView;
        mUiEntry = uiEntry;
        mTruncatedListCardView = cardView;
        mListView.setDefaultItemDecoration(new DrawerMenuListDecoration(mListView.getContext()));
        mProgressBar = (ProgressBar) mContainer.findViewById(R.id.progress);
        mContext = mListView.getContext();
        mPlvAnimationController = new ViewAnimationController(
                mListView, R.anim.car_list_in, R.anim.sdk_list_out, R.anim.car_list_pop_out);
        mRootId = null;

        mContainer.setDrawerListener(this);
        mContainer.setDrawerControllerListener(this);
        setAutoLightDarkMode();
    }


    @Override
    public void onDrawerOpened(View drawerView) {
        mIsDrawerOpen = true;
        mIsDrawerAnimating = false;
        mUiEntry.setMenuProgress(1.0f);
        // This can be null on day/night mode changes
        if (mCarMenuCallbacks != null) {
            mCarMenuCallbacks.onCarMenuOpened();
        }
    }

    @Override
    public void onDrawerClosed(View drawerView) {
        mIsDrawerOpen = false;
        mIsDrawerAnimating = false;
        clearMenu();
        mUiEntry.setMenuProgress(0);
        mUiEntry.setTitle(mContentTitle);
        // This can be null on day/night mode changes
        if (mCarMenuCallbacks != null) {
            mCarMenuCallbacks.onCarMenuClosed();
        }
    }

    @Override
    public void onDrawerStateChanged(int newState) {
    }

    @Override
    public void onDrawerOpening(View drawerView) {
        mIsDrawerAnimating = true;
        // This can be null on day/night mode changes
        if (mCarMenuCallbacks != null) {
            mCarMenuCallbacks.onCarMenuOpening();
        }
    }

    @Override
    public void onDrawerSlide(View drawerView, float slideOffset) {
        mUiEntry.setMenuProgress(slideOffset);
    }

    @Override
    public void onDrawerClosing(View drawerView) {
        mIsDrawerAnimating = true;
        // This can be null on day/night mode changes
        if (mCarMenuCallbacks != null) {
            mCarMenuCallbacks.onCarMenuClosing();
        }
    }

    @Override
    public void onItemClicked(Bundle item, int position) {
        // Don't allow selection while animating
        if (mPlvAnimationController.isAnimating()) {
            return;
        }
        int flags = item.getInt(KEY_FLAGS);
        String id = item.getString(KEY_ID);

        // Page number is 0 index, + 1 for the actual click.
        int clicksUsed = mListView.getPage(position) + 1;
        mClickCountStack.push(clicksUsed);
        mListView.setMaxPages(mListView.getMaxPages() - clicksUsed);
        mCarMenuCallbacks.onItemClicked(id);
        if ((flags & FLAG_BROWSABLE) != 0) {
            if (mListView.getMaxPages() == 0) {
                mIsCapped = true;
            }
            CharSequence title = item.getString(KEY_TITLE);
            if (TextUtils.isEmpty(title)) {
                title = mContentTitle;
            }
            mUiEntry.setTitleText(title);
            mTitles.push(title);
            if (!mSubscriptionIds.isEmpty()) {
                mPlvAnimationController.enqueueExitAnimation(mClearAdapterRunnable);
            }
            mProgressBar.setVisibility(View.VISIBLE);
            if (!mSubscriptionIds.isEmpty()) {
                mCarMenuCallbacks.unsubscribe(mSubscriptionIds.peek(), mSubscriptionCallbacks);
            }
            mSubscriptionIds.push(id);
            subscribe(id);
        } else {
            closeDrawer();
        }
    }

    @Override
    public boolean onItemLongClicked(Bundle item) {
        return mCarMenuCallbacks.onItemLongClicked(item.getString(KEY_ID));
    }

    @Override
    public void onBack() {
        backOrClose();
    }

    @Override
    public boolean onScroll() {
        // Consume scroll event if we are animating.
        return mPlvAnimationController.isAnimating();
    }

    public void setTitle(CharSequence title) {
        Log.d(TAG, "setTitle in drawer" + title);
        if (!TextUtils.isEmpty(title)) {
            mContentTitle = title;
            mUiEntry.showTitle();
            mUiEntry.setTitleText(title);
        } else {
            mUiEntry.hideTitle();
        }
    }

    public void setRootAndCallbacks(String rootId, CarMenuCallbacks callbacks) {
        mAdapter = new DrawerApiAdapter();
        mAdapter.setItemSelectedListener(this);
        mListView.setAdapter(mAdapter);
        mCarMenuCallbacks = callbacks;
        // HACK: Due to the handler, setRootId will be called after onRestoreState.
        // If onRestoreState has been called, the root id will already be set. So nothing to do.
        if (mSubscriptionIds.isEmpty()) {
            setRootId(rootId);
        } else {
            subscribe(mSubscriptionIds.peek());
            openDrawer();
        }
    }

    public void saveState(Bundle out) {
        out.putStringArray(KEY_IDS, mSubscriptionIds.toArray(new String[mSubscriptionIds.size()]));
        out.putStringArray(KEY_TITLES, mTitles.toArray(new String[mTitles.size()]));
        out.putString(KEY_ROOT, mRootId);
        out.putBoolean(KEY_DRAWERSTATE, mIsDrawerOpen);
        out.putIntegerArrayList(KEY_CLICK_STACK, new ArrayList<Integer>(mClickCountStack));
        out.putBoolean(KEY_IS_CAPPED, mIsCapped);
        out.putInt(KEY_MAX_PAGES, mListView.getMaxPages());
    }

    public void restoreState(Bundle in) {
        if (in != null) {
            // Restore subscribed CarMenu ids
            String[] ids = in.getStringArray(KEY_IDS);
            mSubscriptionIds.clear();
            if (ids != null) {
                mSubscriptionIds.addAll(Arrays.asList(ids));
            }
            // Restore drawer titles if there are any
            String[] titles = in.getStringArray(KEY_TITLES);
            mTitles.clear();
            if (titles != null) {
                mTitles.addAll(Arrays.asList(titles));
            }
            if (!mTitles.isEmpty()) {
                mUiEntry.setTitleText(mTitles.peek());
            }
            mRootId = in.getString(KEY_ROOT);
            mIsDrawerOpen = in.getBoolean(KEY_DRAWERSTATE);
            ArrayList<Integer> clickCount = in.getIntegerArrayList(KEY_CLICK_STACK);
            mClickCountStack.clear();
            if (clickCount != null) {
                mClickCountStack.addAll(clickCount);
            }
            mIsCapped = in.getBoolean(KEY_IS_CAPPED);
            mListView.setMaxPages(in.getInt(KEY_MAX_PAGES));
            if (!mRestartedFromDayNightMode && mIsDrawerOpen) {
                closeDrawer();
            }
        }
    }

    public void setScrimColor(int color) {
        mScrimColor = color;
        mContainer.setScrimColor(color);
        updateViewFaders();
    }

    public void setAutoLightDarkMode() {
        mDrawerMode = MODE_AUTO;
        mContainer.setAutoDayNightMode();
        updateViewFaders();
    }

    public void setLightMode() {
        mDrawerMode = MODE_LIGHT;
        mContainer.setLightMode();
        updateViewFaders();
    }

    public void setDarkMode() {
        mDrawerMode = MODE_DARK;
        mContainer.setDarkMode();
        updateViewFaders();
    }

    public void openDrawer() {
        // If we have no root, then we can't open the drawer.
        if (mRootId == null) {
            return;
        }
        mContainer.openDrawer();
    }

    public void closeDrawer() {
        if (mRootId == null) {
            return;
        }
        mTruncatedListCardView.setVisibility(View.GONE);
        mPlvAnimationController.stopAndClearAnimations();
        mContainer.closeDrawer();
        mUiEntry.setTitle(mContentTitle);
    }

    public void setDrawerEnabled(boolean enabled) {
        if (enabled) {
            mContainer.setDrawerLockMode(CarDrawerLayout.LOCK_MODE_UNLOCKED);
        } else {
            mContainer.setDrawerLockMode(CarDrawerLayout.LOCK_MODE_LOCKED_CLOSED);
        }
    }

    public void showMenu(String id, String title) {
        // The app wants to show the menu associated with the given id. Create a fake item using the
        // given inputs and then pretend as if the user clicked on the item, so that the drawer
        // will subscribe to that menu id, set the title appropriately, and properly handle the
        // subscription stack.
        Bundle bundle = new Bundle();
        bundle.putString(KEY_ID, id);
        bundle.putString(KEY_TITLE, title);
        bundle.putInt(KEY_FLAGS, FLAG_BROWSABLE);
        onItemClicked(bundle, 0 /* position */);
    }

    public void setRootId(String rootId) {
        mRootId = rootId;
    }

    public void setRestartedFromDayNightMode(boolean restarted) {
        mRestartedFromDayNightMode = restarted;
    }

    public boolean isTruncatedList() {
        return mItemsNumber > mAdapter.getMaxItemsNumber();
    }

    private void clearMenu() {
        if (!mSubscriptionIds.isEmpty()) {
            mCarMenuCallbacks.unsubscribe(mSubscriptionIds.peek(), mSubscriptionCallbacks);
            mSubscriptionIds.clear();
            mTitles.clear();
        }
        mListView.setVisibility(View.GONE);
        mListView.resetMaxPages();
        mClickCountStack.clear();
        mIsCapped = false;
    }

    /**
     * Check if the drawer is inside of a CarAppLayout and add the relevant views if it is,
     * automagically add view faders for the correct views
     */
    private void updateViewFaders() {
        mContainer.removeViewFader(mStatusViewViewFader);
        mContainer.addViewFader(mStatusViewViewFader);
    }

    private void subscribe(String id) {
        mProgressBar.setVisibility(View.VISIBLE);
        mCarMenuCallbacks.subscribe(id, mSubscriptionCallbacks);
    }

    private final CarDrawerLayout.ViewFader mStatusViewViewFader = new CarDrawerLayout.ViewFader() {
        @Override
        public void setColor(int color) {
            mUiEntry.setMenuButtonColor(color);
        }
    };

    private void backOrClose() {
        if (mSubscriptionIds.size() > 1) {
            mPlvAnimationController.enqueueBackAnimation(mClearAdapterRunnable);
            mProgressBar.setVisibility(View.VISIBLE);
            mCarMenuCallbacks.unsubscribe(mSubscriptionIds.pop(),
                    mSubscriptionCallbacks);
            subscribe(mSubscriptionIds.peek());
            // Restore the title for this menu level.
            mTitles.pop();
            CharSequence title = mTitles.peek();
            if (TextUtils.isEmpty(title)) {
                title = mContentTitle;
            }
            mUiEntry.setTitleText(title);
        } else {
            closeDrawer();
        }
    }

    private final View.OnClickListener mMenuClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            if (mIsDrawerAnimating || mCarMenuCallbacks.onMenuClicked()) {
                return;
            }
            // Check if drawer has root set.
            if (mRootId == null) {
                return;
            }
            mTruncatedListCardView.setVisibility(View.GONE);
            if (mIsDrawerOpen) {
                if (!mClickCountStack.isEmpty()) {
                    mListView.setMaxPages(mListView.getMaxPages() + mClickCountStack.pop());
                }
                mIsCapped = false;
                backOrClose();
            } else {
                mSubscriptionIds.push(mRootId);
                mTitles.push(mContentTitle);
                subscribe(mRootId);
                openDrawer();
            }
        }
    };

    private final Runnable mClearAdapterRunnable = new Runnable() {
        @Override
        public void run() {
            mListView.setVisibility(View.GONE);
        }
    };

    public void updateDayNightMode() {
        mContainer.findViewById(R.id.drawer).setBackgroundColor(
                mContext.getResources().getColor(R.color.car_card));
        mListView.setAutoDayNightMode();
        switch (mDrawerMode) {
            case MODE_AUTO:
                setAutoLightDarkMode();
                break;
            case MODE_LIGHT:
                setLightMode();
                break;
            case MODE_DARK:
                setDarkMode();
                break;
        }
        updateViewFaders();
        RecyclerView rv = mListView.getRecyclerView();
        for (int i = 0; i < mAdapter.getItemCount(); ++i) {
            mAdapter.setDayNightModeColors(rv.findViewHolderForAdapterPosition(i));
        }
    }

    private static class ViewAnimationController implements Animation.AnimationListener {
        private final Animation mExitAnim;
        private final Animation mEnterAnim;
        private final Animation mBackAnim;
        private final View mView;
        private final Context mContext;
        private final Queue<Animation> mQueue = new LinkedList<>();

        private Runnable mOnEnterAnimStartRunnable;
        private Runnable mOnExitAnimCompleteRunnable;

        private Animation mCurrentAnimation;

        public ViewAnimationController(View view, int enter, int exit, int back) {
            mView = view;
            mContext = view.getContext();

            mEnterAnim = AnimationUtils.loadAnimation(mContext, enter);
            mExitAnim = AnimationUtils.loadAnimation(mContext, exit);
            mBackAnim = AnimationUtils.loadAnimation(mContext, back);

            mExitAnim.setAnimationListener(this);
            mEnterAnim.setAnimationListener(this);
            mBackAnim.setAnimationListener(this);
        }

        @Override
        public void onAnimationStart(Animation animation) {
            if (animation == mEnterAnim && mOnEnterAnimStartRunnable != null) {
                mOnEnterAnimStartRunnable.run();
                mOnEnterAnimStartRunnable = null;
            }
        }

        @Override
        public  void onAnimationEnd(Animation animation) {
            if ((animation == mExitAnim || animation == mBackAnim)
                    && mOnExitAnimCompleteRunnable != null) {
                mOnExitAnimCompleteRunnable.run();
                mOnExitAnimCompleteRunnable = null;
            }
            Animation nextAnimation = mQueue.poll();
            if (nextAnimation != null) {
                mCurrentAnimation = animation;
                mView.startAnimation(nextAnimation);
            } else {
                mCurrentAnimation = null;
            }
       }

        @Override
        public void onAnimationRepeat(Animation animation) {

        }

        public void enqueueEnterAnimation(Runnable r) {
            if (r != null) {
                mOnEnterAnimStartRunnable = r;
            }
            enqueueAnimation(mEnterAnim);
        }

        public void enqueueExitAnimation(Runnable r) {
            // If the view isn't visible, don't play the exit animation.
            // It will cause flicker.
            if (mView.getVisibility() != View.VISIBLE) {
                return;
            }
            if (r != null) {
                mOnExitAnimCompleteRunnable = r;
            }
            enqueueAnimation(mExitAnim);
        }

        public void enqueueBackAnimation(Runnable r) {
            // If the view isn't visible, don't play the back animation.
            if (mView.getVisibility() != View.VISIBLE) {
                return;
            }
            if (r != null) {
                mOnExitAnimCompleteRunnable = r;
            }
            enqueueAnimation(mBackAnim);
        }

        public synchronized void stopAndClearAnimations() {
            if (mExitAnim.hasStarted()) {
                mExitAnim.cancel();
            }

            if (mEnterAnim.hasStarted()) {
                mEnterAnim.cancel();
            }

            mQueue.clear();
            mCurrentAnimation = null;
        }

        public boolean isAnimating() {
            return mCurrentAnimation != null;
        }

        private synchronized void enqueueAnimation(final Animation animation) {
            if (mQueue.contains(animation)) {
                return;
            }
            if (mCurrentAnimation != null) {
                mQueue.add(animation);
            } else {
                mCurrentAnimation = animation;
                mView.startAnimation(animation);
            }
        }
    }

    private class SubscriptionCallbacks extends android.car.app.menu.SubscriptionCallbacks {
        private final Object mItemLock = new Object();
        private volatile List<Bundle> mItems;

        @Override
        public void onChildrenLoaded(String parentId, final List<Bundle> items) {
            if (mSubscriptionIds.isEmpty() || parentId.equals(mSubscriptionIds.peek())) {
                // Add unavailable category explanation at the first item of menu.
                if (mIsCapped) {
                    Bundle extra = new Bundle();
                    extra.putString(KEY_ID, KEY_ID_UNAVAILABLE_CATEGORY);
                    items.add(0, extra);
                }
                mItems = items;
                mItemsNumber = mItems.size();
                mProgressBar.setVisibility(View.GONE);
                mPlvAnimationController.enqueueEnterAnimation(new Runnable() {
                    @Override
                    public void run() {
                        synchronized (mItemLock) {
                            mAdapter.setItems(mItems, mIsCapped);
                            mListView.setVisibility(View.VISIBLE);
                            mItems = null;
                        }
                        mListView.scrollToPosition(mAdapter.getFirstItemIndex());
                    }
                });
            }
        }

        @Override
        public void onError(String id) {
            // TODO: do something useful here.
        }

        @Override
        public void onChildChanged(String parentId, Bundle bundle) {
            if (!mSubscriptionIds.isEmpty() && parentId.equals(mSubscriptionIds.peek())) {
                // List is still animating, so adapter hasn't been updated. Update the list that
                // needs to be set.
                String id = bundle.getString(KEY_ID);
                synchronized (mItemLock) {
                    if (mItems != null) {
                        for (Bundle item : mItems) {
                            if (item.getString(KEY_ID).equals(id)) {
                                item.putAll(bundle);
                                break;
                            }
                        }
                        return;
                    }
                }
                RecyclerView rv = mListView.getRecyclerView();
                RecyclerView.ViewHolder holder = rv.findViewHolderForItemId(id.hashCode());
                mAdapter.onChildChanged(holder, bundle);
            }
        }
    }

    private class DrawerMenuListDecoration extends PagedListView.Decoration {

        public DrawerMenuListDecoration(Context context) {
            super(context);
        }

        @Override
        public void onDrawOver(Canvas c, RecyclerView parent, RecyclerView.State state) {
            if (mAdapter != null && mAdapter.isEmptyPlaceholder()) {
                return;
            }
            super.onDrawOver(c, parent, state);
        }
    }
}
