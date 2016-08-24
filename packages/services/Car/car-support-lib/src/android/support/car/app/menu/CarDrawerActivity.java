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

package android.support.car.app.menu;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.LayoutRes;
import android.support.car.Car;
import android.support.car.app.CarFragmentActivity;
import android.support.car.app.menu.compat.CarMenuConstantsComapt.MenuItemConstants;
import android.support.car.input.CarInputManager;
import android.support.v4.app.Fragment;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.EditText;

/**
 * Base class for a car app which wants to use a drawer.
 */
public abstract class CarDrawerActivity extends CarFragmentActivity {
    private static final String TAG = "CarDrawerActivity";

    private static final String KEY_DRAWERSHOWING =
            "android.support.car.app.CarDrawerActivity.DRAWER_SHOWING";
    private static final String KEY_INPUTSHOWING =
            "android.support.car.app.CarDrawerActivity.INPUT_SHOWING";
    private static final String KEY_SEARCHBOXENABLED =
            "android.support.car.app.CarDrawerActivity.SEARCH_BOX_ENABLED";

    private final Handler mHandler = new Handler();
    private final CarUiController mUiController;

    private CarMenuCallbacks mMenuCallbacks;
    private OnMenuClickListener mMenuClickListener;
    private boolean mDrawerShowing;
    private boolean mShowingSearchBox;
    private boolean mSearchBoxEnabled;
    private boolean mOnCreateCalled = false;
    private View.OnClickListener mSearchBoxOnClickListener;

    private CarInputManager mInputManager;
    private EditText mSearchBoxView;

    public interface OnMenuClickListener {
        /**
         * Called when the menu button is clicked.
         *
         * @return True if event was handled. This will prevent the drawer from executing its
         *         default action (opening/closing/going back). False if the event was not handled
         *         so the drawer will execute the default action.
         */
        boolean onClicked();
    }

    public CarDrawerActivity(Proxy proxy, Context context, Car car) {
        super(proxy, context, car);
        mUiController = createCarUiController();
    }

    /**
     * Create a {@link android.support.car.app.menu.CarUiController}.
     *
     * Derived class can override this function to return a customized ui controller.
     */
    protected CarUiController createCarUiController() {
        return CarUiController.createCarUiController(this);
    }

    @Override
    public void setContentView(View view) {
        ViewGroup parent = (ViewGroup) findViewById(mUiController.getFragmentContainerId());
        parent.addView(view);
    }

    @Override
    public void setContentView(@LayoutRes int resourceId) {
        ViewGroup parent = (ViewGroup) findViewById(mUiController.getFragmentContainerId());
        LayoutInflater inflater = getLayoutInflater();
        inflater.inflate(resourceId, parent, true);
    }

