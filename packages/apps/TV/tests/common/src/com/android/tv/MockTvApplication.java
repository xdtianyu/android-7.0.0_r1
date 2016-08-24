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

package com.android.tv;

import android.test.mock.MockApplication;

import com.android.tv.analytics.Analytics;
import com.android.tv.analytics.Tracker;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.ProgramDataManager;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.DvrManager;
import com.android.tv.dvr.DvrSessionManager;
import com.android.tv.util.TvInputManagerHelper;

/**
 * A mock TV Application used for testing.
 */
public class MockTvApplication extends MockApplication implements ApplicationSingletons {
    public final ApplicationSingletons mDelegate;

    public MockTvApplication(ApplicationSingletons delegate) {
        mDelegate = delegate;
    }

    @Override
    public DvrManager getDvrManager() {
        return mDelegate.getDvrManager();
    }

    @Override
    public DvrSessionManager getDvrSessionManger() {
        return mDelegate.getDvrSessionManger();
    }

    @Override
    public Analytics getAnalytics() {
        return mDelegate.getAnalytics();
    }

    @Override
    public Tracker getTracker() {
        return mDelegate.getTracker();
    }

    @Override
    public ChannelDataManager getChannelDataManager() {
        return mDelegate.getChannelDataManager();
    }

    @Override
    public ProgramDataManager getProgramDataManager() {
        return mDelegate.getProgramDataManager();
    }

    @Override
    public DvrDataManager getDvrDataManager() {
        return mDelegate.getDvrDataManager();
    }

    @Override
    public TvInputManagerHelper getTvInputManagerHelper() {
        return mDelegate.getTvInputManagerHelper();
    }

    @Override
    public MainActivityWrapper getMainActivityWrapper() {
        return mDelegate.getMainActivityWrapper();
    }
}
