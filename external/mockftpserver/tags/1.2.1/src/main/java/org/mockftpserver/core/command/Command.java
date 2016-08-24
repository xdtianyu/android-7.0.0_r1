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
package org.mockftpserver.core.command;

import java.util.Arrays;

import org.mockftpserver.core.util.Assert;

/**
 * Represents a command received from an FTP client, containing a command name and parameters.
 * Objects of this class are immutable.
 * 
 * @version $Revision$ - $Date$
 * 
 * @author Chris Mair
 */
public final class Command {

    private String name;
    private String[] parameters;
    
    /**
     * Construct a new immutable instance with the specified command name and parameters
     * @param name - the command name; may not be null
     * @param parameters - the command parameters; may be empty; may not benull
     */
    public Command(String name, String[] parameters) {
        Assert.notNull(name, "name");
        Assert.notNull(parameters, "parameters");
        this.name = name;
        this.parameters = copy(parameters);
    }

    /**
     * @return the name
     */
    public String getName() {
        return name;
    }
    
    /**
     * @return the parameters
     */
    public String[] getParameters() {
        return copy(parameters);
    }

    /**
     * Get the String value of the parameter at the specified index
     * @param index - the index
     * @return the parameter value as a String
     * @throws AssertFailedException if the parameter index is invalid or the value is not a valid String
     */
    public String getRequiredString(int index) {
        assertValidIndex(index);
        return parameters[index];
    }

    /**
     * Get the String value of the parameter at the specified index; return null if no parameter exists for the index
     * @param index - the index
     * @return the parameter value as a String, or null if this Command does not have a parameter for that index
     */
    public String getOptionalString(int index) {
        return (parameters.length > index) ? parameters[index] : null;
    }

    /**
     * @see java.lang.Object#equals(java.lang.Object)
     */
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null || !(obj instanceof Command)) {
            return false;
        }
        return this.hashCode() == obj.hashCode();
    }

    /**
     * @see java.lang.Object#hashCode()
     */
    public int hashCode() {
        String str = name + Arrays.asList(parameters);
        return str.hashCode();
    }
    
    /**
     * Return the String representation of this object
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return "Command[" + name + ": " + Arrays.asList(parameters) + "]";
    }

    /**
     * Return the name, normalized to a common format - convert to upper case.
     * @return the name converted to upper case
     */
    public static String normalizeName(String name) {
        return name.toUpperCase();
    }
    
    /**
     * Construct a shallow copy of the specified array
     * @param array - the array to copy
     * @return a new array with the same contents
     */
    private static String[] copy(String[] array) {
        String[] newArray = new String[array.length];
        System.arraycopy(array, 0, newArray, 0, array.length);
        return newArray;
    }
    
    /**
     * Assert that the index is valid
     * @param index - the index
     * @throws AssertFailedException if the parameter index is invalid
     */
    private void assertValidIndex(int index) {
        Assert.isTrue(index >= 0 && index < parameters.length, "The parameter index " + index + " is not valid for " + this);
    }
    
}
