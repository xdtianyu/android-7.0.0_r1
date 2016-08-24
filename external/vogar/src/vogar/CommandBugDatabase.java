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

import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import vogar.commands.Command;

/**
 * A bug database that shells out to another process.
 */
class CommandBugDatabase implements ExpectationStore.BugDatabase {
    private final Log log;
    private final String openBugsCommand;

    public CommandBugDatabase(Log log, String openBugsCommand) {
        this.log = log;
        this.openBugsCommand = openBugsCommand;
    }

    @Override public Set<Long> bugsToOpenBugs(Set<Long> bugs) {
        // query the external app for open bugs
        List<String> openBugs = new Command.Builder(log)
                .args(openBugsCommand)
                .args(bugs)
                .execute();
        Set<Long> openBugsSet = new LinkedHashSet<Long>();
        for (String bug : openBugs) {
            openBugsSet.add(Long.parseLong(bug));
        }
        return openBugsSet;
    }
}
