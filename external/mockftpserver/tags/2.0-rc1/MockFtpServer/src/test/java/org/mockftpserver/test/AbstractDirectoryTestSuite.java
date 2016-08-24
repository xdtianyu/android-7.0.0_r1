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
import junitx.util.DirectorySuiteBuilder;
import junitx.util.SimpleTestFilter;

/**
 * Abstract superclass for test suites that recursively process a directory of 
 * Java class files and run all matching unit test classes. Subclasses must 
 * implement the {@link #getTestClassesDirectory()} method to specify the root 
 * directory for the Java class files. Only class names that end in 'Test' are 
 * included. Furthermore, only concrete classes (non-abstract and non-interface) 
 * classes are included. Subclasses can perform additional filtering by overriding 
 * the {@link #isClassIncluded(Class)} method to filter based on the class.
 * <p>
 * A typical pattern is to define application-specific marker interfaces, such as
 * 'IntegrationTest' or 'LongRunningTest', and have test classes implement the
 * appropriate interface(s). A subclass that runs unit tests, for instance, can
 * then exclude such classes, while another subclass for integration tests can 
 * include only those classes that implement 'IntegrationTest'.  
 * <p>
 * Note: This class requires the JUnit-addons jar.
 * 
 * @see http://junit-addons.sourceforge.net/
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public abstract class AbstractDirectoryTestSuite extends AbstractTestSuite {

    private static final String TEST_CLASSES_DIR_PROPERTY = "test.classes.dir";
    
    /**
     * Constructor
     */
    protected AbstractDirectoryTestSuite() {

        // Handle (wrap) any Exception to allow subclasses to use default constructor
        try {
            Test suite = buildAllTestsSuite();
            addTest(suite);
        }
        catch(Exception e) {
            throw new RuntimeException(e);
        }
    }

    //-------------------------------------------------------------------------
    // Abstract and Overridable Methods 
    //-------------------------------------------------------------------------

    /**
     * Return the path of the root class file directory
     */
    protected abstract String getTestClassesDirectory();

    /**
     * Return true if the specified class should be included in the test suite.
     * This method will only be invoked for concrete classes whose names end in 'Test'.
     * @param theClass - the class
     * @return true if the specified class should be included in the test suite
     */    
    protected boolean isClassIncluded(Class theClass) {
        // Default to including all classes
        return true;
    }

    /**
     * Return true if all ignored tests should be listed to System.out. Subclasses
     * can override this and return true to list out all ignored test classes. 
     * @return true if all ignored tests should be listed to System.out
     */
    protected boolean listIgnoredTests() {
        return false;
    }

    //-------------------------------------------------------------------------
    // Private Helper Methods 
    //-------------------------------------------------------------------------

    /**
     * Build the TestSuite containing the tests to be run
     * @return the test suite as a Test
     */
    private Test buildAllTestsSuite() throws Exception {

        DirectorySuiteBuilder builder = new DirectorySuiteBuilder();
        builder.setFilter(new SimpleTestFilter() {

            public boolean include(Class theClass) {
                boolean superInclude = super.include(theClass);
                boolean include = isClassIncluded(theClass);
                if (superInclude && !include && listIgnoredTests()) {
                    System.out.println("IGNORED [" + theClass.getName() + "]");
                }
                return superInclude && include;
            }
        });

        return builder.suite(getDirectory());
    }


    /**
     * Return the path of the root class file directory. Use the "test.classes.dir"
     * system property value if it has been set, otherwise use the value returned 
     * from the getDefaultDirectory() method.
     * 
     * @return the path of the root class file directory
     */
    private String getDirectory() {
        
        String sysProp = System.getProperty(TEST_CLASSES_DIR_PROPERTY);
        return (sysProp == null) ? getTestClassesDirectory() : sysProp;
    }

}
