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

import java.util.Collection;
import java.util.Map;

/**
 * Provides static helper methods to make runtime assertions. Throws an
 * <code>AssertFailedException</code> when the assertion fails. All methods are static.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class Assert {
    
    /**
     * Verify that arg is null. Throw an AssertFailedException if it is not null.
     * @param arg - the method parameter value
     * @param argName - the name of the parameter; used in the exception message
     * @throws AssertFailedException - if arg is not null
     */
    public static void isNull(Object arg, String argName) {
        if (arg != null) {
            throw new AssertFailedException("The value for \"" + argName + "\" must be null");
        }
    }

	/**
	 * Verify that arg is not null. Throw an AssertFailedException if it is null.
	 * @param arg - the method parameter value
     * @param argName - the name of the parameter; used in the exception message
	 * @throws AssertFailedException - if arg is null
	 */
    public static void notNull(Object arg, String argName) {
		if (arg == null) {
            throw new AssertFailedException("The value of \"" + argName + "\" is null");
		}
	}

	/**
	 * Verify that condition is true. Throw an AssertFailedException if it is false.
	 * @param condition - the condition that should be true
	 * @throws AssertFailedException - if condition is false
	 */
	public static void isTrue(boolean condition, String message) {
		if (!condition) {
			throw new AssertFailedException(message);
		}
	}

	/**
	 * Verify that condition is false. Throw an AssertFailedException if it is true.
	 * @param condition - the condition that should be false
	 * @throws AssertFailedException - if condition is true
	 */
	public static void isFalse(boolean condition, String message) {
		if (condition) {
			throw new AssertFailedException(message);
		}
	}

	/**
	 * Verify that the collection is not null or empty. Throw an 
	 * AssertFailedException if it is null or empty.
	 * @param collection - the Collection
     * @param argName - the name of the parameter; used in the exception message
	 * @throws AssertFailedException - if collection is null or empty
	 */
    public static void notNullOrEmpty(Collection collection, String argName) {
        notNull(collection, argName);
		if (collection.isEmpty()) {
            throw new AssertFailedException("The \"" + argName + "\" Collection is empty");
		}
	}

	/**
	 * Verify that the Map is not null or empty. Throw an AssertFailedException 
	 * if it is null or empty.
	 * @param map - the Map
     * @param argName - the name of the parameter; used in the exception message
	 * @throws AssertFailedException - if map is null or empty
	 */
    public static void notNullOrEmpty(Map map, String argName) {
        notNull(map, argName);
		if (map.isEmpty()) {
            throw new AssertFailedException("The \"" + argName + "\" Map is empty");
		}
	}

	/**
	 * Verify that the array is not null or empty. Throw an 
	 * AssertFailedException if it is null or empty.
	 * @param array - the array
     * @param argName - the name of the parameter; used in the exception message
	 * @throws AssertFailedException - if array is null or empty
	 */
    public static void notNullOrEmpty(Object[] array, String argName) {
        notNull(array, argName);
		if (array.length == 0) {
            throw new AssertFailedException("The \"" + argName + "\" array is empty");
		}
	}

	/**
	 * Verify that the String is not null or empty. Throw an 
	 * AssertFailedException if it is null or empty.
	 * @param string - the String
     * @param argName - the name of the parameter; used in the exception message
	 * @throws AssertFailedException - if string is null or empty
	 */
    public static void notNullOrEmpty(String string, String argName) {
        notNull(string, argName);
		if (string.trim().length() == 0) {
            throw new AssertFailedException("The \"" + argName + "\" String is empty");
		}
	}

	/**
	 * Private constructor. All methods are static
	 */
	private Assert() {
	}
}
