/*
 * Copyright 2008 the original author or authors.
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
package org.mockftpserver.fake.filesystem;

import org.mockftpserver.core.util.Assert;

/**
 * Represents and encapsulates the read/write/execute permissions for a file or directory.
 * This is conceptually (and somewhat loosely) based on the permissions flags within the Unix
 * file system. An instance of this class is immutable.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class Permissions {
    public static final Permissions ALL = new Permissions("rwxrwxrwx");
    public static final Permissions NONE = new Permissions("---------");
    public static final Permissions DEFAULT = ALL;

    private static final char READ_CHAR = 'r';
    private static final char WRITE_CHAR = 'w';
    private static final char EXECUTE_CHAR = 'x';

    private String rwxString;

    /**
     * Costruct a new instance for the specified read/write/execute specification String
     *
     * @param rwxString - the read/write/execute specification String; must be 9 characters long, with chars
     *                  at index 0,3,6 == '-' or 'r', chars at index 1,4,7 == '-' or 'w' and chars at index 2,5,8 == '-' or 'x'.
     */
    public Permissions(String rwxString) {
        Assert.isTrue(rwxString.length() == 9, "The permissions string must be exactly 9 characters");
        final String RWX = "(-|r)(-|w)(-|x)";
        final String PATTERN = RWX + RWX + RWX;
        Assert.isTrue(rwxString.matches(PATTERN), "The permissions string must match [" + PATTERN + "]");
        this.rwxString = rwxString;
    }

    /**
     * Return the read/write/execute specification String representing the set of permissions. For example:
     * "rwxrwxrwx" or "rw-r-----".
     *
     * @return the String containing 9 characters that represent the read/write/execute permissions.
     */
    public String asRwxString() {
        return rwxString;
    }

    /**
     * @return the RWX string for this instance
     */
    public String getRwxString() {
        return rwxString;
    }

    /**
     * @see java.lang.Object#equals(java.lang.Object)
     */
    public boolean equals(Object object) {
        return (object != null)
                && (object.getClass() == this.getClass())
                && (object.hashCode() == hashCode());
    }

    /**
     * Return the hash code for this object.
     *
     * @see java.lang.Object#hashCode()
     */
    public int hashCode() {
        return rwxString.hashCode();
    }

    /**
     * @return true if and only if the user has read permission
     */
    public boolean canUserRead() {
        return rwxString.charAt(0) == READ_CHAR;
    }

    /**
     * @return true if and only if the user has write permission
     */
    public boolean canUserWrite() {
        return rwxString.charAt(1) == WRITE_CHAR;
    }

    /**
     * @return true if and only if the user has execute permission
     */
    public boolean canUserExecute() {
        return rwxString.charAt(2) == EXECUTE_CHAR;
    }

    /**
     * @return true if and only if the group has read permission
     */
    public boolean canGroupRead() {
        return rwxString.charAt(3) == READ_CHAR;
    }

    /**
     * @return true if and only if the group has write permission
     */
    public boolean canGroupWrite() {
        return rwxString.charAt(4) == WRITE_CHAR;
    }

    /**
     * @return true if and only if the group has execute permission
     */
    public boolean canGroupExecute() {
        return rwxString.charAt(5) == EXECUTE_CHAR;
    }

    /**
     * @return true if and only if the world has read permission
     */
    public boolean canWorldRead() {
        return rwxString.charAt(6) == READ_CHAR;
    }

    /**
     * @return true if and only if the world has write permission
     */
    public boolean canWorldWrite() {
        return rwxString.charAt(7) == WRITE_CHAR;
    }

    /**
     * @return true if and only if the world has execute permission
     */
    public boolean canWorldExecute() {
        return rwxString.charAt(8) == EXECUTE_CHAR;
    }

    /**
     * @return the String representation of this object.
     */
    public String toString() {
        return "Permissions[" + rwxString + "]";
    }
}