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

package vogar.testing;

import org.junit.rules.MethodRule;

/**
 * A simple wrapper to isolate the deprecated code.
 *
 * <p>Although MethodRule has been marked as deprecated in the current version of JUnit (4.10) it
 * has been undeprecated in JUnit 4.11.
 */
// TODO(paulduffin): Remove this once we have upgraded to 4.11 or above.
@SuppressWarnings("deprecation")
public interface UndeprecatedMethodRule extends MethodRule {
}
