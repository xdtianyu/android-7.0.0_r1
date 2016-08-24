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
package com.android.icu4j.srcgen;

import com.google.currysrc.api.process.Context;
import com.google.currysrc.api.process.Processor;
import java.lang.reflect.Modifier;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import org.eclipse.jdt.core.dom.AST;
import org.eclipse.jdt.core.dom.CompilationUnit;
import org.eclipse.jdt.core.dom.ImportDeclaration;
import org.eclipse.jdt.core.dom.Name;
import org.eclipse.jdt.core.dom.QualifiedName;
import org.eclipse.jdt.core.dom.SingleMemberAnnotation;
import org.eclipse.jdt.core.dom.Type;
import org.eclipse.jdt.core.dom.TypeDeclaration;
import org.eclipse.jdt.core.dom.TypeLiteral;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;
import org.eclipse.jdt.core.dom.rewrite.ListRewrite;

/**
 * Adds @RunWith annotations to test classes.
 *
 * <p>A class that extends {@code TestFmwk.TestGroup} will have an annotation
 * {@code @RunWith(IcuTestGroupRunner.class)} added and a class that extends {@code TestFmwk} will
 * have an annotation {@code @RunWith(IcuTestFmwkRunner.class)} added.
 *
 * <p>Ideally, this would operate on an AST that has resolved type information so that it was
 * possible to traverse the class hierarchy to identify classes that are derived both directly and
 * indirectly from the test classes. Unfortunately, that approach is very, very time consuming, an
 * order of magnitude (if not two) slower than not resolving the types. Therefore, it was quicker
 * to simply iteratively annotate and run the tests to find the set of direct dependencies and hard
 * code them in here. That could be an issue if this had to deal with many changes in the test
 * classes but this code should only have a very short lifespan given that the ICU4J team is
 * already well on their way to porting the tests over to JUnit.
 */
class RunWithAnnotator implements Processor {

    private static final String RUN_WITH_CLASS_NAME = "org.junit.runner.RunWith";

    private static final String[] TEST_FMWK_BASE_CLASSES = {
            "BidiTest",
            "CalendarTest",
            "android.icu.dev.test.TestFmwk",
            "CompatibilityTest",
            "CoverageTest",
            "LanguageTestRoot",
            "ModuleTest",
            "TestFmwk",
            "TestFmwk.TestGroup",
            "TestGroup",
            "TransliteratorTest",
    };

    private static final String[] TEST_GROUP_BASE_CLASSES = {
            "TestFmwk.TestGroup",
            "TestGroup",
    };

    private static Map<String, String> BASE_CLASS_2_RUNNER_CLASS = new TreeMap<>();
    static {
        for (String name : TEST_FMWK_BASE_CLASSES) {
            BASE_CLASS_2_RUNNER_CLASS.put(name, "android.icu.junit.IcuTestFmwkRunner");
        }
        for (String name : TEST_GROUP_BASE_CLASSES) {
            BASE_CLASS_2_RUNNER_CLASS.put(name, "android.icu.junit.IcuTestGroupRunner");
        }
    }

    @Override
    public void process(Context context, CompilationUnit cu) {
        List types = cu.types();
        ASTRewrite rewrite = context.rewrite();
        boolean imported = false;
        for (Object type : types) {
            if (type instanceof TypeDeclaration) {
                TypeDeclaration typeDeclaration = (TypeDeclaration) type;
                imported = annotateTypeDeclaration(cu, rewrite, typeDeclaration, true, imported);
            }
        }
    }

    private boolean annotateTypeDeclaration(CompilationUnit cu,
            ASTRewrite rewrite, TypeDeclaration typeDeclaration, boolean topLevelClass,
            boolean imported) {

        int modifiers = typeDeclaration.getModifiers();
        if ((topLevelClass || Modifier.isStatic(modifiers)) && Modifier.isPublic(modifiers)
                && !Modifier.isAbstract(modifiers)) {
            Type superClassType = typeDeclaration.getSuperclassType();
            if (superClassType != null) {
                String name = superClassType.toString();
                String runnerClass = BASE_CLASS_2_RUNNER_CLASS.get(name);
                if (runnerClass != null) {
                    addRunWithAnnotation(cu, rewrite, typeDeclaration, runnerClass, imported);
                    imported = true;
                }
            }
        }

        for (TypeDeclaration innerClass : typeDeclaration.getTypes()) {
            imported = annotateTypeDeclaration(cu, rewrite, innerClass, false, imported);
        }

        return imported;
    }

    private boolean addRunWithAnnotation(
            CompilationUnit cu, ASTRewrite rewrite, TypeDeclaration type, String runnerClass,
            boolean imported) {

        AST ast = cu.getAST();

        QualifiedName qRunWith = (QualifiedName) ast.newName(RUN_WITH_CLASS_NAME);
        QualifiedName qRunner = (QualifiedName) ast.newName(runnerClass);
        if (!imported) {
            appendImport(cu, rewrite, qRunWith);
            appendImport(cu, rewrite, qRunner);
        }
        String runWithName = qRunWith.getName().getIdentifier();
        String runnerName = qRunner.getName().getIdentifier();

        SingleMemberAnnotation annotation = ast.newSingleMemberAnnotation();
        annotation.setTypeName(ast.newSimpleName(runWithName));

        TypeLiteral junit4Literal = ast.newTypeLiteral();
        junit4Literal.setType(ast.newSimpleType(ast.newSimpleName(runnerName)));
        annotation.setValue(junit4Literal);

        ListRewrite lrw = rewrite.getListRewrite(type, type.getModifiersProperty());
        lrw.insertFirst(annotation, null);

        return imported;
    }

    private void appendImport(CompilationUnit cu, ASTRewrite rewriter, Name name) {
        ListRewrite lrw = rewriter.getListRewrite(cu, CompilationUnit.IMPORTS_PROPERTY);
        AST ast = cu.getAST();
        ImportDeclaration importDeclaration = ast.newImportDeclaration();
        importDeclaration.setName(name);
        lrw.insertLast(importDeclaration, null);
    }

}
