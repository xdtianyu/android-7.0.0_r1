/*
 * Copyright (C) 2016 The Android Open Source Project
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

/**
 * An enum for the different language variants supported by vogar.
 */
public enum Language {
  J17("1.7", "1.7", 23),
  JN("1.8", "1.8", 24);

  private final String javacSourceAndTarget;
  private final String jackSourceVersion;
  private final int jackMinApilevel;

  Language(String javacSourceAndTarget, String jackSourceVersion, int jackMinApiLevel) {
    this.javacSourceAndTarget = javacSourceAndTarget;
    this.jackSourceVersion = jackSourceVersion;
    this.jackMinApilevel = jackMinApiLevel;
  }

  public String getJavacSourceAndTarget() {
    return javacSourceAndTarget;
  }

  public String getJackSourceVersion() {
    return jackSourceVersion;
  }

  public int getJackMinApilevel() {
    return jackMinApilevel;
  }
}