    @Override
    public View findViewById(@LayoutRes int id) {
        return super.findViewById(mUiController.getFragmentContainerId()).findViewById(id);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        super.setContentView(mUiController.getContentView());
        mInputManager = getInputManager();
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (mMenuCallbacks != null) {
                    mMenuCallbacks.registerOnChildrenChangedListener(mMenuListener);
                }
                mOnCreateCalled = true;
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (mMenuCallbacks != null) {
                    mMenuCallbacks.unregisterOnChildrenChangedListener(mMenuListener);
                    mMenuCallbacks = null;
                }
            }
        });
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        mDrawerShowing = savedInstanceState.getBoolean(KEY_DRAWERSHOWING);
        mUiController.onRestoreInstanceState(savedInstanceState);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(KEY_DRAWERSHOWING, mDrawerShowing);
        mUiController.onSaveInstanceState(outState);
    }

    @Override
    protected void onStart() {
        super.onStart();
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
        mUiController.onStart();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mUiController.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mUiController.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
        mUiController.onStop();
    }

    /**
     * Set the fragment in the main fragment container.
     */
    public void setContentFragment(Fragment fragment) {
        super.setContentFragment(fragment, mUiController.getFragmentContainerId());
    }

    /**
     * Return the main fragment container id for the app.
     */
    public int getFragmentContainerId() {
        return mUiController.getFragmentContainerId();
    }

    /**
     * Set the callbacks for car menu interactions.
     */
    public void setCarMenuCallbacks(final CarMenuCallbacks callbacks) {
        if (mOnCreateCalled) {
            throw new IllegalStateException(
                    "Cannot call setCarMenuCallbacks after onCreate has been called.");
        }
        mMenuCallbacks = callbacks;
        mUiController.registerCarMenuCallbacks(callbacks);
    }

    /**
     * Listener that listens for when the menu button is pressed.
     *
     * @param listener {@link OnMenuClickListener} that will listen for menu button clicks.
     */
    public void setOnMenuClickedListener(OnMenuClickListener listener) {
        mMenuClickListener = listener;
    }

    /**
     * Restore the menu button drawable
     */
    public void restoreMenuButtonDrawable() {
        mUiController.restoreMenuButtonDrawable();
    }

    /**
     * Sets the menu button bitmap
     *
     * @param bitmap Bitmap to the menu button to.
     */
    public void setMenuButtonBitmap(Bitmap bitmap) {
        mUiController.setMenuButtonBitmap(bitmap);
    }

    /**
     * Set the title of the menu.
     */
    public void setTitle(CharSequence title) {
        mUiController.setTitle(title);
    }

    /**
     * Set the System UI to be light.
     */
    public void setLightMode() {
        mUiController.setLightMode();
    }

    /**
     * Set the System UI to be dark.
     */
    public void setDarkMode() {
        mUiController.setDarkMode();
    }

    /**
     * Set the System UI to be dark during day mode and light during night mode.
     */
    public void setAutoLightDarkMode() {
        mUiController.setAutoLightDarkMode();
    }

    /**
     * Sets the application background to the given {@link android.graphics.Bitmap}.
     *
     * @param bitmap to use as background.
     */
    public void setBackground(Bitmap bitmap) {
        mUiController.setBackground(bitmap);
    }

    /**
     * Sets the color of the scrim to the right of the car menu drawer.
     */
    public void setScrimColor(int color) {
        mUiController.setScrimColor(color);
    }

    /**
     * Show the menu associated with the given id in the drawer.
     *
     * @param id Id of the menu to link to.
     * @param title Title that should be displayed.
     */
    public void showMenu(String id, String title) {
        mUiController.showMenu(id, title);
    }

    public boolean onMenuClicked() {
        if (mMenuClickListener != null) {
            return mMenuClickListener.onClicked();
        }
        return false;
    }

    public void restoreSearchBox() {
        if (isSearchBoxEnabled()) {
            mUiController.showSearchBox(mSearchBoxOnClickListener);
            mShowingSearchBox = true;
        }
    }

    private final CarMenuCallbacks.OnChildrenChangedListener mMenuListener =
            new CarMenuCallbacks.OnChildrenChangedListener() {
                @Override
                public void onChildrenChanged(String parentId) {
                    if (mOnCreateCalled) {
                        mUiController.onChildrenChanged(parentId);
                    }
                }

                @Override
                public void onChildChanged(String parentId, Bundle item,
                        Drawable leftIcon, Drawable rightIcon) {
                    DisplayMetrics metrics = getResources().getDisplayMetrics();
                    if (leftIcon != null) {
                        item.putParcelable(MenuItemConstants.KEY_LEFTICON,
                                Utils.snapshot(metrics, leftIcon));
                    }

                    if (rightIcon != null) {
                        item.putParcelable(MenuItemConstants.KEY_RIGHTICON,
                                Utils.snapshot(metrics, rightIcon));
                    }
                    if (mOnCreateCalled) {
                        mUiController.onChildChanged(parentId, item);
                    }
                }
            };

    public void closeDrawer() {
        mUiController.closeDrawer();
    }

    public void openDrawer() {
        mUiController.openDrawer();
    }

    public boolean isDrawerShowing() {
        return mDrawerShowing;
    }

    public void setDrawerShowing(boolean showing) {
        mDrawerShowing = showing;
    }

    public boolean isSearchBoxEnabled() {
        return mSearchBoxEnabled;
    }

    public boolean isShowingSearchBox() {
        return mShowingSearchBox;
    }

    /**
     * Shows a small clickable {@link android.widget.EditText}.
     *
     * {@link View} will be {@code null} in {@link View.OnClickListener#onClick(View)}.
     *
     * @param listener {@link View.OnClickListener} that is called when user selects the
     *                 {@link android.widget.EditText}.
     */
    public void showSearchBox(View.OnClickListener listener) {
        if (!isDrawerShowing()) {
            mUiController.showSearchBox(listener);
            mShowingSearchBox = true;
        }
        mSearchBoxEnabled = true;
        mSearchBoxOnClickListener = listener;
    }

    public void showSearchBox() {
        showSearchBox(mSearchBoxOnClickListener);
    }

    public void hideSearchBox() {
        if (isShowingSearchBox()) {
            stopInput();
        }
        mSearchBoxEnabled = false;
    }

    public void setSearchBoxEditListener(SearchBoxEditListener listener) {
        mUiController.setSearchBoxEditListener(listener);
    }

    public void stopInput() {
        // STOPSHIP: sometimes focus is lost and we are not able to hide the keyboard.
        // properly fix this before we ship.
        if (mSearchBoxView != null) {
            mSearchBoxView.requestFocusFromTouch();
        }
        mUiController.stopInput();
        mInputManager.stopInput();
        mShowingSearchBox = false;
    }

    /**
     * Start input on the search box that is provided by a car ui provider.
     * TODO: Migrate to use the new input/search api once it becomes stable (b/27108311).
     * @param hint Search hint
     */
    public void startInput(String hint) {
        startInput(hint, mSearchBoxOnClickListener);
    }

    /**
     * Start input on the search box that is provided by a car ui provider.
     * TODO: Migrate to use the new input/search api once it becomes stable (b/27108311).
     * @param hint Search hint
     * @param onClickListener Listener for the search box clicks.
     */
    public void startInput(final String hint, final View.OnClickListener onClickListener) {
        mInputManager = getInputManager();
        EditText inputView = mUiController.startInput(hint, onClickListener);
        getInputManager().startInput(inputView);
        mSearchBoxView = inputView;
        mShowingSearchBox = true;
    }

    public void setSearchBoxColors(int backgroundColor, int searchLogoColor, int textColor,
            int hintTextColor) {
        mUiController.setSearchBoxColors(backgroundColor, searchLogoColor,
                textColor, hintTextColor);
    }

    public void setSearchBoxEndView(View endView) {
        mUiController.setSearchBoxEndView(endView);
    }

    public void showToast(String text, int duration) {
        mUiController.showToast(text, duration);
    }
}
