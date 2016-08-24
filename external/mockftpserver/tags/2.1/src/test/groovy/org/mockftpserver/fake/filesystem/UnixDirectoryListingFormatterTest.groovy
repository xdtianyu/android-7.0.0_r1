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

import java.text.SimpleDateFormat
import org.mockftpserver.test.AbstractGroovyTest

/**
 * Tests for UnixDirectoryListingFormatter
 *
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
class UnixDirectoryListingFormatterTest extends AbstractGroovyTest {

    static final FILE_NAME = "def.txt"
    static final FILE_PATH = "/dir/$FILE_NAME"
    static final DIR_NAME = "etc"
    static final DIR_PATH = "/dir/$DIR_NAME"
    static final OWNER = 'owner123'
    static final GROUP = 'group456'
    static final SIZE = 11L
    static final LAST_MODIFIED = new Date()
    static final FILE_PERMISSIONS = new Permissions('rw-r--r--')
    static final DIR_PERMISSIONS = new Permissions('rwxr-xr-x')

    private formatter
    private lastModifiedFormatted
    private defaultLocale

    // "-rw-rw-r--    1 ftp      ftp           254 Feb 23  2007 robots.txt"
    // "-rw-r--r--    1 ftp      ftp      30014925 Apr 15 00:19 md5.sums.gz"
    // "-rwxr-xr-x   1 c096336  iawebgrp    5778 Dec  1  2005 FU_WyCONN_updateplanaccess.sql"
    // "drwxr-xr-x   2 c096336  iawebgrp    8192 Nov  7  2006 tmp"
    // "drwxr-xr-x   39 ftp      ftp          4096 Mar 19  2004 a"

    void testFormat_File() {
        def fileSystemEntry = new FileEntry(path: FILE_PATH, contents: '12345678901', lastModified: LAST_MODIFIED,
                owner: OWNER, group: GROUP, permissions: FILE_PERMISSIONS)
        LOG.info(fileSystemEntry)
        verifyFormat(fileSystemEntry, "-rw-r--r--  1 owner123 group456              11 $lastModifiedFormatted def.txt")
    }

    void testFormat_File_Defaults() {
        def fileSystemEntry = new FileEntry(path: FILE_PATH, contents: '12345678901', lastModified: LAST_MODIFIED)
        LOG.info(fileSystemEntry)
        verifyFormat(fileSystemEntry, "-rwxrwxrwx  1 none     none                  11 $lastModifiedFormatted def.txt")
    }

    void testFormat_File_NonEnglishDefaultLocale() {
        Locale.setDefault(Locale.GERMAN)
        def fileSystemEntry = new FileEntry(path: FILE_PATH, contents: '12345678901', lastModified: LAST_MODIFIED)
        LOG.info(fileSystemEntry)
        verifyFormat(fileSystemEntry, "-rwxrwxrwx  1 none     none                  11 $lastModifiedFormatted def.txt")
    }

    void testFormat_File_NonEnglishLocale() {
        formatter.setLocale(Locale.FRENCH)
        def fileSystemEntry = new FileEntry(path: FILE_PATH, contents: '12345678901', lastModified: LAST_MODIFIED)
        LOG.info(fileSystemEntry)
        def dateFormat = new SimpleDateFormat(UnixDirectoryListingFormatter.DATE_FORMAT, Locale.FRENCH)
        def formattedDate = dateFormat.format(LAST_MODIFIED)
        def result = formatter.format(fileSystemEntry)
        assert result.contains(formattedDate)
    }

    void testFormat_Directory() {
        def fileSystemEntry = new DirectoryEntry(path: DIR_PATH, lastModified: LAST_MODIFIED,
                owner: OWNER, group: GROUP, permissions: DIR_PERMISSIONS)
        LOG.info(fileSystemEntry)
        verifyFormat(fileSystemEntry, "drwxr-xr-x  1 owner123 group456               0 $lastModifiedFormatted etc")
    }

    void testFormat_Directory_Defaults() {
        def fileSystemEntry = new DirectoryEntry(path: DIR_PATH, lastModified: LAST_MODIFIED)
        LOG.info(fileSystemEntry)
        verifyFormat(fileSystemEntry, "drwxrwxrwx  1 none     none                   0 $lastModifiedFormatted etc")
    }

    void setUp() {
        super.setUp()
        formatter = new UnixDirectoryListingFormatter()
        def dateFormat = new SimpleDateFormat(UnixDirectoryListingFormatter.DATE_FORMAT, Locale.ENGLISH)
        lastModifiedFormatted = dateFormat.format(LAST_MODIFIED)
        defaultLocale = Locale.default
    }

    void tearDown() {
        super.tearDown()
        Locale.setDefault(defaultLocale)
    }

    private void verifyFormat(FileSystemEntry fileSystemEntry, String expectedResult) {
        def result = formatter.format(fileSystemEntry)
        LOG.info("result=  [$result]")
        LOG.info("expected=[$expectedResult]")
        assert result == expectedResult
    }

}