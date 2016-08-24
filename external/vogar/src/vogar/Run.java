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

package vogar;

import com.google.common.base.Splitter;

import com.google.common.base.Supplier;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.Collections;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;

import vogar.android.ActivityMode;
import vogar.android.AndroidSdk;
import vogar.android.DeviceRuntime;
import vogar.android.HostRuntime;
import vogar.commands.CommandFailedException;
import vogar.commands.Jack;
import vogar.commands.Mkdir;
import vogar.commands.Rm;
import vogar.tasks.TaskQueue;
import vogar.util.Strings;

public final class Run {
    /**
     * A list of generic names that we avoid when naming generated files.
     */
    private static final Set<String> BANNED_NAMES = new HashSet<String>();

    private static final String JAR_URI_PREFIX = "jar:file:";

    private static final String FILE_URL_PREFIX = "file:";

    private static final String VOGAR_CLASS_RESOURCE_PATH = "/vogar/Vogar.class";

    static {
        BANNED_NAMES.add("classes");
        BANNED_NAMES.add("javalib");
    }

    public final File xmlReportsDirectory;
    public final File resultsDir;
    public final boolean recordResults;
    public final ExpectationStore expectationStore;
    public final Date date;
    public final String invokeWith;
    public final File keystore;
    public final Log log;
    public final Classpath classpath;
    public final Classpath buildClasspath;
    public final Classpath resourceClasspath;
    public final List<File> sourcepath;
    public final Mkdir mkdir;
    public final Rm rm;
    public final int firstMonitorPort;
    public final int timeoutSeconds;
    public final boolean profile;
    public final boolean profileBinary;
    public final int profileDepth;
    public final int profileInterval;
    public final boolean profileThreadGroup;
    public final File profileFile;
    public final File javaHome;
    public final Integer debugPort;
    public final Language language;
    public final List<String> javacArgs;
    public final boolean benchmark;
    public final File runnerDir;
    public final boolean cleanBefore;
    public final boolean cleanAfter;
    public final File localTemp;
    public final int maxConcurrentActions;
    public final File deviceUserHome;
    public final Console console;
    public final int smallTimeoutSeconds;
    public final String vmCommand;
    public final String dalvikCache;
    public final List<String> additionalVmArgs;
    public final List<String> targetArgs;
    public final boolean useBootClasspath;
    public final int largeTimeoutSeconds;
    public final RetrievedFilesFilter retrievedFiles;
    public final Driver driver;
    public final Mode mode;
    public final Target target;
    public final AndroidSdk androidSdk;
    public final XmlReportPrinter reportPrinter;
    public final JarSuggestions jarSuggestions;
    public final ClassFileIndex classFileIndex;
    public final OutcomeStore outcomeStore;
    public final TaskQueue taskQueue;
    public final boolean testOnly;
    public final boolean useJack;
    public final boolean checkJni;
    public final boolean debugging;
    public final Md5Cache jackCache;

    public Run(Vogar vogar, boolean useJack, Console console, Mkdir mkdir, AndroidSdk androidSdk,
            Rm rm, Target target, File runnerDir)
            throws IOException {
        this.console = console;

        this.localTemp = new File("/tmp/vogar/" + UUID.randomUUID());
        this.log = console;

        this.target = target;

        this.useJack = useJack;
        this.jackCache = useJack ? new Md5Cache(log, "jack", new HostFileCache(log, mkdir)) : null;
        this.vmCommand = vogar.vmCommand;
        this.dalvikCache = vogar.dalvikCache;
        this.additionalVmArgs = vogar.vmArgs;
        this.benchmark = vogar.benchmark;
        this.cleanBefore = vogar.cleanBefore;
        this.cleanAfter = vogar.cleanAfter;
        this.date = new Date();
        this.debugPort = vogar.debugPort;
        this.runnerDir = runnerDir;
        this.deviceUserHome = new File(runnerDir, "user.home");
        this.mkdir = mkdir;
        this.rm = rm;
        this.firstMonitorPort = vogar.firstMonitorPort;
        this.invokeWith = vogar.invokeWith;
        this.language = vogar.language;
        this.javacArgs = vogar.javacArgs;
        this.javaHome = vogar.javaHome;
        this.largeTimeoutSeconds = vogar.timeoutSeconds * Vogar.LARGE_TIMEOUT_MULTIPLIER;
        this.maxConcurrentActions = (vogar.stream || vogar.modeId == ModeId.ACTIVITY)
                    ? 1
                    : Vogar.NUM_PROCESSORS;
        this.timeoutSeconds = vogar.timeoutSeconds;
        this.smallTimeoutSeconds = vogar.timeoutSeconds;
        this.sourcepath = vogar.sourcepath;
        this.resourceClasspath = Classpath.of(vogar.resourceClasspath);
        this.useBootClasspath = vogar.useBootClasspath;
        this.targetArgs = vogar.targetArgs;
        this.xmlReportsDirectory = vogar.xmlReportsDirectory;
        this.profile = vogar.profile;
        this.profileBinary = vogar.profileBinary;
        this.profileFile = vogar.profileFile;
        this.profileDepth = vogar.profileDepth;
        this.profileInterval = vogar.profileInterval;
        this.profileThreadGroup = vogar.profileThreadGroup;
        this.recordResults = vogar.recordResults;
        this.resultsDir = vogar.resultsDir == null
                ? new File(vogar.vogarDir, "results")
                : vogar.resultsDir;
        this.keystore = localFile("activity", "vogar.keystore");
        this.classpath = Classpath.of(vogar.classpath);
        this.classpath.addAll(vogarJar());
        this.testOnly = vogar.testOnly;

        this.androidSdk = androidSdk;

        expectationStore = ExpectationStore.parse(
            console, vogar.expectationFiles, vogar.modeId, vogar.variant);
        if (vogar.openBugsCommand != null) {
            expectationStore.loadBugStatuses(new CommandBugDatabase(log, vogar.openBugsCommand));
        }

        this.mode = createMode(vogar.modeId, vogar.variant);

        this.buildClasspath = Classpath.of(vogar.buildClasspath);
        if (androidSdk != null) {
            buildClasspath.addAll(androidSdk.getCompilationClasspath());
        }

        this.classFileIndex = new ClassFileIndex(log, mkdir, vogar.jarSearchDirs);
        if (vogar.suggestClasspaths) {
            classFileIndex.createIndex();
        }

        this.retrievedFiles = new RetrievedFilesFilter(profile, profileFile);
        this.reportPrinter = new XmlReportPrinter(xmlReportsDirectory, expectationStore, date);
        this.jarSuggestions = new JarSuggestions();
        this.outcomeStore = new OutcomeStore(log, mkdir, rm, resultsDir, recordResults,
                expectationStore, date);
        this.driver = new Driver(this);
        this.taskQueue = new TaskQueue(console, maxConcurrentActions);
        this.checkJni = vogar.checkJni;
        this.debugging = (vogar.debugPort != null) || vogar.debugApp;
    }

