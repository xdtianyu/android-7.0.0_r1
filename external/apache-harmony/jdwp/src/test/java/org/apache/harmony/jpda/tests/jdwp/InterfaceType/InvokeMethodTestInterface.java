/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.apache.harmony.jpda.tests.jdwp.InterfaceType;

import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.share.JPDATestOptions;

/**
 * Used for InterfaceType.InvokeMethodTest
 */
public interface InvokeMethodTestInterface {
    public static final int RETURN_VALUE = 567;
    public static int testInvokeMethodStatic1(boolean needsThrow) throws Throwable {
        if (needsThrow) {
            throw new Throwable("test exception");
        }
        return InvokeMethodTestInterface.RETURN_VALUE;
    }
}
