/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.google.common.annotations.VisibleForTesting;
import com.google.common.collect.ImmutableList;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.List;
import java.util.concurrent.TimeoutException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import vogar.Log;
import vogar.Md5Cache;
import vogar.Target;
import vogar.commands.Command;

public final class AdbTarget extends Target {

    private static final ImmutableList<String> TARGET_PROCESS_PREFIX =
            ImmutableList.of("adb", "shell");

    private final Log log;

    private final DeviceFilesystem deviceFilesystem;

    private final Md5Cache pushCache;

   @VisibleForTesting
   public AdbTarget(Log log, DeviceFilesystem deviceFilesystem, DeviceFileCache deviceFileCache) {
       this.log = log;
       this.deviceFilesystem = deviceFilesystem;
       this.pushCache =
               deviceFileCache == null ? null : new Md5Cache(log, "pushed", deviceFileCache);
    }

    public static File defaultDeviceDir() {
        return new File("/data/local/tmp/vogar");
    }

    @Override protected ImmutableList<String> targetProcessPrefix() {
        return TARGET_PROCESS_PREFIX;
    }

    @Override public void await(File directory) {
        waitForDevice();
        ensureDirectory(directory);
        remount();
    }

    private void waitForDevice() {
        new Command.Builder(log)
            .args("adb", "wait-for-device")
            .permitNonZeroExitStatus(true)
            .execute();
    }

    /**
     * Make sure the directory exists.
     */
    private void ensureDirectory(File directory) {
        String pathArgument = directory.getPath() + "/";
        if (pathArgument.equals("/sdcard/")) {
            // /sdcard is a mount point. If it exists but is empty we do
            // not want to use it. So we wait until it is not empty.
            waitForNonEmptyDirectory(pathArgument, 5 * 60);
        } else {
            Command command = new Command.Builder(log)
                .args("adb", "shell", "ls", pathArgument)
                .permitNonZeroExitStatus(true)
                .build();
            List<String> output = command.execute();
            // TODO: We should avoid checking for the error message, and instead have
            // the Command class understand a non-zero exit code from an adb shell command.
            if (!output.isEmpty()
                && output.get(0).equals(pathArgument + ": No such file or directory")) {
                throw new RuntimeException("'" + pathArgument + "' does not exist on device");
            }
            // Otherwise the directory exists.
        }
    }

    private void remount() {
        new Command(log, "adb", "remount").execute();
    }

    private void waitForNonEmptyDirectory(String pathArgument, int timeoutSeconds) {
        final int millisPerSecond = 1000;
        final long start = System.currentTimeMillis();
        final long deadline = start + (millisPerSecond * timeoutSeconds);

        while (true) {
            final int remainingSeconds =
                    (int) ((deadline - System.currentTimeMillis()) / millisPerSecond);
            Command command = new Command.Builder(log)
                    .args("adb", "shell", "ls", pathArgument)
                    .permitNonZeroExitStatus(true)
                    .build();
            List<String> output;
            try {
                output = command.executeWithTimeout(remainingSeconds);
            } catch (TimeoutException e) {
                throw new RuntimeException("Timed out after " + timeoutSeconds
                        + " seconds waiting for " + pathArgument, e);
            }
            try {
                Thread.sleep(millisPerSecond);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }

            // We just want any output.
            if (!output.isEmpty()) {
                return;
            }

            log.warn("Waiting on " + pathArgument + " to be mounted ");
        }
    }

    @Override public List<File> ls(File directory) throws FileNotFoundException {
        return deviceFilesystem.ls(directory);
    }

    @Override public String getDeviceUserName() {
        // The default environment doesn't include $USER, so dalvikvm doesn't set "user.name".
        // DeviceRuntime uses this to set "user.name" manually with -D.
        String line = new Command(log, "adb", "shell", "id").execute().get(0);
        // TODO: use 'id -un' when we don't need to support anything older than M
        Matcher m = Pattern.compile("^uid=\\d+\\((\\S+)\\) gid=\\d+\\(\\S+\\).*").matcher(line);
        return m.matches() ? m.group(1) : "root";
    }

    @Override public void rm(File file) {
        new Command.Builder(log).args("adb", "shell", "rm", "-r", file.getPath())
                // Note: When all supported versions of Android correctly return the exit code
                // from adb we can rely on the exit code to detect failure. Until then: no.
                .permitNonZeroExitStatus(true)
                .execute();
    }

    @Override public void mkdirs(File file) {
        deviceFilesystem.mkdirs(file);
    }

    @Override public void forwardTcp(int port) {
        new Command(log, "adb", "forward", "tcp:" + port, "tcp:" + port).execute();
    }

    @Override public void push(File local, File remote) {
        Command fallback = new Command(log, "adb", "push", local.getPath(), remote.getPath());
        deviceFilesystem.mkdirs(remote.getParentFile());

        // don't yet cache directories (only used by jtreg tests)
        if (pushCache != null && local.isFile()) {
            String key = pushCache.makeKey(local);
            boolean cacheHit = pushCache.getFromCache(remote, key);
            if (cacheHit) {
                log.verbose("device cache hit for " + local);
                return;
            }
            fallback.execute();
            pushCache.insert(key, remote);
        } else {
            fallback.execute();
        }
    }

    @Override public void pull(File remote, File local) {
        new Command(log, "adb", "pull", remote.getPath(), local.getPath()).execute();
    }
}
