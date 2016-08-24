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
import com.sun.javadoc.ClassDoc;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.Set;
import java.util.TreeMap;

public class ClassInfo extends DocInfo implements ContainerInfo, Comparable, Scoped, Resolvable {

  /**
   * Contains a ClassInfo and a TypeInfo.
   * <p>
   * This is used to match a ClassInfo, which doesn't keep track of its type parameters
   * and a type which does.
   */
  private class ClassTypePair {
    private final ClassInfo mClassInfo;
    private final TypeInfo mTypeInfo;

    public ClassTypePair(ClassInfo cl, TypeInfo t) {
      mClassInfo = cl;
      mTypeInfo = t;
    }

    public ClassInfo classInfo() {
      return mClassInfo;
    }

    public TypeInfo typeInfo() {
      return mTypeInfo;
    }

    public Map<String, TypeInfo> getTypeArgumentMapping() {
      return TypeInfo.getTypeArgumentMapping(classInfo(), typeInfo());
    }
  }

  public static final Comparator<ClassInfo> comparator = new Comparator<ClassInfo>() {
    public int compare(ClassInfo a, ClassInfo b) {
      return a.name().compareTo(b.name());
    }
  };

  public static final Comparator<ClassInfo> qualifiedComparator = new Comparator<ClassInfo>() {
    public int compare(ClassInfo a, ClassInfo b) {
      return a.qualifiedName().compareTo(b.qualifiedName());
    }
  };

  /**
   * Constructs a stub representation of a class.
   */
  public ClassInfo(String qualifiedName) {
    super("", SourcePositionInfo.UNKNOWN);
    mQualifiedName = qualifiedName;
    if (qualifiedName.lastIndexOf('.') != -1) {
      mName = qualifiedName.substring(qualifiedName.lastIndexOf('.') + 1);
    } else {
      mName = qualifiedName;
    }
  }

  public ClassInfo(ClassDoc cl, String rawCommentText, SourcePositionInfo position,
          boolean isPublic, boolean isProtected, boolean isPackagePrivate, boolean isPrivate,
          boolean isStatic, boolean isInterface, boolean isAbstract, boolean isOrdinaryClass,
          boolean isException, boolean isError, boolean isEnum, boolean isAnnotation, boolean isFinal,
          boolean isIncluded, String name, String qualifiedName, String qualifiedTypeName,
          boolean isPrimitive) {
      super(rawCommentText, position);

      initialize(rawCommentText, position,
              isPublic, isProtected, isPackagePrivate, isPrivate,
              isStatic, isInterface, isAbstract, isOrdinaryClass,
              isException, isError, isEnum, isAnnotation, isFinal,
              isIncluded, qualifiedTypeName, isPrimitive, null);

      mName = name;
      mQualifiedName = qualifiedName;
      mNameParts = name.split("\\.");
      mClass = cl;
  }

  public void initialize(String rawCommentText, SourcePositionInfo position,
          boolean isPublic, boolean isProtected, boolean isPackagePrivate, boolean isPrivate,
          boolean isStatic, boolean isInterface, boolean isAbstract, boolean isOrdinaryClass,
          boolean isException, boolean isError, boolean isEnum, boolean isAnnotation, boolean isFinal,
          boolean isIncluded, String qualifiedTypeName, boolean isPrimitive, ArrayList<AnnotationInstanceInfo> annotations) {

    // calls
    setPosition(position);
    setRawCommentText(rawCommentText);
    mIsPublic = isPublic;
    mIsProtected = isProtected;
    mIsPackagePrivate = isPackagePrivate;
    mIsPrivate = isPrivate;
    mIsStatic = isStatic;
    mIsInterface = isInterface;
    mIsAbstract = isAbstract;
    mIsOrdinaryClass = isOrdinaryClass;
    mIsException = isException;
    mIsError = isError;
    mIsEnum = isEnum;
    mIsAnnotation = isAnnotation;
    mIsFinal = isFinal;
    mIsIncluded = isIncluded;
    mQualifiedTypeName = qualifiedTypeName;
    mIsPrimitive = isPrimitive;
    mAnnotations = annotations;
    mShowAnnotations = AnnotationInstanceInfo.getShowAnnotationsIntersection(annotations);
  }

  public void init(TypeInfo typeInfo, ArrayList<ClassInfo> interfaces,
          ArrayList<TypeInfo> interfaceTypes, ArrayList<ClassInfo> innerClasses,
          ArrayList<MethodInfo> constructors, ArrayList<MethodInfo> methods,
          ArrayList<MethodInfo> annotationElements, ArrayList<FieldInfo> fields,
          ArrayList<FieldInfo> enumConstants, PackageInfo containingPackage,
          ClassInfo containingClass, ClassInfo superclass,
      TypeInfo superclassType, ArrayList<AnnotationInstanceInfo> annotations) {
    mTypeInfo = typeInfo;
    mRealInterfaces = new ArrayList<ClassInfo>(interfaces);
    mRealInterfaceTypes = interfaceTypes;
    mInnerClasses = innerClasses;
    // mAllConstructors will not contain *all* constructors. Only the constructors that pass
    // checkLevel. @see {@link Converter#convertMethods(ConstructorDoc[])}
    mAllConstructors = constructors;
    // mAllSelfMethods will not contain *all* self methods. Only the methods that pass
    // checkLevel. @see {@link Converter#convertMethods(MethodDoc[])}
    mAllSelfMethods = methods;
    mAnnotationElements = annotationElements;
    // mAllSelfFields will not contain *all* self fields. Only the fields that pass
    // checkLevel. @see {@link Converter#convetFields(FieldDoc[])}
    mAllSelfFields = fields;
    // mEnumConstants will not contain *all* enum constants. Only the enums that pass
    // checkLevel. @see {@link Converter#convetFields(FieldDoc[])}
    mEnumConstants = enumConstants;
    mContainingPackage = containingPackage;
    mContainingClass = containingClass;
    mRealSuperclass = superclass;
    mRealSuperclassType = superclassType;
    mAnnotations = annotations;
    mShowAnnotations = AnnotationInstanceInfo.getShowAnnotationsIntersection(annotations);

    // after providing new methods and new superclass info,clear any cached
    // lists of self + superclass methods, ctors, etc.
    mSuperclassInit = false;
    mConstructors = null;
    mMethods = null;
    mSelfMethods = null;
    mFields = null;
    mSelfFields = null;
    mSelfAttributes = null;
    mDeprecatedKnown = false;
    mSuperclassesWithTypes = null;
    mInterfacesWithTypes = null;
    mAllInterfacesWithTypes = null;

    Collections.sort(mEnumConstants, FieldInfo.comparator);
    Collections.sort(mInnerClasses, ClassInfo.comparator);
  }

  public void init2() {
    // calling this here forces the AttrTagInfo objects to be linked to the AttribtueInfo
    // objects
    selfAttributes();
  }

  public void init3(ArrayList<TypeInfo> types, ArrayList<ClassInfo> realInnerClasses) {
    mTypeParameters = types;
    mRealInnerClasses = realInnerClasses;
  }

  public ArrayList<ClassInfo> getRealInnerClasses() {
    return mRealInnerClasses;
  }

  public ArrayList<TypeInfo> getTypeParameters() {
    return mTypeParameters;
  }

  /**
   * @return true if this class needs to be shown in api txt, based on the
   * hidden/removed status of the class and the show level setting in doclava.
   */
  public boolean checkLevel() {
    if (mCheckLevel == null) {
      mCheckLevel = Doclava.checkLevel(mIsPublic, mIsProtected, mIsPackagePrivate, mIsPrivate,
          isHiddenOrRemoved());
    }

    return mCheckLevel;
  }

  public int compareTo(Object that) {
    if (that instanceof ClassInfo) {
      return mQualifiedName.compareTo(((ClassInfo) that).mQualifiedName);
    } else {
      return this.hashCode() - that.hashCode();
    }
  }

  @Override
  public ContainerInfo parent() {
    return this;
  }

  public boolean isPublic() {
    return mIsPublic;
  }

  public boolean isProtected() {
    return mIsProtected;
  }

  public boolean isPackagePrivate() {
    return mIsPackagePrivate;
  }

  public boolean isPrivate() {
    return mIsPrivate;
  }

  public boolean isStatic() {
    return mIsStatic;
  }

  public boolean isInterface() {
    return mIsInterface;
  }

  public boolean isAbstract() {
    return mIsAbstract;
  }

  public PackageInfo containingPackage() {
    return mContainingPackage;
  }

  public ClassInfo containingClass() {
    return mContainingClass;
  }

  public boolean isOrdinaryClass() {
    return mIsOrdinaryClass;
  }

  public boolean isException() {
    return mIsException;
  }

  public boolean isError() {
    return mIsError;
  }

  public boolean isEnum() {
    return mIsEnum;
  }

  public boolean isAnnotation() {
    return mIsAnnotation;
  }

  public boolean isFinal() {
    return mIsFinal;
  }

  public boolean isEffectivelyFinal() {
    return mIsFinal || mApiCheckConstructors.isEmpty();
  }

  public boolean isIncluded() {
    return mIsIncluded;
  }

  public HashSet<String> typeVariables() {
    HashSet<String> result = TypeInfo.typeVariables(mTypeInfo.typeArguments());
    ClassInfo cl = containingClass();
    while (cl != null) {
      ArrayList<TypeInfo> types = cl.asTypeInfo().typeArguments();
      if (types != null) {
        TypeInfo.typeVariables(types, result);
      }
      cl = cl.containingClass();
    }
    return result;
  }

