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

package android.widget.cts;

import android.app.Instrumentation;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ListPopupWindow;
import android.widget.Toolbar;
import android.widget.cts.util.TestUtils;
import android.widget.cts.util.ViewTestUtils;

import static org.mockito.Mockito.*;

@MediumTest
public class ToolbarTest extends ActivityInstrumentationTestCase2<ToolbarCtsActivity> {
    private Toolbar mMainToolbar;
    private ToolbarCtsActivity mActivity;

    public ToolbarTest() {
        super("android.widget.cts", ToolbarCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        mMainToolbar = mActivity.getMainToolbar();
    }

    public void testConstructor() {
        new Toolbar(mActivity);

        new Toolbar(mActivity, null);

        new Toolbar(mActivity, null, android.R.attr.toolbarStyle);

        new Toolbar(mActivity, null, 0, android.R.style.Widget_Material_Toolbar);
    }

    public void testTitleAndSubtitleContent() {
        // Note that this method is *not* annotated to run on the UI thread, and every
        // call to setTitle / setSubtitle is wrapped to wait until the next draw pass
        // of our main toolbar. While this is not strictly necessary to check the result
        // of getTitle / getSubtitle, this logic follows the path of deferred layout
        // and invalidation of the TextViews that show the title / subtitle in the Toolbar.

        final Instrumentation instrumentation = getInstrumentation();

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setTitle(R.string.toolbar_title));
        assertEquals(mActivity.getString(R.string.toolbar_title), mMainToolbar.getTitle());

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setTitle("New title"));
        assertEquals("New title", mMainToolbar.getTitle());

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setSubtitle(R.string.toolbar_subtitle));
        assertEquals(mActivity.getString(R.string.toolbar_subtitle), mMainToolbar.getSubtitle());

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setSubtitle("New subtitle"));
        assertEquals("New subtitle", mMainToolbar.getSubtitle());
    }

    public void testTitleAndSubtitleAppearance() {
        final Instrumentation instrumentation = getInstrumentation();

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setTitle(R.string.toolbar_title));
        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setSubtitle(R.string.toolbar_subtitle));

        // Since there are no APIs to get reference to the underlying implementation of
        // title and subtitle, here we are testing that calling the relevant APIs doesn't crash

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setTitleTextColor(Color.RED));
        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setSubtitleTextColor(Color.BLUE));

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setTitleTextAppearance(
                        mActivity, R.style.TextAppearance_NotColors));
        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setSubtitleTextAppearance(
                        mActivity, R.style.TextAppearance_WithColor));
    }

    @UiThreadTest
    public void testGetTitleMargins() {
        assertEquals(5, mMainToolbar.getTitleMarginStart());
        assertEquals(10, mMainToolbar.getTitleMarginTop());
        assertEquals(15, mMainToolbar.getTitleMarginEnd());
        assertEquals(20, mMainToolbar.getTitleMarginBottom());
    }

    @UiThreadTest
    public void testSetTitleMargins() {
        Toolbar toolbar = (Toolbar) mActivity.findViewById(R.id.toolbar2);

        toolbar.setTitleMargin(5, 10, 15, 20);
        assertEquals(5, toolbar.getTitleMarginStart());
        assertEquals(10, toolbar.getTitleMarginTop());
        assertEquals(15, toolbar.getTitleMarginEnd());
        assertEquals(20, toolbar.getTitleMarginBottom());

        toolbar.setTitleMarginStart(25);
        toolbar.setTitleMarginTop(30);
        toolbar.setTitleMarginEnd(35);
        toolbar.setTitleMarginBottom(40);
        assertEquals(25, toolbar.getTitleMarginStart());
        assertEquals(30, toolbar.getTitleMarginTop());
        assertEquals(35, toolbar.getTitleMarginEnd());
        assertEquals(40, toolbar.getTitleMarginBottom());
    }

    public void testMenuContent() {
        final Instrumentation instrumentation = getInstrumentation();

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.inflateMenu(R.menu.toolbar_menu));

        final Menu menu = mMainToolbar.getMenu();

        assertEquals(6, menu.size());
        assertEquals(R.id.action_highlight, menu.getItem(0).getItemId());
        assertEquals(R.id.action_edit, menu.getItem(1).getItemId());
        assertEquals(R.id.action_delete, menu.getItem(2).getItemId());
        assertEquals(R.id.action_ignore, menu.getItem(3).getItemId());
        assertEquals(R.id.action_share, menu.getItem(4).getItemId());
        assertEquals(R.id.action_print, menu.getItem(5).getItemId());

        assertFalse(mMainToolbar.hasExpandedActionView());

        Toolbar.OnMenuItemClickListener menuItemClickListener =
                mock(Toolbar.OnMenuItemClickListener.class);
        mMainToolbar.setOnMenuItemClickListener(menuItemClickListener);

        menu.performIdentifierAction(R.id.action_highlight, 0);
        verify(menuItemClickListener, times(1)).onMenuItemClick(
                menu.findItem(R.id.action_highlight));

        menu.performIdentifierAction(R.id.action_share, 0);
        verify(menuItemClickListener, times(1)).onMenuItemClick(
                menu.findItem(R.id.action_share));
    }

    public void testMenuOverflowShowHide() {
        final Instrumentation instrumentation = getInstrumentation();

        // Inflate menu and check that we're not showing overflow menu yet
        instrumentation.runOnMainSync(() -> mMainToolbar.inflateMenu(R.menu.toolbar_menu));
        assertFalse(mMainToolbar.isOverflowMenuShowing());

        // Ask to show overflow menu and check that it's showing
        instrumentation.runOnMainSync(() -> mMainToolbar.showOverflowMenu());
        instrumentation.waitForIdleSync();
        assertTrue(mMainToolbar.isOverflowMenuShowing());

        // Ask to hide the overflow menu and check that it's not showing
        instrumentation.runOnMainSync(() -> mMainToolbar.hideOverflowMenu());
        instrumentation.waitForIdleSync();
        assertFalse(mMainToolbar.isOverflowMenuShowing());
    }

    public void testMenuOverflowSubmenu() {
        final Instrumentation instrumentation = getInstrumentation();

        // Inflate menu and check that we're not showing overflow menu yet
        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.inflateMenu(R.menu.toolbar_menu));
        assertFalse(mMainToolbar.isOverflowMenuShowing());

        // Ask to show overflow menu and check that it's showing
        instrumentation.runOnMainSync(() -> mMainToolbar.showOverflowMenu());
        instrumentation.waitForIdleSync();
        assertTrue(mMainToolbar.isOverflowMenuShowing());

        // Register a mock menu item click listener on the toolbar
        Toolbar.OnMenuItemClickListener menuItemClickListener =
                mock(Toolbar.OnMenuItemClickListener.class);
        mMainToolbar.setOnMenuItemClickListener(menuItemClickListener);

        final Menu menu = mMainToolbar.getMenu();

        // Ask to "perform" the share action and check that the menu click listener has
        // been notified
        instrumentation.runOnMainSync(() -> menu.performIdentifierAction(R.id.action_share, 0));
        verify(menuItemClickListener, times(1)).onMenuItemClick(
                menu.findItem(R.id.action_share));

        // Ask to dismiss all the popups and check that we're not showing the overflow menu
        instrumentation.runOnMainSync(() -> mMainToolbar.dismissPopupMenus());
        instrumentation.waitForIdleSync();
        assertFalse(mMainToolbar.isOverflowMenuShowing());
    }

    public void testMenuOverflowIcon() {
        final Instrumentation instrumentation = getInstrumentation();

        // Inflate menu and check that we're not showing overflow menu yet
        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.inflateMenu(R.menu.toolbar_menu));

        final Drawable overflowIcon = mActivity.getDrawable(R.drawable.icon_red);
        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setOverflowIcon(overflowIcon));

        final Drawable toolbarOverflowIcon = mMainToolbar.getOverflowIcon();
        TestUtils.assertAllPixelsOfColor("Overflow icon is red", toolbarOverflowIcon,
                toolbarOverflowIcon.getIntrinsicWidth(), toolbarOverflowIcon.getIntrinsicHeight(),
                true, 0XFFFF0000, 1, false);
    }

    public void testActionView() {
        final Instrumentation instrumentation = getInstrumentation();

        // Inflate menu and check that we don't have an expanded action view
        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.inflateMenu(R.menu.toolbar_menu_search));
        assertFalse(mMainToolbar.hasExpandedActionView());

        // Expand search menu item's action view and verify that main toolbar has an expanded
        // action view
        final MenuItem searchMenuItem = mMainToolbar.getMenu().findItem(R.id.action_search);
        instrumentation.runOnMainSync(() -> searchMenuItem.expandActionView());
        instrumentation.waitForIdleSync();
        assertTrue(searchMenuItem.isActionViewExpanded());
        assertTrue(mMainToolbar.hasExpandedActionView());

        // Collapse search menu item's action view and verify that main toolbar doesn't have an
        // expanded action view
        instrumentation.runOnMainSync(() -> searchMenuItem.collapseActionView());
        instrumentation.waitForIdleSync();
        assertFalse(searchMenuItem.isActionViewExpanded());
        assertFalse(mMainToolbar.hasExpandedActionView());

        // Expand search menu item's action view again
        instrumentation.runOnMainSync(() -> searchMenuItem.expandActionView());
        instrumentation.waitForIdleSync();
        assertTrue(searchMenuItem.isActionViewExpanded());
        assertTrue(mMainToolbar.hasExpandedActionView());

        // Now collapse search menu item's action view via toolbar's API and verify that main
        // toolbar doesn't have an expanded action view
        instrumentation.runOnMainSync(() -> mMainToolbar.collapseActionView());
        instrumentation.waitForIdleSync();
        assertFalse(searchMenuItem.isActionViewExpanded());
        assertFalse(mMainToolbar.hasExpandedActionView());
    }

    public void testNavigationConfiguration() {
        final Instrumentation instrumentation = getInstrumentation();

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setNavigationIcon(R.drawable.icon_green));
        Drawable toolbarNavigationIcon = mMainToolbar.getNavigationIcon();
        TestUtils.assertAllPixelsOfColor("Navigation icon is green", toolbarNavigationIcon,
                toolbarNavigationIcon.getIntrinsicWidth(),
                toolbarNavigationIcon.getIntrinsicHeight(),
                true, 0xFF00FF00, 1, false);

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setNavigationIcon(mActivity.getDrawable(R.drawable.icon_blue)));
        toolbarNavigationIcon = mMainToolbar.getNavigationIcon();
        TestUtils.assertAllPixelsOfColor("Navigation icon is blue", toolbarNavigationIcon,
                toolbarNavigationIcon.getIntrinsicWidth(),
                toolbarNavigationIcon.getIntrinsicHeight(),
                true, 0xFF0000FF, 1, false);

        instrumentation.runOnMainSync(
                () -> mMainToolbar.setNavigationContentDescription(R.string.toolbar_navigation));
        assertEquals(mActivity.getResources().getString(R.string.toolbar_navigation),
                mMainToolbar.getNavigationContentDescription());

        instrumentation.runOnMainSync(
                () -> mMainToolbar.setNavigationContentDescription("Navigation legend"));
        assertEquals("Navigation legend", mMainToolbar.getNavigationContentDescription());
    }

    public void testLogoConfiguration() {
        final Instrumentation instrumentation = getInstrumentation();

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setLogo(R.drawable.icon_yellow));
        Drawable toolbarLogo = mMainToolbar.getLogo();
        TestUtils.assertAllPixelsOfColor("Logo is yellow", toolbarLogo,
                toolbarLogo.getIntrinsicWidth(),
                toolbarLogo.getIntrinsicHeight(),
                true, 0xFFFFFF00, 1, false);

        ViewTestUtils.runOnMainAndDrawSync(instrumentation, mMainToolbar,
                () -> mMainToolbar.setLogo(mActivity.getDrawable(R.drawable.icon_red)));
        toolbarLogo = mMainToolbar.getLogo();
        TestUtils.assertAllPixelsOfColor("Logo is red", toolbarLogo,
                toolbarLogo.getIntrinsicWidth(),
                toolbarLogo.getIntrinsicHeight(),
                true, 0xFFFF0000, 1, false);

        instrumentation.runOnMainSync(
                () -> mMainToolbar.setLogoDescription(R.string.toolbar_logo));
        assertEquals(mActivity.getResources().getString(R.string.toolbar_logo),
                mMainToolbar.getLogoDescription());

        instrumentation.runOnMainSync(
                () -> mMainToolbar.setLogoDescription("Logo legend"));
        assertEquals("Logo legend", mMainToolbar.getLogoDescription());
    }

    @UiThreadTest
    public void testContentInsetsLtr() {
        mMainToolbar.setLayoutDirection(View.LAYOUT_DIRECTION_LTR);

        mMainToolbar.setContentInsetsAbsolute(20, 25);
        assertEquals(20, mMainToolbar.getContentInsetLeft());
        assertEquals(20, mMainToolbar.getContentInsetStart());
        assertEquals(25, mMainToolbar.getContentInsetRight());
        assertEquals(25, mMainToolbar.getContentInsetEnd());

        mMainToolbar.setContentInsetsRelative(40, 20);
        assertEquals(40, mMainToolbar.getContentInsetLeft());
        assertEquals(40, mMainToolbar.getContentInsetStart());
        assertEquals(20, mMainToolbar.getContentInsetRight());
        assertEquals(20, mMainToolbar.getContentInsetEnd());
    }

    @UiThreadTest
    public void testContentInsetsRtl() {
        mMainToolbar.setLayoutDirection(View.LAYOUT_DIRECTION_RTL);

        mMainToolbar.setContentInsetsAbsolute(20, 25);
        assertEquals(20, mMainToolbar.getContentInsetLeft());
        assertEquals(25, mMainToolbar.getContentInsetStart());
        assertEquals(25, mMainToolbar.getContentInsetRight());
        assertEquals(20, mMainToolbar.getContentInsetEnd());

        mMainToolbar.setContentInsetsRelative(40, 20);
        assertEquals(20, mMainToolbar.getContentInsetLeft());
        assertEquals(40, mMainToolbar.getContentInsetStart());
        assertEquals(40, mMainToolbar.getContentInsetRight());
        assertEquals(20, mMainToolbar.getContentInsetEnd());
    }

    @UiThreadTest
    public void testPopupTheme() {
        mMainToolbar.setPopupTheme(R.style.ToolbarPopupTheme_Test);
        assertEquals(R.style.ToolbarPopupTheme_Test, mMainToolbar.getPopupTheme());
    }

    public void testNavigationOnClickListener() {
        View.OnClickListener mockListener = mock(View.OnClickListener.class);
        mMainToolbar.setNavigationOnClickListener(mockListener);

        verify(mockListener, never()).onClick(any(View.class));

        getInstrumentation().runOnMainSync(() -> mMainToolbar.getNavigationView().performClick());
        verify(mockListener, times(1)).onClick(any(View.class));

        verifyNoMoreInteractions(mockListener);
    }
}
