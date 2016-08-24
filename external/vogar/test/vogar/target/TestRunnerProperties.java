/*
 * Copyright (C) 2015 The Android Open Source Project
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
package vogar.target;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Allows test properties to be specified for tests.
 */
@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
public @interface TestRunnerProperties {

    int monitorPort() default 9999;

    boolean profile() default false;

    int profileDepth() default 4;

    String profileFile() default "default-profile-file";

    int profileInterval() default 10;

    boolean profileThreadGroup() default false;

    String qualifiedName() default "";

    Class testClass() default Default.class;

    String testClassOrPackage() default "";

    boolean testOnly() default false;

    int timeout() default 0;

    class Default {}
}
