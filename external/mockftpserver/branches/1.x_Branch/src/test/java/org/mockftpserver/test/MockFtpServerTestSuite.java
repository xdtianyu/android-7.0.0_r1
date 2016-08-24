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

import junit.framework.Test;

/**
 * Unit test suite for the StubFtpServer application
 *  
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public class MockFtpServerTestSuite extends AbstractDirectoryTestSuite {

    private static final String CLASSES_DIR = "target/test-classes";

    /**
     * @see org.mockftpserver.test.AbstractDirectoryTestSuite#getTestClassesDirectory()
     */
    protected String getTestClassesDirectory() {
        return CLASSES_DIR;
    }

    
    /**
     * Return the TestSuite containing the tests to be run
     * @return the test suite as a Test
     */
    public static Test suite() {
        return new MockFtpServerTestSuite();
    }
    
    /**
     * Run the test suite
     */
    public static void main(String[] args) throws Exception {
        junit.textui.TestRunner.run(suite());
    }

}