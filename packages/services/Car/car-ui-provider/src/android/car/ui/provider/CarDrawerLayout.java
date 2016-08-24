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
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.support.car.ui.CarUiResourceLoader;
import android.support.car.ui.QuantumInterpolator;
import android.support.car.ui.R;
import android.support.car.ui.ReversibleInterpolator;
import android.support.v4.view.GravityCompat;
import android.support.v4.view.MotionEventCompat;
import android.support.v4.view.ViewCompat;
import android.support.v4.view.ViewGroupCompat;
import android.support.v4.widget.ViewDragHelper;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

/**
 * Acts as a top-level container for window content that allows for
 * interactive "drawer" views to be pulled out from the edge of the window.
 *
 * <p>Drawer positioning and layout is controlled using the <code>android:layout_gravity</code>
 * attribute on child views corresponding to which side of the view you want the drawer
 * to emerge from: left or right. (Or start/end on platform versions that support layout direction.)
 * </p>
 *
 * <p> To use CarDrawerLayout, add your drawer view as the first view in the CarDrawerLayout
 * element and set the <code>layout_gravity</code> appropriately. Drawers commonly use
 * <code>match_parent</code> for height with a fixed width. Add the content views as sibling views
 * after the drawer view.</p>
 *
 * <p>{@link DrawerListener} can be used to monitor the state and motion of drawer views.
 * Avoid performing expensive operations such as layout during animation as it can cause
 * stuttering; try to perform expensive operations during the {@link #STATE_IDLE} state.
 * {@link SimpleDrawerListener} offers default/no-op implementations of each callback method.</p>
 */
public class CarDrawerLayout extends ViewGroup {
    /**
     * Indicates that any drawers are in an idle, settled state. No animation is in progress.
     */
    public static final int STATE_IDLE = ViewDragHelper.STATE_IDLE;

    /**
     * The drawer is unlocked.
     */
    public static final int LOCK_MODE_UNLOCKED = 0;

    /**
     * The drawer is locked closed. The user may not open it, though
     * the app may open it programmatically.
     */
    public static final int LOCK_MODE_LOCKED_CLOSED = 1;

    /**
     * The drawer is locked open. The user may not close it, though the app
     * may close it programmatically.
     */
    public static final int LOCK_MODE_LOCKED_OPEN = 2;

    private static final float MAX_SCRIM_ALPHA = 0.8f;

    private static final boolean SCRIM_ENABLED = true;

    private static final boolean SHADOW_ENABLED = true;

    /**
     * Minimum velocity that will be detected as a fling
     */
    private static final int MIN_FLING_VELOCITY = 400; // dips per second

    /**
     * Experimental feature.
     */
    private static final boolean ALLOW_EDGE_LOCK = false;

    private static final boolean EDGE_DRAG_ENABLED = false;

    private static final boolean CHILDREN_DISALLOW_INTERCEPT = true;

    private static final float TOUCH_SLOP_SENSITIVITY = 1.f;

    private static final int[] LAYOUT_ATTRS = new int[] {
            android.R.attr.layout_gravity
    };

    public static final int DEFAULT_SCRIM_COLOR = 0xff262626;

    private int mScrimColor = DEFAULT_SCRIM_COLOR;
    private final Paint mScrimPaint = new Paint();
    private final Paint mEdgeHighlightPaint = new Paint();

    private final ViewDragHelper mDragger;

    private final Runnable mInvalidateRunnable = new Runnable() {
        @Override
        public void run() {
            requestLayout();
            invalidate();
        }
    };

    // view faders who will be given different colors as the drawer opens
    private final Set<ViewFaderHolder> mViewFaders;
    private final ReversibleInterpolator mViewFaderInterpolator;
    private final ReversibleInterpolator mDrawerFadeInterpolator;
    private final Handler mHandler = new Handler();

    private int mEndingViewColor;
    private int mStartingViewColor;
    private int mDrawerState;
    private boolean mInLayout;
    /** Whether we have done a layout yet. Used to initialize some view-related state. */
    private boolean mFirstLayout = true;
    private boolean mHasInflated;
    private int mLockModeLeft;
    private int mLockModeRight;
    private boolean mChildrenCanceledTouch;
    private DrawerListener mDrawerListener;
    private DrawerControllerListener mDrawerControllerListener;
    private Drawable mShadow;
    private View mDrawerView;
    private View mContentView;
    private boolean mNeedsFocus;
    /** Whether or not the drawer started open for the current gesture */
    private boolean mStartedOpen;
    private boolean mHasWheel;

    /**
     * Listener for monitoring events about drawers.
     */
    public interface DrawerListener {
        /**
         * Called when a drawer's position changes.
         * @param drawerView The child view that was moved
         * @param slideOffset The new offset of this drawer within its range, from 0-1
         */
        void onDrawerSlide(View drawerView, float slideOffset);

        /**
         * Called when a drawer has settled in a completely open state.
         * The drawer is interactive at this point.
         *
         * @param drawerView Drawer view that is now open
         */
        void onDrawerOpened(View drawerView);

        /**
         * Called when a drawer has settled in a completely closed state.
         *
         * @param drawerView Drawer view that is now closed
         */
        void onDrawerClosed(View drawerView);

        /**
         * Called when a drawer is starting to open.
         *
         * @param drawerView Drawer view that is opening
         */
        void onDrawerOpening(View drawerView);

        /**
         * Called when a drawer is starting to close.
         *
         * @param drawerView Drawer view that is closing
         */
        void onDrawerClosing(View drawerView);

        /**
         * Called when the drawer motion state changes. The new state will
         * be one of {@link #STATE_IDLE}, {@link #STATE_DRAGGING} or {@link #STATE_SETTLING}.
         *
         * @param newState The new drawer motion state
         */
        void onDrawerStateChanged(int newState);
    }

    /**
     * Used to execute when the drawer needs to handle state that the underlying views would like
     * to handle in a specific way.
     */
    public interface DrawerControllerListener {
        void onBack();
        boolean onScroll();
    }

