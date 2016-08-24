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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.mockftpserver.core.util.StringUtil;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Locale;

/**
 * Unix-specific implementation of the DirectoryListingFormatter interface.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class UnixDirectoryListingFormatter implements DirectoryListingFormatter {

    private static final Logger LOG = LoggerFactory.getLogger(UnixDirectoryListingFormatter.class);

    private static final String DATE_FORMAT = "MMM dd  yyyy";
    private static final int SIZE_WIDTH = 15;
    private static final int OWNER_WIDTH = 8;
    private static final int GROUP_WIDTH = 8;
    private static final String NONE = "none";

    private Locale locale = Locale.ENGLISH;

    // "-rw-rw-r--    1 ftp      ftp           254 Feb 23  2007 robots.txt"
    // "-rw-r--r--    1 ftp      ftp      30014925 Apr 15 00:19 md5.sums.gz"
    // "-rwxr-xr-x   1 henry    users       5778 Dec  1  2005 planaccess.sql"

    /**
     * Format the directory listing for a single file/directory entry.
     *
     * @param fileSystemEntry - the FileSystemEntry for a single file system entry
     * @return the formatted directory listing
     */
    public String format(FileSystemEntry fileSystemEntry) {
        DateFormat dateFormat = new SimpleDateFormat(DATE_FORMAT, locale);
        String dateStr = dateFormat.format(fileSystemEntry.getLastModified());
        String dirOrFile = fileSystemEntry.isDirectory() ? "d" : "-";
        Permissions permissions = fileSystemEntry.getPermissions() != null ? fileSystemEntry.getPermissions() : Permissions.DEFAULT;
        String permissionsStr = StringUtil.padRight(permissions.asRwxString(), 9);
        String linkCountStr = "1";
        String ownerStr = StringUtil.padRight(stringOrNone(fileSystemEntry.getOwner()), OWNER_WIDTH);
        String groupStr = StringUtil.padRight(stringOrNone(fileSystemEntry.getGroup()), GROUP_WIDTH);
        String sizeStr = StringUtil.padLeft(Long.toString(fileSystemEntry.getSize()), SIZE_WIDTH);
        String listing = "" + dirOrFile + permissionsStr + "  " + linkCountStr + " " + ownerStr + " " + groupStr + " " + sizeStr + " " + dateStr + " " + fileSystemEntry.getName();
        LOG.info("listing=[" + listing + "]");
        return listing;
    }

    /**
     * Set the Locale to be used in formatting the date within file/directory listings
     * @param locale - the Locale instance
     */
    public void setLocale(Locale locale) {
        this.locale = locale;
    }

    private String stringOrNone(String string) {
        return (string == null) ? NONE : string;
    }

}