  /**
   * List of only direct interface's classes, without worrying about type param mapping.
   * This can't be lazy loaded, because its overloads depend on changing type parameters
   * passed in from the callers.
   */
  private List<ClassTypePair> justMyInterfacesWithTypes() {
    return justMyInterfacesWithTypes(Collections.<String, TypeInfo>emptyMap());
  }

  /**
   * List of only direct interface's classes and their parameterized types.
   * This can't be lazy loaded, because of the passed in typeArgumentsMap.
   */
  private List<ClassTypePair> justMyInterfacesWithTypes(Map<String, TypeInfo> typeArgumentsMap) {
    if (mRealInterfaces == null || mRealInterfaceTypes == null) {
      return Collections.<ClassTypePair>emptyList();
    }

    List<ClassTypePair> list = new ArrayList<ClassTypePair>();
    for (int i = 0; i < mRealInterfaces.size(); i++) {
      ClassInfo iface = mRealInterfaces.get(i);
      TypeInfo type = mRealInterfaceTypes.get(i);
      if (iface != null && type != null) {
        type = type.getTypeWithArguments(typeArgumentsMap);
        if (iface.checkLevel()) {
          list.add(new ClassTypePair(iface, type));
        } else {
          // add the interface's interfaces
          Map<String, TypeInfo> map = TypeInfo.getTypeArgumentMapping(iface.asTypeInfo(), type);
          list.addAll(iface.justMyInterfacesWithTypes(map));
        }
      }
    }
    return list;
  }

  /**
   * List of only direct interface's classes, and any hidden superclass's direct interfaces
   * between this class and the first visible superclass and those interface class's parameterized types.
   */
  private ArrayList<ClassTypePair> interfacesWithTypes() {
    if (mInterfacesWithTypes == null) {
      mInterfacesWithTypes = new ArrayList<ClassTypePair>();

      Iterator<ClassTypePair> itr = superClassesWithTypes().iterator();
      // skip the first one, which is this class
      itr.next();
      while (itr.hasNext()) {
        ClassTypePair ctp = itr.next();
        if (ctp.classInfo().checkLevel()) {
          break;
        } else {
          // fill mInterfacesWithTypes from the hidden superclass
          mInterfacesWithTypes.addAll(
              ctp.classInfo().justMyInterfacesWithTypes(ctp.getTypeArgumentMapping()));
        }
      }
      mInterfacesWithTypes.addAll(
          justMyInterfacesWithTypes());
    }
    return mInterfacesWithTypes;
  }

  /**
   * List of all interface's classes reachable in this class's inheritance hierarchy
   * and those interface class's parameterized types.
   */
  private ArrayList<ClassTypePair> allInterfacesWithTypes() {
    if (mAllInterfacesWithTypes == null) {
        mAllInterfacesWithTypes = new ArrayList<ClassTypePair>();
        Queue<ClassTypePair> toParse = new ArrayDeque<ClassTypePair>();
        Set<String> visited = new HashSet<String>();

        Iterator<ClassTypePair> itr = superClassesWithTypes().iterator();
        // skip the first one, which is this class
        itr.next();
        while (itr.hasNext()) {
          ClassTypePair ctp = itr.next();
          toParse.addAll(
              ctp.classInfo().justMyInterfacesWithTypes(ctp.getTypeArgumentMapping()));
        }
        toParse.addAll(justMyInterfacesWithTypes());
        while (!toParse.isEmpty()) {
          ClassTypePair ctp = toParse.remove();
          if (!visited.contains(ctp.typeInfo().fullName())) {
            mAllInterfacesWithTypes.add(ctp);
            visited.add(ctp.typeInfo().fullName());
            toParse.addAll(ctp.classInfo().justMyInterfacesWithTypes(ctp.getTypeArgumentMapping()));
          }
        }
    }
    return mAllInterfacesWithTypes;
  }

  /**
   * A list of ClassTypePairs that contain all superclasses
   * and their corresponding types. The types will have type parameters
   * cascaded upwards so they match, if any classes along the way set them.
   * The list includes the current class, and is an ascending order up the
   * heirarchy tree.
   * */
  private ArrayList<ClassTypePair> superClassesWithTypes() {
    if (mSuperclassesWithTypes == null) {
      mSuperclassesWithTypes = new ArrayList<ClassTypePair>();

      ClassTypePair lastCtp = new ClassTypePair(this, this.asTypeInfo());
      mSuperclassesWithTypes.add(lastCtp);

      Map<String, TypeInfo> typeArgumentsMap;
      ClassInfo superclass = mRealSuperclass;
      TypeInfo supertype = mRealSuperclassType;
      TypeInfo nextType;
      while (superclass != null && supertype != null) {
        typeArgumentsMap = lastCtp.getTypeArgumentMapping();
        lastCtp = new ClassTypePair(superclass, supertype.getTypeWithArguments(typeArgumentsMap));
        mSuperclassesWithTypes.add(lastCtp);

        supertype = superclass.mRealSuperclassType;
        superclass = superclass.mRealSuperclass;
      }
    }
    return mSuperclassesWithTypes;
  }

  private static void gatherHiddenInterfaces(ClassInfo cl, HashSet<ClassInfo> interfaces) {
    for (ClassInfo iface : cl.mRealInterfaces) {
      if (iface.checkLevel()) {
        interfaces.add(iface);
      } else {
        gatherHiddenInterfaces(iface, interfaces);
      }
    }
  }

  public ArrayList<ClassInfo> interfaces() {
    if (mInterfaces == null) {
      if (checkLevel()) {
        HashSet<ClassInfo> interfaces = new HashSet<ClassInfo>();
        ClassInfo superclass = mRealSuperclass;
        while (superclass != null && !superclass.checkLevel()) {
          gatherHiddenInterfaces(superclass, interfaces);
          superclass = superclass.mRealSuperclass;
        }
        gatherHiddenInterfaces(this, interfaces);
        mInterfaces = new ArrayList<ClassInfo>(interfaces);
      } else {
        // put something here in case someone uses it
        mInterfaces = new ArrayList<ClassInfo>(mRealInterfaces);
      }
      Collections.sort(mInterfaces, ClassInfo.qualifiedComparator);
    }
    return mInterfaces;
  }

  public ArrayList<ClassInfo> realInterfaces() {
    return mRealInterfaces;
  }

  ArrayList<TypeInfo> realInterfaceTypes() {
    return mRealInterfaceTypes;
  }

  public void addInterfaceType(TypeInfo type) {
      if (mRealInterfaceTypes == null) {
          mRealInterfaceTypes = new ArrayList<TypeInfo>();
      }

      mRealInterfaceTypes.add(type);
  }

  public String name() {
    return mName;
  }

  public String[] nameParts() {
    return mNameParts;
  }

  public String leafName() {
    return mNameParts[mNameParts.length - 1];
  }

  public String qualifiedName() {
    return mQualifiedName;
  }

  public String qualifiedTypeName() {
    return mQualifiedTypeName;
  }

  public boolean isPrimitive() {
    return mIsPrimitive;
  }

  public ArrayList<MethodInfo> allConstructors() {
    return mAllConstructors;
  }

  public ArrayList<MethodInfo> constructors() {
    if (mConstructors == null) {
      if (mAllConstructors == null) {
        return new ArrayList<MethodInfo>();
      }

      mConstructors = new ArrayList<MethodInfo>();
      for (MethodInfo m : mAllConstructors) {
        if (!m.isHiddenOrRemoved()) {
            mConstructors.add(m);
        }
      }

      Collections.sort(mConstructors, MethodInfo.comparator);
    }
    return mConstructors;
  }

  public ArrayList<ClassInfo> innerClasses() {
    return mInnerClasses;
  }

  public TagInfo[] inlineTags() {
    return comment().tags();
  }

  public TagInfo[] firstSentenceTags() {
    return comment().briefTags();
  }

  public void setDeprecated(boolean deprecated) {
    mDeprecatedKnown = true;
    mIsDeprecated = deprecated;
  }

  public boolean isDeprecated() {
    if (!mDeprecatedKnown) {
      boolean commentDeprecated = comment().isDeprecated();
      boolean annotationDeprecated = false;
      for (AnnotationInstanceInfo annotation : annotations()) {
        if (annotation.type().qualifiedName().equals("java.lang.Deprecated")) {
          annotationDeprecated = true;
          break;
        }
      }

      if (commentDeprecated != annotationDeprecated) {
        Errors.error(Errors.DEPRECATION_MISMATCH, position(), "Class " + qualifiedName()
            + ": @Deprecated annotation and @deprecated comment do not match");
      }

      mIsDeprecated = commentDeprecated | annotationDeprecated;
      mDeprecatedKnown = true;
    }
    return mIsDeprecated;
  }

  public TagInfo[] deprecatedTags() {
    // Should we also do the interfaces?
    return comment().deprecatedTags();
  }

  public ArrayList<MethodInfo> methods() {
      if (mMethods == null) {
          TreeMap<String, MethodInfo> all = new TreeMap<String, MethodInfo>();

          ArrayList<ClassInfo> interfaces = interfaces();
          for (ClassInfo iface : interfaces) {
            if (iface != null) {
              for (MethodInfo method : iface.methods()) {
                all.put(method.getHashableName(), method);
              }
            }
          }

          ClassInfo superclass = superclass();
          if (superclass != null) {
            for (MethodInfo method : superclass.methods()) {
                all.put(method.getHashableName(), method);
            }
          }

          for (MethodInfo method : selfMethods()) {
              all.put(method.getHashableName(), method);
          }

          mMethods = new ArrayList<MethodInfo>(all.values());
          Collections.sort(mMethods, MethodInfo.comparator);
      }
    return mMethods;
  }