    /**
     * Stub/no-op implementations of all methods of {@link DrawerListener}.
     * Override this if you only care about a few of the available callback methods.
     */
    public static abstract class SimpleDrawerListener implements DrawerListener {
        @Override
        public void onDrawerSlide(View drawerView, float slideOffset) {
        }

        @Override
        public void onDrawerOpened(View drawerView) {
        }

        @Override
        public void onDrawerClosed(View drawerView) {
        }

        @Override
        public void onDrawerOpening(View drawerView) {
        }

        @Override
        public void onDrawerClosing(View drawerView) {
        }

        @Override
        public void onDrawerStateChanged(int newState) {
        }
    }

    /**
     * Sets the color of (or tints) a view (or views).
     */
    public interface ViewFader {
        void setColor(int color);
    }

    public CarDrawerLayout(Context context) {
        this(context, null);
    }

    public CarDrawerLayout(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public CarDrawerLayout(final Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        mViewFaders = new HashSet<>();
        mEndingViewColor = getResources().getColor(R.color.car_tint);

        mEdgeHighlightPaint.setColor(getResources().getColor(android.R.color.black));

        final float density = getResources().getDisplayMetrics().density;
        final float minVel = MIN_FLING_VELOCITY * density;

        ViewDragCallback viewDragCallback = new ViewDragCallback();
        mDragger = ViewDragHelper.create(this, TOUCH_SLOP_SENSITIVITY, viewDragCallback);
        mDragger.setMinVelocity(minVel);
        viewDragCallback.setDragger(mDragger);

        ViewGroupCompat.setMotionEventSplittingEnabled(this, false);

        if (SHADOW_ENABLED) {
            setDrawerShadow(CarUiResourceLoader.getDrawable(context, "drawer_shadow"));
        }

        Resources.Theme theme = context.getTheme();
        TypedArray ta = theme.obtainStyledAttributes(new int[] {
                android.R.attr.colorPrimaryDark
        });
        setScrimColor(ta.getColor(0, context.getResources().getColor(R.color.car_grey_900)));

        mViewFaderInterpolator = new ReversibleInterpolator(
                new QuantumInterpolator(QuantumInterpolator.FAST_OUT_SLOW_IN, 0.25f, 0.25f, 0.5f),
                new QuantumInterpolator(QuantumInterpolator.FAST_OUT_SLOW_IN, 0.43f, 0.14f, 0.43f)
        );
        mDrawerFadeInterpolator = new ReversibleInterpolator(
                new QuantumInterpolator(QuantumInterpolator.FAST_OUT_SLOW_IN, 0.625f, 0.25f, 0.125f),
                new QuantumInterpolator(QuantumInterpolator.FAST_OUT_LINEAR_IN, 0.58f, 0.14f, 0.28f)
        );

        mHasWheel = CarUiResourceLoader.getBoolean(context, "has_wheel", false);
    }

    @Override
    public boolean dispatchKeyEvent(@NonNull KeyEvent keyEvent) {
        int action = keyEvent.getAction();
        int keyCode = keyEvent.getKeyCode();
        final View drawerView = findDrawerView();
        if (drawerView != null && getDrawerLockMode(drawerView) == LOCK_MODE_UNLOCKED) {
            if (isDrawerOpen()) {
                if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT
                        || keyCode == KeyEvent.KEYCODE_SOFT_RIGHT) {
                    closeDrawer();
                    return true;
                } else if (keyCode == KeyEvent.KEYCODE_BACK
                        && action == KeyEvent.ACTION_UP
                        && mDrawerControllerListener != null) {
                    mDrawerControllerListener.onBack();
                    return true;
                } else {
                    return drawerView.dispatchKeyEvent(keyEvent);
                }
            }
        }

        return mContentView.dispatchKeyEvent(keyEvent);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent ev) {
        final View drawerView = findDrawerView();
        if (drawerView != null
                && ev.getAction() == MotionEvent.ACTION_SCROLL
                && mDrawerControllerListener != null
                && mDrawerControllerListener.onScroll()) {
            return true;
        }
        return super.dispatchGenericMotionEvent(ev);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mHasInflated = true;
        setAutoDayNightMode();

        setOnGenericMotionListener(new OnGenericMotionListener() {
            @Override
            public boolean onGenericMotion(View view, MotionEvent event) {
                if (getChildCount() == 0) {
                    return false;
                }
                if (isDrawerOpen()) {
                    View drawerView = findDrawerView();
                    ViewGroup viewGroup = (ViewGroup) ((FrameLayout) drawerView).getChildAt(0);
                    return viewGroup.getChildAt(0).onGenericMotionEvent(event);
                }
                View contentView = findContentView();
                ViewGroup viewGroup = (ViewGroup) ((FrameLayout) contentView).getChildAt(0);
                return viewGroup.getChildAt(0).onGenericMotionEvent(event);
            }
        });
    }

    /**
     * Set a simple drawable used for the left or right shadow.
     * The drawable provided must have a nonzero intrinsic width.
     *
     * @param shadowDrawable Shadow drawable to use at the edge of a drawer
     */
    public void setDrawerShadow(Drawable shadowDrawable) {
        mShadow = shadowDrawable;
        invalidate();
    }



   /**
     * Set a color to use for the scrim that obscures primary content while a drawer is open.
     *
     * @param color Color to use in 0xAARRGGBB format.
     */
    public void setScrimColor(int color) {
        mScrimColor = color;
        invalidate();
    }

    /**
     * Set a listener to be notified of drawer events.
     *
     * @param listener Listener to notify when drawer events occur
     * @see DrawerListener
     */
    public void setDrawerListener(DrawerListener listener) {
        mDrawerListener = listener;
    }

    public void setDrawerControllerListener(DrawerControllerListener listener) {
        mDrawerControllerListener = listener;
    }

