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

package com.android.tv.dvr;

import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.os.Build;
import android.support.test.filters.SdkSuppress;
import android.test.ServiceTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.tv.ApplicationSingletons;
import com.android.tv.MockTvApplication;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.feature.TestableFeature;
import com.android.tv.testing.FakeClock;

import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

/**
 * Tests for {@link DvrRecordingService}.
 */
@SmallTest
@SdkSuppress(minSdkVersion = Build.VERSION_CODES.N)
public class DvrRecordingServiceTest extends ServiceTestCase<DvrRecordingService> {

    @Mock Scheduler mMockScheduler;
    @Mock ApplicationSingletons mApplicationSingletons;
    private final TestableFeature mDvrFeature = CommonFeatures.DVR;
    private DvrDataManagerInMemoryImpl mDataManager;
    private DvrRecordingService mService;
    private FakeClock mFakeClock = FakeClock.createWithCurrentTime();

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDvrFeature.enableForTest();
        MockitoAnnotations.initMocks(this);
        mDataManager = new DvrDataManagerInMemoryImpl(getContext(), mFakeClock);
        setApplication(new MockTvApplication(mApplicationSingletons));
        when(mApplicationSingletons.getDvrDataManager()).thenReturn(mDataManager);
        setupService();
        mService = getService();
        mService.setScheduler(mMockScheduler);
    }

    @Override
    protected void tearDown() throws Exception {
        mDvrFeature.resetForTests();
        super.tearDown();
    }

    public DvrRecordingServiceTest() {
        super(DvrRecordingService.class);
    }

    public void testStartService_null() throws Exception {
        startService(null);
        verify(mMockScheduler, Mockito.only()).update();
    }
}