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

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import vogar.Result;
import vogar.util.IoUtils;

public final class ExtractJarResourceTask extends Task {
    private final String jarResource;
    private final File extractedResource;

    public ExtractJarResourceTask(String jarResource, File extractedResource) {
        super("extract " + jarResource + " to " + extractedResource);
        this.jarResource = jarResource;
        this.extractedResource = extractedResource;
    }

    @Override protected Result execute() throws Exception {
        IoUtils.safeMkdirs(extractedResource.getParentFile());
        InputStream in = new BufferedInputStream(
                getClass().getResourceAsStream(jarResource));
        OutputStream out = new BufferedOutputStream(new FileOutputStream(extractedResource));
        byte[] buf = new byte[1024];
        int count;
        while ((count = in.read(buf)) != -1) {
            out.write(buf, 0, count);
        }
        out.close();
        in.close();
        return Result.SUCCESS;
    }
}