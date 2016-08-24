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
 * limitations under the License
 */

package android.widget.cts;

import android.app.Activity;
import android.app.Instrumentation;
import android.os.SystemClock;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SubMenu;
import android.view.View;
import android.widget.EditText;
import android.widget.PopupMenu;

import static org.mockito.Mockito.*;

@SmallTest
public class PopupMenuTest extends
        ActivityInstrumentationTestCase2<PopupMenuCtsActivity> {
    private Instrumentation mInstrumentation;
    private Activity mActivity;

    private Builder mBuilder;
    private PopupMenu mPopupMenu;

    public PopupMenuTest() {
        super("android.widget.cts", PopupMenuCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mInstrumentation = getInstrumentation();
        mActivity = getActivity();

        try {
            runTestOnUiThread(() -> {
                // Disable and remove focusability on the first child of our activity so that
                // it doesn't bring in the soft keyboard that can mess up with some of the tests
                // (such as menu dismissal when we emulate a tap outside the menu bounds).
                final EditText editText = (EditText) mActivity.findViewById(R.id.anchor_upper_left);
                editText.setEnabled(false);
                editText.setFocusable(false);
            });
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    @Override
    protected void tearDown() throws Exception {
        if (mPopupMenu != null) {
            try {
                runTestOnUiThread(() -> mPopupMenu.dismiss());
            } catch (Throwable t) {
                throw new RuntimeException(t);
            }
        }
        super.tearDown();
    }

    private void verifyMenuContent() {
        final Menu menu = mPopupMenu.getMenu();
        assertEquals(6, menu.size());
        assertEquals(R.id.action_highlight, menu.getItem(0).getItemId());
        assertEquals(R.id.action_edit, menu.getItem(1).getItemId());
        assertEquals(R.id.action_delete, menu.getItem(2).getItemId());
        assertEquals(R.id.action_ignore, menu.getItem(3).getItemId());
        assertEquals(R.id.action_share, menu.getItem(4).getItemId());
        assertEquals(R.id.action_print, menu.getItem(5).getItemId());

        final SubMenu shareSubMenu = menu.getItem(4).getSubMenu();
        assertNotNull(shareSubMenu);
        assertEquals(2, shareSubMenu.size());
        assertEquals(R.id.action_share_email, shareSubMenu.getItem(0).getItemId());
        assertEquals(R.id.action_share_circles, shareSubMenu.getItem(1).getItemId());
    }

    public void testPopulateViaInflater() throws Throwable {
        mBuilder = new Builder().inflateWithInflater(true);
        runTestOnUiThread(() -> mBuilder.show());
        mInstrumentation.waitForIdleSync();

        verifyMenuContent();
    }

    public void testDirectPopulate() throws Throwable {
        mBuilder = new Builder().inflateWithInflater(false);
        runTestOnUiThread(() -> mBuilder.show());
        mInstrumentation.waitForIdleSync();

        verifyMenuContent();
    }

    public void testAccessGravity() throws Throwable {
        mBuilder = new Builder();
        runTestOnUiThread(() -> mBuilder.show());

        assertEquals(Gravity.NO_GRAVITY, mPopupMenu.getGravity());
        mPopupMenu.setGravity(Gravity.TOP);
        assertEquals(Gravity.TOP, mPopupMenu.getGravity());
    }

    public void testConstructorWithGravity() throws Throwable {
        mBuilder = new Builder().withGravity(Gravity.TOP);
        runTestOnUiThread(() -> mBuilder.show());

        assertEquals(Gravity.TOP, mPopupMenu.getGravity());
    }

    public void testDismissalViaAPI() throws Throwable {
        mBuilder = new Builder().withDismissListener();
        runTestOnUiThread(() -> mBuilder.show());

        mInstrumentation.waitForIdleSync();
        verify(mBuilder.mOnDismissListener, never()).onDismiss(mPopupMenu);

        runTestOnUiThread(() -> mPopupMenu.dismiss());
        mInstrumentation.waitForIdleSync();
        verify(mBuilder.mOnDismissListener, times(1)).onDismiss(mPopupMenu);

        runTestOnUiThread(() -> mPopupMenu.dismiss());
        mInstrumentation.waitForIdleSync();
        // Shouldn't have any more interactions with our dismiss listener since the menu was
        // already dismissed when we called dismiss()
        verifyNoMoreInteractions(mBuilder.mOnDismissListener);
    }

    public void testNestedDismissalViaAPI() throws Throwable {
        // Use empty popup style to remove all transitions from the popup. That way we don't
        // need to synchronize with the popup window enter transition before proceeding to
        // "click" a submenu item.
        mBuilder = new Builder().withDismissListener()
                .withPopupStyleResource(R.style.PopupWindow_NullTransitions);
        runTestOnUiThread(() -> mBuilder.show());
        mInstrumentation.waitForIdleSync();
        verify(mBuilder.mOnDismissListener, never()).onDismiss(mPopupMenu);

        runTestOnUiThread(() -> mPopupMenu.getMenu().performIdentifierAction(R.id.action_share, 0));
        mInstrumentation.waitForIdleSync();

        runTestOnUiThread(() -> mPopupMenu.getMenu().findItem(R.id.action_share).getSubMenu().
                        performIdentifierAction(R.id.action_share_email, 0));
        mInstrumentation.waitForIdleSync();

        runTestOnUiThread(() -> mPopupMenu.dismiss());
        mInstrumentation.waitForIdleSync();
        verify(mBuilder.mOnDismissListener, times(1)).onDismiss(mPopupMenu);

        runTestOnUiThread(() -> mPopupMenu.dismiss());
        mInstrumentation.waitForIdleSync();
        // Shouldn't have any more interactions with our dismiss listener since the menu was
        // already dismissed when we called dismiss()
        verifyNoMoreInteractions(mBuilder.mOnDismissListener);
    }

    public void testDismissalViaTouch() throws Throwable {
        // Use empty popup style to remove all transitions from the popup. That way we don't
        // need to synchronize with the popup window enter transition before proceeding to
        // emulate a click outside the popup window bounds.
        mBuilder = new Builder().withDismissListener()
                .withPopupMenuContent(R.menu.popup_menu_single)
                .withPopupStyleResource(R.style.PopupWindow_NullTransitions);
        runTestOnUiThread(() -> mBuilder.show());
        mInstrumentation.waitForIdleSync();

        // Determine the location of the anchor on the screen so that we can emulate
        // a tap outside of the popup bounds to dismiss the popup
        final int[] anchorOnScreenXY = new int[2];
        mBuilder.mAnchor.getLocationOnScreen(anchorOnScreenXY);

        int emulatedTapX = anchorOnScreenXY[0] + 10;
        int emulatedTapY = anchorOnScreenXY[1] - 20;

        // The logic below uses Instrumentation to emulate a tap outside the bounds of the
        // displayed popup menu. This tap is then treated by the framework to be "split" as
        // the ACTION_OUTSIDE for the popup itself, as well as DOWN / MOVE / UP for the underlying
        // view root if the popup is not modal.
        // It is not correct to emulate these two sequences separately in the test, as it
        // wouldn't emulate the user-facing interaction for this test. Note that usage
        // of Instrumentation is necessary here since Espresso's actions operate at the level
        // of view or data. Also, we don't want to use View.dispatchTouchEvent directly as
        // that would require emulation of two separate sequences as well.

        // Inject DOWN event
        long downTime = SystemClock.uptimeMillis();
        MotionEvent eventDown = MotionEvent.obtain(
                downTime, downTime, MotionEvent.ACTION_DOWN, emulatedTapX, emulatedTapY, 1);
        mInstrumentation.sendPointerSync(eventDown);

        // Inject MOVE event
        long moveTime = SystemClock.uptimeMillis();
        MotionEvent eventMove = MotionEvent.obtain(
                moveTime, moveTime, MotionEvent.ACTION_MOVE, emulatedTapX, emulatedTapY, 1);
        mInstrumentation.sendPointerSync(eventMove);

        // Inject UP event
        long upTime = SystemClock.uptimeMillis();
        MotionEvent eventUp = MotionEvent.obtain(
                upTime, upTime, MotionEvent.ACTION_UP, emulatedTapX, emulatedTapY, 1);
        mInstrumentation.sendPointerSync(eventUp);

        // Wait for the system to process all events in the queue
        mInstrumentation.waitForIdleSync();

        // At this point our popup should have notified its dismiss listener
        verify(mBuilder.mOnDismissListener, times(1)).onDismiss(mPopupMenu);
    }

    public void testSimpleMenuItemClickViaAPI() throws Throwable {
        mBuilder = new Builder().withMenuItemClickListener().withDismissListener();
        runTestOnUiThread(() -> mBuilder.show());

        // Verify that our menu item click listener hasn't been called yet
        verify(mBuilder.mOnMenuItemClickListener, never()).onMenuItemClick(any(MenuItem.class));

        runTestOnUiThread(
                () -> mPopupMenu.getMenu().performIdentifierAction(R.id.action_highlight, 0));

        // Verify that our menu item click listener has been called with the expected menu item
        verify(mBuilder.mOnMenuItemClickListener, times(1)).onMenuItemClick(
                mPopupMenu.getMenu().findItem(R.id.action_highlight));

        // Popup menu should be automatically dismissed on selecting an item
        verify(mBuilder.mOnDismissListener, times(1)).onDismiss(mPopupMenu);
        verifyNoMoreInteractions(mBuilder.mOnDismissListener);
    }

    public void testSubMenuClickViaAPI() throws Throwable {
        // Use empty popup style to remove all transitions from the popup. That way we don't
        // need to synchronize with the popup window enter transition before proceeding to
        // "click" a submenu item.
        mBuilder = new Builder().withDismissListener().withMenuItemClickListener()
                .withPopupStyleResource(R.style.PopupWindow_NullTransitions);
        runTestOnUiThread(() -> mBuilder.show());
        mInstrumentation.waitForIdleSync();

        // Verify that our menu item click listener hasn't been called yet
        verify(mBuilder.mOnMenuItemClickListener, never()).onMenuItemClick(any(MenuItem.class));

        runTestOnUiThread(() -> mPopupMenu.getMenu().performIdentifierAction(R.id.action_share, 0));
        // Verify that our menu item click listener has been called on "share" action
        // and that the dismiss listener hasn't been called just as a result of opening the submenu.
        verify(mBuilder.mOnMenuItemClickListener, times(1)).onMenuItemClick(
                mPopupMenu.getMenu().findItem(R.id.action_share));
        verify(mBuilder.mOnDismissListener, never()).onDismiss(mPopupMenu);

        runTestOnUiThread(() -> mPopupMenu.getMenu().findItem(R.id.action_share).getSubMenu().
                        performIdentifierAction(R.id.action_share_email, 0));

        // Verify that out menu item click listener has been called with the expected menu item
        verify(mBuilder.mOnMenuItemClickListener, times(1)).onMenuItemClick(
                mPopupMenu.getMenu().findItem(R.id.action_share).getSubMenu()
                        .findItem(R.id.action_share_email));
        verifyNoMoreInteractions(mBuilder.mOnMenuItemClickListener);

        // Popup menu should be automatically dismissed on selecting an item
        verify(mBuilder.mOnDismissListener, times(1)).onDismiss(mPopupMenu);
        verifyNoMoreInteractions(mBuilder.mOnDismissListener);
    }

    /**
     * Inner helper class to configure an instance of {@link PopupMenu} for the specific test.
     * The main reason for its existence is that once a popup menu is shown with the show() method,
     * most of its configuration APIs are no-ops. This means that we can't add logic that is
     * specific to a certain test once it's shown and we have a reference to a displayed
     * {@link PopupMenu}.
     */
    public class Builder {
        private boolean mHasDismissListener;
        private boolean mHasMenuItemClickListener;
        private boolean mInflateWithInflater;

        private int mPopupMenuContent = R.menu.popup_menu;

        private boolean mUseCustomPopupResource;
        private int mPopupStyleResource = 0;

        private boolean mUseCustomGravity;
        private int mGravity = Gravity.NO_GRAVITY;

        private PopupMenu.OnMenuItemClickListener mOnMenuItemClickListener;
        private PopupMenu.OnDismissListener mOnDismissListener;

        private View mAnchor;

        public Builder withMenuItemClickListener() {
            mHasMenuItemClickListener = true;
            return this;
        }

        public Builder withDismissListener() {
            mHasDismissListener = true;
            return this;
        }

        public Builder inflateWithInflater(boolean inflateWithInflater) {
            mInflateWithInflater = inflateWithInflater;
            return this;
        }

        public Builder withPopupStyleResource(int popupStyleResource) {
            mUseCustomPopupResource = true;
            mPopupStyleResource = popupStyleResource;
            return this;
        }

        public Builder withPopupMenuContent(int popupMenuContent) {
            mPopupMenuContent = popupMenuContent;
            return this;
        }

        public Builder withGravity(int gravity) {
            mUseCustomGravity = true;
            mGravity = gravity;
            return this;
        }

        private void configure() {
            mAnchor = mActivity.findViewById(R.id.anchor_middle_left);
            if (!mUseCustomGravity && !mUseCustomPopupResource) {
                mPopupMenu = new PopupMenu(mActivity, mAnchor);
            } else if (!mUseCustomPopupResource) {
                mPopupMenu = new PopupMenu(mActivity, mAnchor, mGravity);
            } else {
                mPopupMenu = new PopupMenu(mActivity, mAnchor, Gravity.NO_GRAVITY,
                        0, mPopupStyleResource);
            }

            if (mInflateWithInflater) {
                final MenuInflater menuInflater = mPopupMenu.getMenuInflater();
                menuInflater.inflate(mPopupMenuContent, mPopupMenu.getMenu());
            } else {
                mPopupMenu.inflate(mPopupMenuContent);
            }

            if (mHasMenuItemClickListener) {
                // Register a mock listener to be notified when a menu item in our popup menu has
                // been clicked.
                mOnMenuItemClickListener = mock(PopupMenu.OnMenuItemClickListener.class);
                mPopupMenu.setOnMenuItemClickListener(mOnMenuItemClickListener);
            }

            if (mHasDismissListener) {
                // Register a mock listener to be notified when our popup menu is dismissed.
                mOnDismissListener = mock(PopupMenu.OnDismissListener.class);
                mPopupMenu.setOnDismissListener(mOnDismissListener);
            }
        }

        public void show() {
            configure();
            // Show the popup menu
            mPopupMenu.show();
        }
    }
}