  public ArrayList<MethodInfo> annotationElements() {
    return mAnnotationElements;
  }

  public ArrayList<AnnotationInstanceInfo> annotations() {
    return mAnnotations;
  }

  private static void addFields(ClassInfo cl, TreeMap<String, FieldInfo> all) {
    for (FieldInfo field : cl.fields()) {
        all.put(field.name(), field);
    }
  }

  public ArrayList<FieldInfo> fields() {
    if (mFields == null) {
      TreeMap<String, FieldInfo> all = new TreeMap<String, FieldInfo>();

      for (ClassInfo iface : interfaces()) {
        addFields(iface, all);
      }

      ClassInfo superclass = superclass();
      if (superclass != null) {
        addFields(superclass, all);
      }

      for (FieldInfo field : selfFields()) {
        if (!field.isHiddenOrRemoved()) {
            all.put(field.name(), field);
        }
      }

      for (FieldInfo enumConst : mEnumConstants) {
        if (!enumConst.isHiddenOrRemoved()) {
            all.put(enumConst.name(), enumConst);
        }
      }

      mFields = new ArrayList<FieldInfo>(all.values());
    }
    return mFields;
  }

  public void gatherFields(ClassInfo owner, ClassInfo cl, HashMap<String, FieldInfo> fields) {
    for (FieldInfo f : cl.selfFields()) {
      if (f.checkLevel()) {
        fields.put(f.name(), f.cloneForClass(owner));
      }
    }
  }

  public ArrayList<FieldInfo> selfFields() {
    if (mSelfFields == null) {
        HashMap<String, FieldInfo> fields = new HashMap<String, FieldInfo>();
      // our hidden parents
      if (mRealSuperclass != null && !mRealSuperclass.checkLevel()) {
        gatherFields(this, mRealSuperclass, fields);
      }
      for (ClassInfo iface : mRealInterfaces) {
        if (!iface.checkLevel()) {
          gatherFields(this, iface, fields);
        }
      }

      for (FieldInfo f : mAllSelfFields) {
          if (!f.isHiddenOrRemoved()) {
              fields.put(f.name(), f);
          }
      }

      mSelfFields = new ArrayList<FieldInfo>(fields.values());
      Collections.sort(mSelfFields, FieldInfo.comparator);
    }
    return mSelfFields;
  }

  public ArrayList<FieldInfo> allSelfFields() {
    return mAllSelfFields;
  }

  private void gatherMethods(ClassInfo owner, ClassTypePair ctp, HashMap<String, MethodInfo> methods) {
    for (MethodInfo m : ctp.classInfo().selfMethods()) {
      if (m.checkLevel()) {
        methods.put(m.name() + m.signature(), m.cloneForClass(owner, ctp.getTypeArgumentMapping()));
      }
    }
  }

  public ArrayList<MethodInfo> selfMethods() {
    if (mSelfMethods == null) {
        HashMap<String, MethodInfo> methods = new HashMap<String, MethodInfo>();
      // our hidden parents
      for (ClassTypePair ctp : superClassesWithTypes()) {
        // this class is included in this list, so skip it!
        if (ctp.classInfo() != this) {
          if (ctp.classInfo().checkLevel()) {
            break;
          }
          gatherMethods(this, ctp, methods);
        }
      }
      for (ClassTypePair ctp : justMyInterfacesWithTypes(Collections.<String, TypeInfo>emptyMap())) {
        if (!ctp.classInfo().checkLevel()) {
          gatherMethods(this, ctp, methods);
        }
      }
      // mine
      if (mAllSelfMethods != null) {
        for (MethodInfo m : mAllSelfMethods) {
          if (m.checkLevel()) {
            methods.put(m.name() + m.signature(), m);
          }
        }
      }

      // sort it
      mSelfMethods = new ArrayList<MethodInfo>(methods.values());
      Collections.sort(mSelfMethods, MethodInfo.comparator);
    }
    return mSelfMethods;
  }

  public ArrayList<MethodInfo> allSelfMethods() {
    return mAllSelfMethods;
  }

  /**
   * @param removedMethods the removed methods regardless of access levels.
   */
  public void setRemovedMethods(List<MethodInfo> removedMethods) {
    Collections.sort(removedMethods, MethodInfo.comparator);
    mRemovedMethods = Collections.unmodifiableList(removedMethods);
  }

  /**
   * @param allMethods all methods regardless of access levels. Selects the
   * removed, public/protected ones and store them. If a class is removed, all its members
   * are removed, even if the member may not have a @removed tag.
   */
  public void setRemovedSelfMethods(List<MethodInfo> allMethods) {
    List<MethodInfo> removedSelfMethods = new ArrayList<MethodInfo>();
    for (MethodInfo method : allMethods) {
      if ((this.isRemoved() || method.isRemoved()) && (method.isPublic() || method.isProtected()) &&
          (this.isPublic() || this.isProtected()) &&
          (method.findOverriddenMethod(method.name(), method.signature()) == null)) {
        removedSelfMethods.add(method);
      }
    }

    Collections.sort(removedSelfMethods, MethodInfo.comparator);
    mRemovedSelfMethods = Collections.unmodifiableList(removedSelfMethods);
  }

  /**
   * @param allCtors all constructors regardless of access levels.
   * But only the public/protected removed constructors will be stored by the method.
   * Removed constructors should never be deleted from source code because
   * they were once public API.
   */
  public void setRemovedConstructors(List<MethodInfo> allCtors) {
    List<MethodInfo> ctors = new ArrayList<MethodInfo>();
    for (MethodInfo ctor : allCtors) {
      if ((this.isRemoved() || ctor.isRemoved()) && (ctor.isPublic() || ctor.isProtected()) &&
          (this.isPublic() || this.isProtected())) {
        ctors.add(ctor);
      }
    }

    Collections.sort(ctors, MethodInfo.comparator);
    mRemovedConstructors = Collections.unmodifiableList(ctors);
  }

  /**
   * @param allFields all fields regardless of access levels.  Selects the
   * removed, public/protected ones and store them. If a class is removed, all its members
   * are removed, even if the member may not have a @removed tag.
   */
  public void setRemovedSelfFields(List<FieldInfo> allFields) {
    List<FieldInfo> fields = new ArrayList<FieldInfo>();
    for (FieldInfo field : allFields) {
      if ((this.isRemoved() || field.isRemoved()) && (field.isPublic() || field.isProtected()) &&
          (this.isPublic() || this.isProtected())) {
        fields.add(field);
      }
    }

    Collections.sort(fields, FieldInfo.comparator);
    mRemovedSelfFields = Collections.unmodifiableList(fields);
  }

  /**
   * @param allEnumConstants all enum constants regardless of access levels. Selects the
   * removed, public/protected ones and store them. If a class is removed, all its members
   * are removed, even if the member may not have a @removed tag.
   */
  public void setRemovedEnumConstants(List<FieldInfo> allEnumConstants) {
    List<FieldInfo> enums = new ArrayList<FieldInfo>();
    for (FieldInfo field : allEnumConstants) {
      if ((this.isRemoved() || field.isRemoved()) && (field.isPublic() || field.isProtected()) &&
          (this.isPublic() || this.isProtected())) {
        enums.add(field);
      }
    }

    Collections.sort(enums, FieldInfo.comparator);
    mRemovedEnumConstants = Collections.unmodifiableList(enums);
  }

  /**
   * @return all methods that are marked as removed, regardless of access levels.
   * The returned list is sorted and unmodifiable.
   */
  public List<MethodInfo> getRemovedMethods() {
    return mRemovedMethods;
  }

  /**
   * @return all public/protected methods that are removed. @removed methods should never be
   * deleted from source code because they were once public API. Methods that override
   * a parent method will not be included, because deleting them does not break the API.
   */
  public List<MethodInfo> getRemovedSelfMethods() {
    return mRemovedSelfMethods;
  }

  /**
   * @return all public constructors that are removed.
   * removed constructors should never be deleted from source code because they
   * were once public API.
   * The returned list is sorted and unmodifiable.
   */
  public List<MethodInfo> getRemovedConstructors() {
    return mRemovedConstructors;
  }

  /**
   * @return all public/protected fields that are removed.
   * removed members should never be deleted from source code because they were once public API.
   * The returned list is sorted and unmodifiable.
   */
  public List<FieldInfo> getRemovedSelfFields() {
    return mRemovedSelfFields;
  }

  /**
   * @return all public/protected enumConstants that are removed.
   * removed members should never be deleted from source code
   * because they were once public API.
   * The returned list is sorted and unmodifiable.
   */
  public List<FieldInfo> getRemovedSelfEnumConstants() {
    return mRemovedEnumConstants;
  }

  /**
   * @return true if this class contains any self members that are removed
   */
  public boolean hasRemovedSelfMembers() {
    List<FieldInfo> removedSelfFields = getRemovedSelfFields();
    List<FieldInfo> removedSelfEnumConstants = getRemovedSelfEnumConstants();
    List<MethodInfo> removedSelfMethods = getRemovedSelfMethods();
    List<MethodInfo> removedConstructors = getRemovedConstructors();
    if (removedSelfFields.size() + removedSelfEnumConstants.size()
        + removedSelfMethods.size() + removedConstructors.size() == 0) {
      return false;
    } else {
      return true;
    }
  }

