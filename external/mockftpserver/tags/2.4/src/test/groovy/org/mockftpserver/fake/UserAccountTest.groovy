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
package org.mockftpserver.fake

import org.mockftpserver.fake.filesystem.FileEntry
import org.mockftpserver.fake.filesystem.FileSystemEntry
import org.mockftpserver.fake.filesystem.Permissions
import org.mockftpserver.test.AbstractGroovyTestCase

/**
 * Tests for UserAccount
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class UserAccountTest extends AbstractGroovyTestCase {

    private static final USERNAME = "user123"
    private static final PASSWORD = "password123"
    private static final HOME_DIR = "/usr/user123"
    private static final GROUP = 'group'

    private UserAccount userAccount

    void testConstructor() {
        def acct = new UserAccount(USERNAME, PASSWORD, HOME_DIR)
        assert acct.username == USERNAME
        assert acct.password == PASSWORD
        assert acct.homeDirectory == HOME_DIR
    }

    void testGetPrimaryGroup() {
        assert userAccount.primaryGroup == UserAccount.DEFAULT_GROUP

        userAccount.groups = ['abc']
        assert userAccount.primaryGroup == 'abc'

        userAccount.groups.add('def')
        assert userAccount.primaryGroup == 'abc'

        userAccount.groups = []
        assert userAccount.primaryGroup == UserAccount.DEFAULT_GROUP
    }

    void testIsValidPassword() {
        userAccount.username = USERNAME
        userAccount.password = PASSWORD
        assert userAccount.isValidPassword(PASSWORD)

        assert !userAccount.isValidPassword("")
        assert !userAccount.isValidPassword("wrong")
        assert !userAccount.isValidPassword(null)
    }

    void testIsValidPassword_UsernameNullOrEmpty() {
        userAccount.password = PASSWORD
        shouldFailWithMessageContaining('username') { userAccount.isValidPassword(PASSWORD) }

        userAccount.username = ''
        shouldFailWithMessageContaining('username') { userAccount.isValidPassword(PASSWORD) }
    }

    void testIsValidPassword_OverrideComparePassword() {
        def customUserAccount = new CustomUserAccount()
        customUserAccount.username = USERNAME
        customUserAccount.password = PASSWORD
        println customUserAccount
        assert customUserAccount.isValidPassword(PASSWORD) == false
        assert customUserAccount.isValidPassword(PASSWORD + "123")
    }

    void testIsValidPassword_PasswordNotCheckedDuringValidation() {
        userAccount.username = USERNAME
        userAccount.password = PASSWORD
        userAccount.passwordCheckedDuringValidation = false
        assert userAccount.isValidPassword("wrong")
    }

    void testIsValid() {
        assert !userAccount.valid
        userAccount.homeDirectory = ""
        assert !userAccount.valid
        userAccount.homeDirectory = "/abc"
        assert userAccount.valid
    }

    void testCanRead() {
        // No file permissions - readable by all
        testCanRead(USERNAME, GROUP, null, true)

        // UserAccount has no username or group; use World permissions
        testCanRead(USERNAME, GROUP, '------r--', true)
        testCanRead(USERNAME, GROUP, 'rwxrwx-wx', false)

        userAccount.username = USERNAME
        userAccount.groups = [GROUP]

        testCanRead(USERNAME, GROUP, 'rwxrwxrwx', true)     // ALL
        testCanRead(USERNAME, GROUP, '---------', false)    // NONE

        testCanRead(USERNAME, null, 'r--------', true)      // User
        testCanRead(USERNAME, null, '-wxrwxrwx', false)

        testCanRead(null, GROUP, '---r-----', true)         // Group
        testCanRead(null, GROUP, 'rwx-wxrwx', false)

        testCanRead(null, null, '------r--', true)          // World
        testCanRead(null, null, 'rwxrwx-wx', false)
    }

    void testCanWrite() {
        // No file permissions - writable by all
        testCanWrite(USERNAME, GROUP, null, true)

        // UserAccount has no username or group; use World permissions
        testCanWrite(USERNAME, GROUP, '-------w-', true)
        testCanWrite(USERNAME, GROUP, 'rwxrwxr-x', false)

        userAccount.username = USERNAME
        userAccount.groups = [GROUP]

        testCanWrite(USERNAME, GROUP, 'rwxrwxrwx', true)     // ALL
        testCanWrite(USERNAME, GROUP, '---------', false)    // NONE

        testCanWrite(USERNAME, null, '-w-------', true)      // User
        testCanWrite(USERNAME, null, 'r-xrwxrwx', false)

        testCanWrite(null, GROUP, '----w----', true)         // Group
        testCanWrite(null, GROUP, 'rwxr-xrwx', false)

        testCanWrite(null, null, '-------w-', true)          // World
        testCanWrite(null, null, 'rwxrwxr-x', false)
    }

    void testCanExecute() {
        // No file permissions - executable by all
        testCanExecute(USERNAME, GROUP, null, true)

        // UserAccount has no username or group; use World permissions
        testCanExecute(USERNAME, GROUP, '--------x', true)
        testCanExecute(USERNAME, GROUP, 'rwxrwxrw-', false)

        userAccount.username = USERNAME
        userAccount.groups = [GROUP]

        testCanExecute(USERNAME, GROUP, 'rwxrwxrwx', true)     // ALL
        testCanExecute(USERNAME, GROUP, '---------', false)    // NONE

        testCanExecute(USERNAME, null, '--x------', true)      // User
        testCanExecute(USERNAME, null, 'rw-rwxrwx', false)

        testCanExecute(null, GROUP, '-----x---', true)         // Group
        testCanExecute(null, GROUP, 'rwxrw-rwx', false)

        testCanExecute(null, null, '--------x', true)          // World
        testCanExecute(null, null, 'rwxrwxrw-', false)
    }

    void testDefaultPermissions() {
        assert userAccount.defaultPermissionsForNewFile == new Permissions('rw-rw-rw-')
        assert userAccount.defaultPermissionsForNewDirectory == Permissions.ALL
    }

    //--------------------------------------------------------------------------
    // Helper Methods
    //--------------------------------------------------------------------------

    private void testCanRead(owner, group, permissionsString, expectedResult) {
        def file = createFileEntry(owner, permissionsString, group)
        assert userAccount.canRead(file) == expectedResult, file
    }

    private void testCanWrite(owner, group, permissionsString, expectedResult) {
        def file = createFileEntry(owner, permissionsString, group)
        assert userAccount.canWrite(file) == expectedResult, file
    }

    private void testCanExecute(owner, group, permissionsString, expectedResult) {
        def file = createFileEntry(owner, permissionsString, group)
        assert userAccount.canExecute(file) == expectedResult, file
    }

    private FileSystemEntry createFileEntry(owner, permissionsString, group) {
        def permissions = permissionsString ? new Permissions(permissionsString) : null
        return new FileEntry(path: '', owner: owner, group: group, permissions: permissions)
    }

    void setUp() {
        super.setUp()
        userAccount = new UserAccount()
    }
}