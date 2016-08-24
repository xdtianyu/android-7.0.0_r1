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
import vogar.Result;
import vogar.Run;
import vogar.Target;
import vogar.Vogar;

public final class PrepareTarget extends Task {
    private final Run run;
    private final Target target;

    public PrepareTarget(Run run, Target target) {
        super("prepare target");
        this.run = run;
        this.target = target;
    }

    @Override protected Result execute() throws Exception {
        // Even if runner dir is /vogar/run, the grandparent will be / (and non-null)
        target.await(run.runnerDir.getParentFile().getParentFile());
        if (run.cleanBefore) {
            target.rm(run.runnerDir);
        }
        target.mkdirs(run.runnerDir);
        target.mkdirs(run.vogarTemp());
        target.mkdirs(run.dalvikCache());
        for (int i = 0; i < Vogar.NUM_PROCESSORS; i++) {
            target.forwardTcp(run.firstMonitorPort + i);
        }
        // Only forward port if we need to bind to a remote port ourselves. In app debugging DDMS
        // takes care of opening a port on the device and forwarding it.
        if (run.debugPort != null) {
            target.forwardTcp(run.debugPort);
        }
        target.mkdirs(run.deviceUserHome);

        // push ~/.caliperrc to device if found
        File hostCaliperRc = Vogar.dotFile(".caliperrc");
        if (hostCaliperRc.exists()) {
            target.push(hostCaliperRc, new File(run.deviceUserHome, ".caliperrc"));
        }
        return Result.SUCCESS;
    }
}
