/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.messageformat;

import java.util.Locale;

public class MessageFormatBenchmark {
    public void timePlurals(int nreps) throws Exception {
        final Locale sr = new Locale("sr");
        for (int i = 0; i < nreps; ++i) {
            String msg = "{num,plural,offset:1 =1{only {name}}=2{{name} and one other}" +
                    "one{{name} and #-one others}few{{name} and #-few others}" +
                    "other{{name} and #... others}}";
            MessageFormat.formatNamedArgs(sr, msg, "num", 1, "name", "Peter");
        }
    }

    public void timeGenders(int nreps) throws Exception {
        for (int i = 0; i < nreps; ++i) {
            String msg = "{gender,select,female{her book}male{his book}other{their book}}";
            MessageFormat.formatNamedArgs(Locale.US, msg, "gender", "female");
        }
    }
}
