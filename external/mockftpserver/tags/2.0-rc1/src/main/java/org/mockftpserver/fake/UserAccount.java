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
package org.mockftpserver.fake;

import org.mockftpserver.core.util.Assert;
import org.mockftpserver.fake.filesystem.FileSystemEntry;
import org.mockftpserver.fake.filesystem.Permissions;

import java.util.List;

/**
 * Represents a single user account on the server, including the username, password and home
 * directory. The <code>username</code> and <code>homeDirectory</code> property must be non-null
 * and non-empty. The <code>homeDirectory</code> property must also match the name of an existing
 * directory within the file system configured for the <code>FakeFtpServer</code>/
 * <p/>
 * This class also includes several configuration flags, described below.
 * <p/>
 * The <code>isValidPassword()</code> method returns true if the specified password matches
 * the password value configured for this user account. This implementation uses the
 * <code>isEquals()</code> method to compare passwords.
 * <p/>
 * If you want to provide a custom comparison, for instance using encrypted passwords, you can
 * subclass this class and override the <code>comparePassword()</code> method to provide your own
 * custom implementation.
 * <p/>
 * If the <code>passwordCheckedDuringValidation</code> property is set to false, then the password
 * value is ignored, and the <code>isValidPassword()</code> method just returns <code<true</code>.
 * <p/>
 * The <code>accountRequiredForLogin</code> property defaults to false. If it is set to true, then
 * it is expected that the login for this account will require an ACCOUNT (ACCT) command after the
 * PASSWORD (PASS) command is completed.
 */
public class UserAccount {

    public static final String DEFAULT_USER = "system";
    public static final String DEFAULT_GROUP = "users";
    public static final Permissions DEFAULT_PERMISSIONS_FOR_NEW_FILE = new Permissions("rw-rw-rw-");
    public static final Permissions DEFAULT_PERMISSIONS_FOR_NEW_DIRECTORY = Permissions.ALL;

    private String username;
    private String password;
    private String homeDirectory;
    private List groups;
    private boolean passwordRequiredForLogin = true;
    private boolean passwordCheckedDuringValidation = true;
    private boolean accountRequiredForLogin = false;
    private Permissions defaultPermissionsForNewFile = DEFAULT_PERMISSIONS_FOR_NEW_FILE;

    /**
     * Construct a new uninitialized instance.
     */
    public UserAccount() {
    }

    /**
     * Construct a new initialized instance.
     *
     * @param username      - the user name
     * @param password      - the password
     * @param homeDirectory - the home directory
     */
    public UserAccount(String username, String password, String homeDirectory) {
        setUsername(username);
        setPassword(password);
        setHomeDirectory(homeDirectory);
    }

    public String getUsername() {
        return username;
    }