    private Mode createMode(ModeId modeId, Variant variant) {
        switch (modeId) {
            case JVM:
                return new JavaVm(this);
            case HOST:
                return new HostRuntime(this, modeId, variant);
            case DEVICE:
            case APP_PROCESS:
                return new DeviceRuntime(this, modeId, variant, new Supplier<String>() {
                    @Override
                    public String get() {
                        return target.getDeviceUserName();
                    }
                });
            case ACTIVITY:
                return new ActivityMode(this);
            default:
                throw new IllegalArgumentException("Unsupported mode: " + modeId);
        }
    }

    public final File localFile(Object... path) {
        return new File(localTemp + "/" + Strings.join("/", path));
    }

    private File vogarJar() {
        URL jarUrl = Vogar.class.getResource(VOGAR_CLASS_RESOURCE_PATH);
        if (jarUrl == null) {
            // should we add an option for IDE users, to use a user-specified vogar.jar?
            throw new IllegalStateException("Vogar cannot find its own .jar");
        }

        /*
         * Parse a URI like jar:file:/Users/jessewilson/vogar/vogar.jar!/vogar/Vogar.class
         * to yield a .jar file like /Users/jessewilson/vogar/vogar.jar.
         */
        String url = jarUrl.toString();
        int bang = url.indexOf("!");
        if (url.startsWith(JAR_URI_PREFIX) && bang != -1) {
            return new File(url.substring(JAR_URI_PREFIX.length(), bang));
        } else if (url.startsWith(FILE_URL_PREFIX) && url.endsWith(VOGAR_CLASS_RESOURCE_PATH)) {
            // Vogar is being run from a classes directory.
            return new File(url.substring(FILE_URL_PREFIX.length(),
                    url.length() - VOGAR_CLASS_RESOURCE_PATH.length()));
        } else {
            throw new IllegalStateException("Vogar cannot find the .jar file in " + jarUrl);
        }
    }

    public final File hostJar(Object nameOrAction) {
        return localFile(nameOrAction, nameOrAction + ".jar");
    }

    public File hostJack(Object nameOrAction) {
        return localFile(nameOrAction, nameOrAction + ".jack");
    }

    /**
     * Returns a path for a Java tool such as java, javac, jar where
     * the Java home is used if present, otherwise assumes it will
     * come from the path.
     */
    public String javaPath(String tool) {
        return (javaHome == null)
            ? tool
            : new File(new File(javaHome, "bin"), tool).getPath();
    }

    public File targetDexFile(String name) {
        return new File(runnerDir, name + ".dex.jar");
    }

    public File localDexFile(String name) {
        return localFile(name, name + ".dex.jar");
    }

    /**
     * Returns a recognizable readable name for the given generated .jar file,
     * appropriate for use in naming derived files.
     *
     * @param file a product of the android build system, such as
     *     "out/core-libart_intermediates/javalib.jar".
     * @return a recognizable base name like "core-libart_intermediates".
     */
    public String basenameOfJar(File file) {
        String name = file.getName().replaceAll("(\\.jar|\\.jack)$", "");
        while (BANNED_NAMES.contains(name)) {
            file = file.getParentFile();
            name = file.getName();
        }
        return name;
    }

    public File vogarTemp() {
        return new File(runnerDir, "tmp");
    }

    public File dalvikCache() {
        return new File(runnerDir.getParentFile(), dalvikCache);
    }

    /**
     * Returns the directory where the VM stores its dexopt files.
     */
    public String getAndroidDataPath() {
        // The VM wants the parent directory of a directory named "dalvik-cache"
        return dalvikCache().getParentFile().getPath();
    }

    /**
     * Returns a parsed list of the --invoke-with command and its
     * arguments, or an empty list if no --invoke-with was provided.
     */
    public Iterable<String> invokeWith() {
        if (invokeWith == null) {
            return Collections.emptyList();
        }
        return Splitter.onPattern("\\s+").omitEmptyStrings().split(invokeWith);
    }
}
