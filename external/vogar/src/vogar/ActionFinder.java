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
import java.util.Map;

/**
 * Handles finding actions to perform, given files and classes.
 */
public final class ActionFinder {
    private final Log log;
    private final Map<String, Action> actions;
    private final Map<String, Outcome> outcomes;

    public ActionFinder(Log log, Map<String, Action> actions, Map<String, Outcome> outcomes) {
        this.log = log;
        this.actions = actions;
        this.outcomes = outcomes;
    }

    public void findActions(File file) {
        findActionsRecursive(file, 0);
    }

    private void findActionsRecursive(File file, int depth) {
        if (file.isDirectory()) {
            int size = actions.size();
            for (File child : file.listFiles()) {
                findActionsRecursive(child, depth + 1);
            }
            if (depth < 3) {
                log.verbose("found " + (actions.size() - size) + " actions in " + file);
            }
            return;
        }

        // Don't try to treat this file as a class unless it resembles a .java file
        if (!matches(file)) {
            return;
        }

        try {
            Action action = fileToAction(file);
            actions.put(action.getName(), action);
        } catch (IllegalArgumentException e) {
            String actionName = Action.nameForJavaFile(file);
            Action action = new Action(actionName, null, null, null, file);
            actions.put(actionName, action);
            outcomes.put(actionName, new Outcome(actionName, Result.UNSUPPORTED, e));
        }
    }

    private boolean matches(File file) {
        return !file.getName().startsWith(".") && file.getName().endsWith(".java");
    }

    /**
     * Returns an action for the given .java file.
     */
    private Action fileToAction(File javaFile) {
        try {
            DotJavaFile dotJavaFile = DotJavaFile.parse(javaFile);
            File resourcesDir = dotJavaFile.isJtreg() ? javaFile.getParentFile() : null;
            return new Action(dotJavaFile.getActionName(), dotJavaFile.getClassName(), resourcesDir,
                    getSourcePath(javaFile, dotJavaFile.getClassName()), javaFile);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Returns the source path of {@code file}.
     */
    private File getSourcePath(File file, String className) {
        String path = file.getPath();
        String relativePath = className.replace('.', File.separatorChar) + ".java";
        if (!path.endsWith(relativePath)) {
            throw new IllegalArgumentException("Expected a file ending in " + relativePath + " but found " + path);
        }
        return new File(path.substring(0, path.length() - relativePath.length()));
    }
}