  public void addMethod(MethodInfo method) {
    mApiCheckMethods.put(method.getHashableName(), method);

    mAllSelfMethods.add(method);
    mSelfMethods = null; // flush this, hopefully it hasn't been used yet.
  }

  public void addAnnotationElement(MethodInfo method) {
    mAnnotationElements.add(method);
  }

  // Called by PackageInfo when a ClassInfo is added to a package.
  // This is needed because ApiCheck uses PackageInfo.addClass
  // rather than using setContainingPackage to dispatch to the
  // appropriate method. TODO: move ApiCheck away from addClass.
  void setPackage(PackageInfo pkg) {
    mContainingPackage = pkg;
  }

  public void setContainingPackage(PackageInfo pkg) {
    mContainingPackage = pkg;

    if (mContainingPackage != null) {
        if (mIsEnum) {
            mContainingPackage.addEnum(this);
        } else if (mIsInterface) {
            mContainingPackage.addInterface(this);
        } else {
            mContainingPackage.addOrdinaryClass(this);
        }
    }
  }

  public ArrayList<AttributeInfo> selfAttributes() {
    if (mSelfAttributes == null) {
      TreeMap<FieldInfo, AttributeInfo> attrs = new TreeMap<FieldInfo, AttributeInfo>();

      // the ones in the class comment won't have any methods
      for (AttrTagInfo tag : comment().attrTags()) {
        FieldInfo field = tag.reference();
        if (field != null) {
          AttributeInfo attr = attrs.get(field);
          if (attr == null) {
            attr = new AttributeInfo(this, field);
            attrs.put(field, attr);
          }
          tag.setAttribute(attr);
        }
      }

      // in the methods
      for (MethodInfo m : selfMethods()) {
        for (AttrTagInfo tag : m.comment().attrTags()) {
          FieldInfo field = tag.reference();
          if (field != null) {
            AttributeInfo attr = attrs.get(field);
            if (attr == null) {
              attr = new AttributeInfo(this, field);
              attrs.put(field, attr);
            }
            tag.setAttribute(attr);
            attr.methods.add(m);
          }
        }
      }

      // constructors too
      for (MethodInfo m : constructors()) {
        for (AttrTagInfo tag : m.comment().attrTags()) {
          FieldInfo field = tag.reference();
          if (field != null) {
            AttributeInfo attr = attrs.get(field);
            if (attr == null) {
              attr = new AttributeInfo(this, field);
              attrs.put(field, attr);
            }
            tag.setAttribute(attr);
            attr.methods.add(m);
          }
        }
      }

      mSelfAttributes = new ArrayList<AttributeInfo>(attrs.values());
      Collections.sort(mSelfAttributes, AttributeInfo.comparator);
    }
    return mSelfAttributes;
  }

  public ArrayList<FieldInfo> enumConstants() {
    return mEnumConstants;
  }

  public ClassInfo superclass() {
    if (!mSuperclassInit) {
      if (this.checkLevel()) {
        // rearrange our little inheritance hierarchy, because we need to hide classes that
        // don't pass checkLevel
        ClassInfo superclass = mRealSuperclass;
        while (superclass != null && !superclass.checkLevel()) {
          superclass = superclass.mRealSuperclass;
        }
        mSuperclass = superclass;
      } else {
        mSuperclass = mRealSuperclass;
      }
    }
    return mSuperclass;
  }

  public ClassInfo realSuperclass() {
    return mRealSuperclass;
  }

  /**
   * always the real superclass, not the collapsed one we get through superclass(), also has the
   * type parameter info if it's generic.
   */
  public TypeInfo superclassType() {
    return mRealSuperclassType;
  }

  public TypeInfo asTypeInfo() {
    return mTypeInfo;
  }

  ArrayList<TypeInfo> interfaceTypes() {
      ArrayList<TypeInfo> types = new ArrayList<TypeInfo>();
      for (ClassInfo iface : interfaces()) {
          types.add(iface.asTypeInfo());
      }
      return types;
  }

  public String htmlPage() {
    String s = containingPackage().name();
    s = s.replace('.', '/');
    s += '/';
    s += name();
    s += ".html";
    s = Doclava.javadocDir + s;
    return s;
  }

  /** Even indirectly */
  public boolean isDerivedFrom(ClassInfo cl) {
    return isDerivedFrom(cl.qualifiedName());
  }

  /** Even indirectly */
  public boolean isDerivedFrom(String qualifiedName) {
    ClassInfo dad = this.superclass();
    if (dad != null) {
      if (dad.mQualifiedName.equals(qualifiedName)) {
        return true;
      } else {
        if (dad.isDerivedFrom(qualifiedName)) {
          return true;
        }
      }
    }
    for (ClassInfo iface : interfaces()) {
      if (iface.mQualifiedName.equals(qualifiedName)) {
        return true;
      } else {
        if (iface.isDerivedFrom(qualifiedName)) {
          return true;
        }
      }
    }
    return false;
  }

  public void makeKeywordEntries(List<KeywordEntry> keywords) {
    if (!checkLevel()) {
      return;
    }

    String htmlPage = htmlPage();
    String qualifiedName = qualifiedName();

    keywords.add(new KeywordEntry(name(), htmlPage, "class in " + containingPackage().name()));

    ArrayList<FieldInfo> fields = selfFields();
    //ArrayList<FieldInfo> enumConstants = enumConstants();
    ArrayList<MethodInfo> ctors = constructors();
    ArrayList<MethodInfo> methods = selfMethods();

    // enum constants
    for (FieldInfo field : enumConstants()) {
      if (field.checkLevel()) {
        keywords.add(new KeywordEntry(field.name(), htmlPage + "#" + field.anchor(),
            "enum constant in " + qualifiedName));
      }
    }

    // constants
    for (FieldInfo field : fields) {
      if (field.isConstant() && field.checkLevel()) {
        keywords.add(new KeywordEntry(field.name(), htmlPage + "#" + field.anchor(), "constant in "
            + qualifiedName));
      }
    }

    // fields
    for (FieldInfo field : fields) {
      if (!field.isConstant() && field.checkLevel()) {
        keywords.add(new KeywordEntry(field.name(), htmlPage + "#" + field.anchor(), "field in "
            + qualifiedName));
      }
    }

    // public constructors
    for (MethodInfo m : ctors) {
      if (m.isPublic() && m.checkLevel()) {
        keywords.add(new KeywordEntry(m.prettySignature(), htmlPage + "#" + m.anchor(),
            "constructor in " + qualifiedName));
      }
    }

    // protected constructors
    if (Doclava.checkLevel(Doclava.SHOW_PROTECTED)) {
      for (MethodInfo m : ctors) {
        if (m.isProtected() && m.checkLevel()) {
          keywords.add(new KeywordEntry(m.prettySignature(),
              htmlPage + "#" + m.anchor(), "constructor in " + qualifiedName));
        }
      }
    }

    // package private constructors
    if (Doclava.checkLevel(Doclava.SHOW_PACKAGE)) {
      for (MethodInfo m : ctors) {
        if (m.isPackagePrivate() && m.checkLevel()) {
          keywords.add(new KeywordEntry(m.prettySignature(),
              htmlPage + "#" + m.anchor(), "constructor in " + qualifiedName));
        }
      }
    }

    // private constructors
    if (Doclava.checkLevel(Doclava.SHOW_PRIVATE)) {
      for (MethodInfo m : ctors) {
        if (m.isPrivate() && m.checkLevel()) {
          keywords.add(new KeywordEntry(m.name() + m.prettySignature(),
              htmlPage + "#" + m.anchor(), "constructor in " + qualifiedName));
        }
      }
    }

    // public methods
    for (MethodInfo m : methods) {
      if (m.isPublic() && m.checkLevel()) {
        keywords.add(new KeywordEntry(m.name() + m.prettySignature(), htmlPage + "#" + m.anchor(),
            "method in " + qualifiedName));
      }
    }

    // protected methods
    if (Doclava.checkLevel(Doclava.SHOW_PROTECTED)) {
      for (MethodInfo m : methods) {
        if (m.isProtected() && m.checkLevel()) {
          keywords.add(new KeywordEntry(m.name() + m.prettySignature(),
              htmlPage + "#" + m.anchor(), "method in " + qualifiedName));
        }
      }
    }

    // package private methods
    if (Doclava.checkLevel(Doclava.SHOW_PACKAGE)) {
      for (MethodInfo m : methods) {
        if (m.isPackagePrivate() && m.checkLevel()) {
          keywords.add(new KeywordEntry(m.name() + m.prettySignature(),
              htmlPage + "#" + m.anchor(), "method in " + qualifiedName));
        }
      }
    }

    // private methods
    if (Doclava.checkLevel(Doclava.SHOW_PRIVATE)) {
      for (MethodInfo m : methods) {
        if (m.isPrivate() && m.checkLevel()) {
          keywords.add(new KeywordEntry(m.name() + m.prettySignature(),
              htmlPage + "#" + m.anchor(), "method in " + qualifiedName));
        }
      }
    }
  }

  public void makeLink(Data data, String base) {
    data.setValue(base + ".label", this.name());
    if (!this.isPrimitive() && this.isIncluded() && this.checkLevel()) {
      data.setValue(base + ".link", this.htmlPage());
    }
  }

