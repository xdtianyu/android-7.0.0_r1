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
 * @version $Revision: 51 $ - $Date: 2008-05-09 22:16:32 -0400 (Fri, 09 May 2008) $
 */
public class StringUtil {

    public static String padRight(String string, int width) {
        int numSpaces = width - string.length();
        return (numSpaces > 0) ? string + spaces(numSpaces) : string;
    }

    public static String padLeft(String string, int width) {
        int numSpaces = width - string.length();
        return (numSpaces > 0) ? spaces(numSpaces) + string : string;
    }

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