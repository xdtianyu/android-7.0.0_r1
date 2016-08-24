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
package org.mockftpserver.core.util;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;


import org.apache.log4j.Logger;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.AssertFailedException;
import org.mockftpserver.test.AbstractTest;

/**
 * Tests for the Assert class
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public class AssertTest extends AbstractTest {

    private static final Logger LOG = Logger.getLogger(AssertTest.class);
    
    /**
     * This interface defines a generic closure (a generic wrapper for a block of code).
     */
    private static interface ExceptionClosure {
        /**
         * Execute arbitrary logic that can throw any type of Exception
         * @throws Exception
         */
        public void execute() throws Exception;
    }

    
	private static final String MESSAGE = "exception message";
    
    /**
     * Test the assertNull() method
     */
    public void testAssertNull() {

        Assert.isNull(null, MESSAGE);

        try {
            Assert.isNull("OK", MESSAGE);
            fail("Expected IllegalArumentException");
        }
        catch (AssertFailedException expected) {
            LOG.info("Expected: " + expected);
            assertExceptionMessageContains(expected, MESSAGE);
        }
    }


	/**
	 * Test the assertNotNull() method
	 */
	public void testAssertNotNull() {

		Assert.notNull("OK", MESSAGE);

		try {
			Assert.notNull(null, MESSAGE);
			fail("Expected IllegalArumentException");
		}
		catch (AssertFailedException expected) {
			LOG.info("Expected: " + expected);
            assertExceptionMessageContains(expected, MESSAGE);
		}
	}

	/**
	 * Test the assertTrue() method
	 */
	public void testAssertTrue() throws Exception {

		Assert.isTrue(true, MESSAGE);

		verifyThrowsAssertFailedException(true, new ExceptionClosure() {
			public void execute() throws Exception {
				Assert.isTrue(false, MESSAGE);
			}
		});
	}

	/**
	 * Test the assertFalse() method
	 */
	public void testAssertFalse() throws Exception {

		Assert.isFalse(false, MESSAGE);

		verifyThrowsAssertFailedException(true, new ExceptionClosure() {
			public void execute() throws Exception {
				Assert.isFalse(true, MESSAGE);
			}
		});
	}

	/**
	 * Test the assertNotEmpty(Collection,String) method
	 */
	public void testAssertNotNullOrEmpty_Collection() throws Exception {

		final Collection COLLECTION = Collections.singletonList("item");
		Assert.notNullOrEmpty(COLLECTION, MESSAGE);

		verifyThrowsAssertFailedException(true, new ExceptionClosure() {
			public void execute() throws Exception {
				Assert.notNullOrEmpty((Collection) null, MESSAGE);
			}
		});

		verifyThrowsAssertFailedException(true, new ExceptionClosure() {
			public void execute() throws Exception {
				Assert.notNullOrEmpty(new ArrayList(), MESSAGE);
			}
		});
	}

	/**
	 * Test the assertNotEmpty(Map,String) method
	 */
	public void testAssertNotNullOrEmpty_Map() throws Exception {

		final Map MAP = Collections.singletonMap("key", "value");
		Assert.notNullOrEmpty(MAP, MESSAGE);

		verifyThrowsAssertFailedException(true, new ExceptionClosure() {
			public void execute() throws Exception {
				Assert.notNullOrEmpty((Map) null, MESSAGE);
			}
		});

		verifyThrowsAssertFailedException(true, new ExceptionClosure() {
			public void execute() throws Exception {
				Assert.notNullOrEmpty(new HashMap(), MESSAGE);
			}
		});
	}

	/**
	 * Test the assertNotEmpty(Objecct[],String) method
	 */
	public void testAssertNotNullOrEmpty_array() throws Exception {

		final Object[] ARRAY = { "1", "2" };
		Assert.notNullOrEmpty(ARRAY, MESSAGE);

		verifyThrowsAssertFailedException(true, new ExceptionClosure() {
			public void execute() throws Exception {
				Assert.notNullOrEmpty((Object[]) null, MESSAGE);
			}
		});

		verifyThrowsAssertFailedException(true, new ExceptionClosure() {
			public void execute() throws Exception {
                Assert.notNullOrEmpty(new String[] { }, MESSAGE);
			}
		});
	}

	/**
	 * Test the assertNotEmpty(String,String) method
	 */
	public void testAssertNotNullOrEmpty_String() throws Exception {

		Assert.notNullOrEmpty("OK", MESSAGE);

		verifyThrowsAssertFailedException(true, new ExceptionClosure() {
			public void execute() throws Exception {
				Assert.notNullOrEmpty((String) null, MESSAGE);
			}
		});

		verifyThrowsAssertFailedException(true, new ExceptionClosure() {
			public void execute() throws Exception {
				Assert.notNullOrEmpty("", MESSAGE);
			}
		});
	}

	//-------------------------------------------------------------------------
	// Helper Methods 
	//-------------------------------------------------------------------------

    private void assertExceptionMessageContains(Throwable exception, String text) {
        String message = exception.getMessage();
        assertTrue("Exception message [" + message + "] does not contain [" + text + "]", message.indexOf(text) != -1);
    }
    
	/**
	 * Verify that execution of the ExceptionClosure (code block) results in an
	 * AssertFailedException being thrown with the constant MESSAGE as its message.
	 * @param closure - the ExceptionClosure encapsulating the code to execute
	 */
	private void verifyThrowsAssertFailedException(boolean checkMessage, ExceptionClosure closure)
		throws Exception {

		try {
			closure.execute();
			fail("Expected IllegalArumentException");
		}
		catch (AssertFailedException expected) {
			LOG.info("Expected: " + expected);
			if (checkMessage) {
				assertExceptionMessageContains(expected, MESSAGE);
			}
		}
	}

}