  public static void makeLinkListHDF(Data data, String base, ClassInfo[] classes) {
    final int N = classes.length;
    for (int i = 0; i < N; i++) {
      ClassInfo cl = classes[i];
      if (cl.checkLevel()) {
        cl.asTypeInfo().makeHDF(data, base + "." + i);
      }
    }
  }

  /**
   * Used in lists of this class (packages, nested classes, known subclasses)
   */
  public void makeShortDescrHDF(Data data, String base) {
    mTypeInfo.makeHDF(data, base + ".type");
    data.setValue(base + ".kind", this.kind());
    TagInfo.makeHDF(data, base + ".shortDescr", this.firstSentenceTags());
    TagInfo.makeHDF(data, base + ".deprecated", deprecatedTags());
    data.setValue(base + ".since", getSince());
    if (isDeprecated()) {
      data.setValue(base + ".deprecatedsince", getDeprecatedSince());
    }

    ArrayList<AnnotationInstanceInfo> showAnnos = getShowAnnotationsIncludeOuters();
    AnnotationInstanceInfo.makeLinkListHDF(
      data,
      base + ".showAnnotations",
      showAnnos.toArray(new AnnotationInstanceInfo[showAnnos.size()]));

    setFederatedReferences(data, base);
  }

  /**
   * Turns into the main class page
   */
  public void makeHDF(Data data) {
    int i, j, n;
    String name = name();
    String qualified = qualifiedName();
    ArrayList<AttributeInfo> selfAttributes = selfAttributes();
    ArrayList<MethodInfo> methods = selfMethods();
    ArrayList<FieldInfo> fields = selfFields();
    ArrayList<FieldInfo> enumConstants = enumConstants();
    ArrayList<MethodInfo> ctors = constructors();
    ArrayList<ClassInfo> inners = innerClasses();

    // class name
    mTypeInfo.makeHDF(data, "class.type");
    mTypeInfo.makeQualifiedHDF(data, "class.qualifiedType");
    data.setValue("class.name", name);
    data.setValue("class.qualified", qualified);
    if (isProtected()) {
      data.setValue("class.scope", "protected");
    } else if (isPublic()) {
      data.setValue("class.scope", "public");
    }
    if (isStatic()) {
      data.setValue("class.static", "static");
    }
    if (isFinal()) {
      data.setValue("class.final", "final");
    }
    if (isAbstract() && !isInterface()) {
      data.setValue("class.abstract", "abstract");
    }

    int numAnnotationDocumentation = 0;
    for (AnnotationInstanceInfo aii : annotations()) {
      String annotationDocumentation = Doclava.getDocumentationStringForAnnotation(
          aii.type().qualifiedName());
      if (annotationDocumentation != null) {
        data.setValue("class.annotationdocumentation." + numAnnotationDocumentation + ".text",
            annotationDocumentation);
        numAnnotationDocumentation++;
      }
    }

    ArrayList<AnnotationInstanceInfo> showAnnos = getShowAnnotationsIncludeOuters();
    AnnotationInstanceInfo.makeLinkListHDF(
      data,
      "class.showAnnotations",
      showAnnos.toArray(new AnnotationInstanceInfo[showAnnos.size()]));

    // class info
    String kind = kind();
    if (kind != null) {
      data.setValue("class.kind", kind);
    }
    data.setValue("class.since", getSince());
    if (isDeprecated()) {
      data.setValue("class.deprecatedsince", getDeprecatedSince());
    }
    setFederatedReferences(data, "class");

    // the containing package -- note that this can be passed to type_link,
    // but it also contains the list of all of the packages
    containingPackage().makeClassLinkListHDF(data, "class.package");

    // inheritance hierarchy
    List<ClassTypePair> ctplist = superClassesWithTypes();
    n = ctplist.size();
    for (i = 0; i < ctplist.size(); i++) {
      // go in reverse order
      ClassTypePair ctp = ctplist.get(n - i - 1);
      if (ctp.classInfo().checkLevel()) {
        ctp.typeInfo().makeQualifiedHDF(data, "class.inheritance." + i + ".class");
        ctp.typeInfo().makeHDF(data, "class.inheritance." + i + ".short_class");
        j = 0;
        for (ClassTypePair t : ctp.classInfo().interfacesWithTypes()) {
          t.typeInfo().makeHDF(data, "class.inheritance." + i + ".interfaces." + j);
          j++;
        }
      }
    }

    // class description
    TagInfo.makeHDF(data, "class.descr", inlineTags());
    TagInfo.makeHDF(data, "class.seeAlso", comment().seeTags());
    TagInfo.makeHDF(data, "class.deprecated", deprecatedTags());

    // known subclasses
    TreeMap<String, ClassInfo> direct = new TreeMap<String, ClassInfo>();
    TreeMap<String, ClassInfo> indirect = new TreeMap<String, ClassInfo>();
    ClassInfo[] all = Converter.rootClasses();
    for (ClassInfo cl : all) {
      if (cl.superclass() != null && cl.superclass().equals(this)) {
        direct.put(cl.name(), cl);
      } else if (cl.isDerivedFrom(this)) {
        indirect.put(cl.name(), cl);
      }
    }
    // direct
    i = 0;
    for (ClassInfo cl : direct.values()) {
      if (cl.checkLevel()) {
        cl.makeShortDescrHDF(data, "class.subclasses.direct." + i);
      }
      i++;
    }
    // indirect
    i = 0;
    for (ClassInfo cl : indirect.values()) {
      if (cl.checkLevel()) {
        cl.makeShortDescrHDF(data, "class.subclasses.indirect." + i);
      }
      i++;
    }

    // hide special cases
    if ("java.lang.Object".equals(qualified) || "java.io.Serializable".equals(qualified)) {
      data.setValue("class.subclasses.hidden", "1");
    } else {
      data.setValue("class.subclasses.hidden", "0");
    }

    // nested classes
    i = 0;
    for (ClassInfo inner : inners) {
      if (inner.checkLevel()) {
        inner.makeShortDescrHDF(data, "class.inners." + i);
      }
      i++;
    }

    // enum constants
    i = 0;
    for (FieldInfo field : enumConstants) {
      field.makeHDF(data, "class.enumConstants." + i);
      i++;
    }

    // constants
    i = 0;
    for (FieldInfo field : fields) {
      if (field.isConstant()) {
        field.makeHDF(data, "class.constants." + i);
        i++;
      }
    }

    // fields
    i = 0;
    for (FieldInfo field : fields) {
      if (!field.isConstant()) {
        field.makeHDF(data, "class.fields." + i);
        i++;
      }
    }

    // public constructors
    i = 0;
    for (MethodInfo ctor : ctors) {
      if (ctor.isPublic()) {
        ctor.makeHDF(data, "class.ctors.public." + i);
        i++;
      }
    }

    // protected constructors
    if (Doclava.checkLevel(Doclava.SHOW_PROTECTED)) {
      i = 0;
      for (MethodInfo ctor : ctors) {
        if (ctor.isProtected()) {
          ctor.makeHDF(data, "class.ctors.protected." + i);
          i++;
        }
      }
    }

    // package private constructors
    if (Doclava.checkLevel(Doclava.SHOW_PACKAGE)) {
      i = 0;
      for (MethodInfo ctor : ctors) {
        if (ctor.isPackagePrivate()) {
          ctor.makeHDF(data, "class.ctors.package." + i);
          i++;
        }
      }
    }

    // private constructors
    if (Doclava.checkLevel(Doclava.SHOW_PRIVATE)) {
      i = 0;
      for (MethodInfo ctor : ctors) {
        if (ctor.isPrivate()) {
          ctor.makeHDF(data, "class.ctors.private." + i);
          i++;
        }
      }
    }

    // public methods
    i = 0;
    for (MethodInfo method : methods) {
      if (method.isPublic()) {
        method.makeHDF(data, "class.methods.public." + i);
        i++;
      }
    }

    // protected methods
    if (Doclava.checkLevel(Doclava.SHOW_PROTECTED)) {
      i = 0;
      for (MethodInfo method : methods) {
        if (method.isProtected()) {
          method.makeHDF(data, "class.methods.protected." + i);
          i++;
        }
      }
    }

    // package private methods
    if (Doclava.checkLevel(Doclava.SHOW_PACKAGE)) {
      i = 0;
      for (MethodInfo method : methods) {
        if (method.isPackagePrivate()) {
          method.makeHDF(data, "class.methods.package." + i);
          i++;
        }
      }
    }

    // private methods
    if (Doclava.checkLevel(Doclava.SHOW_PRIVATE)) {
      i = 0;
      for (MethodInfo method : methods) {
        if (method.isPrivate()) {
          method.makeHDF(data, "class.methods.private." + i);
          i++;
        }
      }
    }

    // xml attributes
    i = 0;
    for (AttributeInfo attr : selfAttributes) {
      if (attr.checkLevel()) {
        attr.makeHDF(data, "class.attrs." + i);
        i++;
      }
    }

    // inherited methods
    Iterator<ClassTypePair> superclassesItr = superClassesWithTypes().iterator();
    superclassesItr.next(); // skip the first one, which is the current class
    ClassTypePair superCtp;
    i = 0;
    while (superclassesItr.hasNext()) {
      superCtp = superclassesItr.next();
      if (superCtp.classInfo().checkLevel()) {
        makeInheritedHDF(data, i, superCtp);
        i++;
      }
    }
    Iterator<ClassTypePair> interfacesItr = allInterfacesWithTypes().iterator();
    while (interfacesItr.hasNext()) {
      superCtp = interfacesItr.next();
      if (superCtp.classInfo().checkLevel()) {
        makeInheritedHDF(data, i, superCtp);
        i++;
      }
    }
  }