    /**
     * Enable or disable interaction with all drawers.
     *
     * <p>This allows the application to restrict the user's ability to open or close
     * any drawer within this layout. DrawerLayout will still respond to calls to
     * {@link #openDrawer()}, {@link #closeDrawer()} and friends if a drawer is locked.</p>
     *
     * <p>Locking drawers open or closed will implicitly open or close
     * any drawers as appropriate.</p>
     *
     * @param lockMode The new lock mode for the given drawer. One of {@link #LOCK_MODE_UNLOCKED},
     *                 {@link #LOCK_MODE_LOCKED_CLOSED} or {@link #LOCK_MODE_LOCKED_OPEN}.
     */
    public void setDrawerLockMode(int lockMode) {
        LayoutParams lp = (LayoutParams) findDrawerView().getLayoutParams();
        setDrawerLockMode(lockMode, lp.gravity);
    }

    /**
     * Enable or disable interaction with the given drawer.
     *
     * <p>This allows the application to restrict the user's ability to open or close
     * the given drawer. DrawerLayout will still respond to calls to {@link #openDrawer()},
     * {@link #closeDrawer()} and friends if a drawer is locked.</p>
     *
     * <p>Locking a drawer open or closed will implicitly open or close
     * that drawer as appropriate.</p>
     *
     * @param lockMode The new lock mode for the given drawer. One of {@link #LOCK_MODE_UNLOCKED},
     *                 {@link #LOCK_MODE_LOCKED_CLOSED} or {@link #LOCK_MODE_LOCKED_OPEN}.
     * @param edgeGravity Gravity.LEFT, RIGHT, START or END.
     *                    Expresses which drawer to change the mode for.
     *
     * @see #LOCK_MODE_UNLOCKED
     * @see #LOCK_MODE_LOCKED_CLOSED
     * @see #LOCK_MODE_LOCKED_OPEN
     */
    public void setDrawerLockMode(int lockMode, int edgeGravity) {
        final int absGravity = GravityCompat.getAbsoluteGravity(edgeGravity,
                ViewCompat.getLayoutDirection(this));
        if (absGravity == Gravity.LEFT) {
            mLockModeLeft = lockMode;
        } else if (absGravity == Gravity.RIGHT) {
            mLockModeRight = lockMode;
        }
        if (lockMode != LOCK_MODE_UNLOCKED) {
            // Cancel interaction in progress
            mDragger.cancel();
        }
        switch (lockMode) {
            case LOCK_MODE_LOCKED_OPEN:
                openDrawer();
                break;
            case LOCK_MODE_LOCKED_CLOSED:
                closeDrawer();
                break;
            // default: do nothing
        }
    }

    /**
     * All view faders will be light when the drawer is open and fade to dark and it closes.
     * NOTE: this will clear any existing view faders.
     */
    public void setLightMode() {
        mStartingViewColor = getResources().getColor(R.color.car_title_light);
        mEndingViewColor = getResources().getColor(R.color.car_tint);
        updateViewFaders();
    }

    /**
     * All view faders will be dark when the drawer is open and stay that way when it closes.
     * NOTE: this will clear any existing view faders.
     */
    public void setDarkMode() {
        mStartingViewColor = getResources().getColor(R.color.car_title_dark);
        mEndingViewColor = getResources().getColor(R.color.car_tint);
        updateViewFaders();
    }

    /**
     * All view faders will be dark during the day and light at night.
     * NOTE: this will clear any existing view faders.
     */
    public void setAutoDayNightMode() {
        mStartingViewColor = getResources().getColor(R.color.car_title);
        mEndingViewColor = getResources().getColor(R.color.car_tint);
        updateViewFaders();
    }

    private void resetViewFaders() {
        mViewFaders.clear();
    }

    /**
     * Check the lock mode of the given drawer view.
     *
     * @param drawerView Drawer view to check lock mode
     * @return one of {@link #LOCK_MODE_UNLOCKED}, {@link #LOCK_MODE_LOCKED_CLOSED} or
     *         {@link #LOCK_MODE_LOCKED_OPEN}.
     */
    public int getDrawerLockMode(View drawerView) {
        final int absGravity = getDrawerViewAbsoluteGravity(drawerView);
        if (absGravity == Gravity.LEFT) {
            return mLockModeLeft;
        } else if (absGravity == Gravity.RIGHT) {
            return mLockModeRight;
        }
        return LOCK_MODE_UNLOCKED;
    }

    /**
     * Resolve the shared state of all drawers from the component ViewDragHelpers.
     * Should be called whenever a ViewDragHelper's state changes.
     */
    private void updateDrawerState(int activeState) {
        View drawerView = findDrawerView();
        if (drawerView != null && activeState == STATE_IDLE) {
            if (onScreen() == 0) {
                dispatchOnDrawerClosed(drawerView);
            } else if (onScreen() == 1) {
                dispatchOnDrawerOpened(drawerView);
            }
        }

        if (mDragger.getViewDragState() != mDrawerState) {
            mDrawerState = mDragger.getViewDragState();

            if (mDrawerListener != null) {
                mDrawerListener.onDrawerStateChanged(mDragger.getViewDragState());
            }
        }
    }

    private void dispatchOnDrawerClosed(View drawerView) {
        final LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
        if (lp.knownOpen) {
            lp.knownOpen = false;
            if (mDrawerListener != null) {
                mDrawerListener.onDrawerClosed(drawerView);
            }
        }
    }

    private void dispatchOnDrawerOpened(View drawerView) {
        final LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
        if (!lp.knownOpen) {
            lp.knownOpen = true;
            if (mDrawerListener != null) {
                mDrawerListener.onDrawerOpened(drawerView);
            }
        }
    }

    private void dispatchOnDrawerSlide(View drawerView, float slideOffset) {
        if (mDrawerListener != null) {
            mDrawerListener.onDrawerSlide(drawerView, slideOffset);
        }
    }

    private void dispatchOnDrawerOpening(View drawerView) {
        if (mDrawerListener != null) {
            mDrawerListener.onDrawerOpening(drawerView);
        }
    }

