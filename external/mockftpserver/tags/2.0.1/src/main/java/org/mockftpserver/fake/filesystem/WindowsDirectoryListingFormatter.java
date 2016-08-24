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

import org.mockftpserver.core.util.StringUtil;

import java.text.DateFormat;
import java.text.SimpleDateFormat;

/**
 * Windows-specific implementation of the DirectoryListingFormatter interface.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public class WindowsDirectoryListingFormatter implements DirectoryListingFormatter {

    private static final String DATE_FORMAT = "MM-dd-yy hh:mmaa";
    private static final int SIZE_WIDTH = 15;

    /**
     * Format the directory listing for a single file/directory entry.
     *
     * @param fileSystemEntry - the FileSystemEntry for a single file system entry
     * @return the formatted directory listing
     */
    public String format(FileSystemEntry fileSystemEntry) {
        DateFormat dateFormat = new SimpleDateFormat(DATE_FORMAT);
        String dateStr = dateFormat.format(fileSystemEntry.getLastModified());
        String dirOrSize = fileSystemEntry.isDirectory()
                ? StringUtil.padRight("<DIR>", SIZE_WIDTH)
                : StringUtil.padLeft(Long.toString(fileSystemEntry.getSize()), SIZE_WIDTH);
        return dateStr + "  " + dirOrSize + "  " + fileSystemEntry.getName();
    }

}