  private static void makeInheritedHDF(Data data, int index, ClassTypePair ctp) {
    int i;

    String base = "class.inherited." + index;
    data.setValue(base + ".qualified", ctp.classInfo().qualifiedName());
    if (ctp.classInfo().checkLevel()) {
      data.setValue(base + ".link", ctp.classInfo().htmlPage());
    }
    String kind = ctp.classInfo().kind();
    if (kind != null) {
      data.setValue(base + ".kind", kind);
    }

    if (ctp.classInfo().mIsIncluded) {
      data.setValue(base + ".included", "true");
    } else {
      Doclava.federationTagger.tagAll(new ClassInfo[] {ctp.classInfo()});
      if (!ctp.classInfo().getFederatedReferences().isEmpty()) {
        FederatedSite site = ctp.classInfo().getFederatedReferences().iterator().next();
        data.setValue(base + ".link", site.linkFor(ctp.classInfo().htmlPage()));
        data.setValue(base + ".federated", site.name());
      }
    }

    // xml attributes
    i = 0;
    for (AttributeInfo attr : ctp.classInfo().selfAttributes()) {
      attr.makeHDF(data, base + ".attrs." + i);
      i++;
    }

    // methods
    i = 0;
    for (MethodInfo method : ctp.classInfo().selfMethods()) {
      method.makeHDF(data, base + ".methods." + i, ctp.getTypeArgumentMapping());
      i++;
    }

    // fields
    i = 0;
    for (FieldInfo field : ctp.classInfo().selfFields()) {
      if (!field.isConstant()) {
        field.makeHDF(data, base + ".fields." + i);
        i++;
      }
    }

    // constants
    i = 0;
    for (FieldInfo field : ctp.classInfo().selfFields()) {
      if (field.isConstant()) {
        field.makeHDF(data, base + ".constants." + i);
        i++;
      }
    }
  }

  @Override
  public boolean isHidden() {
    if (mHidden == null) {
      mHidden = isHiddenImpl();
    }

    return mHidden;
  }

  /**
   * @return true if the containing package has @hide comment, or an ancestor
   * class of this class is hidden, or this class has @hide comment.
   */
  public boolean isHiddenImpl() {
    ClassInfo cl = this;
    while (cl != null) {
      if (cl.hasShowAnnotation()) {
        return false;
      }
      PackageInfo pkg = cl.containingPackage();
      if (pkg != null && pkg.hasHideComment()) {
        return true;
      }
      if (cl.comment().isHidden()) {
        return true;
      }
      cl = cl.containingClass();
    }
    return false;
  }

  @Override
  public boolean isRemoved() {
    if (mRemoved == null) {
      mRemoved = isRemovedImpl();
    }

    return mRemoved;
  }

  /**
   * @return true if the containing package has @removed comment, or an ancestor
   * class of this class is removed, or this class has @removed comment.
   */
  public boolean isRemovedImpl() {
    ClassInfo cl = this;
    while (cl != null) {
      if (cl.hasShowAnnotation()) {
        return false;
      }
      PackageInfo pkg = cl.containingPackage();
      if (pkg != null && pkg.hasRemovedComment()) {
        return true;
      }
      if (cl.comment().isRemoved()) {
        return true;
      }
      cl = cl.containingClass();
    }
    return false;
  }

  @Override
  public boolean isHiddenOrRemoved() {
    return isHidden() || isRemoved();
  }

  public boolean hasShowAnnotation() {
    return mShowAnnotations != null && mShowAnnotations.size() > 0;
  }

  public ArrayList<AnnotationInstanceInfo> showAnnotations() {
    return mShowAnnotations;
  }

  public ArrayList<AnnotationInstanceInfo> getShowAnnotationsIncludeOuters() {
    ArrayList<AnnotationInstanceInfo> allAnnotations = new ArrayList<AnnotationInstanceInfo>();
    ClassInfo cl = this;
    while (cl != null) {
      if (cl.showAnnotations() != null) {
        // Don't allow duplicates into the merged list
        for (AnnotationInstanceInfo newAii : cl.showAnnotations()) {
          boolean addIt = true;
          for (AnnotationInstanceInfo existingAii : allAnnotations) {
            if (existingAii.type().name() == newAii.type().name()) {
              addIt = false;
              break;
            }
          }
          if (addIt) {
            allAnnotations.add(newAii);
          }
        }
      }
      cl = cl.containingClass();
    }
    return allAnnotations;
  }

  private MethodInfo matchMethod(ArrayList<MethodInfo> methods, String name, String[] params,
      String[] dimensions, boolean varargs) {
    for (MethodInfo method : methods) {
      if (method.name().equals(name)) {
        if (params == null) {
          return method;
        } else {
          if (method.matchesParams(params, dimensions, varargs)) {
            return method;
          }
        }
      }
    }
    return null;
  }

  public MethodInfo findMethod(String name, String[] params, String[] dimensions, boolean varargs) {
    // first look on our class, and our superclasses

    // for methods
    MethodInfo rv;
    rv = matchMethod(methods(), name, params, dimensions, varargs);

    if (rv != null) {
      return rv;
    }

    // for constructors
    rv = matchMethod(constructors(), name, params, dimensions, varargs);
    if (rv != null) {
      return rv;
    }

    // then recursively look at our containing class
    ClassInfo containing = containingClass();
    if (containing != null) {
      return containing.findMethod(name, params, dimensions, varargs);
    }

    return null;
  }

  public boolean supportsMethod(MethodInfo method) {
    for (MethodInfo m : methods()) {
      if (m.getHashableName().equals(method.getHashableName())) {
        return true;
      }
    }
    return false;
  }

  private ClassInfo searchInnerClasses(String[] nameParts, int index) {
    String part = nameParts[index];

    ArrayList<ClassInfo> inners = mInnerClasses;
    for (ClassInfo in : inners) {
      String[] innerParts = in.nameParts();
      if (part.equals(innerParts[innerParts.length - 1])) {
        if (index == nameParts.length - 1) {
          return in;
        } else {
          return in.searchInnerClasses(nameParts, index + 1);
        }
      }
    }
    return null;
  }

  public ClassInfo extendedFindClass(String className) {
    // ClassDoc.findClass has this bug that we're working around here:
    // If you have a class PackageManager with an inner class PackageInfo
    // and you call it with "PackageInfo" it doesn't find it.
    return searchInnerClasses(className.split("\\."), 0);
  }

  public ClassInfo findClass(String className) {
    return Converter.obtainClass(mClass.findClass(className));
  }

  public ClassInfo findInnerClass(String className) {
    // ClassDoc.findClass won't find inner classes. To deal with that,
    // we try what they gave us first, but if that didn't work, then
    // we see if there are any periods in className, and start searching
    // from there.
    String[] nodes = className.split("\\.");
    ClassDoc cl = mClass;

    int N = nodes.length;
    for (int i = 0; i < N; ++i) {
      final String n = nodes[i];
      if (n.isEmpty() && i == 0) {
        // We skip over an empty classname component if it's at location 0. This is
        // to deal with names like ".Inner". java7 will return a bogus ClassInfo when
        // we call "findClass("") and the next iteration of the loop will throw a
        // runtime exception.
        continue;
      }

      cl = cl.findClass(n);
      if (cl == null) {
        return null;
      }
    }

    return Converter.obtainClass(cl);
  }

  public FieldInfo findField(String name) {
    // first look on our class, and our superclasses
    for (FieldInfo f : fields()) {
      if (f.name().equals(name)) {
        return f;
      }
    }

    // then look at our enum constants (these are really fields, maybe
    // they should be mixed into fields(). not sure)
    for (FieldInfo f : enumConstants()) {
      if (f.name().equals(name)) {
        return f;
      }
    }

    // then recursively look at our containing class
    ClassInfo containing = containingClass();
    if (containing != null) {
      return containing.findField(name);
    }

    return null;
  }

  public static ClassInfo[] sortByName(ClassInfo[] classes) {
    int i;
    Sorter[] sorted = new Sorter[classes.length];
    for (i = 0; i < sorted.length; i++) {
      ClassInfo cl = classes[i];
      sorted[i] = new Sorter(cl.name(), cl);
    }

    Arrays.sort(sorted);

    ClassInfo[] rv = new ClassInfo[classes.length];
    for (i = 0; i < rv.length; i++) {
      rv[i] = (ClassInfo) sorted[i].data;
    }

    return rv;
  }

  public boolean equals(ClassInfo that) {
    if (that != null) {
      return this.qualifiedName().equals(that.qualifiedName());
    } else {
      return false;
    }
  }

  public void setNonWrittenConstructors(ArrayList<MethodInfo> nonWritten) {
    mNonWrittenConstructors = nonWritten;
  }

  public ArrayList<MethodInfo> getNonWrittenConstructors() {
    return mNonWrittenConstructors;
  }

  public String kind() {
    if (isOrdinaryClass()) {
      return "class";
    } else if (isInterface()) {
      return "interface";
    } else if (isEnum()) {
      return "enum";
    } else if (isError()) {
      return "class";
    } else if (isException()) {
      return "class";
    } else if (isAnnotation()) {
      return "@interface";
    }
    return null;
  }

  public String scope() {
    if (isPublic()) {
      return "public";
    } else if (isProtected()) {
      return "protected";
    } else if (isPackagePrivate()) {
      return "";
    } else if (isPrivate()) {
      return "private";
    } else {
      throw new RuntimeException("invalid scope for object " + this);
    }
  }

