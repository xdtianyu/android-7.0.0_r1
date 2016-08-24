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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.easymock.MockControl;
import org.mockftpserver.core.MockFtpServerException;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.AssertFailedException;

import java.io.File;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Abstract superclass for all project test classes
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public abstract class AbstractTestCase extends TestCase {

    private static final Logger LOG = LoggerFactory.getLogger(AbstractTestCase.class);
    protected static final List EMPTY_LIST = Collections.EMPTY_LIST;
    protected static final String[] EMPTY = new String[0];
    protected static final InetAddress DEFAULT_HOST = inetAddress(null);

    /**
     * Constructor
     */
    public AbstractTestCase() {
        super();
    }

    //-------------------------------------------------------------------------
    // Manage EasyMock Control objects under the covers, and provide a syntax
    // somewhat similar to EasyMock 2.2 for createMock, verify and replay.
    //-------------------------------------------------------------------------

    private Map mocks = new HashMap();

    /**
     * Create a new mock for the specified interface. Keep track of the associated control object
     * under the covers to support the associated  method.
     *
     * @param interfaceToMock - the Class of the interface to be mocked
     * @return the new mock
     */
    protected Object createMock(Class interfaceToMock) {
        MockControl control = MockControl.createControl(interfaceToMock);
        Object mock = control.getMock();
        mocks.put(mock, control);
        return mock;
    }

    /**
     * Put the mock object into replay mode
     *
     * @param mock - the mock to set in replay mode
     * @throws AssertFailedException - if mock is null
     * @throws AssertFailedException - if mock is not a mock object created using {@link #createMock(Class)}
     */
    protected void replay(Object mock) {
        control(mock).replay();
    }

    /**
     * Put all mocks created with createMock() into replay mode.
     */
    protected void replayAll() {
        for (Iterator iter = mocks.keySet().iterator(); iter.hasNext();) {
            Object mock = iter.next();
            replay(mock);
        }
    }

    /**
     * Verify the mock object
     *
     * @param mock - the mock to verify
     * @throws AssertFailedException - if mock is null
     * @throws AssertFailedException - if mock is not a mock object created using {@link #createMock(Class)}
     */
    protected void verify(Object mock) {
        control(mock).verify();
    }

    /**
     * Verify all mocks created with createMock() into replay mode.
     */
    protected void verifyAll() {
        for (Iterator iter = mocks.keySet().iterator(); iter.hasNext();) {
            Object mock = iter.next();
            verify(mock);
        }
    }

    /**
     * Return the mock control associated with the mock
     *
     * @param mock - the mock
     * @return the associated MockControl
     * @throws AssertFailedException - if mock is null
     * @throws AssertFailedException - if mock is not a mock object created using {@link #createMock(Class)}
     */
    protected MockControl control(Object mock) {
        Assert.notNull(mock, "mock");
        MockControl control = (MockControl) mocks.get(mock);
        Assert.notNull(control, "control");
        return control;
    }

    //-------------------------------------------------------------------------
    // Other Helper Methods
    //-------------------------------------------------------------------------

    /**
     * Assert that the two objects are not equal
     *
     * @param object1 - the first object
     * @param object2 - the second object
     */
    protected void assertNotEquals(String message, Object object1, Object object2) {
        assertFalse(message, object1.equals(object2));
    }

    /**
     * Assert that the two byte arrays have the same length and content
     *
     * @param array1 - the first array
     * @param array2 - the second array
     */
    protected void assertEquals(String message, byte[] array1, byte[] array2) {
        assertTrue("Arrays not equal: " + message, Arrays.equals(array1, array2));
    }

    /**
     * Assert that the two Object arrays have the same length and content
     *
     * @param array1 - the first array
     * @param array2 - the second array
     */
    protected void assertEquals(String message, Object[] array1, Object[] array2) {
        assertTrue("Arrays not equal: " + message, Arrays.equals(array1, array2));
    }

    /**
     * Create and return a one-element Object[] containing the specified Object
     *
     * @param o - the object
     * @return the Object array, of length 1, containing o
     */
    protected static Object[] objArray(Object o) {
        return new Object[]{o};
    }

    /**
     * Create and return a one-element String[] containing the specified String
     *
     * @param s - the String
     * @return the String array, of length 1, containing s
     */
    protected static String[] array(String s) {
        return new String[]{s};
    }

    /**
     * Create and return a two-element String[] containing the specified Strings
     *
     * @param s1 - the first String
     * @param s2 - the second String
     * @return the String array, of length 2, containing s1 and s2
     */
    protected static String[] array(String s1, String s2) {
        return new String[]{s1, s2};
    }

    /**
     * Create a new InetAddress from the specified host String, using the
     * {@link InetAddress#getByName(String)} method, wrapping any checked
     * exception within a unchecked MockFtpServerException.
     *
     * @param host
     * @return an InetAddress for the specified host
     * @throws MockFtpServerException - if an UnknownHostException is thrown
     */
    protected static InetAddress inetAddress(String host) {
        try {
            return InetAddress.getByName(host);
        }
        catch (UnknownHostException e) {
            throw new MockFtpServerException(e);
        }
    }

    /**
     * Create and return a List containing the Objects passed as arguments to this method
     *
     * @param e1- the first element to add
     * @param e2- the second element to add
     * @return the List containing the specified elements
     */
    protected static List list(Object e1, Object e2) {
        List list = new ArrayList();
        list.add(e1);
        list.add(e2);
        return list;
    }

    /**
     * Create and return a List containing the single Object passed as an argument to this method
     *
     * @param element- the element to add
     * @return the List containing the specified element
     */
    protected static List list(Object element) {
        return Collections.singletonList(element);
    }

    /**
     * Create and return a Set containing the Objects passed as arguments to this method
     *
     * @param e1 - the first element to add
     * @param e2 - the second element to add
     * @return the Set containing the specified elements
     */
    protected static Set set(Object e1, Object e2) {
        Set set = new HashSet();
        set.add(e1);
        set.add(e2);
        return set;
    }

    /**
     * Create and return a Set containing the Objects passed as arguments to this method
     *
     * @param e1 - the first element to add
     * @param e2 - the second element to add
     * @param e3 - the third element to add
     * @return the Set containing the specified elements
     */
    protected static Set set(Object e1, Object e2, Object e3) {
        Set set = set(e1, e2);
        set.add(e3);
        return set;
    }

    /**
     * Override the default test run behavior to write out the current test name
     * and handle Errors and Exceptions in a standard way.
     *
     * @see junit.framework.TestCase#runBare()
     */
    public void runBare() throws Throwable {

        LoggingUtil loggingUtil = null;
        try {
            loggingUtil = LoggingUtil.getTestCaseLogger(this);
            loggingUtil.logStartOfTest();
            super.runBare();
        }
        catch (Exception e) {
            handleException(e);
        }
        catch (Error e) {
            handleError(e);
        }
        finally {
            if (loggingUtil != null) {
                loggingUtil.logEndOfTest();
            }
        }
    }

    /**
     * Setup before each test.
     */
    protected void setUp() throws Exception {
        super.setUp();
    }

    /**
     * Cleanup after each test.
     */
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    //-----------------------------------------------------------
    // Private Internal Methods
    //-----------------------------------------------------------

    /**
     * Handle an exception
     *
     * @param e the Exception
     * @throws Exception
     */
    private void handleException(Exception e) throws Exception {

        LOG.error("EXCEPTION: ", e);
        throw e;
    }

    /**
     * Handle an Error
     *
     * @param e the Error
     * @throws Exception
     */
    private void handleError(Error e) throws Exception {
        LOG.error("ERROR: ", e);
        throw e;
    }

    //-------------------------------------------------------------------------
    // Helper methods
    //-------------------------------------------------------------------------

    /**
     * Delete the named file if it exists
     *
     * @param filename - the full pathname of the file
     */
    protected void deleteFile(String filename) {
        File keyFile = new File(filename);
        boolean deleted = keyFile.delete();
        LOG.info("Deleted [" + filename + "]: " + deleted);
    }

    //-------------------------------------------------------------------------
    // Common validation helper methods
    //-------------------------------------------------------------------------

    /**
     * Verify that the named file exists
     *
     * @param filename - the full pathname of the file
     */
    protected void verifyFileExists(String filename) {
        File keyFile = new File(filename);
        assertTrue("File does not exist [" + filename + "]", keyFile.exists());
    }

}
