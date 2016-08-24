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
package org.mockftpserver.core.util;

import java.util.Collection;
import java.util.Iterator;

/**
 * Contains static String-related utility methods.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class StringUtil {

    /**
     * Pad the specified String with spaces to the right to the specified width. If the length
     * of string is already equal to or greater than width, then just return string.
     *
     * @param string - the String to pad
     * @param width  - the target width
     * @return a String of at least width characters, padded on the right with spaces as necessary
     */
    public static String padRight(String string, int width) {
        int numSpaces = width - string.length();
        return (numSpaces > 0) ? string + spaces(numSpaces) : string;
    }

    /**
     * Pad the specified String with spaces to the left to the specified width. If the length
     * of string is already equal to or greater than width, then just return string.
     *
     * @param string - the String to pad
     * @param width  - the target width
     * @return a String of at least width characters, padded on the left with spaces as necessary
     */
    public static String padLeft(String string, int width) {
        int numSpaces = width - string.length();
        return (numSpaces > 0) ? spaces(numSpaces) + string : string;
    }

    /**
     * Join the Strings within the parts Collection, inserting the delimiter in between elements
     *
     * @param parts     - the Collection of Strings to join
     * @param delimiter - the delimiter String to insert between the parts
     * @return the Strings within the parts collection joined together using the specified delimiter
     */
    public static String join(Collection parts, String delimiter) {
        Assert.notNull(parts, "parts");
        Assert.notNull(delimiter, "delimiter");

        StringBuffer buf = new StringBuffer();
        Iterator iter = parts.iterator();
        while (iter.hasNext()) {
            String component = (String) iter.next();
            buf.append(component);
            if (iter.hasNext()) {
                buf.append(delimiter);
            }
        }
        return buf.toString();
    }

    //--------------------------------------------------------------------------
    // Internal Helper Methods
    //--------------------------------------------------------------------------

    private static String spaces(int numSpaces) {
        StringBuffer buf = new StringBuffer();
        for (int i = 0; i < numSpaces; i++) {
            buf.append(" ");
        }
        return buf.toString();
    }

    /**
     * Private constructor to prevent instantiation. All members are static.
     */
    private StringUtil() {
    }

}