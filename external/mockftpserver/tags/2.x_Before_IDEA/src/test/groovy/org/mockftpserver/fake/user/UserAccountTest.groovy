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
package org.mockftpserver.fake.user

import org.mockftpserver.test.*

/**
 * Tests for UserAccount
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
class UserAccountTest extends AbstractGroovyTest {

    private static final USERNAME = "user123"
    private static final PASSWORD = "password123"
    private static final HOME_DIR = "/usr/user123"

    private UserAccount userAccount
    
    void testIsValidPassword() {
        userAccount.username = USERNAME
        userAccount.password = PASSWORD
        assert userAccount.isValidPassword(PASSWORD)
        
        assert userAccount.isValidPassword("") == false
        assert userAccount.isValidPassword("wrong") == false
        assert userAccount.isValidPassword(null) == false
    }
    
    void testIsValidPassword_UsernameNullOrEmpty() {
        userAccount.password = PASSWORD
        shouldFail(AssertionError) { userAccount.isValidPassword(PASSWORD) }

        userAccount.username = ''
        shouldFail(AssertionError) { userAccount.isValidPassword(PASSWORD) }
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
    
    void setUp() {
        userAccount = new UserAccount()
    }
}