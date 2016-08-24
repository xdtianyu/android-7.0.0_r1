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

package vogar.tasks;

import java.io.File;
import vogar.Action;
import vogar.Result;
import vogar.Target;

public final class PrepareUserDirTask extends Task {
    private final Target target;
    private final Action action;

    public PrepareUserDirTask(Target target, Action action) {
        super("prepare " + action.getUserDir());
        this.target = target;
        this.action = action;
    }

    @Override protected Result execute() throws Exception {
        File userDir = action.getUserDir();
        target.mkdirs(userDir);
        File resourcesDirectory = action.getResourcesDirectory();
        if (resourcesDirectory != null) {
            target.push(resourcesDirectory, userDir);
        }
        return Result.SUCCESS;
    }
}
