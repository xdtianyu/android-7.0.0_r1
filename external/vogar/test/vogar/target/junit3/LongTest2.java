/*
 * Copyright (C) 2010 The Android Open Source Project
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

package vogar.target.junit3;

import junit.framework.TestCase;

public class LongTest2 extends TestCase {
    public LongTest2(String name) {
        super(name);
    }

    private void sleep() {
        try {
            Thread.sleep(1 * 1000 / 4);
        } catch (InterruptedException e) {
        }
    }

    public void test1() {
        sleep();
    }

    public void test2() {
        sleep();
    }

    public void test3() {
        sleep();
    }

    public void test4() {
        sleep();
    }

    public void test5() {
        sleep();
    }

    public void test6() {
        sleep();
    }

    public void test7() {
        sleep();
    }

    public void test8() {
        sleep();
    }
}
