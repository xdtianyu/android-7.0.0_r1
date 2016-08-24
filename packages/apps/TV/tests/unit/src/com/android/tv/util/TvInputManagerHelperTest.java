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

package com.android.tv.util;

import android.content.pm.ResolveInfo;
import android.media.tv.TvInputInfo;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;
import android.test.suitebuilder.annotation.Suppress;

import com.android.tv.testing.ComparatorTester;

import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.util.LinkedHashMap;

/**
 * Test for {@link TvInputManagerHelper}
 */
@SmallTest
public class TvInputManagerHelperTest extends AndroidTestCase {
    @Suppress  // http://b/26903987
    public void testComparator() throws Exception {
        final LinkedHashMap<String, Boolean> INPUT_ID_TO_PARTNER_INPUT = new LinkedHashMap<>();
        INPUT_ID_TO_PARTNER_INPUT.put("2_partner_input", true);
        INPUT_ID_TO_PARTNER_INPUT.put("3_partner_input", true);
        INPUT_ID_TO_PARTNER_INPUT.put("1_3rd_party_input", false);
        INPUT_ID_TO_PARTNER_INPUT.put("4_3rd_party_input", false);

        TvInputManagerHelper manager = Mockito.mock(TvInputManagerHelper.class);
        Mockito.doAnswer(new Answer<Boolean>() {
            @Override
            public Boolean answer(InvocationOnMock invocation) throws Throwable {
                TvInputInfo info = (TvInputInfo) invocation.getArguments()[0];
                return INPUT_ID_TO_PARTNER_INPUT.get(info.getId());
            }
        }).when(manager).isPartnerInput(Mockito.<TvInputInfo>any());
        Mockito.doAnswer(new Answer<String>() {
            @Override
            public String answer(InvocationOnMock invocation) throws Throwable {
                TvInputInfo info = (TvInputInfo) invocation.getArguments()[0];
                return info.getId();
            }
        }).when(manager).loadLabel(Mockito.<TvInputInfo>any());

        ComparatorTester<TvInputInfo> comparatorTester =
                ComparatorTester.withoutEqualsTest(
                        new TvInputManagerHelper.TvInputInfoComparator(manager));
        ResolveInfo resolveInfo1 = TestUtils.createResolveInfo("1_test", "1_test");
        ResolveInfo resolveInfo2 = TestUtils.createResolveInfo("2_test", "2_test");
        for (String inputId : INPUT_ID_TO_PARTNER_INPUT.keySet()) {
            TvInputInfo info1 = TestUtils.createTvInputInfo(resolveInfo1, inputId, null, 0, false);
            TvInputInfo info2 = TestUtils.createTvInputInfo(resolveInfo2, inputId, null, 0, false);
            comparatorTester.addComparableGroup(info1, info2);
        }
        comparatorTester.test();
    }
}
