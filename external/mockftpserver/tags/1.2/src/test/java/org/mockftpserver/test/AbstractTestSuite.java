/*
 * Copyright 2007 the original author or authors.
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
package org.mockftpserver.test;

import junit.framework.TestResult;
import junit.framework.TestSuite;

/**
 * The absttract superclass for TestSuites that allow registering 
 * TestSuiteListeners to add customized behavior before and after the
 * test suite is executed. 
 *
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public class AbstractTestSuite extends TestSuite {

    /**
     * Constructor that takes the test name
     * @param testName Name of test
     */
    public AbstractTestSuite() {}

    //-----------------------------------------------------------
    // Overridden TestSuite Methods
    //-----------------------------------------------------------

    /**
     * Override this method to insert hooks for before and after the test suite run
     * @see junit.framework.Test#run(junit.framework.TestResult)
     */
    public void run(TestResult arg0) {

        LoggingUtil loggingUtil = LoggingUtil.getTestSuiteLogger(this);
        loggingUtil.logStartOfTest();

        // Run this test suite
        super.run(arg0);

        loggingUtil.logEndOfTest();
    }

}
