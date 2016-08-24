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
package android.media.cts;

import android.content.Context;
import android.media.MediaRouter;
import android.media.MediaRouter.RouteCategory;
import android.media.MediaRouter.RouteInfo;
import android.media.MediaRouter.UserRouteInfo;
import android.test.InstrumentationTestCase;

import java.util.List;
import java.util.ArrayList;

/**
 * Test {@link android.media.MediaRouter}.
 */
public class MediaRouterTest extends InstrumentationTestCase {

    private MediaRouter mMediaRouter;
    private RouteCategory mTestCategory;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        final Context context = getInstrumentation().getContext();
        mMediaRouter = (MediaRouter) context.getSystemService(Context.MEDIA_ROUTER_SERVICE);
        mTestCategory = mMediaRouter.createRouteCategory("testCategory", false);
    }

    protected void tearDown() throws Exception {
        mMediaRouter.clearUserRoutes();
        super.tearDown();
    }

    public void testSelectRoute() throws Exception {
        RouteInfo prevSelectedRoute = mMediaRouter.getSelectedRoute(
                MediaRouter.ROUTE_TYPE_LIVE_AUDIO | MediaRouter.ROUTE_TYPE_LIVE_VIDEO
                | MediaRouter.ROUTE_TYPE_REMOTE_DISPLAY);

        final String newRouteName = "User route's name";
        UserRouteInfo newRoute = mMediaRouter.createUserRoute(mTestCategory);
        newRoute.setName(newRouteName);
        mMediaRouter.addUserRoute(newRoute);
        mMediaRouter.selectRoute(newRoute.getSupportedTypes(), newRoute);

        RouteInfo nowSelectedRoute = mMediaRouter.getSelectedRoute(MediaRouter.ROUTE_TYPE_USER);
        assertEquals(newRoute, nowSelectedRoute);
        assertEquals(mTestCategory, nowSelectedRoute.getCategory());

        mMediaRouter.selectRoute(prevSelectedRoute.getSupportedTypes(), prevSelectedRoute);
    }

    public void testGetRouteCount() throws Exception {
        final int count = mMediaRouter.getRouteCount();
        assertTrue("By default, a media router has at least one route.", count > 0);

        UserRouteInfo userRoute1 = mMediaRouter.createUserRoute(mTestCategory);
        mMediaRouter.addUserRoute(userRoute1);
        assertEquals(count + 1, mMediaRouter.getRouteCount());

        mMediaRouter.removeUserRoute(userRoute1);
        assertEquals(count, mMediaRouter.getRouteCount());

        UserRouteInfo userRoute2 = mMediaRouter.createUserRoute(mTestCategory);
        mMediaRouter.addUserRoute(userRoute1);
        mMediaRouter.addUserRoute(userRoute2);
        assertEquals(count + 2, mMediaRouter.getRouteCount());

        mMediaRouter.clearUserRoutes();
        assertEquals(count, mMediaRouter.getRouteCount());
    }

    public void testRouteCategory() throws Exception {
        final int count = mMediaRouter.getCategoryCount();
        assertTrue("By default, a media router has at least one route category.", count > 0);

        UserRouteInfo newRoute = mMediaRouter.createUserRoute(mTestCategory);
        mMediaRouter.addUserRoute(newRoute);
        assertEquals(count + 1, mMediaRouter.getCategoryCount());

        for (int i = 0; i < mMediaRouter.getCategoryCount(); i++) {
            if (mMediaRouter.getCategoryAt(i) == mTestCategory) {
                List<RouteInfo> routesInCategory = new ArrayList<RouteInfo>();
                mTestCategory.getRoutes(routesInCategory);
                assertEquals(1, routesInCategory.size());

                RouteInfo route = routesInCategory.get(0);
                assertEquals(newRoute, route);
                return;
            }
        }
        assertTrue(false);
    }

    public void testRouteInfo_getDeviceType() throws Exception {
        final RouteInfo defaultRoute = mMediaRouter.getDefaultRoute();
        assertTrue(defaultRoute != null);

        final int deviceType = defaultRoute.getDeviceType();
        assertEquals(RouteInfo.DEVICE_TYPE_UNKNOWN, deviceType);
    }
}
