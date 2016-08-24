/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.ide.eclipse.adt.internal.lint;

import com.android.annotations.NonNull;
import com.android.tools.lint.checks.*;
import com.android.tools.lint.detector.api.Issue;

import java.util.ArrayList;
import java.util.List;

public class EclipseLintIssueRegistry extends BuiltinIssueRegistry {
    private static List<Issue> sFilteredIssues;

    public EclipseLintIssueRegistry() {
    }

    @NonNull
    @Override
    public List<Issue> getIssues() {
        if (sFilteredIssues == null) {
            // Remove issues that do not work properly in Eclipse
            List<Issue> sIssues = super.getIssues();
            List<Issue> result = new ArrayList<Issue>(sIssues.size());
            for (Issue issue : sIssues) {
                if (issue == MissingClassDetector.INSTANTIATABLE) {
                    // Apparently violated by
                    // android.support.v7.internal.widget.ActionBarView.HomeView
                    // See issue 72760
                    continue;
                } else if (issue == DuplicateIdDetector.WITHIN_LAYOUT) {
                    // Apparently violated by
                    // sdk/extras/android/support/v7/appcompat/abc_activity_chooser_view_include.xml
                    // See issue 72760
                    continue;
                } else if (issue == AppCompatResourceDetector.ISSUE
                        || issue == AppCompatCallDetector.ISSUE) {
                    // Apparently has some false positives in Eclipse; see issue
                    // 72824
                    continue;
                } else if (issue.getImplementation().getDetectorClass() == RtlDetector.class) {
                    // False positives in Eclipse; see issue 78780
                    continue;
                }
                result.add(issue);
            }
            sFilteredIssues = result;
        }

        return sFilteredIssues;
    }
}
