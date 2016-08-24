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
package org.mockftpserver.fake.filesystem

import org.mockftpserver.test.AbstractGroovyTest

/**
 * Tests for the Permissions class
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class PermissionsTest extends AbstractGroovyTest {

    void testConstructor() {
        testConstructorWithValidString('rwxrwxrwx')
        testConstructorWithValidString('rwxr--r--')
        testConstructorWithValidString('---------')
    }

    void testConstructor_InvalidString() {
        testConstructorWithInvalidString('')
        testConstructorWithInvalidString('------')
        testConstructorWithInvalidString('-')
        testConstructorWithInvalidString('r')
        testConstructorWithInvalidString('rwx')
        testConstructorWithInvalidString('rwxrwxrw')
        testConstructorWithInvalidString('123456789')
        testConstructorWithInvalidString('rwxrZxrwx')
        testConstructorWithInvalidString('--------Z')
    }

    void testCanReadWriteExecute() {
        testCanReadWriteExecute('rwxrwxrwx', true, true, true, true, true, true, true, true, true)
        testCanReadWriteExecute('r--r--r--', true, false, false, true, false, false, true, false, false)
        testCanReadWriteExecute('-w-r----x', false, true, false, true, false, false, false, false, true)
        testCanReadWriteExecute('---------', false, false, false, false, false, false, false, false, false)
    }

    void testHashCode() {
        assert new Permissions('rwxrwxrwx').hashCode() == Permissions.DEFAULT.hashCode()
        assert new Permissions('---------').hashCode() == Permissions.NONE.hashCode()
    }

    void testEquals() {
        assert new Permissions('rwxrwxrwx').equals(Permissions.DEFAULT)
        assert new Permissions('---------').equals(Permissions.NONE)
        assert Permissions.NONE.equals(Permissions.NONE)

        assert !(new Permissions('------rwx').equals(Permissions.NONE))
        assert !Permissions.NONE.equals(null)
        assert !Permissions.NONE.equals(123)
    }

    //--------------------------------------------------------------------------
    // Helper Methods
    //--------------------------------------------------------------------------

    private testCanReadWriteExecute(rwxString,
                                    canUserRead, canUserWrite, canUserExecute,
                                    canGroupRead, canGroupWrite, canGroupExecute,
                                    canWorldRead, canWorldWrite, canWorldExecute) {

        def permissions = new Permissions(rwxString)
        LOG.info("Testing can read/write/execute for $permissions")
        assert permissions.canUserRead() == canUserRead
        assert permissions.canUserWrite() == canUserWrite
        assert permissions.canUserExecute() == canUserExecute
        assert permissions.canGroupRead() == canGroupRead
        assert permissions.canGroupWrite() == canGroupWrite
        assert permissions.canGroupExecute() == canGroupExecute
        assert permissions.canWorldRead() == canWorldRead
        assert permissions.canWorldWrite() == canWorldWrite
        assert permissions.canWorldExecute() == canWorldExecute
    }

    private testConstructorWithInvalidString(String string) {
        LOG.info("Verifying invalid: [$string]")
        shouldFail { new Permissions(string) }
    }

    private testConstructorWithValidString(String string) {
        LOG.info("Verifying valid: [$string]")
        def permissions = new Permissions(string)
        LOG.info(permissions)
        assert permissions.asRwxString() == string
    }
}