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

import junit.framework.TestCase;
import junit.framework.TestSuite;

/**
 * Provides facilities to log the start and end of a test run.
 * 
 * May want to refactor this and create two subclasses: TestCaseLogger
 * and TestSuiteLogger.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class LoggingUtil {

    private static final String TEST_CASE_SEPARATOR = "---------------";
    private static final String TEST_SUITE_SEPARATOR = "####################";

    private String testTitle;
    private String separator;
    private long startTime; 

    
    //-------------------------------------------------------------------------
    // General-purpose API to log messages
    //-------------------------------------------------------------------------
    
    /**
     * Log the specified message from the caller object.
     * @param caller the calling object
     * @param message the message to log
     */
    public static void log(Object caller, Object message) {
        
        String classNameNoPackage = getClassName(caller);
        String messageStr = (message==null) ? "null" : message.toString();
        String formattedMessage = "[" + classNameNoPackage + "] " + messageStr;
        writeLogMessage(formattedMessage);
    }
    
    
    //-------------------------------------------------------------------------
    // Factory Methods to get instance for TestCase or TestSuite
    //-------------------------------------------------------------------------
    
    /**
     * Return a LoggingUtil instance suitable for logging TestCase start and end
     * @param testCase the TestCase
     * @return a LoggingUtil
     */
    public static LoggingUtil getTestCaseLogger(TestCase testCase) {
        
        String title = getClassName(testCase) + "." + testCase.getName();
        return new LoggingUtil(title, TEST_CASE_SEPARATOR);
    }


    /**
     * Return a LoggingUtil instance suitable for logging TestSuite start and end
     * @param testSuite the TestSuite
     * @return a LoggingUtil
     */
    public static LoggingUtil getTestSuiteLogger(TestSuite testCase) {
        
        String title = "SUITE " + getClassName(testCase);
        return new LoggingUtil(title, TEST_SUITE_SEPARATOR);
    }


    /**
     * Constructor. Private to force access through the factory method(s) 
     */
    private LoggingUtil(String title, String separator) {
        this.startTime = System.currentTimeMillis();
        this.testTitle = title;
        this.separator = separator;
    }


    /**
     * Write out the the name of the test class and test name to the log
     */
    public void logStartOfTest() {
        
        writeLogMessage(separator + " [ START: " + testTitle + " ] " + separator);
    }


    /**
     * Write out the the name of the test class and test name to the log
     */
    public void logEndOfTest() {
        
        long elapsedTime = System.currentTimeMillis() - startTime;
        writeLogMessage(separator + " [ END: " 
            + testTitle
            + "   Time=" + elapsedTime
            + "ms ] "+ separator + "\n");
    }


    /**
     * Return the name of the class for the specified object, stripping off the package name
     * @return the name of the class, stripping off the package name
     */
    private static String getClassName(Object object) {
        
        // If it's already a class, then use as is
        Class theClass = (object instanceof Class) ? ((Class)object) : object.getClass();
        String className =  theClass.getName();
        
        int index = className.lastIndexOf(".");
        if (index != -1) {
            className = className.substring(index+1);
        }
        return className;
    }


    /**
     * Write the specified message out to the log
     * @param message the message to write
     */
    private static void writeLogMessage(String message) {
        // Don't want to use Trace -- it requires initialization of the system configuration
        //Trace.trace(message);
        System.out.println(message);
    }

}
