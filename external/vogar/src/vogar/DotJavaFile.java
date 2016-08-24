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

package vogar;

import java.io.File;
import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import vogar.util.Strings;

/**
 * A {@code .java} file for execution as an action.
 */
public final class DotJavaFile {
    private static final Pattern PACKAGE_PATTERN = Pattern.compile(
            "(?m)^\\s*package\\s+(\\S+)\\s*;");
    private static final Pattern TYPE_DECLARATION_PATTERN = Pattern.compile(
            "(?m)\\b(?:public|private\\s+)?(?:final\\s+)?(?:interface|class|enum)\\b");
    private static final Pattern AT_TEST_PATTERN = Pattern.compile("\\W@test\\W");

    private final String simpleName;
    private final String packageName;
    private final String actionName;
    private final boolean isJtreg;

    private DotJavaFile(String simpleName, String packageName, String actionName, boolean isJtreg) {
        this.simpleName = simpleName;
        this.packageName = packageName;
        this.actionName = actionName;
        this.isJtreg = isJtreg;
    }

    public String getActionName() {
        return actionName;
    }

    /**
     * Returns true if this file looks like a jtreg test. Jtreg tests usually
     * require additional target setup; in particular they expect to be
     * able to load files from their working directory.
     */
    public boolean isJtreg() {
        return isJtreg;
    }

    public String getClassName() {
        return packageName != null ? packageName + "." + simpleName : simpleName;
    }

    public static DotJavaFile parse(File javaFile) throws IOException {
        // We can get the unqualified class name from the path.
        // It's the last element minus the trailing ".java".
        String filename = javaFile.getName();
        String simpleName = filename.substring(0, filename.length() - ".java".length());

        // For the package, the only foolproof way is to look for the package
        // declaration inside the file.
        String content = Strings.readFile(javaFile);
        boolean isjtreg = AT_TEST_PATTERN.matcher(content).find();

        String packageName;
        String actionName;
        Matcher packageMatcher = PACKAGE_PATTERN.matcher(content);
        if (packageMatcher.find()) {
            packageName = packageMatcher.group(1);
            actionName = packageName + "." + simpleName;
        } else {
            packageName = null;
            actionName = Action.nameForJavaFile(javaFile);
        }

        if (!TYPE_DECLARATION_PATTERN.matcher(content).find()) {
            throw new IllegalArgumentException("Malformed .java file: " + javaFile);
        }

        return new DotJavaFile(simpleName, packageName, actionName, isjtreg);
    }
}
