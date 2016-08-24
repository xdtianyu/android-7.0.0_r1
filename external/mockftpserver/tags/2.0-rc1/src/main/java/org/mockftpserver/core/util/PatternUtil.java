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

/**
 * Contains static utility methods related to pattern-matching and regular expressions.
 *
 * @author Chris Mair
 * @version $Revision: 85 $ - $Date: 2008-07-22 21:43:19 -0400 (Tue, 22 Jul 2008) $
 */
public class PatternUtil {

    /**
     * Return true if the specified String contains one or more wildcard characters ('?' or '*')
     *
     * @param string - the String to check
     * @return true if the String contains wildcards
     */
    public static boolean containsWildcards(String string) {
        return string.indexOf("*") != -1 || string.indexOf("?") != -1;
    }

    /**
     * Convert the specified String, optionally containing wildcards (? or *), to a regular expression String
     *
     * @param stringWithWildcards - the String to convert, optionally containing wildcards (? or *)
     * @return an equivalent regex String
     * @throws AssertionError - if the stringWithWildcards is null
     */
    public static String convertStringWithWildcardsToRegex(String stringWithWildcards) {
        Assert.notNull(stringWithWildcards, "stringWithWildcards");

        StringBuffer result = new StringBuffer();
        for (int i = 0; i < stringWithWildcards.length(); i++) {
            char ch = stringWithWildcards.charAt(i);
            switch (ch) {
                case '*':
                    result.append(".*");
                    break;
                case '?':
                    result.append('.');
                    break;
                case '$':
                case '|':
                case '[':
                case ']':
                case '(':
                case ')':
                case '.':
                case ':':
                case '{':
                case '}':
                case '\\':
                case '^':
                    result.append('\\');
                    result.append(ch);
                    break;
                default:
                    result.append(ch);
            }
        }
        return result.toString();
    }

    /**
     * Private constructor to prevent instantiation. All members are static.
     */
    private PatternUtil() {
    }

}