  public void setHiddenMethods(ArrayList<MethodInfo> mInfo) {
    mHiddenMethods = mInfo;
  }

  public ArrayList<MethodInfo> getHiddenMethods() {
    return mHiddenMethods;
  }

  @Override
  public String toString() {
    return this.qualifiedName();
  }

  public void setReasonIncluded(String reason) {
    mReasonIncluded = reason;
  }

  public String getReasonIncluded() {
    return mReasonIncluded;
  }

  private ClassDoc mClass;

  // ctor
  private boolean mIsPublic;
  private boolean mIsProtected;
  private boolean mIsPackagePrivate;
  private boolean mIsPrivate;
  private boolean mIsStatic;
  private boolean mIsInterface;
  private boolean mIsAbstract;
  private boolean mIsOrdinaryClass;
  private boolean mIsException;
  private boolean mIsError;
  private boolean mIsEnum;
  private boolean mIsAnnotation;
  private boolean mIsFinal;
  private boolean mIsIncluded;
  private String mName;
  private String mQualifiedName;
  private String mQualifiedTypeName;
  private boolean mIsPrimitive;
  private TypeInfo mTypeInfo;
  private String[] mNameParts;

  // init
  private ArrayList<ClassInfo> mRealInterfaces = new ArrayList<ClassInfo>();
  private ArrayList<ClassInfo> mInterfaces;
  private ArrayList<TypeInfo> mRealInterfaceTypes;
  private ArrayList<ClassInfo> mInnerClasses;
  // mAllConstructors will not contain *all* constructors. Only the constructors that pass
  // checkLevel. @see {@link Converter#convertMethods(ConstructorDoc[])}
  private ArrayList<MethodInfo> mAllConstructors = new ArrayList<MethodInfo>();
  // mAllSelfMethods will not contain *all* self methods. Only the methods that pass
  // checkLevel. @see {@link Converter#convertMethods(MethodDoc[])}
  private ArrayList<MethodInfo> mAllSelfMethods = new ArrayList<MethodInfo>();
  private ArrayList<MethodInfo> mAnnotationElements = new ArrayList<MethodInfo>(); // if this class is an annotation
  private ArrayList<FieldInfo> mAllSelfFields = new ArrayList<FieldInfo>();
  private ArrayList<FieldInfo> mEnumConstants = new ArrayList<FieldInfo>();
  private PackageInfo mContainingPackage;
  private ClassInfo mContainingClass;
  private ClassInfo mRealSuperclass;
  private TypeInfo mRealSuperclassType;
  private ClassInfo mSuperclass;
  private ArrayList<AnnotationInstanceInfo> mAnnotations;
  private ArrayList<AnnotationInstanceInfo> mShowAnnotations;
  private boolean mSuperclassInit;
  private boolean mDeprecatedKnown;

  // lazy
  private ArrayList<ClassTypePair> mSuperclassesWithTypes;
  private ArrayList<ClassTypePair> mInterfacesWithTypes;
  private ArrayList<ClassTypePair> mAllInterfacesWithTypes;
  private ArrayList<MethodInfo> mConstructors;
  private ArrayList<ClassInfo> mRealInnerClasses;
  private ArrayList<MethodInfo> mSelfMethods;
  private ArrayList<FieldInfo> mSelfFields;
  private ArrayList<AttributeInfo> mSelfAttributes;
  private ArrayList<MethodInfo> mMethods;
  private ArrayList<FieldInfo> mFields;
  private ArrayList<TypeInfo> mTypeParameters;
  private ArrayList<MethodInfo> mHiddenMethods;
  private Boolean mHidden = null;
  private Boolean mRemoved = null;
  private Boolean mCheckLevel = null;
  private String mReasonIncluded;
  private ArrayList<MethodInfo> mNonWrittenConstructors;
  private boolean mIsDeprecated;

  // TODO: Temporary members from apicheck migration.
  private HashMap<String, MethodInfo> mApiCheckConstructors = new HashMap<String, MethodInfo>();
  private HashMap<String, MethodInfo> mApiCheckMethods = new HashMap<String, MethodInfo>();
  private HashMap<String, FieldInfo> mApiCheckFields = new HashMap<String, FieldInfo>();
  private HashMap<String, FieldInfo> mApiCheckEnumConstants = new HashMap<String, FieldInfo>();

  // Resolutions
  private ArrayList<Resolution> mResolutions;

  private List<MethodInfo> mRemovedConstructors; // immutable after you set its value.
  // @removed self methods that do not override any parent methods
  private List<MethodInfo> mRemovedSelfMethods; // immutable after you set its value.
  private List<MethodInfo> mRemovedMethods; // immutable after you set its value.
  private List<FieldInfo> mRemovedSelfFields; // immutable after you set its value.
  private List<FieldInfo> mRemovedEnumConstants; // immutable after you set its value.

  /**
   * Returns true if {@code cl} implements the interface {@code iface} either by either being that
   * interface, implementing that interface or extending a type that implements the interface.
   */
  public boolean implementsInterface(String iface) {
    if (qualifiedName().equals(iface)) {
      return true;
    }
    for (ClassInfo clImplements : realInterfaces()) {
      if (clImplements.implementsInterface(iface)) {
        return true;
      }
    }
    if (mSuperclass != null && mSuperclass.implementsInterface(iface)) {
      return true;
    }
    return false;
  }

  /**
   * Returns true if {@code this} extends the class {@code ext}.
   */
  public boolean extendsClass(String cl) {
    if (qualifiedName().equals(cl)) {
      return true;
    }
    if (mSuperclass != null && mSuperclass.extendsClass(cl)) {
      return true;
    }
    return false;
  }

  /**
   * Returns true if {@code this} is assignable to cl
   */
  public boolean isAssignableTo(String cl) {
    return implementsInterface(cl) || extendsClass(cl);
  }

  public void addInterface(ClassInfo iface) {
    mRealInterfaces.add(iface);
  }

  public void addConstructor(MethodInfo ctor) {
    mApiCheckConstructors.put(ctor.getHashableName(), ctor);

    mAllConstructors.add(ctor);
    mConstructors = null; // flush this, hopefully it hasn't been used yet.
  }

  public void addField(FieldInfo field) {
    mApiCheckFields.put(field.name(), field);

    mAllSelfFields.add(field);

    mSelfFields = null; // flush this, hopefully it hasn't been used yet.
  }

  public void addEnumConstant(FieldInfo field) {
    mApiCheckEnumConstants.put(field.name(), field);

    mEnumConstants.add(field);
  }

  public void setSuperClass(ClassInfo superclass) {
    mRealSuperclass = superclass;
    mSuperclass = superclass;
  }

  public Map<String, MethodInfo> allConstructorsMap() {
    return mApiCheckConstructors;
  }

  public Map<String, FieldInfo> allFields() {
    return mApiCheckFields;
  }

  public Map<String, FieldInfo> allEnums() {
    return mApiCheckEnumConstants;
  }

  /**
   * Returns all methods defined directly in this class. For a list of all
   * methods supported by this class, see {@link #methods()}.
   */
  public Map<String, MethodInfo> allMethods() {
    return mApiCheckMethods;
  }

  /**
   * Returns the class hierarchy for this class, starting with this class.
   */
  public Iterable<ClassInfo> hierarchy() {
    List<ClassInfo> result = new ArrayList<ClassInfo>(4);
    for (ClassInfo c = this; c != null; c = c.mSuperclass) {
      result.add(c);
    }
    return result;
  }

  public String superclassName() {
    if (mSuperclass == null) {
      if (mQualifiedName.equals("java.lang.Object")) {
        return null;
      }
      throw new UnsupportedOperationException("Superclass not set for " + qualifiedName());
    }
    return mSuperclass.mQualifiedName;
  }

  public void setAnnotations(ArrayList<AnnotationInstanceInfo> annotations) {
    mAnnotations = annotations;
  }

  public boolean isConsistent(ClassInfo cl) {
    return isConsistent(cl, null, null);
  }

