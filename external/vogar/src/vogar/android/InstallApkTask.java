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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import vogar.Action;
import vogar.Classpath;
import vogar.Result;
import vogar.Run;
import vogar.TestProperties;
import vogar.commands.Command;
import vogar.tasks.Task;

public final class InstallApkTask extends Task {
    public static final String ACTIVITY_CLASS = "vogar.target.TestActivity";

    private final Action action;
    private final File jar;
    private final Run run;

    public InstallApkTask(Run run, Action action, File jar) {
        super("aapt and push " + action.getName());
        this.action = action;
        this.jar = jar;
        this.run = run;
    }

    @Override protected Result execute() throws Exception {
        // We can't put multiple dex files in one apk.
        // We can't just give dex multiple jars with conflicting class names

        // With that in mind, the APK packaging strategy is as follows:
        // 1. dx to create a dex
        // 2. aapt the dex to create apk
        // 3. sign the apk
        // 4. install the apk
        File dex = createDex(action, jar);
        File apk = createApk(action, dex);
        signApk(apk);
        installApk(action, apk);
        return Result.SUCCESS;
    }

    /**
     * Returns a single dexfile containing {@code action}'s classes and all
     * dependencies.
     */
    private File createDex(Action action, File actionJar) {
        File dex = run.localFile(action, "classes.dex");
        Classpath classesToDex = Classpath.of(actionJar);
        classesToDex.addAll(run.classpath);
        if (run.useJack) {
            // TODO Implement Jack support for mode=activity.
            throw new UnsupportedOperationException(
                    "Jack support for --mode=activity not yet implemented");
        }
        run.androidSdk.dex(dex, classesToDex);
        return dex;
    }

    private File createApk (Action action, File dex) {
        String androidManifest =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" +
            "<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"\n" +
            "      package=\"" + packageName(action) + "\">\n" +
            "    <uses-permission android:name=\"android.permission.INTERNET\" />\n" +
            "    <application" +
                    ((run.debugging) ? " android:debuggable=\"true\"" : "") + ">\n" +
            "        <activity android:name=\"" + ACTIVITY_CLASS + "\">\n" +
            "            <intent-filter>\n" +
            "                <action android:name=\"android.intent.action.MAIN\" />\n" +
            "                <category android:name=\"android.intent.category.LAUNCHER\" />\n" +
            "            </intent-filter>\n" +
            "        </activity>\n" +
            "    </application>\n" +
            "</manifest>\n";
        File androidManifestFile = run.localFile(action, "classes", "AndroidManifest.xml");
        try {
            FileOutputStream androidManifestOut =
                    new FileOutputStream(androidManifestFile);
            androidManifestOut.write(androidManifest.getBytes("UTF-8"));
            androidManifestOut.close();
        } catch (IOException e) {
            throw new RuntimeException("Problem writing " + androidManifestFile, e);
        }

        File apk = run.localFile(action, action + ".apk");
        run.androidSdk.packageApk(apk, androidManifestFile);
        run.androidSdk.addToApk(apk, dex);
        run.androidSdk.addToApk(apk, run.localFile(action, "classes", TestProperties.FILE));
        return apk;
    }

    /**
     * According to android.content.pm.PackageParser, package name
     * "must have at least one '.' separator" Since the qualified name
     * may not contain a dot, we prefix containing one to ensure we
     * are compliant.
     */
    public static String packageName(Action action) {
        return "vogar.test." + action.getName();
    }

    private void signApk(File apkUnsigned) {
        /*
         * key generated with this command, using "password" for the key and keystore passwords:
         *     keytool -genkey -v -keystore src/vogar/vogar.keystore \
         *         -keyalg RSA -validity 10000 -alias vogar
         */
        new Command(run.log, "jarsigner",
                "--storepass", "password",
                "-keystore", run.keystore.getPath(),
                apkUnsigned.getPath(),
                "vogar")
                .execute();
    }

    private void installApk(Action action, File apkSigned) {
        // install the local apk ona the device
        run.androidSdk.uninstall(packageName(action));
        run.androidSdk.install(apkSigned);
    }
}
