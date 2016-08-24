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
package org.mockftpserver.core.util

import org.mockftpserver.test.AbstractGroovyTest

/**
 * Tests for the IoUtil class
 *
 * @version $Revision: 85 $ - $Date: 2008-07-22 21:43:19 -0400 (Tue, 22 Jul 2008) $
 *
 * @author Chris Mair
 */
class StringUtilTest extends AbstractGroovyTest {

    void testPadRight() {
        assert StringUtil.padRight('', 0) == ''
        assert StringUtil.padRight('', 1) == ' '
        assert StringUtil.padRight('z', 1) == 'z'
        assert StringUtil.padRight(' z', 3) == ' z '
        assert StringUtil.padRight('z', 1) == 'z'
        assert StringUtil.padRight('zzz', 1) == 'zzz'
        assert StringUtil.padRight('z', 5) == 'z    '
    }

    void testPadLeft() {
        assert StringUtil.padLeft('', 0) == ''
        assert StringUtil.padLeft('', 1) == ' '
        assert StringUtil.padLeft('z', 1) == 'z'
        assert StringUtil.padLeft(' z', 3) == '  z'
        assert StringUtil.padLeft('z', 1) == 'z'
        assert StringUtil.padLeft('zzz', 1) == 'zzz'
        assert StringUtil.padLeft('z', 5) == '    z'
    }

    void testJoin() {
        assert StringUtil.join([], ' ') == ''
        assert StringUtil.join([], 'x') == ''
        assert StringUtil.join(['a'], 'x') == 'a'
        assert StringUtil.join(['a', 'b'], '') == 'ab'
        assert StringUtil.join(['a', 'b'], ',') == 'a,b'
        assert StringUtil.join(['a', 'b', 'c'], ':') == 'a:b:c'

        shouldFailWithMessageContaining('parts') { StringUtil.join(null, '') }
        shouldFailWithMessageContaining('delimiter') { StringUtil.join([], null) }
    }

}