  public boolean isConsistent(ClassInfo cl, List<MethodInfo> newCtors, List<MethodInfo> newMethods) {
    boolean consistent = true;
    boolean diffMode = (newCtors != null) && (newMethods != null);

    if (isInterface() != cl.isInterface()) {
      Errors.error(Errors.CHANGED_CLASS, cl.position(), "Class " + cl.qualifiedName()
          + " changed class/interface declaration");
      consistent = false;
    }
    for (ClassInfo iface : mRealInterfaces) {
      if (!cl.implementsInterface(iface.mQualifiedName)) {
        Errors.error(Errors.REMOVED_INTERFACE, cl.position(), "Class " + qualifiedName()
            + " no longer implements " + iface);
      }
    }
    for (ClassInfo iface : cl.mRealInterfaces) {
      if (!implementsInterface(iface.mQualifiedName)) {
        Errors.error(Errors.ADDED_INTERFACE, cl.position(), "Added interface " + iface
            + " to class " + qualifiedName());
        consistent = false;
      }
    }

    for (MethodInfo mInfo : mApiCheckMethods.values()) {
      if (cl.mApiCheckMethods.containsKey(mInfo.getHashableName())) {
        if (!mInfo.isConsistent(cl.mApiCheckMethods.get(mInfo.getHashableName()))) {
          consistent = false;
        }
      } else {
        /*
         * This class formerly provided this method directly, and now does not. Check our ancestry
         * to see if there's an inherited version that still fulfills the API requirement.
         */
        MethodInfo mi = ClassInfo.overriddenMethod(mInfo, cl);
        if (mi == null) {
          mi = ClassInfo.interfaceMethod(mInfo, cl);
        }
        if (mi == null) {
          Errors.error(Errors.REMOVED_METHOD, mInfo.position(), "Removed public method "
              + mInfo.prettyQualifiedSignature());
          consistent = false;
        }
      }
    }
    for (MethodInfo mInfo : cl.mApiCheckMethods.values()) {
      if (!mApiCheckMethods.containsKey(mInfo.getHashableName())) {
        /*
         * Similarly to the above, do not fail if this "new" method is really an override of an
         * existing superclass method.
         * But we should fail if this is overriding an abstract method, because method's
         * abstractness affects how users use it. See also Stubs.methodIsOverride().
         */
        MethodInfo mi = ClassInfo.overriddenMethod(mInfo, this);
        if (mi == null ||
            mi.isAbstract() != mInfo.isAbstract()) {
          Errors.error(Errors.ADDED_METHOD, mInfo.position(), "Added public method "
              + mInfo.prettyQualifiedSignature());
          if (diffMode) {
            newMethods.add(mInfo);
          }
          consistent = false;
        }
      }
    }
    if (diffMode) {
      Collections.sort(newMethods, MethodInfo.comparator);
    }

    for (MethodInfo mInfo : mApiCheckConstructors.values()) {
      if (cl.mApiCheckConstructors.containsKey(mInfo.getHashableName())) {
        if (!mInfo.isConsistent(cl.mApiCheckConstructors.get(mInfo.getHashableName()))) {
          consistent = false;
        }
      } else {
        Errors.error(Errors.REMOVED_METHOD, mInfo.position(), "Removed public constructor "
            + mInfo.prettyQualifiedSignature());
        consistent = false;
      }
    }
    for (MethodInfo mInfo : cl.mApiCheckConstructors.values()) {
      if (!mApiCheckConstructors.containsKey(mInfo.getHashableName())) {
        Errors.error(Errors.ADDED_METHOD, mInfo.position(), "Added public constructor "
            + mInfo.prettyQualifiedSignature());
        if (diffMode) {
          newCtors.add(mInfo);
        }
        consistent = false;
      }
    }
    if (diffMode) {
      Collections.sort(newCtors, MethodInfo.comparator);
    }

    for (FieldInfo mInfo : mApiCheckFields.values()) {
      if (cl.mApiCheckFields.containsKey(mInfo.name())) {
        if (!mInfo.isConsistent(cl.mApiCheckFields.get(mInfo.name()))) {
          consistent = false;
        }
      } else {
        Errors.error(Errors.REMOVED_FIELD, mInfo.position(), "Removed field "
            + mInfo.qualifiedName());
        consistent = false;
      }
    }
    for (FieldInfo mInfo : cl.mApiCheckFields.values()) {
      if (!mApiCheckFields.containsKey(mInfo.name())) {
        Errors.error(Errors.ADDED_FIELD, mInfo.position(), "Added public field "
            + mInfo.qualifiedName());
        consistent = false;
      }
    }

    for (FieldInfo info : mApiCheckEnumConstants.values()) {
      if (cl.mApiCheckEnumConstants.containsKey(info.name())) {
        if (!info.isConsistent(cl.mApiCheckEnumConstants.get(info.name()))) {
          consistent = false;
        }
      } else {
        Errors.error(Errors.REMOVED_FIELD, info.position(), "Removed enum constant "
            + info.qualifiedName());
        consistent = false;
      }
    }
    for (FieldInfo info : cl.mApiCheckEnumConstants.values()) {
      if (!mApiCheckEnumConstants.containsKey(info.name())) {
        Errors.error(Errors.ADDED_FIELD, info.position(), "Added enum constant "
            + info.qualifiedName());
        consistent = false;
      }
    }

    if (mIsAbstract != cl.mIsAbstract) {
      consistent = false;
      Errors.error(Errors.CHANGED_ABSTRACT, cl.position(), "Class " + cl.qualifiedName()
          + " changed abstract qualifier");
    }

    if (!mIsFinal && cl.mIsFinal) {
      /*
       * It is safe to make a class final if it did not previously have any public
       * constructors because it was impossible for an application to create a subclass.
       */
      if (mApiCheckConstructors.isEmpty()) {
        consistent = false;
        Errors.error(Errors.ADDED_FINAL_UNINSTANTIABLE, cl.position(),
            "Class " + cl.qualifiedName() + " added final qualifier but "
            + "was previously uninstantiable and therefore could not be subclassed");
      } else {
        consistent = false;
        Errors.error(Errors.ADDED_FINAL, cl.position(), "Class " + cl.qualifiedName()
            + " added final qualifier");
      }
    } else if (mIsFinal && !cl.mIsFinal) {
      consistent = false;
      Errors.error(Errors.REMOVED_FINAL, cl.position(), "Class " + cl.qualifiedName()
          + " removed final qualifier");
    }

    if (mIsStatic != cl.mIsStatic) {
      consistent = false;
      Errors.error(Errors.CHANGED_STATIC, cl.position(), "Class " + cl.qualifiedName()
          + " changed static qualifier");
    }

    if (!scope().equals(cl.scope())) {
      consistent = false;
      Errors.error(Errors.CHANGED_SCOPE, cl.position(), "Class " + cl.qualifiedName()
          + " scope changed from " + scope() + " to " + cl.scope());
    }

    if (!isDeprecated() == cl.isDeprecated()) {
      consistent = false;
      Errors.error(Errors.CHANGED_DEPRECATED, cl.position(), "Class " + cl.qualifiedName()
          + " has changed deprecation state " + isDeprecated() + " --> " + cl.isDeprecated());
    }

    if (superclassName() != null) { // java.lang.Object can't have a superclass.
      if (!cl.extendsClass(superclassName())) {
        consistent = false;
        Errors.error(Errors.CHANGED_SUPERCLASS, cl.position(), "Class " + qualifiedName()
            + " superclass changed from " + superclassName() + " to " + cl.superclassName());
      }
    }

    return consistent;
  }

  // Find a superclass implementation of the given method based on the methods in mApiCheckMethods.
  public static MethodInfo overriddenMethod(MethodInfo candidate, ClassInfo newClassObj) {
    if (newClassObj == null) {
      return null;
    }
    for (MethodInfo mi : newClassObj.mApiCheckMethods.values()) {
      if (mi.matches(candidate)) {
        // found it
        return mi;
      }
    }

    // not found here. recursively search ancestors
    return ClassInfo.overriddenMethod(candidate, newClassObj.mSuperclass);
  }

  // Find a superinterface declaration of the given method.
  public static MethodInfo interfaceMethod(MethodInfo candidate, ClassInfo newClassObj) {
    if (newClassObj == null) {
      return null;
    }
    for (ClassInfo interfaceInfo : newClassObj.interfaces()) {
      for (MethodInfo mi : interfaceInfo.mApiCheckMethods.values()) {
        if (mi.matches(candidate)) {
          return mi;
        }
      }
    }
    return ClassInfo.interfaceMethod(candidate, newClassObj.mSuperclass);
  }

  public boolean hasConstructor(MethodInfo constructor) {
    String name = constructor.getHashableName();
    for (MethodInfo ctor : mApiCheckConstructors.values()) {
      if (name.equals(ctor.getHashableName())) {
        return true;
      }
    }
    return false;
  }

  public void setTypeInfo(TypeInfo typeInfo) {
    mTypeInfo = typeInfo;
  }

  public TypeInfo type() {
      return mTypeInfo;
  }

  public void addInnerClass(ClassInfo innerClass) {
      if (mInnerClasses == null) {
          mInnerClasses = new ArrayList<ClassInfo>();
      }

      mInnerClasses.add(innerClass);
  }

  public void setContainingClass(ClassInfo containingClass) {
      mContainingClass = containingClass;
  }

  public void setSuperclassType(TypeInfo superclassType) {
      mRealSuperclassType = superclassType;
  }

  public void printResolutions() {
      if (mResolutions == null || mResolutions.isEmpty()) {
          return;
      }

      System.out.println("Resolutions for Class " + mName + ":");

      for (Resolution r : mResolutions) {
          System.out.println(r);
      }
  }

  public void addResolution(Resolution resolution) {
      if (mResolutions == null) {
          mResolutions = new ArrayList<Resolution>();
      }

      mResolutions.add(resolution);
  }

  public boolean resolveResolutions() {
      ArrayList<Resolution> resolutions = mResolutions;
      mResolutions = new ArrayList<Resolution>();

      boolean allResolved = true;
      for (Resolution resolution : resolutions) {
          StringBuilder qualifiedClassName = new StringBuilder();
          InfoBuilder.resolveQualifiedName(resolution.getValue(), qualifiedClassName,
                  resolution.getInfoBuilder());

          // if we still couldn't resolve it, save it for the next pass
          if ("".equals(qualifiedClassName.toString())) {
              mResolutions.add(resolution);
              allResolved = false;
          } else if ("superclassQualifiedName".equals(resolution.getVariable())) {
              setSuperClass(InfoBuilder.Caches.obtainClass(qualifiedClassName.toString()));
          } else if ("interfaceQualifiedName".equals(resolution.getVariable())) {
              addInterface(InfoBuilder.Caches.obtainClass(qualifiedClassName.toString()));
          }
      }

      return allResolved;
  }
}