    private void dispatchOnDrawerClosing(View drawerView) {
        if (mDrawerListener != null) {
            mDrawerListener.onDrawerClosing(drawerView);
        }
    }

    private void setDrawerViewOffset(View drawerView, float slideOffset) {
        if (slideOffset == onScreen()) {
            return;
        }

        LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
        lp.onScreen = slideOffset;
        dispatchOnDrawerSlide(drawerView, slideOffset);
    }

    private float onScreen() {
        return ((LayoutParams) findDrawerView().getLayoutParams()).onScreen;
    }

    /**
     * @return the absolute gravity of the child drawerView, resolved according
     *         to the current layout direction
     */
    private int getDrawerViewAbsoluteGravity(View drawerView) {
        final int gravity = ((LayoutParams) drawerView.getLayoutParams()).gravity;
        return GravityCompat.getAbsoluteGravity(gravity, ViewCompat.getLayoutDirection(this));
    }

    private boolean checkDrawerViewAbsoluteGravity(View drawerView, int checkFor) {
        final int absGravity = getDrawerViewAbsoluteGravity(drawerView);
        return (absGravity & checkFor) == checkFor;
    }

    /**
     * @return the drawer view
     */
    private View findDrawerView() {
        if (mDrawerView != null) {
            return mDrawerView;
        }

        final int childCount = getChildCount();
        for (int i = 0; i < childCount; i++) {
            final View child = getChildAt(i);
            final int childAbsGravity = getDrawerViewAbsoluteGravity(child);
            if (childAbsGravity != Gravity.NO_GRAVITY) {
                mDrawerView = child;
                return child;
            }
        }
        throw new IllegalStateException("No drawer view found.");
    }

    /**
     * @return the content. NOTE: this is the view with no gravity.
     */
    private View findContentView() {
        if (mContentView != null) {
            return mContentView;
        }

        final int childCount = getChildCount();
        for (int i = childCount - 1; i >= 0; --i) {
            final View child = getChildAt(i);
            if (isDrawerView(child)) {
                continue;
            }
            mContentView = child;
            return child;
        }
        throw new IllegalStateException("No content view found.");
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
    }

