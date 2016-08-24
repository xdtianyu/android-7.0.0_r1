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
import android.car.app.menu.RootMenu;
import android.car.app.menu.SearchBoxEditListener;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.support.car.input.CarRestrictedEditText;
import android.support.car.ui.DrawerArrowDrawable;
import android.support.car.ui.PagedListView;
import android.support.v7.widget.CardView;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.view.View;
import android.view.LayoutInflater;

import android.support.car.ui.R;

public class CarUiEntry extends android.car.app.menu.CarUiEntry {
    private static final String TAG = "Embedded_CarUiEntry";

    // These values and setSearchBoxMode exist rather than separate methods to make sure exactly the
    // same set of things get set for each mode, just to different values.
    /** The search box is not visible. */
    private static final int SEARCH_BOX_MODE_NONE = 0;
    /** The small search box is shown in the header beneath the microphone button. */
    private static final int SEARCH_BOX_MODE_SMALL = 1;
    /** The whole header between the menu button and the microphone button is taken up by the
     * search box. */
    private static final int SEARCH_BOX_MODE_LARGE = 2;

    private View mContentView;
    private ImageView mMenuButton;
    private TextView mTitleView;
    private CardView mTruncatedListCardView;
    private CarDrawerLayout mDrawerLayout;
    private DrawerController mDrawerController;
    private PagedListView mListView;
    private DrawerArrowDrawable mDrawerArrowDrawable;
    private CarRestrictedEditText mCarRestrictedEditText;
    private SearchBoxClickListener mSearchBoxClickListener;

    private View mSearchBox;
    private View mSearchBoxContents;
    private View mSearchBoxSearchLogoContainer;
    private ImageView mSearchBoxSearchLogo;
    private ImageView mSearchBoxSuperSearchLogo;
    private FrameLayout mSearchBoxEndView;
    private View mTitleContainer;
    private SearchBoxEditListener mSearchBoxEditListener;

    public interface SearchBoxClickListener {
        /**
         * The user clicked the search box while it was in small mode.
         */
        void onClick();
    }

    public CarUiEntry(Context providerContext, Context appContext) {
        super(providerContext, appContext);
    }

