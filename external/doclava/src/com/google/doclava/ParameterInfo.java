/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.doclava;

import com.google.clearsilver.jsilver.data.Data;

import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

public class ParameterInfo {
  public ParameterInfo(String name, String typeName, TypeInfo type, boolean isVarArg,
      SourcePositionInfo position, List<AnnotationInstanceInfo> annotationInstanceInfos) {
    mName = name;
    mTypeName = typeName;
    mType = type;
    mIsVarArg = isVarArg;
    mPosition = position;
    mAnnotations = annotationInstanceInfos;
  }

  /**
   * Clone this Parameter, but replace the type according to the typeArgumentMapping provided.
   */
  public ParameterInfo cloneWithTypeArguments(Map<String, TypeInfo> typeArgumentMapping) {
    return new ParameterInfo(
        mName, mTypeName, mType.getTypeWithArguments(typeArgumentMapping),
        mIsVarArg, mPosition, mAnnotations);
  }

  TypeInfo type() {
    return mType;
  }

  String name() {
    return mName;
  }

  String typeName() {
    return mTypeName;
  }

  SourcePositionInfo position() {
    return mPosition;
  }

  boolean isVarArg() {
    return mIsVarArg;
  }

  List<AnnotationInstanceInfo> annotations() {
    return mAnnotations;
  }

  public void makeHDF(Data data, String base, boolean isLastVararg, HashSet<String> typeVariables) {
    makeHDF(data, base, isLastVararg, typeVariables, Collections.<String, TypeInfo>emptyMap());
  }

  public void makeHDF(Data data, String base, boolean isLastVararg, HashSet<String> typeVariables,
      Map<String, TypeInfo> typeMapping) {
    data.setValue(base + ".name", this.name());
    type().getTypeWithArguments(typeMapping).makeHDF(
        data, base + ".type", isLastVararg, typeVariables);
  }

  public static void makeHDF(Data data, String base, ParameterInfo[] params, boolean isVararg,
      HashSet<String> typeVariables) {
    makeHDF(data, base, params, isVararg, typeVariables, Collections.<String, TypeInfo>emptyMap());
  }

  public static void makeHDF(Data data, String base, ParameterInfo[] params, boolean isVararg,
      HashSet<String> typeVariables, Map<String, TypeInfo> typeMapping) {
    for (int i = 0; i < params.length; i++) {
      params[i].makeHDF(
          data, base + "." + i, isVararg && (i == params.length - 1), typeVariables, typeMapping);
    }
  }

  /**
   * Returns true if this parameter's dimension information agrees
   * with the represented callee's dimension information.
   */
  public boolean matchesDimension(String dimension, boolean varargs) {
    if (varargs) {
      dimension += "[]";
    }
    return mType.dimension().equals(dimension);
  }

  String mName;
  String mTypeName;
  TypeInfo mType;
  boolean mIsVarArg;
  SourcePositionInfo mPosition;
  List<AnnotationInstanceInfo> mAnnotations;
}