    @Override
    public boolean requestFocus(int direction, Rect rect) {
        // Optimally we want to check isInTouchMode(), but that value isn't always correct.
        if (mHasWheel) {
            mNeedsFocus = true;
        }
        return super.requestFocus(direction, rect);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        mFirstLayout = true;
        // There needs to be a layout pending if we're not going to animate the drawer until the
        // next layout, so make it so.
        requestLayout();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        int widthSize = MeasureSpec.getSize(widthMeasureSpec);
        int heightSize = MeasureSpec.getSize(heightMeasureSpec);

        if (widthMode != MeasureSpec.EXACTLY || heightMode != MeasureSpec.EXACTLY) {
            if (isInEditMode()) {
                // Don't crash the layout editor. Consume all of the space if specified
                // or pick a magic number from thin air otherwise.
                // TODO Better communication with tools of this bogus state.
                // It will crash on a real device.
                if (widthMode == MeasureSpec.UNSPECIFIED) {
                    widthSize = 300;
                }
                else if (heightMode == MeasureSpec.UNSPECIFIED) {
                    heightSize = 300;
                }
            } else {
                throw new IllegalArgumentException(
                        "DrawerLayout must be measured with MeasureSpec.EXACTLY.");
            }
        }

        setMeasuredDimension(widthSize, heightSize);

        View view = findContentView();
        LayoutParams lp = ((LayoutParams) view.getLayoutParams());
        // Content views get measured at exactly the layout's size.
        final int contentWidthSpec = MeasureSpec.makeMeasureSpec(
                widthSize - lp.leftMargin - lp.rightMargin, MeasureSpec.EXACTLY);
        final int contentHeightSpec = MeasureSpec.makeMeasureSpec(
                heightSize - lp.topMargin - lp.bottomMargin, MeasureSpec.EXACTLY);
        view.measure(contentWidthSpec, contentHeightSpec);

        view = findDrawerView();
        lp = ((LayoutParams) view.getLayoutParams());
        final int drawerWidthSpec = getChildMeasureSpec(widthMeasureSpec,
                lp.leftMargin + lp.rightMargin,
                lp.width);
        final int drawerHeightSpec = getChildMeasureSpec(heightMeasureSpec,
                lp.topMargin + lp.bottomMargin,
                lp.height);
        view.measure(drawerWidthSpec, drawerHeightSpec);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        mInLayout = true;
        final int width = r - l;

        View contentView = findContentView();
        View drawerView = findDrawerView();

        LayoutParams drawerLp = (LayoutParams) drawerView.getLayoutParams();
        LayoutParams contentLp = (LayoutParams) contentView.getLayoutParams();

        int contentRight = contentLp.getMarginStart() + getWidth();
        contentView.layout(contentRight - contentView.getMeasuredWidth(),
                contentLp.topMargin, contentRight,
                contentLp.topMargin + contentView.getMeasuredHeight());

        final int childHeight = drawerView.getMeasuredHeight();
        int onScreen = (int) (drawerView.getWidth() * drawerLp.onScreen);
        int offset;
        if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.LEFT)) {
            offset = onScreen - drawerView.getWidth();
        } else {
            offset = width - onScreen;
        }
        drawerView.layout(drawerLp.getMarginStart() + offset, drawerLp.topMargin,
                width - drawerLp.getMarginEnd() + offset,
                childHeight + drawerLp.topMargin);
        updateDrawerAlpha();
        updateViewFaders();
        if (mFirstLayout) {

            // TODO(b/15394507): Normally, onMeasure()/onLayout() are called three times when
            // you create CarDrawerLayout, but when you pop it back it's only called once which
            // leaves us in a weird state. This is a pretty ugly hack to fix that.
            mHandler.post(mInvalidateRunnable);

            mFirstLayout = false;
        }

        if (mNeedsFocus) {
            if (initializeFocus()) {
                mNeedsFocus = false;
            }
        }

        mInLayout = false;
    }

    private boolean initializeFocus() {
        // Only request focus if the current view that needs focus doesn't already have it. This
        // prevents some nasty bugs where focus ends up snapping to random elements and also saves
        // a bunch of cycles in the average case.
        mDrawerView.setFocusable(false);
        mContentView.setFocusable(false);
        boolean needFocus = !mDrawerView.hasFocus() && !mContentView.hasFocus();
        if (!needFocus) {
            return true;
        }

        // Find something in the hierarchy to give focus to.
        List<View> focusables;
        boolean drawerOpen = isDrawerOpen();
        if (drawerOpen) {
            focusables = mDrawerView.getFocusables(FOCUS_DOWN);
        } else {
            focusables = mContentView.getFocusables(FOCUS_DOWN);
        }

        // The 2 else cases here are a catch all for when nothing is focusable in view hierarchy.
        // If you don't have anything focusable on screen, key events will not be delivered to
        // the view hierarchy and you end up getting stuck without being able to open / close the
        // drawer or launch gsa.

        if (!focusables.isEmpty()) {
            focusables.get(0).requestFocus();
            return true;
        } else if (drawerOpen) {
            mDrawerView.setFocusable(true);
        } else {
            mContentView.setFocusable(true);
        }
        return false;
    }

    @Override
    public void requestLayout() {
        if (!mInLayout) {
            super.requestLayout();
        }
    }

    @Override
    public void computeScroll() {
        if (mDragger.continueSettling(true)) {
            ViewCompat.postInvalidateOnAnimation(this);
        }
    }

    private static boolean hasOpaqueBackground(View v) {
        final Drawable bg = v.getBackground();
        return bg != null && bg.getOpacity() == PixelFormat.OPAQUE;
    }

    @Override
    protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
        final int height = getHeight();
        final boolean drawingContent = isContentView(child);
        int clipLeft = findContentView().getLeft();
        int clipRight = findContentView().getRight();
        final int baseAlpha = (mScrimColor & 0xff000000) >>> 24;

        final int restoreCount = canvas.save();
        if (drawingContent) {
            final int childCount = getChildCount();
            for (int i = 0; i < childCount; i++) {
                final View v = getChildAt(i);
                if (v == child || v.getVisibility() != VISIBLE ||
                        !hasOpaqueBackground(v) || !isDrawerView(v) ||
                        v.getHeight() < height) {
                    continue;
                }

                if (checkDrawerViewAbsoluteGravity(v, Gravity.LEFT)) {
                    final int vright = v.getRight();
                    if (vright > clipLeft) {
                        clipLeft = vright;
                    }
                } else {
                    final int vleft = v.getLeft();
                    if (vleft < clipRight) {
                        clipRight = vleft;
                    }
                }
            }
            canvas.clipRect(clipLeft, 0, clipRight, getHeight());
        }
        final boolean result = super.drawChild(canvas, child, drawingTime);
        canvas.restoreToCount(restoreCount);

        if (drawingContent) {
            int scrimAlpha = SCRIM_ENABLED ?
                    (int) (baseAlpha * Math.max(0, Math.min(1, onScreen())) * MAX_SCRIM_ALPHA) : 0;

            if (scrimAlpha > 0) {
                int color = scrimAlpha << 24 | (mScrimColor & 0xffffff);
                mScrimPaint.setColor(color);

                canvas.drawRect(clipLeft, 0, clipRight, getHeight(), mScrimPaint);

                canvas.drawRect(clipLeft - 1, 0, clipLeft, getHeight(), mEdgeHighlightPaint);
            }

            LayoutParams drawerLp = (LayoutParams) findDrawerView().getLayoutParams();
            if (mShadow != null
                    && checkDrawerViewAbsoluteGravity(findDrawerView(), Gravity.LEFT)) {
                final int offScreen = (int) ((1 - drawerLp.onScreen) * findDrawerView().getWidth());
                final int drawerRight = getWidth() - drawerLp.getMarginEnd() - offScreen;
                final int shadowWidth = mShadow.getIntrinsicWidth();
                final float alpha =
                        Math.max(0, Math.min((float) drawerRight / mDragger.getEdgeSize(), 1.f));
                mShadow.setBounds(drawerRight, child.getTop(),
                        drawerRight + shadowWidth, child.getBottom());
                mShadow.setAlpha((int) (255 * alpha * alpha * alpha));
                mShadow.draw(canvas);
            } else if (mShadow != null
                    && checkDrawerViewAbsoluteGravity(findDrawerView(),Gravity.RIGHT)) {
                final int onScreen = (int) (findDrawerView().getWidth() * drawerLp.onScreen);
                final int drawerLeft = drawerLp.getMarginStart() + getWidth() - onScreen;
                final int shadowWidth = mShadow.getIntrinsicWidth();
                final float alpha =
                        Math.max(0, Math.min((float) onScreen / mDragger.getEdgeSize(), 1.f));
                canvas.save();
                canvas.translate(2 * drawerLeft - shadowWidth, 0);
                canvas.scale(-1.0f, 1.0f);
                mShadow.setBounds(drawerLeft - shadowWidth, child.getTop(),
                        drawerLeft, child.getBottom());
                mShadow.setAlpha((int) (255 * alpha * alpha * alpha * alpha));
                mShadow.draw(canvas);
                canvas.restore();
            }
        }
        return result;
    }

    private boolean isContentView(View child) {
        return child == findContentView();
    }

    private boolean isDrawerView(View child) {
        return child == findDrawerView();
    }

    private void updateDrawerAlpha() {
        float alpha;
        if (mStartedOpen) {
            alpha = mDrawerFadeInterpolator.getReverseInterpolation(onScreen());
        } else {
            alpha = mDrawerFadeInterpolator.getForwardInterpolation(onScreen());
        }
        ViewGroup drawerView = (ViewGroup) findDrawerView();
        int drawerChildCount = drawerView.getChildCount();
        for (int i = 0; i < drawerChildCount; i++) {
            drawerView.getChildAt(i).setAlpha(alpha);
        }
    }

    /**
     * Add a view fader whose color will be set as the drawer opens and closes.
     */
    public void addViewFader(ViewFader viewFader) {
        addViewFader(viewFader, mStartingViewColor, mEndingViewColor);
    }

    public void addViewFader(ViewFader viewFader, int startingColor, int endingColor) {
        mViewFaders.add(new ViewFaderHolder(viewFader, startingColor, endingColor));
        updateViewFaders();
    }

    public void removeViewFader(ViewFader viewFader) {
        for (Iterator<ViewFaderHolder> it = mViewFaders.iterator(); it.hasNext(); ) {
            ViewFaderHolder viewFaderHolder = it.next();
            if (viewFaderHolder.viewFader.equals(viewFader)) {
                it.remove();
            }
        }
    }

    private void updateViewFaders() {
        if (!mHasInflated) {
            return;
        }

        float fadeProgress;
        if (mStartedOpen) {
            fadeProgress = mViewFaderInterpolator.getReverseInterpolation(onScreen());
        } else {
            fadeProgress = mViewFaderInterpolator.getForwardInterpolation(onScreen());
        }
        for (Iterator<ViewFaderHolder> it = mViewFaders.iterator(); it.hasNext(); ) {
            ViewFaderHolder viewFaderHolder = it.next();
            int startingColor = viewFaderHolder.startingColor;
            int endingColor = viewFaderHolder.endingColor;
            int alpha = weightedAverage(Color.alpha(startingColor),
                    Color.alpha(endingColor), fadeProgress);
            int red = weightedAverage(Color.red(startingColor),
                    Color.red(endingColor), fadeProgress);
            int green = weightedAverage(Color.green(startingColor),
                    Color.green(endingColor), fadeProgress);
            int blue = weightedAverage(Color.blue(startingColor),
                    Color.blue(endingColor), fadeProgress);
            viewFaderHolder.viewFader.setColor(alpha << 24 | red << 16 | green << 8 | blue);
        }
    }

    private int weightedAverage(int starting, int ending, float weight) {
        return (int) ((1f - weight) * starting + weight * ending);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        final int action = MotionEventCompat.getActionMasked(ev);

        // "|" used deliberately here; both methods should be invoked.
        final boolean interceptForDrag = mDragger.shouldInterceptTouchEvent(ev);

        boolean interceptForTap = false;

        switch (action) {
            case MotionEvent.ACTION_DOWN: {
                final float x = ev.getX();
                final float y = ev.getY();
                if (onScreen() > 0 && isContentView(mDragger.findTopChildUnder((int) x, (int) y))) {
                    interceptForTap = true;
                }
                mChildrenCanceledTouch = false;
                break;
            }
            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_UP: {
                mChildrenCanceledTouch = false;
            }
        }

        return interceptForDrag || interceptForTap || mChildrenCanceledTouch;
    }

    @Override
    public boolean onTouchEvent(@NonNull MotionEvent ev) {
        mDragger.processTouchEvent(ev);
        final int absGravity = getDrawerViewAbsoluteGravity(findDrawerView());
        final int edge;
        if (absGravity == Gravity.LEFT) {
            edge = ViewDragHelper.EDGE_LEFT;
        } else {
            edge = ViewDragHelper.EDGE_RIGHT;
        }

        // don't allow views behind the drawer to be touched
        boolean drawerPartiallyOpen = onScreen() > 0;
        return mDragger.isEdgeTouched(edge) ||
                mDragger.getCapturedView() != null ||
                drawerPartiallyOpen;
    }

    @Override
    public void requestDisallowInterceptTouchEvent(boolean disallowIntercept) {
        if (CHILDREN_DISALLOW_INTERCEPT) {
            // If we have an edge touch we want to skip this and track it for later instead.
            super.requestDisallowInterceptTouchEvent(disallowIntercept);
        }

        View drawerView = findDrawerView();
        if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.LEFT)) {
            super.requestDisallowInterceptTouchEvent(disallowIntercept);
        }

        if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.RIGHT)) {
            super.requestDisallowInterceptTouchEvent(disallowIntercept);
        }
    }

    /**
     * Open the drawer view by animating it into view.
     */
    public void openDrawer() {
        ViewGroup drawerView = (ViewGroup) findDrawerView();
        mStartedOpen = false;

        if (hasWindowFocus()) {
            int left;
            LayoutParams drawerLp = (LayoutParams) drawerView.getLayoutParams();
            if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.LEFT)) {
                left = drawerLp.getMarginStart();
            } else {
                left = drawerLp.getMarginStart() + getWidth() - drawerView.getWidth();
            }
            mDragger.smoothSlideViewTo(drawerView, left, drawerView.getTop());
            dispatchOnDrawerOpening(drawerView);
        } else {
            final LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
            lp.onScreen = 1.f;
            dispatchOnDrawerOpened(drawerView);
        }

        ViewGroup contentView = (ViewGroup) findContentView();
        contentView.setDescendantFocusability(ViewGroup.FOCUS_BLOCK_DESCENDANTS);
        drawerView.setDescendantFocusability(ViewGroup. FOCUS_AFTER_DESCENDANTS);

        View focusable = drawerView.getChildAt(0);
        if (focusable != null) {
            focusable.requestFocus();
        }
        invalidate();
    }

    /**
     * Close the specified drawer view by animating it into view.
     */
    public void closeDrawer() {
        ViewGroup drawerView = (ViewGroup) findDrawerView();
        if (!isDrawerView(drawerView)) {
            throw new IllegalArgumentException("View " + drawerView + " is not a sliding drawer");
        }
        mStartedOpen = true;

        // Don't trigger the close drawer animation if drawer is not open.
        if (hasWindowFocus() && isDrawerOpen()) {
            int left;
            LayoutParams drawerLp = (LayoutParams) drawerView.getLayoutParams();
            if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.LEFT)) {
                left = drawerLp.getMarginStart() - drawerView.getWidth();
            } else {
                left = drawerLp.getMarginStart() + getWidth();
            }
            mDragger.smoothSlideViewTo(drawerView, left, drawerView.getTop());
            dispatchOnDrawerClosing(drawerView);
        } else {
            final LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
            lp.onScreen = 0.f;
            dispatchOnDrawerClosed(drawerView);
        }

        ViewGroup contentView = (ViewGroup) findContentView();
        drawerView.setDescendantFocusability(ViewGroup.FOCUS_BLOCK_DESCENDANTS);
        contentView.setDescendantFocusability(ViewGroup. FOCUS_AFTER_DESCENDANTS);

        if (!isInTouchMode()) {
            List<View> focusables = contentView.getFocusables(FOCUS_DOWN);
            if (focusables.size() > 0) {
                View candidate = focusables.get(0);
                candidate.requestFocus();
            }
        }
        invalidate();
    }

    @Override
    public void addFocusables(@NonNull ArrayList<View> views, int direction, int focusableMode) {
        boolean drawerOpen = isDrawerOpen();
        if (drawerOpen) {
            findDrawerView().addFocusables(views, direction, focusableMode);
        } else {
            findContentView().addFocusables(views, direction, focusableMode);
        }
    }

    /**
     * Check if the given drawer view is currently in an open state.
     * To be considered "open" the drawer must have settled into its fully
     * visible state. To check for partial visibility use
     * {@link #isDrawerVisible(android.view.View)}.
     *
     * @return true if the given drawer view is in an open state
     * @see #isDrawerVisible(android.view.View)
     */
    public boolean isDrawerOpen() {
        return ((LayoutParams) findDrawerView().getLayoutParams()).knownOpen;
    }

    /**
     * Check if a given drawer view is currently visible on-screen. The drawer
     * may be fully extended or anywhere in between.
     *
     * @param drawer Drawer view to check
     * @return true if the given drawer is visible on-screen
     * @see #isDrawerOpen()
     */
    public boolean isDrawerVisible(View drawer) {
        if (!isDrawerView(drawer)) {
            throw new IllegalArgumentException("View " + drawer + " is not a drawer");
        }
        return onScreen() > 0;
    }

    /**
     * Check if a given drawer view is currently visible on-screen. The drawer
     * may be fully extended or anywhere in between.
     * If there is no drawer with the given gravity this method will return false.
     *
     * @return true if the given drawer is visible on-screen
     */
    public boolean isDrawerVisible() {
        final View drawerView = findDrawerView();
        return drawerView != null && isDrawerVisible(drawerView);
    }

    @Override
    protected ViewGroup.LayoutParams generateDefaultLayoutParams() {
        return new LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT);
    }

    @Override
    protected ViewGroup.LayoutParams generateLayoutParams(ViewGroup.LayoutParams p) {
        return p instanceof LayoutParams
                ? new LayoutParams((LayoutParams) p)
                : p instanceof MarginLayoutParams
                ? new LayoutParams((MarginLayoutParams) p)
                : new LayoutParams(p);
    }

    @Override
    protected boolean checkLayoutParams(ViewGroup.LayoutParams p) {
        return p instanceof LayoutParams && super.checkLayoutParams(p);
    }

    @Override
    public ViewGroup.LayoutParams generateLayoutParams(AttributeSet attrs) {
        return new LayoutParams(getContext(), attrs);
    }

    private boolean hasVisibleDrawer() {
        return findVisibleDrawer() != null;
    }

    private View findVisibleDrawer() {
        final int childCount = getChildCount();
        for (int i = 0; i < childCount; i++) {
            final View child = getChildAt(i);
            if (isDrawerView(child) && isDrawerVisible(child)) {
                return child;
            }
        }
        return null;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        SavedState ss = null;
        if (state.getClass().getClassLoader() != getClass().getClassLoader()) {
            // Class loader mismatch, recreate from parcel.
            Parcel stateParcel = Parcel.obtain();
            state.writeToParcel(stateParcel, 0);
            ss = SavedState.CREATOR.createFromParcel(stateParcel);
        } else {
            ss = (SavedState) state;
        }
        super.onRestoreInstanceState(ss.getSuperState());

        if (ss.openDrawerGravity != Gravity.NO_GRAVITY) {
            openDrawer();
        }

        setDrawerLockMode(ss.lockModeLeft, Gravity.LEFT);
        setDrawerLockMode(ss.lockModeRight, Gravity.RIGHT);
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        final Parcelable superState = super.onSaveInstanceState();

        final SavedState ss = new SavedState(superState);

        final int childCount = getChildCount();
        for (int i = 0; i < childCount; i++) {
            final View child = getChildAt(i);
            if (!isDrawerView(child)) {
                continue;
            }

            final LayoutParams lp = (LayoutParams) child.getLayoutParams();
            if (lp.knownOpen) {
                ss.openDrawerGravity = lp.gravity;
                // Only one drawer can be open at a time.
                break;
            }
        }

        ss.lockModeLeft = mLockModeLeft;
        ss.lockModeRight = mLockModeRight;

        return ss;
    }

    /**
     * State persisted across instances
     */
    protected static class SavedState extends BaseSavedState {
        int openDrawerGravity = Gravity.NO_GRAVITY;
        int lockModeLeft = LOCK_MODE_UNLOCKED;
        int lockModeRight = LOCK_MODE_UNLOCKED;

        public SavedState(Parcel in) {
            super(in);
            openDrawerGravity = in.readInt();
            lockModeLeft = in.readInt();
            lockModeRight = in.readInt();
        }

        public SavedState(Parcelable superState) {
            super(superState);
        }

        @Override
        public void writeToParcel(@NonNull Parcel dest, int flags) {
            super.writeToParcel(dest, flags);
            dest.writeInt(openDrawerGravity);
            dest.writeInt(lockModeLeft);
            dest.writeInt(lockModeRight);
        }

        @SuppressWarnings("hiding")
        public static final Creator<SavedState> CREATOR =
                new Creator<SavedState>() {
                    @Override
                    public SavedState createFromParcel(Parcel source) {
                        return new SavedState(source);
                    }

                    @Override
                    public SavedState[] newArray(int size) {
                        return new SavedState[size];
                    }
                };
    }

    private class ViewDragCallback extends ViewDragHelper.Callback {
        @SuppressWarnings("hiding")
        private ViewDragHelper mDragger;

        public void setDragger(ViewDragHelper dragger) {
            mDragger = dragger;
        }

        @Override
        public boolean tryCaptureView(View child, int pointerId) {
            CarDrawerLayout.LayoutParams lp = (LayoutParams) findDrawerView().getLayoutParams();
            int edges = EDGE_DRAG_ENABLED ? ViewDragHelper.EDGE_ALL : 0;
            boolean captured = isContentView(child) &&
                    getDrawerLockMode(child) == LOCK_MODE_UNLOCKED &&
                    (lp.knownOpen || mDragger.isEdgeTouched(edges));
            if (captured && lp.knownOpen) {
                mStartedOpen = true;
            } else if (captured && !lp.knownOpen) {
                mStartedOpen = false;
            }
            // We want dragging starting on the content view to drag the drawer. Therefore when
            // touch events try to capture the content view, we force capture of the drawer view.
            if (captured) {
                mDragger.captureChildView(findDrawerView(), pointerId);
            }
            return false;
        }

        @Override
        public void onViewDragStateChanged(int state) {
            updateDrawerState(state);
        }

        @Override
        public void onViewPositionChanged(View changedView, int left, int top, int dx, int dy) {
            float offset;
            View drawerView = findDrawerView();
            final int drawerWidth = drawerView.getWidth();
            // This reverses the positioning shown in onLayout.
            if (checkDrawerViewAbsoluteGravity(findDrawerView(), Gravity.LEFT)) {
                offset = (float) (left + drawerWidth) / drawerWidth;
            } else {
                offset = (float) (getWidth() - left) / drawerWidth;
            }
            setDrawerViewOffset(findDrawerView(), offset);

            updateDrawerAlpha();

            updateViewFaders();
            invalidate();
        }

        @Override
        public void onViewReleased(View releasedChild, float xvel, float yvel) {
            final View drawerView = findDrawerView();
            final LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
            int left;
            if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.LEFT)) {
                // Open the drawer if they are swiping right or if they are not currently moving but
                // have moved the drawer in the current gesture and released the drawer when it was
                // fully open.
                // Close otherwise.
                left = xvel > 0 ? lp.getMarginStart() : lp.getMarginStart() - drawerView.getWidth();
            } else {
                // See comment for left drawer.
                left = xvel < 0 ? lp.getMarginStart() + getWidth() - drawerView.getWidth()
                        : lp.getMarginStart() + getWidth();
            }

            mDragger.settleCapturedViewAt(left, releasedChild.getTop());
            invalidate();
        }

        @Override
        public boolean onEdgeLock(int edgeFlags) {
            if (ALLOW_EDGE_LOCK) {
                if (!isDrawerOpen()) {
                    closeDrawer();
                }
                return true;
            }
            return false;
        }

        @Override
        public void onEdgeDragStarted(int edgeFlags, int pointerId) {
            View drawerView = findDrawerView();
            if ((edgeFlags & ViewDragHelper.EDGE_LEFT) == ViewDragHelper.EDGE_LEFT) {
                if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.RIGHT)) {
                    drawerView = null;
                }
            } else {
                if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.LEFT)) {
                    drawerView = null;
                }
            }

            if (drawerView != null && getDrawerLockMode(drawerView) == LOCK_MODE_UNLOCKED) {
                mDragger.captureChildView(drawerView, pointerId);
            }
        }

        @Override
        public int getViewHorizontalDragRange(View child) {
            return child.getWidth();
        }

        @Override
        public int clampViewPositionHorizontal(View child, int left, int dx) {
            final View drawerView = findDrawerView();
            LayoutParams drawerLp = (LayoutParams) drawerView.getLayoutParams();
            if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.LEFT)) {
                return Math.max(drawerLp.getMarginStart() - drawerView.getWidth(),
                        Math.min(left, drawerLp.getMarginStart()));
            } else {
                return Math.max(drawerLp.getMarginStart() + getWidth() - drawerView.getWidth(),
                        Math.min(left, drawerLp.getMarginStart() + getWidth()));
            }
        }

        @Override
        public int clampViewPositionVertical(View child, int top, int dy) {
            return child.getTop();
        }
    }

    public static class LayoutParams extends MarginLayoutParams {

        public int gravity = Gravity.NO_GRAVITY;
        float onScreen;
        boolean knownOpen;

        public LayoutParams(Context c, AttributeSet attrs) {
            super(c, attrs);

            final TypedArray a = c.obtainStyledAttributes(attrs, LAYOUT_ATTRS);
            gravity = a.getInt(0, Gravity.NO_GRAVITY);
            a.recycle();
        }

        public LayoutParams(int width, int height) {
            super(width, height);
        }

        public LayoutParams(int width, int height, int gravity) {
            this(width, height);
            this.gravity = gravity;
        }

        public LayoutParams(LayoutParams source) {
            super(source);
            gravity = source.gravity;
        }

        public LayoutParams(ViewGroup.LayoutParams source) {
            super(source);
        }

        public LayoutParams(MarginLayoutParams source) {
            super(source);
        }
    }

    private static final class ViewFaderHolder {
        public final ViewFader viewFader;
        public final int startingColor;
        public final int endingColor;

        public ViewFaderHolder(ViewFader viewFader, int startingColor, int endingColor) {
            this.viewFader = viewFader;
            this.startingColor = startingColor;
            this.endingColor = endingColor;
        }

    }
}