    @Override
    public View getContentView() {
        LayoutInflater inflater = LayoutInflater.from(mUiLibContext);
        mContentView = inflater.inflate(R.layout.car_activity, null);
        mDrawerLayout = (CarDrawerLayout) mContentView.findViewById(R.id.drawer_container);
        adjustDrawer();
        mMenuButton = (ImageView) mContentView.findViewById(R.id.car_drawer_button);
        mTitleView = (TextView) mContentView.findViewById(R.id.car_drawer_title);
        mTruncatedListCardView = (CardView) mContentView.findViewById(R.id.truncated_list_card);
        mDrawerArrowDrawable = new DrawerArrowDrawable(mUiLibContext);
        restoreMenuDrawable();
        mListView = (PagedListView) mContentView.findViewById(R.id.list_view);
        mListView.setOnScrollBarListener(mOnScrollBarListener);
        mMenuButton.setOnClickListener(mMenuListener);
        mDrawerController = new DrawerController(this, mMenuButton,
                 mDrawerLayout, mListView, mTruncatedListCardView);
        mTitleContainer = mContentView.findViewById(R.id.car_drawer_title_container);

        mSearchBoxEndView = (FrameLayout) mContentView.findViewById(R.id.car_search_box_end_view);
        mSearchBox = mContentView.findViewById(R.id.car_search_box);
        mSearchBoxContents = mContentView.findViewById(R.id.car_search_box_contents);
        mSearchBoxSearchLogoContainer = mContentView.findViewById(
                R.id.car_search_box_search_logo_container);
        mSearchBoxSearchLogoContainer.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mSearchBoxClickListener != null) {
                    mSearchBoxClickListener.onClick();
                }
            }
        });
        mSearchBoxSearchLogo = (ImageView) mContentView.findViewById(
                R.id.car_search_box_search_logo);
        mSearchBoxSearchLogo.setImageDrawable(mUiLibContext.getResources()
                .getDrawable(R.drawable.ic_google));
        mSearchBoxSuperSearchLogo = (ImageView) mContentView.findViewById(
                R.id.car_search_box_super_logo);
        mSearchBoxSuperSearchLogo.setImageDrawable(mUiLibContext.getResources()
                .getDrawable(R.drawable.ic_googleg));

        mCarRestrictedEditText = (CarRestrictedEditText) mContentView.findViewById(
                R.id.car_search_box_edit_text);
        mCarRestrictedEditText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                if (mSearchBoxEditListener != null) {
                    mSearchBoxEditListener.onSearch(mCarRestrictedEditText.getText().toString());
                }
                return false;
            }
        });
        mCarRestrictedEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence text, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence text, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable text) {
                if (mSearchBoxEditListener != null) {
                    mSearchBoxEditListener.onEdit(text.toString());
                }
            }
        });
        setSearchBoxMode(SEARCH_BOX_MODE_NONE);
        return mContentView;
    }

    private final View.OnClickListener mMenuListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            CarUiEntry.this.mDrawerController.openDrawer();
        }
    };

    @Override
    public void setCarMenuCallbacks(CarMenuCallbacks callbacks){
        RootMenu rootMenu = callbacks.getRootMenu(null);
        if (rootMenu != null) {
            mDrawerController.setRootAndCallbacks(
                    rootMenu.getId(), callbacks);
            mDrawerController.setDrawerEnabled(true);
        } else {
            hideMenuButton();
        }
    }

    @Override
    public int getFragmentContainerId() {
        return R.id.container;
    }

    @Override
    public void setBackground(Bitmap bitmap) {
        BitmapDrawable bd = new BitmapDrawable(mUiLibContext.getResources(), bitmap);
        ImageView bg = (ImageView) mContentView.findViewById(R.id.background);
        bg.setBackground(bd);
    }

    @Override
    public void hideMenuButton() {
        mMenuButton.setVisibility(View.GONE);
    }

    @Override
    public void restoreMenuDrawable() {
        mMenuButton.setImageDrawable(mDrawerArrowDrawable);
    }

    public void setMenuButtonBitmap(Bitmap bitmap) {
        mMenuButton.setImageDrawable(new BitmapDrawable(mUiLibContext.getResources(), bitmap));
    }

    @Override
    public void setScrimColor(int color) {
        mDrawerLayout.setScrimColor(color);
    }

    @Override
    public void setTitle(CharSequence title) {
        mDrawerController.setTitle(title);
    }

    @Override
    public void closeDrawer() {
        mDrawerController.closeDrawer();
    }

    @Override
    public void openDrawer() {
        mDrawerController.openDrawer();
    }

    @Override
    public void showMenu(String id, String title) {
        mDrawerController.showMenu(id, title);
    }


    @Override
    public void setMenuButtonColor(int color) {
        setViewColor(mMenuButton, color);
        setViewColor(mTitleView, color);
    }

    @Override
    public void showTitle() {
        mTitleView.setVisibility(View.VISIBLE);
    }

    @Override
    public void hideTitle() {
        mTitleView.setVisibility(View.GONE);
    }

    @Override
    public void setLightMode() {
        mDrawerController.setLightMode();
    }

    @Override
    public void setDarkMode() {
        mDrawerController.setDarkMode();
    }

    @Override
    public void setAutoLightDarkMode() {
        mDrawerController.setAutoLightDarkMode();
    }

    @Override
    public void showToast(String msg, long duration) {
        // TODO: add toast support
    }

    @Override
    public CharSequence getSearchBoxText() {
        return mCarRestrictedEditText.getText();
    }

    @Override
    public EditText startInput(String hint,
            View.OnClickListener searchBoxClickListener) {
        mSearchBoxClickListener = wrapSearchBoxClickListener(searchBoxClickListener);
        setSearchBoxModeLarge(hint);
        return mCarRestrictedEditText;
    }


    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        if (mDrawerController != null) {
            mDrawerController.restoreState(savedInstanceState);
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        if (mDrawerController != null) {
            mDrawerController.saveState(outState);
        }
    }

    @Override
    public void onStart() {

    }

    @Override
    public void onResume() {

    }

    @Override
    public void onPause() {

    }

    @Override
    public void onStop() {

    }

    /**
     * Sets the colors of all the parts of the search box (regardless of whether it is currently
     * showing).
     */
    @Override
    public void setSearchBoxColors(int backgroundColor, int searchLogoColor, int textColor,
                                   int hintTextColor) {
        // set background color of mSearchBox to get rid of the animation artifact in b/23767062
        mSearchBox.setBackgroundColor(backgroundColor);
        mSearchBoxContents.setBackgroundColor(backgroundColor);
        mSearchBoxSearchLogo.setColorFilter(searchLogoColor, PorterDuff.Mode.SRC_IN);
        mCarRestrictedEditText.setTextColor(textColor);
        mCarRestrictedEditText.setHintTextColor(hintTextColor);
    }

    /**
     * Sets the view to be displayed at the end of the search box, or null to clear any existing
     * views.
     */
    @Override
    public void setSearchBoxEndView(View endView) {
        if (endView == null) {
            mSearchBoxEndView.removeAllViews();
        } else if (mSearchBoxEndView.getChildCount() == 0) {
            mSearchBoxEndView.addView(endView);
        } else if (mSearchBoxEndView.getChildAt(0) != endView) {
            mSearchBoxEndView.removeViewAt(0);
            mSearchBoxEndView.addView(endView);
        }
    }

    @Override
    public void showSearchBox(final View.OnClickListener listener) {
        setSearchBoxMode(SEARCH_BOX_MODE_SMALL);
        mSearchBoxClickListener = wrapSearchBoxClickListener(listener);
    }

    @Override
    public void stopInput() {
        setSearchBoxMode(SEARCH_BOX_MODE_NONE);
    }

    @Override
    public void setSearchBoxEditListener(SearchBoxEditListener listener) {
        mSearchBoxEditListener = listener;
    }


    /**
     * Set the progress of the animated {@link DrawerArrowDrawable}.
     * @param progress 0f displays a menu button
     *                 1f displays a back button
     *                 anything in between will be an interpolation of the drawable between
     *                 back and menu
     */
    public void setMenuProgress(float progress) {
        mDrawerArrowDrawable.setProgress(progress);
    }

    private void setSearchBoxModeLarge(String hint) {
        mCarRestrictedEditText.setHint(hint);
        setSearchBoxMode(SEARCH_BOX_MODE_LARGE);
    }

    public void setTitleText(CharSequence title) {
        mTitleView.setText(title);
    }

    /**
     * Sets all the view visibilities and layout params for a search box mode.
     */
    private void setSearchBoxMode(int searchBoxMode) {
        // Set the visibility and width of the search box, and whether the rest of the header sits
        // beside or beneath the microphone button.
        LinearLayout.LayoutParams searchBoxLayoutParams =
                (LinearLayout.LayoutParams) mSearchBox.getLayoutParams();
        if (searchBoxMode == SEARCH_BOX_MODE_LARGE) {
            int screenWidth = mAppContext.getResources().getDisplayMetrics().widthPixels;
            int searchBoxMargin = mUiLibContext.getResources()
                    .getDimensionPixelSize(R.dimen.car_drawer_header_menu_button_size);
            int maxSearchBoxWidth = mUiLibContext.getResources().getDimensionPixelSize(
                    R.dimen.car_card_max_width);
            int searchBoxMarginStart = 0;
            int searchBoxMarginEnd = searchBoxMargin;
            // If the width of search bar is larger than max card width, we adjust margin to fix it.
            if (screenWidth - searchBoxMargin * 2 > maxSearchBoxWidth) {
                searchBoxMarginEnd = (screenWidth - maxSearchBoxWidth) / 2;
                searchBoxMarginStart = searchBoxMarginEnd - searchBoxMargin;
            }
            searchBoxLayoutParams.width = 0;
            searchBoxLayoutParams.weight = 1.0f;
            searchBoxLayoutParams.setMarginStart(searchBoxMarginStart);
            searchBoxLayoutParams.setMarginEnd(searchBoxMarginEnd);
        } else if (searchBoxMode == SEARCH_BOX_MODE_SMALL) {
            searchBoxLayoutParams.width = mUiLibContext.getResources().getDimensionPixelSize(
                    R.dimen.car_app_layout_search_box_small_width);
            searchBoxLayoutParams.weight = 0.0f;
            searchBoxLayoutParams.setMarginStart(mUiLibContext.getResources()
                    .getDimensionPixelOffset(R.dimen.car_app_layout_search_box_small_margin));
            searchBoxLayoutParams.setMarginEnd(mUiLibContext.getResources().getDimensionPixelOffset(
                    R.dimen.car_app_layout_search_box_small_margin));
        } else {
            searchBoxLayoutParams.width = mUiLibContext.getResources().getDimensionPixelSize(
                    R.dimen.car_app_layout_search_box_small_width);
            searchBoxLayoutParams.weight = 0.0f;
            searchBoxLayoutParams.setMarginStart(mUiLibContext.getResources().getDimensionPixelSize(
                    R.dimen.car_drawer_header_menu_button_size));
            searchBoxLayoutParams.setMarginEnd(-searchBoxLayoutParams.width);
        }
        mSearchBox.setLayoutParams(searchBoxLayoutParams);

        // Animate the visibility of the contents of the search box - either the Search logo or the
        // edit text is visible (the super logo also is visible when the edit text is visible).
        View searchBoxEditTextContainer = (View) mCarRestrictedEditText.getParent();
        if (searchBoxMode == SEARCH_BOX_MODE_SMALL) {
            if (mSearchBoxSearchLogoContainer.getVisibility() != View.VISIBLE) {
                mSearchBoxSearchLogoContainer.setAlpha(0f);
                mSearchBoxSearchLogoContainer.setVisibility(View.VISIBLE);
            }
            // 300ms delay to stagger the fade in behind the fade out animation.
            mSearchBoxSearchLogoContainer.animate().alpha(1f).setStartDelay(300);
            // Animate the container so it includes the super G logo.
            if (searchBoxEditTextContainer.getVisibility() == View.VISIBLE) {
                searchBoxEditTextContainer.animate().alpha(0f).setStartDelay(0)
                        .withEndAction(mSetEditTextGoneRunnable);
            }
        } else if (searchBoxMode == SEARCH_BOX_MODE_LARGE) {
            if (searchBoxEditTextContainer.getVisibility() != View.VISIBLE) {
                searchBoxEditTextContainer.setAlpha(0f);
                searchBoxEditTextContainer.setVisibility(View.VISIBLE);
            }
            searchBoxEditTextContainer.animate().alpha(1f).setStartDelay(300);
            if (mSearchBoxSearchLogoContainer.getVisibility() == View.VISIBLE) {
                mSearchBoxSearchLogoContainer.animate().alpha(0f).setStartDelay(0)
                        .withEndAction(mSetSearchBoxLogoGoneRunnable);
            }
        } else {
            searchBoxEditTextContainer.setVisibility(View.GONE);
        }

        // Set the visibility of the title and status containers.
        if (searchBoxMode == SEARCH_BOX_MODE_LARGE) {
            mTitleContainer.setVisibility(View.GONE);
        } else {
            mTitleContainer.setVisibility(View.VISIBLE);
        }
    }


    private final Runnable mSetEditTextGoneRunnable = new Runnable() {
        @Override
        public void run() {
            ((View) mCarRestrictedEditText.getParent()).setVisibility(View.GONE);
        }
    };

    private final Runnable mSetSearchBoxLogoGoneRunnable = new Runnable() {
        @Override
        public void run() {
            mSearchBoxSearchLogoContainer.setVisibility(View.GONE);
        }
    };


    private SearchBoxClickListener wrapSearchBoxClickListener(final View.OnClickListener listener) {
        return new SearchBoxClickListener() {
            @Override
            public void onClick() {
                listener.onClick(null);
            }
        };
    }


    private static void setViewColor(View view, int color) {
        if (view instanceof TextView) {
            ((TextView) view).setTextColor(color);
        } else if (view instanceof ImageView) {
            ImageView imageView = (ImageView) view;
            PorterDuffColorFilter filter =
                    new PorterDuffColorFilter(color, PorterDuff.Mode.SRC_IN);
            imageView.setColorFilter(filter);
        } else {
            if (Log.isLoggable(TAG, Log.WARN)) {
                Log.w(TAG, "Setting color is only supported for TextView and ImageView.");
            }
        }
    }

    private void adjustDrawer() {
        Resources resources = mUiLibContext.getResources();
        float width = resources.getDisplayMetrics().widthPixels;
        CarDrawerLayout.LayoutParams layoutParams = new CarDrawerLayout.LayoutParams(
                CarDrawerLayout.LayoutParams.MATCH_PARENT,
                CarDrawerLayout.LayoutParams.MATCH_PARENT);
        layoutParams.gravity = Gravity.LEFT;
        // 1. If the screen width is larger than 800dp, the drawer width is kept as 704dp;
        // 2. Else the drawer width is adjusted to keep the margin end of drawer as 96dp.
        if (width > resources.getDimension(R.dimen.car_standard_width)) {
            layoutParams.setMarginEnd(
                    (int) (width - resources.getDimension(R.dimen.car_drawer_standard_width)));
        } else {
            layoutParams.setMarginEnd(
                    (int) resources.getDimension(R.dimen.car_card_margin));
        }
        mContentView.findViewById(R.id.drawer).setLayoutParams(layoutParams);
    }

    private final PagedListView.OnScrollBarListener mOnScrollBarListener =
            new PagedListView.OnScrollBarListener() {

                @Override
                public void onReachBottom() {
                    if (mDrawerController.isTruncatedList()) {
                        mTruncatedListCardView.setVisibility(View.VISIBLE);
                    }
                }

                @Override
                public void onLeaveBottom() {
                    mTruncatedListCardView.setVisibility(View.GONE);
                }
            };
}