    public void setUsername(String username) {
        this.username = username;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public String getHomeDirectory() {
        return homeDirectory;
    }

    public void setHomeDirectory(String homeDirectory) {
        this.homeDirectory = homeDirectory;
    }

    public List getGroups() {
        return groups;
    }

    public void setGroups(List groups) {
        this.groups = groups;
    }

    public boolean isPasswordRequiredForLogin() {
        return passwordRequiredForLogin;
    }

    public void setPasswordRequiredForLogin(boolean passwordRequiredForLogin) {
        this.passwordRequiredForLogin = passwordRequiredForLogin;
    }

    public boolean isPasswordCheckedDuringValidation() {
        return passwordCheckedDuringValidation;
    }

    public void setPasswordCheckedDuringValidation(boolean passwordCheckedDuringValidation) {
        this.passwordCheckedDuringValidation = passwordCheckedDuringValidation;
    }

    public boolean isAccountRequiredForLogin() {
        return accountRequiredForLogin;
    }

    public void setAccountRequiredForLogin(boolean accountRequiredForLogin) {
        this.accountRequiredForLogin = accountRequiredForLogin;
    }

    public Permissions getDefaultPermissionsForNewFile() {
        return defaultPermissionsForNewFile;
    }

    public void setDefaultPermissionsForNewFile(Permissions defaultPermissionsForNewFile) {
        this.defaultPermissionsForNewFile = defaultPermissionsForNewFile;
    }

    public Permissions getDefaultPermissionsForNewDirectory() {
        return defaultPermissionsForNewDirectory;
    }

    public void setDefaultPermissionsForNewDirectory(Permissions defaultPermissionsForNewDirectory) {
        this.defaultPermissionsForNewDirectory = defaultPermissionsForNewDirectory;
    }

    private Permissions defaultPermissionsForNewDirectory = DEFAULT_PERMISSIONS_FOR_NEW_DIRECTORY;

    /**
     * Return the name of the primary group to which this user belongs. If this account has no associated
     * groups set, then this method returns the <code>DEFAULT_GROUP</code>. Otherwise, this method
     * returns the first group name in the <code>groups</code> list.
     *
     * @return the name of the primary group for this user
     */
    public String getPrimaryGroup() {
        return (groups == null || groups.isEmpty()) ? DEFAULT_GROUP : (String) groups.get(0);
    }

    /**
     * Return true if the specified password is the correct, valid password for this user account.
     * This implementation uses standard (case-sensitive) String comparison. Subclasses can provide
     * custom comparison behavior, for instance using encrypted password values, by overriding this
     * method.
     *
     * @param password - the password to compare against the configured value
     * @return true if the password is correct and valid
     * @throws org.mockftpserver.core.util.AssertFailedException
     *          - if the username property is null
     */
    public boolean isValidPassword(String password) {
        Assert.notNullOrEmpty(username, "username");
        return passwordCheckedDuringValidation ? comparePassword(password) : true;
    }

    /**
     * @return true if this UserAccount object is valid; i.e. if the homeDirectory is non-null and non-empty.
     */
    public boolean isValid() {
        return homeDirectory != null && homeDirectory.length() > 0;
    }

    /**
     * @return the String representation of this object
     */
    public String toString() {
        return "UserAccount[username=" + username + "; password=" + password + "; homeDirectory="
                + homeDirectory + "; passwordRequiredForLogin=" + passwordRequiredForLogin + "]";
    }

    /**
     * Return true if this user has read access to the file/directory represented by the specified FileSystemEntry object.
     *
     * @param entry - the FileSystemEntry representing the file or directory
     * @return true if this use has read access
     */
    public boolean canRead(FileSystemEntry entry) {
        Permissions permissions = entry.getPermissions();
        if (permissions == null) {
            return true;
        }

        if (username == entry.getOwner()) {
            return permissions.canUserRead();
        }
        if (groups != null && groups.contains(entry.getGroup())) {
            return permissions.canGroupRead();
        }
        return permissions.canWorldRead();
    }

    /**
     * Return true if this user has write access to the file/directory represented by the specified FileSystemEntry object.
     *
     * @param entry - the FileSystemEntry representing the file or directory
     * @return true if this use has write access
     */
    public boolean canWrite(FileSystemEntry entry) {
        Permissions permissions = entry.getPermissions();
        if (permissions == null) {
            return true;
        }

        if (username == entry.getOwner()) {
            return permissions.canUserWrite();
        }
        if (groups != null && groups.contains(entry.getGroup())) {
            return permissions.canGroupWrite();
        }
        return permissions.canWorldWrite();
    }

    /**
     * Return true if this user has execute access to the file/directory represented by the specified FileSystemEntry object.
     *
     * @param entry - the FileSystemEntry representing the file or directory
     * @return true if this use has execute access
     */
    public boolean canExecute(FileSystemEntry entry) {
        Permissions permissions = entry.getPermissions();
        if (permissions == null) {
            return true;
        }

        if (username == entry.getOwner()) {
            return permissions.canUserExecute();
        }
        if (groups != null && groups.contains(entry.getGroup())) {
            return permissions.canGroupExecute();
        }
        return permissions.canWorldExecute();
    }

    /**
     * Return true if the specified password matches the password configured for this user account.
     * This implementation uses standard (case-sensitive) String comparison. Subclasses can provide
     * custom comparison behavior, for instance using encrypted password values, by overriding this
     * method.
     *
     * @param password - the password to compare against the configured value
     * @return true if the passwords match
     */
    protected boolean comparePassword(String password) {
        return password != null && password.equals(this.password);
    }

}