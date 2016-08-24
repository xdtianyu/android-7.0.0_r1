/*
 * Copyright (C) 2011 The Android Open Source Project
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

package vogar.android;

import com.google.common.collect.ImmutableList;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import vogar.Log;
import vogar.commands.Command;
import vogar.commands.CommandFailedException;

/**
 * Make directories on a remote filesystem.
 */
public final class DeviceFilesystem {
    private final Set<File> mkdirCache = new HashSet<File>();
    private final List<String> targetProcessPrefix;
    private final Log log;

    public DeviceFilesystem(Log log, ImmutableList<String> targetProcessPrefix) {
        this.log = log;
        this.targetProcessPrefix = targetProcessPrefix;
    }

    public void mkdirs(File name) {
        LinkedList<File> directoryStack = new LinkedList<File>();
        File dir = name;
        // Do some directory bootstrapping since "mkdir -p" doesn't work in adb shell. Don't bother
        // trying to create /sdcard or /. This might reach dir == null if given a relative path,
        // otherwise it should terminate with "/sdcard" or "/".
        while (dir != null && !dir.getPath().equals("/sdcard") && !dir.getPath().equals("/")) {
            directoryStack.addFirst(dir);
            dir = dir.getParentFile();
        }
        // would love to do "adb shell mkdir DIR1 DIR2 DIR3 ..." but unfortunately this will stop
        // if any of the directories fail to be created (even for a reason like "file exists"), so
        // they have to be created one by one.
        for (File createDir : directoryStack) {
            // to reduce adb traffic, only try to make a directory if we haven't tried before.
            if (!mkdirCache.contains(createDir)) {
                mkdir(createDir);
                mkdirCache.add(createDir);
            }
        }
    }

    private void mkdir(File name) {
        List<String> args = new ArrayList<String>();
        args.addAll(targetProcessPrefix);
        args.add("mkdir");
        args.add(name.getPath());

        List<String> rawResult = new Command.Builder(log)
                .args(args)
                .permitNonZeroExitStatus(true)
                .execute();
        // fail if this failed for any reason other than the file existing.
        if (!rawResult.isEmpty() && !rawResult.get(0).contains("File exists")) {
            throw new CommandFailedException(args, rawResult);
        }
    }

    public List<File> ls(File dir) throws FileNotFoundException {
        List<String> args = new ArrayList<String>();
        args.addAll(targetProcessPrefix);
        args.add("ls");
        args.add(dir.getPath());

        List<String> rawResult = new Command.Builder(log)
                .args(args)
                // Note: When all supported versions of Android correctly return the exit code
                // from adb we can rely on the exit code to detect failure. Until then: no.
                .permitNonZeroExitStatus(true)
                .execute();
        List<File> files = new ArrayList<File>();
        for (String fileString : rawResult) {
            if (fileString.equals(dir.getPath() + ": No such file or directory")) {
                throw new FileNotFoundException(dir + " not found.");
            }
            if (fileString.equals(dir.getPath())) {
                // The argument must have been a file or symlink, not a directory
                files.add(dir);
            } else {
                files.add(new File(dir, fileString));
            }
        }
        return files;
    }
}
