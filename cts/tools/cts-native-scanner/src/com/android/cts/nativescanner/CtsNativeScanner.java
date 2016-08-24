/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.cts.nativescanner;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.Arrays;
import java.util.List;

/**
 * Class that searches a source directory for native gTests and outputs a
 * list of test classes and methods.
 */
public class CtsNativeScanner {

    private static void usage(String[] args) {
        System.err.println("Arguments: " + Arrays.asList(args));
        System.err.println("Usage: cts-native-scanner -t TEST_SUITE");
        System.err.println("  This code reads from stdin the list of tests.");
        System.err.println("  The format expected:");
        System.err.println("    TEST_CASE_NAME.");
        System.err.println("      TEST_NAME");
        System.exit(1);
    }

    public static void main(String[] args) throws Exception {
        String testSuite = null;
        for (int i = 0; i < args.length; i++) {
            if ("-t".equals(args[i])) {
                testSuite = getArg(args, ++i, "Missing value for test suite");
            } else {
                System.err.println("Unsupported flag: " + args[i]);
                usage(args);
            }
        }

        if (testSuite == null) {
            System.out.println("Test suite is required");
            usage(args);
        }

        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        TestScanner scanner = new TestScanner(reader, testSuite);
        for (String name : scanner.getTestNames()) {
            System.out.println(name);
        }
    }

    private static String getArg(String[] args, int index, String message) {
        if (index < args.length) {
            return args[index];
        } else {
            System.err.println(message);
            usage(args);
            return null;
        }
    }
}
